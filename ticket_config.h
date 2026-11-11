#ifndef TICKET_CONFIG_H
#define TICKET_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// FILE CẤU HÌNH THÔNG TIN IN TRÊN PHIẾU (QMS TICKET CONFIGURATION)
// Chỉnh sửa các thông số dưới đây theo nhu cầu của đơn vị/cơ quan
// ============================================================================

// 1. Tùy chọn in ấn
#define ENABLE_PRINT_LOGO           true    // true: In Logo ở đầu phiếu; false: Bỏ qua logo
#define ENABLE_VIETNAMESE_ACCENT    true    // true: In tiếng Việt có dấu. Nếu máy in bị lỗi font, đổi thành false (tự động chuyển không dấu)
#define TICKET_PAPER_WIDTH_CHARS    32      // Số ký tự trên một dòng (32 ký tự cho khổ 58mm, 42-48 ký tự cho 80mm)

// 2. Tiêu đề cơ quan / Trung tâm hành chính (Header)
#define TICKET_HEADER_LINE1         "ỦY BAN NHÂN DÂN XÃ CHÂU THÀNH"
#define TICKET_HEADER_LINE2         "TRUNG TÂM PHỤC VỤ HÀNH CHÍNH CÔNG"
#define TICKET_HEADER_ADDRESS       "79 Tân Túc, xã Tân Nhựt, TP. Hồ Chí Minh"

// 3. Phân loại dịch vụ & Quầy phục vụ
#define TICKET_SERVICE_NAME         "LĨNH VỰC BHXH,BHYT,BHTN,BHTNLD,BNN"
#define TICKET_COUNTER_NAME         "(QUẦY 1)"

// 4. Số thứ tự (STT) & Tự động tăng
#define TICKET_START_NUMBER         1002    // Số bắt đầu khi khởi động (Ví dụ: 1002 giống phiếu mẫu)
#define TICKET_MAX_NUMBER           9999    // Quay vòng về START_NUMBER khi vượt quá

// 5. Lời nhắn / Hướng dẫn khách hàng (Body Notice)
#define TICKET_NOTICE_LINE1         "Quý khách vui lòng chờ đến lượt phục vụ"
#define TICKET_NOTICE_LINE2         "theo STT trên màn hình điện tử. Xin cảm ơn"

// 6. Thông tin chân trang (Footer Metadata)
#define TICKET_CURRENT_CALLING      0       // Số đang phục vụ tại quầy
#define TICKET_EST_WAIT_PER_PERSON  5       // Số phút ước tính cho mỗi lượt chờ

#endif // TICKET_CONFIG_H
