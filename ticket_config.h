#ifndef TICKET_CONFIG_H
#define TICKET_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// FILE CẤU HÌNH THÔNG TIN IN TRÊN PHIẾU (QMS TICKET CONFIGURATION)
// ============================================================================
// HƯỚNG DẪN:
// - Muốn KHÔNG IN dòng nào, bạn chỉ cần thêm 2 dấu gạch chéo // ở đầu dòng đó
//   (hoặc gán chuỗi rỗng: "").
// - Máy in sẽ TỰ ĐỘNG BỎ QUA dòng đó, hoàn toàn KHÔNG để lại khoảng trống thừa!
// - Không bao giờ lo bị lỗi biên dịch khi comment bỏ bất kỳ dòng nào.
// ============================================================================

// ----------------------------------------------------------------------------
// 1. TIÊU ĐỀ ĐƠN VỊ / CƠ QUAN (Comment // dòng nào để KHÔNG IN dòng đó)
// ----------------------------------------------------------------------------
#define TICKET_HEADER_LINE1         "ỦY BAN NHÂN DÂN XÃ CHÂU THÀNH"
#define TICKET_HEADER_LINE2         "TRUNG TÂM PHỤC VỤ HÀNH CHÍNH CÔNG"
//#define TICKET_HEADER_LINE3       "" // Bỏ // nếu muốn thêm dòng tiêu đề thứ 3
#define TICKET_HEADER_ADDRESS       "79 Tân Túc, xã Tân Nhựt, TP. Hồ Chí Minh"

// Đường gạch ngang phân cách tiêu đề (Comment // hoặc đặt false để không in)
#define ENABLE_HEADER_DIVIDER       true

// ----------------------------------------------------------------------------
// 2. LĨNH VỰC & QUẦY PHỤC VỤ (Comment // dòng nào để KHÔNG IN dòng đó)
// ----------------------------------------------------------------------------
#define TICKET_SERVICE_NAME         "LĨNH VỰC BHXH,BHYT,BHTN,BHTNLD,BNN"
#define TICKET_COUNTER_NAME         "(QUẦY 1)"

// ----------------------------------------------------------------------------
// 3. SỐ THỨ TỰ (STT HERO NUMBER)
// ----------------------------------------------------------------------------
#define TICKET_START_NUMBER         1002
#define TICKET_MAX_NUMBER           9999
#define STT_FONT_SIZE               3       // Cỡ chữ STT: 3 hoặc 4 (to rõ, nổi bật)
#define ENABLE_STT_PADDING          true    // true: Khoảng đệm trước & sau STT cho thoáng đẹp

// ----------------------------------------------------------------------------
// 4. LỜI NHẮN / HƯỚNG DẪN KHÁCH HÀNG (Comment // dòng nào để KHÔNG IN dòng đó)
// ----------------------------------------------------------------------------
#define TICKET_NOTICE_LINE1         "Quý khách vui lòng chờ đến lượt phục vụ"
#define TICKET_NOTICE_LINE2         "theo STT trên màn hình điện tử. Xin cảm ơn"
//#define TICKET_NOTICE_LINE3       "" // Bỏ // nếu muốn thêm dòng hướng dẫn 3

// ----------------------------------------------------------------------------
// 5. THÔNG TIN CHÂN TRANG / FOOTER (Comment // dòng nào để KHÔNG IN dòng đó)
// ----------------------------------------------------------------------------
#define ENABLE_FOOTER_DIVIDER       true    // true: Đường gạch ngang trước footer (hoặc // để tắt)
#define ENABLE_FOOTER_TIME_INFO     true    // true: Dòng thời gian in & số đang gọi (hoặc // để tắt)
#define ENABLE_FOOTER_WAIT_INFO     true    // true: Dòng ước tính lượt & thời gian chờ (hoặc // để tắt)
#define TICKET_CURRENT_CALLING      0       // Số thứ tự đang được gọi hiện tại
#define TICKET_EST_WAIT_PER_PERSON  5       // Số phút ước tính phục vụ 1 người (phút/lượt)

