#ifndef THERMAL_PRINTER_H
#define THERMAL_PRINTER_H

#include <stdint.h>
#include <string.h>
#include <algorithm>
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Hàm giới hạn giá trị (thay thế constrain() của Arduino)
template <typename T>
static inline T constrain(T val, T min_val, T max_val) {
    return (val < min_val) ? min_val : ((val > max_val) ? max_val : val);
}

class ThermalPrinter {
private:
    uart_port_t _uart_num;
    
    static const uint8_t ESC = 0x1B;
    static const uint8_t GS  = 0x1D;
    
    struct TextSettings {
        uint8_t size = 1;
        bool bold = false;
        bool underline = false;
        uint8_t align = 0;
        uint8_t lineSpacing = 30;
        uint8_t charSpacing = 0;
    } _currentSettings;
    
    void sendByte(uint8_t data) {
        uart_write_bytes(_uart_num, (const char *)&data, 1);
    }

    void sendBytes(const uint8_t *data, size_t len) {
        if (data && len > 0) {
            uart_write_bytes(_uart_num, (const char *)data, len);
        }
    }

    void updatePrintMode() {
        uint8_t mode = 0;
        if (_currentSettings.bold) mode |= 0x08;
        if (_currentSettings.underline) mode |= 0x80;
        
        // Tính toán kích thước cho cả chiều rộng và chiều cao
        uint8_t widthMultiplier = (_currentSettings.size - 1) & 0x07;
        uint8_t heightMultiplier = ((_currentSettings.size - 1) & 0x07) << 4;
        uint8_t sizeCmd = widthMultiplier | heightMultiplier;

        // Cập nhật chế độ in cơ bản
        sendByte(ESC);
        sendByte(0x21);
        sendByte(mode);
        
        // Cập nhật kích thước ký tự
        sendByte(GS);
        sendByte('!');  // 0x21
        sendByte(sizeCmd);
    }

public:
    // Các hằng số căn lề
    enum Alignment {
        LEFT = 0,    // Căn trái
        CENTER = 1,  // Căn giữa
        RIGHT = 2    // Căn phải
    };
    
    // Khởi tạo đối tượng máy in với cổng UART ESP-IDF (mặc định UART_NUM_1)
    ThermalPrinter(uart_port_t uart_num = UART_NUM_1) : _uart_num(uart_num) {}

