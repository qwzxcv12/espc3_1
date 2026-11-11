/**
 * ============================================================================
 * HỆ THỐNG LẤY SỐ TỰ ĐỘNG (QMS KIOSK) - ESP32-C3 ESP-IDF v5.1+
 * In phiếu qua cổng COM / UART Máy in nhiệt ESC/POS (hoặc Module RS232-to-TTL)
 * ============================================================================
 * 
 * PHẦN CỨNG:
 * - UART1 Máy in nhiệt ESC/POS (hoặc Module RS232-to-TTL):
 *     TX_PIN : GPIO 3 (Nối chân RX của Máy in / RXD Module RS232)
 *     RX_PIN : GPIO 4 (Nối chân TX của Máy in / TXD Module RS232)
 *     Baud   : 9600, 8N1
 * - WS2812 / Status LED:
 *     LED_PIN: GPIO 8 (Onboard LED trên ESP32-C3)
 * - Nút nhấn dịch vụ:
 *     GPIO 0, 1, 2, 5, 6, 7, 10 (Chế độ INPUT_PULLUP, nhấn nối GND = LOW)
 * - Cổng COM Serial Console:
 *     USB Type-C onboard của ESP32-C3 (gõ PRINT, LOGO, CONFIG, HELP)
 * ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <string>
#include <vector>
#include <algorithm>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "mqtt_client.h"
#include "cJSON.h"

// File cấu hình thông tin in & Logo bitmap
#include "ticket_config.h"
#include "logo_bitmap.h"
#include "thermal_printer.h"

#if __has_include("led_strip.h")
#include "led_strip.h"
#define HAS_LED_STRIP 1
#else
#define HAS_LED_STRIP 0
#endif

static const char *TAG = "QMS_C3";

// ================== CẤU HÌNH PHẦN CỨNG ==================
#define LED_PIN         GPIO_NUM_8       // Onboard LED ESP32-C3
#define NUM_LEDS        1
#define TX_PIN          GPIO_NUM_3       // TX UART1 (Gửi lệnh in ESC/POS)
#define RX_PIN          GPIO_NUM_4       // RX UART1
#define PRINTER_UART    UART_NUM_1
#define PRINTER_BAUD    9600

// ================== CẤU HÌNH NÚT NHẤN (ESP32-C3) ==================
static const gpio_num_t BUTTON_PINS[] = {
    GPIO_NUM_0,   // Nút 1 (Nhấn để in phiếu)
    GPIO_NUM_1,   // Nút 2
    GPIO_NUM_2,   // Nút 3
    GPIO_NUM_5,   // Nút 4
    GPIO_NUM_6,   // Nút 5
    GPIO_NUM_7,   // Nút 6
    GPIO_NUM_10   // Nút 7
};
static const int NUM_BUTTONS = sizeof(BUTTON_PINS) / sizeof(BUTTON_PINS[0]);

// ================== WIFI & MQTT ==================
static const char *WIFI_SSID     = "HG156";
static const char *WIFI_PASS     = "hg123456!";
static const char *MQTT_SERVER   = "qms2.camdvr.org";
static const int   MQTT_PORT     = 1993;
static const char *MQTT_USER     = "thom";
static const char *MQTT_PASS     = "301258";

// ================== ĐỊNH DANH THIẾT BỊ ==================
static const char *DEVICE_ID     = "1a66322e8825d4d0ebebd49bf245edfbaa7c77ece266b32f50b09d76b1fc560b";
static const char *SECRET_KEY    = "592aa2019867361c72b09941184cfca86563db35fd91d190fc7537f72e66dbc3";

// ================== DỊCH VỤ ==================
struct ServiceItem {
    int id;
    char name[96];
};

static ServiceItem services[15];
static int serviceCount = 0;

// Biến lưu số thứ tự hiện tại (tự tăng sau mỗi lần in)
static int current_ticket_counter = TICKET_START_NUMBER;

// ================== TRẠNG THÁI HỆ THỐNG ==================
static bool isWaitingForTicket = false;
static uint32_t lastRequestTime = 0;
static int retryCount = 0;
static const uint32_t TIMEOUT_MS = 1500;
static int currentRequestedServiceIndex = -1;

static std::string lastTopic = "";
static std::string lastPayload = "";

// Nút nhấn & Debounce
static int lastButtonState[NUM_BUTTONS];
static int buttonState[NUM_BUTTONS];
static uint32_t lastDebounceTime[NUM_BUTTONS];
static const uint32_t DEBOUNCE_DELAY_MS = 50;

// Đối tượng máy in ESC/POS
static ThermalPrinter printer(PRINTER_UART);

// FreeRTOS Handles
static esp_mqtt_client_handle_t mqtt_client = NULL;
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

#if HAS_LED_STRIP
static led_strip_handle_t led_strip = NULL;
#endif

// ================== HÀM THỜI GIAN MILLIS TƯƠNG ĐƯƠNG ARDUINO ==================
static inline uint32_t millis(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

// ================== ĐÈN LED (WS2812 HOẶC GPIO) ==================
static void init_led(void)
{
#if HAS_LED_STRIP
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_PIN,
        .max_leds = NUM_LEDS,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags = {
            .invert_out = false,
        }
    };
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags = {
            .with_dma = false,
        }
    };
    if (led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip) == ESP_OK) {
        led_strip_clear(led_strip);
        return;
    }
#endif
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 1);
}

void setLEDColor(int r, int g, int b)
{
#if HAS_LED_STRIP
    if (led_strip) {
        led_strip_set_pixel(led_strip, 0, r / 5, g / 5, b / 5);
        led_strip_refresh(led_strip);
        return;
    }
#endif
    gpio_set_level(LED_PIN, (r == 0 && g == 0 && b == 0) ? 1 : 0);
}

// ================== BỘ LỌC CHỮ KHÔNG DẤU TIẾNG VIỆT ==================
std::string removeDiacritics(const std::string &input)
{
    std::string output = "";

    const char *viet_lower[] = {
        "à", "a", "á", "a", "ả", "a", "ã", "a", "ạ", "a",
        "ă", "a", "ằ", "a", "ắ", "a", "ẳ", "a", "ẵ", "a", "ặ", "a",
        "â", "a", "ầ", "a", "ấ", "a", "ẩ", "a", "ẫ", "a", "ậ", "a",
        "è", "e", "é", "e", "ẻ", "e", "ẽ", "e", "ẹ", "e",
        "ê", "e", "ề", "e", "ế", "e", "ể", "e", "ễ", "e", "ệ", "e",
        "ì", "i", "í", "i", "ỉ", "i", "ĩ", "i", "ị", "i",
        "ò", "o", "ó", "o", "ỏ", "o", "õ", "o", "ọ", "o",
        "ô", "o", "ồ", "o", "ố", "o", "ổ", "o", "ỗ", "o", "ộ", "o",
        "ơ", "o", "ờ", "o", "ớ", "o", "ở", "o", "ỡ", "o", "ợ", "o",
        "ù", "u", "ú", "u", "ủ", "u", "ũ", "u", "ụ", "u",
        "ư", "u", "ừ", "u", "ứ", "u", "ử", "u", "ữ", "u", "ự", "u",
        "ỳ", "y", "ý", "y", "ỷ", "y", "ỹ", "y", "ỵ", "y",
        "đ", "d"
    };

    const char *viet_upper[] = {
        "À", "A", "Á", "A", "Ả", "A", "Ã", "A", "Ạ", "A",
        "Ă", "A", "Ằ", "A", "Ắ", "A", "Ẳ", "A", "Ẵ", "A", "Ặ", "A",
        "Â", "A", "Ầ", "A", "Ấ", "A", "Ẩ", "A", "Ẫ", "A", "Ậ", "A",
        "È", "E", "É", "E", "Ẻ", "E", "Ẽ", "E", "Ẹ", "E",
        "Ê", "E", "Ề", "E", "Ế", "E", "Ể", "E", "Ễ", "E", "Ệ", "E",
        "Ì", "I", "Í", "I", "Ỉ", "I", "Ĩ", "I", "Ị", "I",
        "Ò", "O", "Ó", "O", "Ỏ", "O", "Õ", "O", "Ọ", "O",
        "Ô", "O", "Ồ", "O", "Ố", "O", "Ổ", "O", "Ỗ", "O", "Ộ", "O",
        "Ơ", "O", "Ờ", "O", "Ớ", "O", "Ở", "O", "Ỡ", "O", "Ợ", "O",
        "Ù", "U", "Ú", "U", "Ủ", "U", "Ũ", "U", "Ụ", "U",
        "Ư", "U", "Ừ", "U", "Ứ", "U", "Ử", "U", "Ữ", "U", "Ự", "U",
        "Ỳ", "Y", "Ý", "Y", "Ỷ", "Y", "Ỹ", "Y", "Ỵ", "Y",
        "Đ", "D"
    };

    int len_lower = sizeof(viet_lower) / sizeof(viet_lower[0]);
    int len_upper = sizeof(viet_upper) / sizeof(viet_upper[0]);

    for (size_t i = 0; i < input.length(); ) {
        bool found = false;

        for (int j = 0; j < len_lower; j += 2) {
            size_t viet_len = strlen(viet_lower[j]);
            if (input.compare(i, viet_len, viet_lower[j]) == 0) {
                output += viet_lower[j + 1];
                i += viet_len;
                found = true;
                break;
            }
        }

        if (!found) {
            for (int j = 0; j < len_upper; j += 2) {
                size_t viet_len = strlen(viet_upper[j]);
                if (input.compare(i, viet_len, viet_upper[j]) == 0) {
                    output += viet_upper[j + 1];
                    i += viet_len;
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            output += input[i];
            i++;
        }
    }

    return output;
}

std::string sanitize(const std::string &input)
{
    std::string cleaned = removeDiacritics(input);

    while (!cleaned.empty() && (cleaned.front() == ' ' || cleaned.front() == '\t' || cleaned.front() == '\r' || cleaned.front() == '\n')) {
        cleaned.erase(cleaned.begin());
    }
    while (!cleaned.empty() && (cleaned.back() == ' ' || cleaned.back() == '\t' || cleaned.back() == '\r' || cleaned.back() == '\n')) {
        cleaned.pop_back();
    }

    for (char &c : cleaned) {
        c = (char)toupper((unsigned char)c);
    }

    std::string result = "";
    for (char c : cleaned) {
        if ((c >= 32 && c <= 126) || c == '\n' || c == '\r') {
            result += c;
        }
    }

    return result;
}

/**
 * @brief In văn bản có kiểm soát Tiếng Việt có dấu / không dấu từ ticket_config.h
 */