// ----------------------------------------------------------------------------
// 6. CẤU HÌNH KHOẢNG CÁCH DÒNG & THÔNG SỐ IN
// ----------------------------------------------------------------------------
// Khoảng cách dòng chuẩn (Line Spacing):
// - 38 dots: Khoảng cách tiêu chuẩn thoáng đãng, đẹp mắt.
// - Muốn các hàng giãn rộng hơn nữa: 42 - 50 dots.
// - Muốn các hàng sát lại một chút: 30 - 34 dots.
#define TICKET_LINE_SPACING_DOTS    38

// Khoảng cách dòng phần lời nhắn hướng dẫn:
#define TICKET_NOTICE_SPACING_DOTS  36

// Khổ giấy: 58 (58mm) hoặc 80 (80mm)
#define PRINTER_PAPER_WIDTH_MM      58

// Chế độ in logo: false = BỎ LOGO để tiêu đề rộng hết cỡ khổ giấy
#define ENABLE_PRINT_LOGO           false

// Tùy chọn Tiếng Việt: false = lọc không dấu chuẩn máy in tránh ký tự lạ
#define ENABLE_VIETNAMESE_ACCENT    false

// Chế độ Header: 0 = Văn bản động tràn khổ giấy, 1 = Ảnh Bitmap
#define TICKET_HEADER_MODE          0


// ============================================================================
// GIÁ TRỊ DỰ PHÒNG TỰ ĐỘNG (FALLBACK AN TOÀN KHI BỊ COMMENT // BẤT KỲ DÒNG NÀO)
// ============================================================================
#ifndef TICKET_HEADER_LINE1
#define TICKET_HEADER_LINE1         ""
#endif

#ifndef TICKET_HEADER_LINE2
#define TICKET_HEADER_LINE2         ""
#endif

#ifndef TICKET_HEADER_LINE3
#define TICKET_HEADER_LINE3         ""
#endif

#ifndef TICKET_HEADER_ADDRESS
#define TICKET_HEADER_ADDRESS       ""
#endif

#ifndef ENABLE_HEADER_DIVIDER
#define ENABLE_HEADER_DIVIDER       false
#endif

#ifndef TICKET_SERVICE_NAME
#define TICKET_SERVICE_NAME         ""
#endif

#ifndef TICKET_COUNTER_NAME
#define TICKET_COUNTER_NAME         ""
#endif

#ifndef TICKET_START_NUMBER
#define TICKET_START_NUMBER         1002
#endif

#ifndef TICKET_MAX_NUMBER
#define TICKET_MAX_NUMBER           9999
#endif

#ifndef STT_FONT_SIZE
#define STT_FONT_SIZE               3
#endif

#ifndef ENABLE_STT_PADDING
#define ENABLE_STT_PADDING          false
#endif

#ifndef TICKET_NOTICE_LINE1
#define TICKET_NOTICE_LINE1         ""
#endif

#ifndef TICKET_NOTICE_LINE2
#define TICKET_NOTICE_LINE2         ""
#endif

#ifndef TICKET_NOTICE_LINE3
#define TICKET_NOTICE_LINE3         ""
#endif

#ifndef ENABLE_FOOTER_DIVIDER
#define ENABLE_FOOTER_DIVIDER       false
#endif

#ifndef ENABLE_FOOTER_TIME_INFO
#define ENABLE_FOOTER_TIME_INFO     false
#endif

#ifndef ENABLE_FOOTER_WAIT_INFO
#define ENABLE_FOOTER_WAIT_INFO     false
#endif

#ifndef TICKET_CURRENT_CALLING
#define TICKET_CURRENT_CALLING      0
#endif

#ifndef TICKET_EST_WAIT_PER_PERSON
#define TICKET_EST_WAIT_PER_PERSON  5
#endif

#ifndef TICKET_LINE_SPACING_DOTS
#define TICKET_LINE_SPACING_DOTS    38
#endif

#ifndef TICKET_NOTICE_SPACING_DOTS
#define TICKET_NOTICE_SPACING_DOTS  36
#endif

#ifndef PRINTER_PAPER_WIDTH_MM
#define PRINTER_PAPER_WIDTH_MM      58
#endif

#ifndef ENABLE_PRINT_LOGO
#define ENABLE_PRINT_LOGO           false
#endif

#ifndef ENABLE_VIETNAMESE_ACCENT
#define ENABLE_VIETNAMESE_ACCENT    false
#endif

#ifndef TICKET_HEADER_MODE
#define TICKET_HEADER_MODE          0
#endif

#endif // TICKET_CONFIG_H
