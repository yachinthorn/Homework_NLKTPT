# Homework 3 – Dynamic Analyses Illustrated in C++

Tài liệu dựa trên nội dung slide 3 (tracing, dynamic slicing, execution indexing, fault localization). Mỗi mục sử dụng cùng một đoạn mã C++ quen thuộc để việc đối chiếu dễ dàng.

## 1. Minh hoạ phân tích chương trình bằng Tracing
**Ý tưởng:** giống slide về tracing/logging các sự kiện gây ảnh hưởng nhân quả. Ta ghi lại những điểm kiểm soát quan trọng (entry, exit, biến thay đổi) để khôi phục hành vi khi cần.

```cpp
#include <chrono>
#include <fstream>
#include <string>

struct TraceEvent {
    std::string region;
    std::string action;
    long long timestamp;
};

class Tracer {
public:
    explicit Tracer(std::string region)
        : region_(std::move(region)) { log("enter"); }
    ~Tracer() { log("exit"); }
    void log(const std::string& action) {
        TraceEvent evt{region_, action, now()};
        std::ofstream("trace.log", std::ios::app) << evt.region << ' '
                                                 << evt.action << ' '
                                                 << evt.timestamp << '\n';
    }
private:
    static long long now() {
        return std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::steady_clock::now().time_since_epoch())
            .count();
    }
    std::string region_;
};

int compute(int n) {
    Tracer scope("compute");
    int total = 0;
    for (int i = 0; i < n; ++i) {
        Tracer iter("loop");
        total += i;
    }
    return total;
}
```
- File `trace.log` chính là chuỗi sự kiện để phân tích nhân quả (có thể gửi qua mạng / lưu stable storage giống slide).
- Khi hệ thống lỗi, ta tái hiện lại trình tự `enter/exit` để xem vùng nào chạy trước/ảnh hưởng sau.

## 2. Minh hoạ Dynamic Slicing
**Ý tưởng:** thu execution trace và truy vết ngược các định nghĩa ảnh hưởng tới biến quan sát như trong slide “dynamic slicing”. Ví dụ dưới đây giữ lại dependency từ câu lệnh tới biến.

```cpp
#include <vector>

struct Statement {
    int id;
    std::vector<int> uses;
    int def;
};

class SliceRecorder {
public:
    void exec(int stmtId, int defVar, std::vector<int> uses) {
        history_.push_back({stmtId, std::move(uses), defVar});
    }
    std::vector<int> slice(int stmtId, int var) const {
        std::vector<int> sliceIds;
        int targetVar = var;
        for (auto it = history_.rbegin(); it != history_.rend(); ++it) {
            if (it->id == stmtId && (it->def == var || contains(it->uses, var))) {
                sliceIds.push_back(it->id);
                break;
            }
            if (it->def == targetVar) {
                sliceIds.push_back(it->id);
                targetVar = it->uses.empty() ? -1 : it->uses.front();
            }
        }
        return sliceIds;
    }
private:
    static bool contains(const std::vector<int>& vec, int v) {
        for (int x : vec) {
            if (x == v) return true;
        }
        return false;
    }
    std::vector<Statement> history_;
};
```
- Khi chạy chương trình kiểm thử, mỗi statement gọi `exec(id, defVar, uses)` với mã hoá biến (ví dụ `1=sum`, `2=i`).
- Hàm `slice` tương ứng tiêu chí `<statement, variable>` (slide 73-81) và trả về danh sách câu lệnh ảnh hưởng thực tế cho đầu vào cụ thể.
- Với đầu vào khác, trace khác → slice khác (đúng tinh thần dynamic slice nhỏ hơn static slice).

## 3. Minh hoạ Execution Indexing
**Ý tưởng:** giống EDL/region stacking trong slide, ta dùng stack để lưu chỉ số `<context, statement, instance>` nhằm khớp các lần chạy khác nhau.

```cpp
#include <stack>
#include <tuple>
#include <unordered_map>
#include <vector>

struct IndexEntry {
    int regionId;   // ví dụ: 1=main, 2=if-true, 3=loop-body
    int stmtId;     // số dòng được gán thủ công
    int instance;   // đếm số lần statement xuất hiện
};

class ExecutionIndex {
public:
    void enterRegion(int regionId) {
        stack_.push({regionId, -1, 0});
    }
    void exitRegion() {
        if (!stack_.empty()) stack_.pop();
    }
    IndexEntry mark(int stmtId) {
        if (stack_.empty()) enterRegion(0);
        auto top = stack_.top();
        top.stmtId = stmtId;
        top.instance = ++counters_[stmtId];
        history_.push_back(top);
        return top;
    }
    const std::vector<IndexEntry>& history() const { return history_; }
private:
    std::stack<IndexEntry> stack_;
    std::unordered_map<int, int> counters_;
    std::vector<IndexEntry> history_;
};
```
- Khi instrument, ta gọi `enterRegion` ở đầu hàm/nhánh và `exitRegion` tại post-dominator, giống mô tả “stack giống Control Dependence Stack”.
- Hai lần chạy có thể so sánh chuỗi `IndexEntry` để căn chỉnh điểm break, khắc phục vấn đề Heisenbug (slide 96-105).

## 4. Minh hoạ Fault Localization
**Ý tưởng:** dùng spectra-based ranking (Dice/Tarantula). Ví dụ C++ dưới đây thu coverage và tính điểm nghi ngờ.

```cpp
#include <cmath>
#include <map>
#include <vector>

struct Spectrum {
    int passed = 0;
    int failed = 0;
};

class FaultLocalizer {
public:
    void report(int stmtId, bool failed) {
        auto& s = spectra_[stmtId];
        failed ? ++s.failed : ++s.passed;
    }
    double suspiciousness(int stmtId) const {
        auto it = spectra_.find(stmtId);
        if (it == spectra_.end()) return 0.0;
        double f = static_cast<double>(it->second.failed);
        double p = static_cast<double>(it->second.passed);
        double totalF = failedTests_;
        double totalP = passedTests_;
        if (totalF == 0 || (p == 0 && f == 0)) return 0.0;
        double numerator = f / totalF;
        double denominator = numerator + (p / totalP);
        return denominator == 0.0 ? 0.0 : numerator / denominator;
    }
    void endTest(bool failed) {
        failed ? ++failedTests_ : ++passedTests_;
    }
private:
    std::map<int, Spectrum> spectra_;
    double failedTests_ = 0;
    double passedTests_ = 0;
};
```
- `report(stmtId, failed)` được gọi mỗi khi statement chạy trong test. Sau test gọi `endTest(failed)`.
- `suspiciousness` sử dụng công thức Tarantula (slide 140-143 nhấn mạnh ranking dựa trên spectra).
- Dev sẽ kiểm tra các statement có điểm cao trước → giảm thời gian debug.

> Các ví dụ đều có thể chạy độc lập hoặc gộp vào framework kiểm thử để đáp ứng yêu cầu Homework 3: tracing, dynamic slicing, execution indexing, fault localization, tất cả bằng C++.