static void printFormattedText(const char *text, bool newline = true)
{
    if (!text) return;
    if (ENABLE_VIETNAMESE_ACCENT) {
        // In trực tiếp chuỗi UTF-8 (nếu máy in hỗ trợ tiếng Việt)
        if (newline) printer.println(text);
        else printer.print(text);
    } else {
        // Tự động lọc dấu sang ASCII chuẩn (nếu máy in chỉ hỗ trợ ASCII)
        std::string unaccented = removeDiacritics(text);
        if (newline) printer.println(unaccented.c_str());
        else printer.print(unaccented.c_str());
    }
}

// ============================================================================
// HÀM IN PHIẾU CHUẨN THEO MẪU (GỒM LOGO, TIÊU ĐỀ, STT TO, LỜI NHẮN, FOOTER)
// ============================================================================
void printSampleTicket(int ticket_num = -1)
{
    if (ticket_num <= 0) {
        ticket_num = current_ticket_counter++;
        if (current_ticket_counter > TICKET_MAX_NUMBER) {
            current_ticket_counter = TICKET_START_NUMBER;
        }
    }

    ESP_LOGI(TAG, ">>> DANG IN PHIEU THEO MAU | SO THU TU: %d <<<", ticket_num);

    printer.init();
    printer.resetSettings();

    // 1. IN LOGO HÌNH TRÒN (nếu được bật trong ticket_config.h)
#if ENABLE_PRINT_LOGO
    printer.setAlignment(ThermalPrinter::CENTER);
    printer.printBitmap(LOGO_BITMAP, LOGO_WIDTH_DOTS, LOGO_HEIGHT_DOTS);
    printer.println();
#endif

    // 2. TIÊU ĐỀ ĐƠN VỊ / CƠ QUAN
    printer.setAlignment(ThermalPrinter::CENTER);
    printer.setBold(true);
    printer.setSize(1);

    printFormattedText(TICKET_HEADER_LINE1, true);
    printFormattedText(TICKET_HEADER_LINE2, true);

    printer.setBold(false);
    printFormattedText(TICKET_HEADER_ADDRESS, true);

    // 3. ĐƯỜNG KẺ PHÂN CÁCH
    printer.line("================================");

    // 4. LĨNH VỰC & QUẦY
    printer.setBold(true);
    printFormattedText(TICKET_SERVICE_NAME, true);
    printFormattedText(TICKET_COUNTER_NAME, true);
    printer.setBold(false);

    // 5. SỐ THỨ TỰ (STT) TO CĂN GIỮA
    printer.println();
    printer.setBold(true);
    printer.setSize(5); // Chữ cỡ 5 rất to giống hệt phiếu mẫu
    char num_buf[16];
    snprintf(num_buf, sizeof(num_buf), "%d", ticket_num);
    printer.println(num_buf);
    printer.setSize(1);
    printer.setBold(false);
    printer.println();

    // 6. LỜI NHẮN / HƯỚNG DẪN KHÁCH HÀNG
    printer.setAlignment(ThermalPrinter::CENTER);
    printFormattedText(TICKET_NOTICE_LINE1, true);
    printFormattedText(TICKET_NOTICE_LINE2, true);
    printer.println();

    // 7. CHÂN TRANG (FOOTER METADATA)
    printer.setAlignment(ThermalPrinter::LEFT);

    char time_str[64];
    snprintf(time_str, sizeof(time_str), "STT in luc: 13:10 - 11/11/2026   Dang goi: %d", TICKET_CURRENT_CALLING);
    printFormattedText(time_str, true);

    int waiting_turns = ticket_num - TICKET_CURRENT_CALLING;
    if (waiting_turns < 0) waiting_turns = 1;
    int est_minutes = waiting_turns * TICKET_EST_WAIT_PER_PERSON;

    char wait_str[64];
    snprintf(wait_str, sizeof(wait_str), "Con cho %d luot - khoang %d phut nua (uoc tinh)", waiting_turns, est_minutes);
    printFormattedText(wait_str, true);

    // 8. KẾT THÚC VÀ CẮT GIẤY
    printer.println();
    printer.println();
    printer.println();
    printer.cut();

    ESP_LOGI(TAG, ">>> DA IN PHIEU %d THANH CONG! <<<", ticket_num);
}

