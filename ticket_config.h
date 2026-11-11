#ifndef TICKET_CONFIG_H
#define TICKET_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// FILE CẤU HÌNH THÔNG TIN IN TRÊN PHIẾU (QMS TICKET CONFIGURATION)
// ============================================================================

// 1. TÙY CHỌN BỐ CỤC & HÌNH ẢNH
// Bật chế độ in Header Bitmap: Logo nằm BÊN TRÁI + Tiêu đề BÊN PHẢI (giống hệt ảnh mẫu)
// Chế độ này in được TIẾNG VIỆT CÓ DẤU HOÀN HẢO trên tiêu đề vì xuất theo dạng ảnh!
#define ENABLE_BITMAP_HEADER        true
#define ENABLE_PRINT_LOGO           true

// Tùy chọn Tiếng Việt cho phần text thân phiếu:
// Vì phần lớn máy in nhiệt POS không có font tiếng Việt trong ROM (bị lỗi ký tự lạ),
// để false sẽ tự động chuyển thành tiếng Việt không dấu chuẩn đẹp:
#define ENABLE_VIETNAMESE_ACCENT    false

// Cỡ chữ số thứ tự (STT): 3 là vừa vặn, đẹp và gọn (không quá to làm dài phiếu)
#define STT_FONT_SIZE               3

// 2. NỘI DUNG VĂN BẢN (DÙNG KHI CẦN THAY ĐỔI)
#define TICKET_HEADER_LINE1         "ỦY BAN NHÂN DÂN XÃ CHÂU THÀNH"
#define TICKET_HEADER_LINE2         "TRUNG TÂM PHỤC VỤ HÀNH CHÍNH CÔNG"
#define TICKET_HEADER_ADDRESS       "79 Tân Túc, xã Tân Nhựt, TP. Hồ Chí Minh"

// Lĩnh vực & Quầy
#define TICKET_SERVICE_NAME         "LĨNH VỰC BHXH,BHYT,BHTN,BHTNLD,BNN"
#define TICKET_COUNTER_NAME         "(QUẦY 1)"

// Số bắt đầu khi khởi động kit
#define TICKET_START_NUMBER         1002
#define TICKET_MAX_NUMBER           9999

// Lời nhắc nhở khách hàng
#define TICKET_NOTICE_LINE1         "Quý khách vui lòng chờ đến lượt phục vụ"
#define TICKET_NOTICE_LINE2         "theo STT trên màn hình điện tử. Xin cảm ơn"

// Số đang gọi hiện tại & ước tính
#define TICKET_CURRENT_CALLING      0
#define TICKET_EST_WAIT_PER_PERSON  5

#endif // TICKET_CONFIG_H