    /**
     * @brief Khởi tạo máy in (ESC @)
     */
    void init() {
        sendByte(ESC);
        sendByte('@');
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    /**
     * @brief Đặt kích thước chữ (đồng thời chiều rộng và chiều cao)
     * @param size Hệ số phóng to (1-8)
     */
    void setSize(uint8_t size) {
        _currentSettings.size = constrain<uint8_t>(size, 1, 8);  // Giới hạn kích thước từ 1-8
        updatePrintMode();
    }

    /**
     * @brief Đặt kích thước chữ riêng biệt chiều rộng và chiều cao (1-8)
     */
    void setSize(uint8_t width, uint8_t height) {
        uint8_t w = constrain<uint8_t>(width, 1, 8);
        uint8_t h = constrain<uint8_t>(height, 1, 8);
        uint8_t sizeCmd = ((w - 1) & 0x07) | (((h - 1) & 0x07) << 4);
        sendByte(GS);
        sendByte('!');
        sendByte(sizeCmd);
    }
    
    /**
     * @brief Bật/tắt chế độ in đậm
     * @param enable true để in đậm, false để in thường
     */
    void setBold(bool enable) {
        _currentSettings.bold = enable;
        updatePrintMode();
    }
    
    /**
     * @brief Bật/tắt gạch chân
     * @param enable true để gạch chân, false để bỏ gạch chân
     */
    void setUnderline(bool enable) {
        _currentSettings.underline = enable;
        updatePrintMode();
    }

    /**
     * @brief Bật/tắt chế độ nhân đôi chiều ngang (Double-Width) - Chữ to ngang phủ kín khổ giấy
     */
    void setDoubleWidth(bool enable) {
        uint8_t cmd[] = {ESC, '!', (uint8_t)(enable ? 0x20 : 0x00)};
        sendBytes(cmd, sizeof(cmd));
    }

    /**
     * @brief Bật/tắt chế độ nhân đôi cả chiều ngang và chiều cao (Double-Width + Double-Height + Bold)
     */
    void setDoubleWidthHeight(bool enable) {
        uint8_t cmd[] = {ESC, '!', (uint8_t)(enable ? 0x38 : 0x00)};
        sendBytes(cmd, sizeof(cmd));
    }
    
    /**
     * @brief Đặt căn lề văn bản
     * @param align LEFT (trái), CENTER (giữa), hoặc RIGHT (phải)
     */
    void setAlignment(Alignment align) {
        _currentSettings.align = align;
        sendByte(ESC);
        sendByte(0x61);
        sendByte((uint8_t)align);
    }
    
    /**
     * @brief Đặt khoảng cách giữa các dòng
     * @param spacing Khoảng cách (đơn vị dots)
     */
    void setLineSpacing(uint8_t spacing) {
        _currentSettings.lineSpacing = spacing;
        sendByte(ESC);
        sendByte(0x33);
        sendByte(spacing);
    }
    
    /**
     * @brief Đặt khoảng cách giữa các ký tự
     * @param spacing Khoảng cách 0-13 dots
     */
    void setCharSpacing(uint8_t spacing) {
        _currentSettings.charSpacing = constrain<uint8_t>(spacing, 0, 13);
        sendByte(ESC);
        sendByte(0x52);
        sendByte(_currentSettings.charSpacing);
    }
    
    /**
     * @brief Chọn bảng mã ký tự (Character Code Page: ESC t n)
     * @param codePage 0: CP437, 2: CP850, 16: WCP1252, 255: Vietnamese CP1258
     */
    void setCodePage(uint8_t codePage) {
        sendByte(ESC);
        sendByte('t');
        sendByte(codePage);
    }

    /**
     * @brief In hình ảnh đơn sắc Bitmap (ESC/POS Raster Bit Image: GS v 0)
     * @param bitmap Mảng byte dữ liệu ảnh 1-bit (1: đen, 0: trắng)
     * @param widthDots Chiều rộng ảnh tính bằng dots (chia hết cho 8)
     * @param heightDots Chiều cao ảnh tính bằng dots
     * @param mode 0: Bình thường, 1: Gấp đôi chiều rộng, 2: Gấp đôi chiều cao, 3: Cả hai
     */
    void printBitmap(const uint8_t *bitmap, uint16_t widthDots, uint16_t heightDots, uint8_t mode = 0) {
        if (!bitmap || widthDots == 0 || heightDots == 0) return;
        
        uint16_t widthBytes = (widthDots + 7) / 8;
        uint8_t xL = (uint8_t)(widthBytes & 0xFF);
        uint8_t xH = (uint8_t)((widthBytes >> 8) & 0xFF);
        uint8_t yL = (uint8_t)(heightDots & 0xFF);
        uint8_t yH = (uint8_t)((heightDots >> 8) & 0xFF);

        sendByte(GS);
        sendByte('v');
        sendByte('0');
        sendByte(mode);
        sendByte(xL);
        sendByte(xH);
        sendByte(yL);
        sendByte(yH);

        size_t totalBytes = (size_t)widthBytes * heightDots;
        uart_write_bytes(_uart_num, (const char *)bitmap, totalBytes);
    }

    /**
     * @brief Cắt giấy
     * @param partial true để cắt một phần, false để cắt hoàn toàn
     */
    void cut(bool partial = true) {
        sendByte(GS);
        sendByte(0x56);
        sendByte(66);
        sendByte(partial ? 1 : 0);
    }
    
    /**
     * @brief In văn bản (chuỗi)
     * @param str Chuỗi cần in
     */
    void print(const char* str) {
        if (!str) return;
        size_t len = strlen(str);
        if (len > 0) {
            uart_write_bytes(_uart_num, str, len);
        }
    }
    
    /**
     * @brief In văn bản và xuống dòng (CR + LF)
     * @param str Chuỗi cần in
     */
    void println(const char* str = "") {
        if (str && strlen(str) > 0) {
            print(str);
        }
        sendByte('\r');
        sendByte('\n');
    }

    /**
     * @brief In đường kẻ gạch ngang phân cách
     */
    void line(const char* line_str = "--------------------------------") {
        println(line_str);
    }
    
    /**
     * @brief Áp dụng kiểu tiêu đề (chữ to, đậm, căn giữa)
     */
    void useHeaderStyle() {
        setSize(2);              // Cỡ chữ gấp đôi
        setBold(true);           // In đậm
        setAlignment(CENTER);    // Căn giữa
        setLineSpacing(50);      // Tăng khoảng cách dòng
    }
    
    /**
     * @brief Áp dụng kiểu văn bản thường
     */
    void useBodyStyle() {
        setSize(1);              // Cỡ chữ bình thường
        setBold(false);          // Không in đậm
        setAlignment(LEFT);      // Căn trái
        setLineSpacing(30);      // Khoảng cách dòng bình thường
        setCharSpacing(0);       // Không thêm khoảng cách ký tự
    }
    
    /**
     * @brief Đặt lại tất cả thiết lập về mặc định
     */
    void resetSettings() {
        _currentSettings = TextSettings();
        updatePrintMode();
        setAlignment(LEFT);
        setLineSpacing(30);
        setCharSpacing(0);
    }
};

#endif // THERMAL_PRINTER_H
