# Lab 3 – Đề 2: Giám sát vườn cây công nghiệp bằng ESP32, FreeRTOS và MQTT

Hệ thống giám sát và điều khiển môi trường vườn cây công nghiệp (cà phê, hồ tiêu) dùng **ESP32 + cảm biến DHT22**, chạy **FreeRTOS** trên hai nhân, truyền dữ liệu qua **MQTT** và hiển thị trên **Web Dashboard**. Dự án được mô phỏng trên **Wokwi**.

| | |
|---|---|
| **Sinh viên** | Nguyễn Tiến Hiếu |
| **MSSV** | 231432233 |
| **Lớp** | Cử nhân Kỹ thuật Điện tử Viễn thông K64 |
| **Bài thực hành** | Lab 3 – Đề 2 |

## Giới thiệu

Vườn cây công nghiệp cần độ ẩm không khí ổn định trong khoảng **40% – 60%**. ESP32 đọc nhiệt độ và độ ẩm từ DHT22, hiển thị qua Serial và gửi lên MQTT Broker. Khi độ ẩm xuống thấp, **máy phun sương** tự động bật theo ba mức công suất. Người dùng theo dõi số liệu, biểu đồ và điều khiển thủ công **máy bơm**, **quạt** qua Web Dashboard.

## Chức năng chính

- Đọc nhiệt độ, độ ẩm từ DHT22 mỗi 2 giây, hiển thị trên Serial Monitor.
- Gửi dữ liệu lên MQTT Broker (HiveMQ) qua Wi-Fi.
- Web Dashboard: thẻ nhiệt độ, độ ẩm, mức phun sương, 2 biểu đồ thời gian thực, nút BẬT/TẮT máy bơm và quạt.
- Máy phun sương điều khiển bằng PWM, tự động theo độ ẩm, đếm ngược thời gian không chặn.

## Phần cứng và sơ đồ chân

| Thiết bị | Chân ESP32 | Ghi chú |
|---|---|---|
| DHT22 (DATA) | GPIO15 | Nhiệt độ, độ ẩm |
| Máy phun sương (LED đỏ) | GPIO23 | PWM 0 – 255 |
| Máy bơm (LED xanh dương) | GPIO18 | BẬT/TẮT từ web |
| Quạt (LED xanh lá) | GPIO19 | BẬT/TẮT từ web |

Board: ESP32 DevKitC V4. Ba LED đại diện cho các cơ cấu chấp hành trong mô phỏng.

## Tổ chức FreeRTOS

| Task | Core | Chức năng |
|---|---|---|
| `TaskMQTT` | 0 | Kết nối Wi-Fi/MQTT, gửi dữ liệu, nhận lệnh bơm/quạt |
| `TaskReadData` | 1 | Đọc DHT22, in Serial, gửi dữ liệu vào 2 Queue |
| `TaskAlert` | 1 | Nhận độ ẩm, điều khiển máy phun sương |

Hai Queue: `dataQueue` (nhiệt độ + độ ẩm, đến `TaskMQTT`) và `dataAlert` (độ ẩm, đến `TaskAlert`).

## Thuật toán máy phun sương

| Độ ẩm (%) | Công suất | Thời gian |
|---|---|---|
| ≥ 40 | Tắt | – |
| 35 ≤ H < 40 | 25% | 10 giây |
| 30 ≤ H < 35 | 50% | 20 giây |
| < 30 | 100% | 30 giây |

## Giao thức MQTT

Broker: `broker.hivemq.com`

| Topic | Hướng | Nội dung |
|---|---|---|
| `lab3/de2/sensor` | ESP32 → Web | `{"temperature":28.0,"humidity":71.5}` |
| `lab3/de2/pump/set` | Web → ESP32 | `ON` / `OFF` |
| `lab3/de2/fan/set` | Web → ESP32 | `ON` / `OFF` |

## Cách chạy

1. Mở project Wokwi (link ở cuối trang) và bấm **Start**.
2. Mở Web Dashboard (link ở cuối trang).
3. Trong Wokwi, kéo thanh Humidity của DHT22 để kiểm tra từng mức phun sương.

> Web chỉ hiển thị số liệu khi mô phỏng Wokwi đang chạy. Nếu chưa chạy, các thẻ số hiện `--.-`.

## Cấu trúc kho lưu trữ

| File | Nội dung |
|---|---|
| `Nguyễn Tiến Hiếu_231432233_lab3_đề2` (.docx, .pdf) | Báo cáo thực hành |
| `sketch.ino` | Mã nguồn ESP32 |
| `diagram.json`, `libraries.txt`, `wokwi-project.txt` | Sơ đồ mạch và cấu hình project Wokwi |
| `index.html` | Giao diện Web Dashboard |
| `README.md` | Giới thiệu tổng quan dự án |

## Liên kết trực quan

Mô phỏng Wokwi: [Xem mô phỏng trên Wokwi](https://wokwi.com/projects/479992109740419073)

Web Demo: [Xem trang web demo](https://hieuuanh512005.github.io/NguyenTienHieu_231432233_Lab3_De2/)
