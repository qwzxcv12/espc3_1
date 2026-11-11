# Hệ Thống In Phiếu Lấy Số Thứ Tự Tự Động (QMS Kiosk) - ESP32-C3

Hệ thống điều khiển máy in nhiệt ESC/POS (hoặc module RS232-to-TTL) trên **ESP32-C3 (ESP-IDF v5.1+)**, in phiếu lấy số thứ tự tự động theo mẫu thực tế của cơ quan hành chính công.

---

## 1. Sơ Đồ Đấu Nối Phần Cứng

| Chân ESP32-C3 | Tín hiệu | Thiết Bị Ngoại Vi | Chức Năng |
| :--- | :---: | :--- | :--- |
| **GPIO 3** (TXD) | `--->` | **RXD** Máy In / Module RS232 | Truyền lệnh in ESC/POS (Baud: **9600**, 8N1) |
| **GPIO 4** (RXD) | `<---` | **TXD** Máy In / Module RS232 | Nhận tín hiệu phản hồi |
| **GND** | `<--->` | **GND** Máy In & Nút Bấm | Nối chung Mass (Ground) |
| **GPIO 0** | `<---` | **Nút Bấm In Phiếu** | Nút nhấn nhả nối GPIO 0 với GND (Chế độ Pullup) |
| **GPIO 8** | - | **LED Status** | Đèn trạng thái (Xanh lá: Đang in; Xanh dương: Sẵn sàng) |
| **USB Type-C** | `<--->` | **Máy tính (Cổng COM)** | Cấp nguồn, xem Log và gõ lệnh test in trực tiếp |

---

## 2. File Cấu Hình Nội Dung Phiếu In: [`ticket_config.h`](ticket_config.h)

Bạn có thể chỉnh sửa các thông tin in trên phiếu tại file **`ticket_config.h`**:

```c
// 1. Tùy chọn in ấn
#define ENABLE_PRINT_LOGO           true    // In Logo hình tròn ở đầu phiếu (true: Bật, false: Tắt)
#define ENABLE_VIETNAMESE_ACCENT    true    // In tiếng Việt có dấu (true: UTF-8, false: Tự động chuyển không dấu)

// 2. Tiêu đề cơ quan / Đơn vị
#define TICKET_HEADER_LINE1         "ỦY BAN NHÂN DÂN XÃ CHÂU THÀNH"
#define TICKET_HEADER_LINE2         "TRUNG TÂM PHỤC VỤ HÀNH CHÍNH CÔNG"
#define TICKET_HEADER_ADDRESS       "79 Tân Túc, xã Tân Nhựt, TP. Hồ Chí Minh"

// 3. Lĩnh vực phục vụ & Quầy
#define TICKET_SERVICE_NAME         "LĨNH VỰC BHXH,BHYT,BHTN,BHTNLD,BNN"
#define TICKET_COUNTER_NAME         "(QUẦY 1)"

// 4. Số thứ tự (STT) bắt đầu
#define TICKET_START_NUMBER         1002    // Bắt đầu từ 1002 (tự tăng 1003, 1004...)

// 5. Lời nhắn chân trang
#define TICKET_NOTICE_LINE1         "Quý khách vui lòng chờ đến lượt phục vụ"
#define TICKET_NOTICE_LINE2         "theo STT trên màn hình điện tử. Xin cảm ơn"
```

---

## 3. Cách Sử Dụng & Thử Nghiệm

### Cách 1: Nhấn Nút Phần Cứng
- Nhấn nút nối giữa **GPIO 0** và **GND**: Máy in sẽ lập tức in 1 phiếu theo mẫu và tự động nhảy số thứ tự tiếp theo.

### Cách 2: Điều Khiển Qua Cổng COM (Serial Monitor)
Mở Serial Monitor (Baud rate: `115200` trên cổng `COM6` của ESP32-C3) và gõ:
- **`PRINT`** hoặc phím **`Enter`**: In 1 phiếu số tiếp theo.
- **`<số>`** (Ví dụ: `1002`, `2005`): In phiếu với số chỉ định theo yêu cầu.
- **`LOGO`**: In thử riêng hình ảnh Logo để kiểm tra đầu in.
- **`CONFIG`**: Xem thông tin cấu hình đang nạp trong máy.
- **`HELP`**: Xem danh sách lệnh.

---

## 4. Biên Dịch & Nạp Lên ESP32-C3

Chạy lệnh tự động hóa:
```powershell
python cloud_builder.py
```
Hệ thống sẽ build firmware bằng Docker ESP-IDF v5.1 trên GitHub Actions và nạp trực tiếp qua cổng COM vào ESP32-C3.
