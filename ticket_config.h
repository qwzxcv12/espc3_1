#ifndef TICKET_CONFIG_H
#define TICKET_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// FILE CẤU HÌNH THÔNG TIN IN TRÊN PHIẾU (QMS TICKET CONFIGURATION)
// ============================================================================

// 1. TÙY CHỌN BỐ CỤC & TỐI ƯU TỐC ĐỘ IN
// - HEADER_MODE = 0: Chế độ in Văn Bản Động: Tiêu đề cơ quan in to, ĐẬM, TRẢI RỘNG TOÀN BỘ BỀ NGANG KHỔ GIẤY
#define TICKET_HEADER_MODE          0

#define ENABLE_PRINT_LOGO           false   // false: BỎ LOGO HOÀN TOÀN, dành trọn vẹn bề ngang cho tiêu đề
#define PRINTER_PAPER_WIDTH_MM      58      // Khổ giấy: 58 (58mm) hoặc 80 (80mm)

// Vì máy in nhiệt không có font tiếng Việt trong ROM (bị lỗi ký tự lạ),
// để false sẽ tự động chuyển thành tiếng Việt không dấu chuẩn đẹp:
#define ENABLE_VIETNAMESE_ACCENT    false

// Cỡ chữ số thứ tự (STT): 3 hoặc 4 (vừa vặn, to rõ, không làm dài phiếu)
#define STT_FONT_SIZE               3

// 2. NỘI DUNG VĂN BẢN (CHỈNH SỬA DỄ DÀNG BẤT KỲ LÚC NÀO TẠI ĐÂY)
#define TICKET_HEADER_LINE1         "ỦY BAN NHÂN DÂN XÃ CHÂU THÀNH"
#define TICKET_HEADER_LINE2         "TRUNG TÂM PHỤC VỤ HÀNH CHÍNH CÔNG"
#define TICKET_HEADER_ADDRESS       "79 Tân Túc, xã Tân Nhựt, TP. Hồ Chí Minh"

// Lĩnh vực & Quầy phục vụ
#define TICKET_SERVICE_NAME         "LĨNH VỰC BHXH,BHYT,BHTN,BHTNLD,BNN"
#define TICKET_COUNTER_NAME         "(QUẦY 1)"

// Số thứ tự (STT) bắt đầu
#define TICKET_START_NUMBER         1002
#define TICKET_MAX_NUMBER           9999

// Lời nhắn / Hướng dẫn khách hàng
#define TICKET_NOTICE_LINE1         "Quý khách vui lòng chờ đến lượt phục vụ"
#define TICKET_NOTICE_LINE2         "theo STT trên màn hình điện tử. Xin cảm ơn"

// Số đang gọi hiện tại & ước tính
#define TICKET_CURRENT_CALLING      0
#define TICKET_EST_WAIT_PER_PERSON  5

#endif // TICKET_CONFIG_H