// ================== IN PHIẾU LỖI ==================
void printErrorTicket(const std::string &serviceName)
{
    printer.init();
    printer.resetSettings();

    printer.setAlignment(ThermalPrinter::CENTER);
    printer.setSize(2);
    printer.setBold(true);
    printer.println("HE THONG LAY SO TU DONG");
    printer.setBold(false);

    printer.line();

    printer.setSize(3);
    printer.println("THONG BAO");
    printer.setSize(1);

    printer.line();

    printer.setAlignment(ThermalPrinter::CENTER);
    printer.println();

    printer.setSize(2);
    printer.println("DICH VU");
    printer.setSize(1);
    printer.println();

    printer.setBold(true);
    printer.println(serviceName.c_str());
    printer.setBold(false);

    printer.println();

    printer.setSize(2);
    printer.println("DANG TAM NGUNG");
    printer.println("PHUC VU");
    printer.setSize(1);

    printer.println();
    printer.line();

    printer.println();
    printer.println("Rat tiec vi su bat tien nay");
    printer.println();
    printer.setBold(true);
    printer.setSize(2);
    printer.println("XIN CAM ON!");
    printer.setSize(1);
    printer.setBold(false);

    printer.println();
    printer.println("Vui long quay lai sau hoac");
    printer.println("lien he quay thong tin");

    printer.println();
    printer.println();
    printer.cut();
}

