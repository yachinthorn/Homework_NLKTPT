# Homework 2 – Program Analysis Illustrations in C++

## 1. Minh hoạ Dataflow Analysis
**Mục tiêu:** xác định tập định nghĩa biến `sum` và `i` có thể đến từng điểm trong chương trình để chứng minh tính đúng đắn của phép cộng dồn.

```cpp
int accumulateRange(int n) {
    int sum = 0;          // d1: sum
    int i = 1;            // d2: i
    while (i <= n) {      // use i, sum
        sum = sum + i;    // d3: sum
        i = i + 1;        // d4: i
    }
    return sum;           // use sum
}
```

| Basic block | GEN set (định nghĩa mới) | KILL set (định nghĩa bị ghi đè) | OUT (định nghĩa khả dĩ sau khối) |
|-------------|---------------------------|----------------------------------|----------------------------------|
| B1 (`sum=0; i=1;`) | {d1, d2} | {d3, d4} | {d1, d2} |
| B2 (`while` test) | ∅ | ∅ | {d1, d2, d3, d4} |
| B3 (`sum=sum+i;`) | {d3} | {d1} | {d2, d3, d4} |
| B4 (`i=i+1;`) | {d4} | {d2} | {d1, d3, d4} |

- Phân tích đạt điểm cố định khi `OUT(B2)` không đổi qua các vòng lặp, đảm bảo mọi đường đi vào khối trả về đều có định nghĩa hợp lệ của `sum` (tập {d1, d3}).
- Đây là ví dụ reaching definitions – thông tin này giúp compiler/chuyên gia xác minh rằng `sum` luôn được khởi tạo trước khi dùng.

## 2. Minh hoạ CFG Analysis
Áp dụng cho hàm dưới để xác định các nhánh và đường đi cần test.

```cpp
int classify(int value) {
    if (value < 0) {
        return -1;
    } else if (value == 0) {
        return 0;
    }
    return 1;
}
```

- **Nút:** Start → `value<0?` → `value==0?` → `return` tương ứng → End.
- **Cạnh:**
  - Từ Start tới nút điều kiện `value<0?`.
  - Nhánh true dẫn tới `return -1` (sau đó End).
  - Nhánh false tiến tới điều kiện `value==0?` với hai cạnh tới `return 0` và `return 1`.
- **Phân tích:** CFG cho biết cần ít nhất 3 ca kiểm thử để phủ mọi đường đi (ví dụ: `-5`, `0`, `10`). Ngoài ra, nút `value==0?` bị **dominated** bởi Start và `value<0?`, nên bất kỳ lỗi tại đây đều phụ thuộc việc nhánh `value<0?` trả về false.

## 3. Minh hoạ kỹ thuật cấp phát / giải phóng bộ nhớ

```cpp
#include <memory>

struct Node {
    int value;
    Node* next;
};

void demoAllocation() {
    Node stackNode{42, nullptr};              // ❶ Cấp phát tĩnh (stack), tự giải phóng khi scope kết thúc.

    Node* heapNode = new Node{13, nullptr};   // ❷ Cấp phát động bằng `new`.
    heapNode->value = 99;
    delete heapNode;                          // ➌ Giải phóng thủ công, tránh leak.

    auto smart = std::make_unique<Node>();    // ➍ RAII: smart pointer tự giải phóng khi ra khỏi scope.
    smart->value = 2025;
}                                             // ➎ `stackNode` và `smart` được thu hồi tự động.
```
- Ví dụ cho thấy ba kỹ thuật chính được đề cập trong bài giảng: stack allocation, dynamic allocation dùng heap, và cơ chế tự động nhờ RAII/smart pointer để kiểm soát vòng đời.

## 4. Minh hoạ sử dụng và giải phóng pointer

```cpp
#include <iostream>

void usePointer() {
    int* numbers = new int[3]{1, 2, 3};   // tạo mảng động

    for (int i = 0; i < 3; ++i) {
        std::cout << numbers[i] << " ";
    }
    std::cout << "\n";

    delete[] numbers;                     // giải phóng đúng cặp với new[]
    numbers = nullptr;                    // tránh dangling pointer
}

void safePointer() {
    std::unique_ptr<int[]> safe(new int[2]{5, 7});
    safe[1] = 42;                         // unique_ptr quản lý bộ nhớ tự động
}                                         // tự động delete[] khi thoát hàm
```
- Đoạn `usePointer` minh hoạ quy tắc **new/new[] ↔ delete/delete[]** và việc đặt con trỏ về `nullptr` sau khi giải phóng.
- `safePointer` trình bày cách dùng smart pointer để loại bỏ lỗi double-free/leak theo đúng tinh thần bài học "sử dụng và giải phóng pointer".

