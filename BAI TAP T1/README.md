## Homework 1 – Principles and Techniques of Program Analysis

### 1. Ý nghĩa của Abstract Interpretation trong phát triển Static Program Analysis
- Abstract Interpretation (AI) cung cấp một khung toán học để xấp xỉ ngữ nghĩa chương trình bằng các cấu trúc có thứ tự (lattices, posets). Nhờ đó, ta có thể mô hình hóa vô số trạng thái thực thi bằng một miền hữu hạn nhưng vẫn đảm bảo tính sound (mọi cảnh báo đều tương ứng với một khả năng thực tế).
- Khi xây dựng static analyzer, AI định nghĩa rõ: (1) miền trừu tượng chứa thông tin cần kiểm tra (ví dụ giá trị khả dĩ của biến, quan hệ giữa biến), (2) các toán tử/transfer function mô phỏng luồng điều khiển và dữ liệu trên miền trừu tượng, (3) điều kiện hội tụ qua kỹ thuật widening/narrowing để đảm bảo phân tích kết thúc trên CFG hữu hạn.
- Cách tiếp cận này biến các câu hỏi không quyết định được trên ngữ nghĩa cụ thể (ví dụ termination, absence of overflows) thành bài toán tính toán được trên miền trừu tượng, giúp static analysis phát hiện lỗi, tối ưu hóa và chứng minh tính chất chương trình ngay cả khi không thể thực sự chạy mã.

### 2. Vì sao công cụ dựa trên Abstract Interpretation thường mang tính lý thuyết
- Thiết kế miền trừu tượng phù hợp đòi hỏi chuyên môn sâu và thường phải cân bằng rất kỹ giữa độ chính xác, độ phủ và chi phí tính toán; việc xây dựng miền mới cho mỗi ngôn ngữ hoặc domain cụ thể khiến sản phẩm khó thương mại hóa rộng rãi.
- Các bảo đảm soundness yêu cầu chứng minh hình thức phức tạp, dẫn đến thời gian triển khai dài và khó tích hợp vào quy trình CI/CD vốn ưu tiên tốc độ phản hồi.
- Nhiều công cụ AI đòi hỏi cấu hình chi tiết (loop invariants, contract, annotations) nên trải nghiệm người dùng chưa thân thiện; ngoài ra việc xử lý đầy đủ đặc trưng ngôn ngữ hiện đại (reflection, concurrency, dynamic loading) vẫn là thách thức.
- Do đó AI thường xuất hiện trong nghiên cứu hoặc trong các tool chuyên biệt (ví dụ cho hệ thống an toàn cao), thay vì phổ biến như linting hoặc phân tích dựa trên heuristics.

### 3. Công cụ ứng dụng Abstract Interpretation trong công nghiệp (2024)
- **Astrée** (AbsInt): dùng AI để chứng minh không có lỗi runtime (overflow, divide-by-zero) trong mã nhúng C/C++; được Airbus và các hãng hàng không dùng cho phần mềm điều khiển bay.
- **Polyspace Code Prover** (MathWorks): phân tích C/C++/Ada bằng AI nhằm phát hiện lỗi runtime và prove absence; tích hợp vào quy trình ISO 26262, DO-178C.
- **Frama-C** (CEA List): framework mã nguồn mở cho C với nhiều plugin AI (Value, EVA) sử dụng trong quốc phòng và năng lượng hạt nhân.
- **Infer** (Meta): analyzer mã nguồn mở cho Java, C, C++, Objective-C; tận dụng AI (kết hợp bi-abstract domains và separation logic) để phát hiện null dereference, resource leak trong quy mô build lớn.
- **AstréeA++ / IKOS / Tia Portal Checker**: ví dụ bổ sung cho lĩnh vực công nghiệp tự động hoá và phân tích bảo mật nhúng.