// ================== IN PHIẾU TỪ JSON MQTT ==================
void printTicketFromJSON(cJSON *doc)
{
    printer.init();
    printer.resetSettings();

#if ENABLE_PRINT_LOGO
    printer.setAlignment(ThermalPrinter::CENTER);
    printer.printBitmap(LOGO_BITMAP, LOGO_WIDTH_DOTS, LOGO_HEIGHT_DOTS);
    printer.println();
#endif

    cJSON *item_unit = cJSON_GetObjectItem(doc, "unit_name");
    cJSON *item_ticket = cJSON_GetObjectItem(doc, "ticket_number");
    cJSON *item_service = cJSON_GetObjectItem(doc, "service_name");

    cJSON *item_call = cJSON_GetObjectItem(doc, "calling_ticket");
    if (!item_call) item_call = cJSON_GetObjectItem(doc, "current_call");

    cJSON *item_wait = cJSON_GetObjectItem(doc, "waiting_count");
    cJSON *item_date = cJSON_GetObjectItem(doc, "date");
    cJSON *item_time = cJSON_GetObjectItem(doc, "time");

    std::string unitName = sanitize(item_unit && item_unit->valuestring ? item_unit->valuestring : TICKET_HEADER_LINE1);
    std::string ticketNum = sanitize(item_ticket && item_ticket->valuestring ? item_ticket->valuestring : "1002");
    std::string serviceName = sanitize(item_service && item_service->valuestring ? item_service->valuestring : TICKET_SERVICE_NAME);

    std::string currentCall = "";
    if (item_call && item_call->valuestring) {
        currentCall = sanitize(item_call->valuestring);
    }
    if (currentCall.empty()) {
        currentCall = "--";
    }

    int waitCountVal = item_wait ? item_wait->valueint : 0;
    std::string waitCount = std::to_string(waitCountVal);

    std::string dateStr = sanitize(item_date && item_date->valuestring ? item_date->valuestring : "");
    std::string timeStr = sanitize(item_time && item_time->valuestring ? item_time->valuestring : "");

    std::string dateTime = "";
    if (!dateStr.empty() && !timeStr.empty()) {
        dateTime = dateStr + " " + timeStr;
    } else if (!timeStr.empty()) {
        dateTime = timeStr;
    } else {
        dateTime = "--";
    }

    printer.setAlignment(ThermalPrinter::CENTER);
    printer.setBold(true);
    printer.setSize(1);
    printFormattedText(unitName.c_str(), true);
    printFormattedText(TICKET_HEADER_LINE2, true);
    printer.setBold(false);
    printFormattedText(TICKET_HEADER_ADDRESS, true);

    printer.line("================================");

    printer.setBold(true);
    printFormattedText(serviceName.c_str(), true);
    printFormattedText(TICKET_COUNTER_NAME, true);
    printer.setBold(false);

    printer.println();
    printer.setBold(true);
    printer.setSize(5);
    printer.println(ticketNum.c_str());
    printer.setSize(1);
    printer.setBold(false);
    printer.println();

    printer.setAlignment(ThermalPrinter::CENTER);
    printFormattedText(TICKET_NOTICE_LINE1, true);
    printFormattedText(TICKET_NOTICE_LINE2, true);
    printer.println();

    printer.setAlignment(ThermalPrinter::LEFT);
    printer.print("        So dang goi : ");
    printer.println(currentCall.c_str());

    printer.print("        Dang cho    : ");
    printer.print(waitCount.c_str());
    printer.println(" nguoi");

    printer.print("        Ngay gio    : ");
    printer.println(dateTime.c_str());

    printer.println();
    printer.println();
    printer.cut();
}

// ================== TASK ĐỌC NÚT NHẤN (DEBOUNCE) ==================
static void button_task(void *pvParameters)
{
    while (1) {
        for (int i = 0; i < NUM_BUTTONS; i++) {
            int reading = gpio_get_level(BUTTON_PINS[i]);

            if (reading != lastButtonState[i]) {
                lastDebounceTime[i] = millis();
            }

            if ((millis() - lastDebounceTime[i]) > DEBOUNCE_DELAY_MS) {
                if (reading != buttonState[i]) {
                    buttonState[i] = reading;

                    // Chỉ kích hoạt khi nhấn xuống (HIGH -> LOW)
                    if (buttonState[i] == 0) {
                        ESP_LOGI(TAG, "\n>>> PHAT HIEN NUT NHAN %d (GPIO %d)! TIEN HANH IN 1 PHIEU... <<<", i + 1, BUTTON_PINS[i]);
                        setLEDColor(0, 255, 0); // Xanh lá: Đang in
                        
                        // IN PHIẾU NGAY LẬP TỨC
                        printSampleTicket();
                        
                        vTaskDelay(pdMS_TO_TICKS(500));
                        setLEDColor(0, 0, 255); // Trở về xanh dương
                    }
                }
            }

            lastButtonState[i] = reading;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ================== TASK LỆNH QUA CỔNG COM / SERIAL MONITOR ==================
static void console_command_task(void *pvParameters)
{
    char line[128];
    ESP_LOGI(TAG, "--------------------------------------------------");
    ESP_LOGI(TAG, "  COM COMMAND READY: Ban co the go cac lenh sau:");
    ESP_LOGI(TAG, "    - 'PRINT' hoac phim Enter : In 1 phieu tiep theo");
    ESP_LOGI(TAG, "    - '<so>' (vi du: 1002)    : In phieu voi so chi dinh");
    ESP_LOGI(TAG, "    - 'LOGO'                  : In thu rieng Logo");
    ESP_LOGI(TAG, "    - 'CONFIG'                : Xem thong tin ticket_config.h");
    ESP_LOGI(TAG, "--------------------------------------------------");

    while (1) {
        if (fgets(line, sizeof(line), stdin)) {
            char *p = line;
            while (*p == ' ' || *p == '\r' || *p == '\n') p++;
            char *end = p + strlen(p) - 1;
            while (end >= p && (*end == ' ' || *end == '\r' || *end == '\n')) {
                *end = '\0';
                end--;
            }

            if (strlen(p) == 0 || strcasecmp(p, "PRINT") == 0 || strcasecmp(p, "P") == 0) {
                ESP_LOGI(TAG, "Nhan lenh PRINT tu cong COM -> Dang in phieu...");
                setLEDColor(0, 255, 0);
                printSampleTicket();
                vTaskDelay(pdMS_TO_TICKS(500));
                setLEDColor(0, 0, 255);
            } else if (strcasecmp(p, "LOGO") == 0) {
                ESP_LOGI(TAG, "Nhan lenh LOGO tu cong COM -> In thu Logo...");
                printer.init();
                printer.setAlignment(ThermalPrinter::CENTER);
                printer.printBitmap(LOGO_BITMAP, LOGO_WIDTH_DOTS, LOGO_HEIGHT_DOTS);
                printer.println("\n\n");
                printer.cut();
            } else if (strcasecmp(p, "CONFIG") == 0) {
                ESP_LOGI(TAG, "=== THONG TIN TRONG ticket_config.h ===");
                ESP_LOGI(TAG, "Header 1 : %s", TICKET_HEADER_LINE1);
                ESP_LOGI(TAG, "Header 2 : %s", TICKET_HEADER_LINE2);
                ESP_LOGI(TAG, "Dia chi  : %s", TICKET_HEADER_ADDRESS);
                ESP_LOGI(TAG, "Linh vuc : %s", TICKET_SERVICE_NAME);
                ESP_LOGI(TAG, "Quay     : %s", TICKET_COUNTER_NAME);
                ESP_LOGI(TAG, "In Logo  : %s", ENABLE_PRINT_LOGO ? "BAT" : "TAT");
                ESP_LOGI(TAG, "Tieng Viet: %s", ENABLE_VIETNAMESE_ACCENT ? "CO DAU" : "KHONG DAU");
                ESP_LOGI(TAG, "=======================================");
            } else if (isdigit((unsigned char)p[0])) {
                int custom_num = atoi(p);
                if (custom_num > 0) {
                    ESP_LOGI(TAG, "In phieu voi so chi dinh: %d", custom_num);
                    setLEDColor(0, 255, 0);
                    printSampleTicket(custom_num);
                    vTaskDelay(pdMS_TO_TICKS(500));
                    setLEDColor(0, 0, 255);
                }
            } else if (strcasecmp(p, "HELP") == 0) {
                ESP_LOGI(TAG, "--- DANH SACH LENH COM ---");
                ESP_LOGI(TAG, "  PRINT (hoac Enter) : In 1 phieu");
                ESP_LOGI(TAG, "  <so> (vd: 1002)    : In phieu so 1002");
                ESP_LOGI(TAG, "  LOGO               : In thu Logo");
                ESP_LOGI(TAG, "  CONFIG             : Xem cau hinh");
                ESP_LOGI(TAG, "--------------------------");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// ================== XỬ LÝ MQTT PAYLOAD ==================
static void handle_mqtt_message(const char *topic, int topic_len, const char *payload, int payload_len)
{
    cJSON *doc = cJSON_ParseWithLength(payload, payload_len);
    if (!doc) return;

    cJSON *cmd_item = cJSON_GetObjectItem(doc, "cmd");
    if (cmd_item && cmd_item->valuestring) {
        const char *cmd = cmd_item->valuestring;

        if (strcmp(cmd, "print_ticket") == 0) {
            isWaitingForTicket = false;
            retryCount = 0;
            currentRequestedServiceIndex = -1;
            setLEDColor(0, 255, 0);
            printTicketFromJSON(doc);
            vTaskDelay(pdMS_TO_TICKS(1000));
            setLEDColor(0, 0, 255);
        } else if (strcmp(cmd, "init_config") == 0) {
            cJSON *services_arr = cJSON_GetObjectItem(doc, "services");
            if (services_arr && cJSON_IsArray(services_arr)) {
                serviceCount = 0;
                int sz = cJSON_GetArraySize(services_arr);
                for (int i = 0; i < sz && serviceCount < 15; i++) {
                    cJSON *s = cJSON_GetArrayItem(services_arr, i);
                    cJSON *s_id = cJSON_GetObjectItem(s, "id");
                    cJSON *s_name = cJSON_GetObjectItem(s, "name");
                    if (s_id && s_name && s_name->valuestring) {
                        services[serviceCount].id = s_id->valueint;
                        strncpy(services[serviceCount].name, s_name->valuestring, sizeof(services[serviceCount].name) - 1);
                        services[serviceCount].name[sizeof(services[serviceCount].name) - 1] = '\0';
                        serviceCount++;
                    }
                }
            }
        }
    }

    cJSON_Delete(doc);
}

// ================== MQTT EVENT HANDLER ==================
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED: {
        ESP_LOGI(TAG, "MQTT Connected OK!");
        setLEDColor(0, 0, 255);

        std::string sub_topic = std::string("qms/sender/") + DEVICE_ID + "/#";
        esp_mqtt_client_subscribe(mqtt_client, sub_topic.c_str(), 0);

        std::string req_topic = std::string("qms/sender/") + DEVICE_ID + "/request";
        std::string req_payload = std::string("{\"cmd\":\"get_config\",\"secret_key\":\"") + SECRET_KEY + "\"}";
        esp_mqtt_client_publish(mqtt_client, req_topic.c_str(), req_payload.c_str(), 0, 0, 0);
        break;
    }

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT Disconnected");
        setLEDColor(255, 255, 0);
        break;

    case MQTT_EVENT_DATA:
        handle_mqtt_message(event->topic, event->topic_len, event->data, event->data_len);
        break;

    default:
        break;
    }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.hostname = MQTT_SERVER;
    mqtt_cfg.broker.address.port = MQTT_PORT;
    mqtt_cfg.broker.address.transport = MQTT_TRANSPORT_OVER_TCP;
    mqtt_cfg.credentials.client_id = DEVICE_ID;
    mqtt_cfg.credentials.username = MQTT_USER;
    mqtt_cfg.credentials.authentication.password = MQTT_PASS;
    mqtt_cfg.buffer.size = 2048;

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

// ================== WIFI EVENT HANDLER ==================
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Dang ket noi WiFi: %s...", WIFI_SSID);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "WiFi Connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        // Khi co IP, tu dong khoi dong MQTT
        mqtt_app_start();
    }
}

static void wifi_init_sta(void)
{
    wifi_event_group = xEventGroupCreate();

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_config = {};
    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

// ================== KHỞI TẠO UART1 (MÁY IN / RS232) ==================
static void printer_uart_init(void)
{
    ESP_LOGI(TAG, "Khoi tao UART1 cho May in ESC/POS...");
    ESP_LOGI(TAG, "TX_PIN: GPIO %d (Nối RX May in), RX_PIN: GPIO %d, Baud: %d", TX_PIN, RX_PIN, PRINTER_BAUD);

    uart_config_t uart_config = {
        .baud_rate = PRINTER_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_param_config(PRINTER_UART, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(PRINTER_UART, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(PRINTER_UART, 1024, 1024, 0, NULL, 0));
}

// ================== APP_MAIN ==================
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "\n\n");
    ESP_LOGI(TAG, "================================================");
    ESP_LOGI(TAG, "   HE THONG LAY SO TU DONG v2.2 (ESP32-C3)      ");
    ESP_LOGI(TAG, "   - In qua COM / Phim bam 1 cham               ");
    ESP_LOGI(TAG, "   - Ho tro in Logo Bitmap va Tieng Viet        ");
    ESP_LOGI(TAG, "================================================");

    // 1. Khoi tao NVS Flash
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "NVS Flash init: %d", ret);
    }

    // 2. Khoi tao LED
    init_led();
    setLEDColor(0, 0, 255); // Xanh duong: Khoi dong xong

    // 3. Khoi tao UART1 cho may in nhiet
    printer_uart_init();

    // 4. Cau hinh GPIO cho cac nut nhan
    ESP_LOGI(TAG, "Cau hinh cac nut nhan:");
    for (int i = 0; i < NUM_BUTTONS; i++) {
        gpio_reset_pin(BUTTON_PINS[i]);
        gpio_set_direction(BUTTON_PINS[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode(BUTTON_PINS[i], GPIO_PULLUP_ONLY);

        buttonState[i] = 1;
        lastButtonState[i] = 1;
        lastDebounceTime[i] = 0;
        ESP_LOGI(TAG, "  - Nut %d: GPIO %d", i + 1, BUTTON_PINS[i]);
    }

    // 5. Tao Task doc nut nhan (nhan nut in 1 phieu ngay lap tuc)
    xTaskCreate(button_task, "button_task", 3072, NULL, 10, NULL);

    // 6. Tao Task nhan lenh dieu khien in qua cong COM
    xTaskCreate(console_command_task, "console_task", 3072, NULL, 5, NULL);

    // 7. Khoi tao WiFi ngam trong background (khong lam block che do in offline)
    wifi_init_sta();

    ESP_LOGI(TAG, "\n>>> HE THONG DA SAN SANG! Nhan nut hoac go 'PRINT' tren COM de in phieu. <<<");
}
