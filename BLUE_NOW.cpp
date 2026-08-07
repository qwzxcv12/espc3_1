#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

// Bluetooth & ESP-HID Host Headers
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_gatt_common_api.h"
#include "esp_hidh.h"
#include "esp_hid_common.h"

static const char *TAG = "ESP32C3_BLE_REMOTE";
#define LED_GPIO GPIO_NUM_8

#define MAC_STR "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_VAL(bda) (bda)[0], (bda)[1], (bda)[2], (bda)[3], (bda)[4], (bda)[5]

static bool is_connected = false;

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
        ESP_LOGI(TAG, "Da thiet lap thong so quet BLE. Bat dau quet...");
        esp_ble_gap_start_scanning(5);
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        if (scan_result->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
            uint8_t *adv_name = NULL;
            uint8_t adv_name_len = 0;
            adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                                ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
            if (!adv_name) {
                adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                                    ESP_BLE_AD_TYPE_NAME_SHORT, &adv_name_len);
            }

            char name_str[32] = {0};
            if (adv_name && adv_name_len > 0) {
                snprintf(name_str, sizeof(name_str), "%.*s", adv_name_len, (char *)adv_name);
                ESP_LOGI(TAG, "Tim thay BLE: %s [MAC: " MAC_STR " | RSSI: %d]", 
                         name_str, MAC_VAL(scan_result->scan_rst.bda), scan_result->scan_rst.rssi);
            }

            if (!is_connected) {
                // Ket noi neu ten thiet bi co chua chu Remote, TV, BRL, G10 hoac Voice
                if (adv_name_len > 0 && (strstr(name_str, "Remote") || strstr(name_str, "BRL") || 
                    strstr(name_str, "TV") || strstr(name_str, "G10") || strstr(name_str, "Voice"))) {
                    
                    ESP_LOGI(TAG, ">>> PHAT HIEN REMOTE! DANG KET NOI TOI [%s]...", name_str);
                    esp_ble_gap_stop_scanning();
                    esp_hidh_dev_open(scan_result->scan_rst.bda, ESP_HID_TRANSPORT_BLE, scan_result->scan_rst.ble_addr_type);
                }
            }
        } else if (scan_result->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT) {
            ESP_LOGI(TAG, "Hoan tat luot quet BLE.");
        }
        break;
    }
    default:
        break;
    }
}

static void hidh_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    esp_hidh_event_t event = (esp_hidh_event_t)id;
    esp_hidh_event_data_t *param = (esp_hidh_event_data_t *)event_data;

    switch (event) {
    case ESP_HIDH_START_EVENT: {
        ESP_LOGI(TAG, "=================================================");
        ESP_LOGI(TAG, "  HID Host Da Khoi Dong! Dang quet Remote BLE...");
        ESP_LOGI(TAG, "  (Hay nhan giu OK + Back tren Remote de bat Pairing)");
        ESP_LOGI(TAG, "=================================================");
        break;
    }
    case ESP_HIDH_OPEN_EVENT: {
        if (param->open.status == ESP_OK) {
            const uint8_t *bda = esp_hidh_dev_bda_get(param->open.dev);
            is_connected = true;
            ESP_LOGI(TAG, "-------------------------------------------------");
            ESP_LOGI(TAG, ">>> DA KET NOI THANH CONG VOI REMOTE BLE [" MAC_STR "] <<<", MAC_VAL(bda));
            ESP_LOGI(TAG, "-------------------------------------------------");
            
            // Nhap nhay LED nhanh 4 lan de xac nhan da ket noi
            for (int i = 0; i < 8; i++) {
                gpio_set_level(LED_GPIO, i % 2);
                vTaskDelay(pdMS_TO_TICKS(80));
            }
            gpio_set_level(LED_GPIO, 1);
        } else {
            ESP_LOGE(TAG, "Ket noi vao Remote BLE that bai, status: %d", param->open.status);
            is_connected = false;
        }
        break;
    }
    case ESP_HIDH_INPUT_EVENT: {
        ESP_LOGI(TAG, "\n[TIN HIEU REMOTE] Nhan Report ID: %d, Do dai: %d bytes", 
                 param->input.map_index, param->input.length);
        
        ESP_LOG_BUFFER_HEX(TAG, param->input.data, param->input.length);

        // Dao trang thai LED GPIO 8 moi khi co phim bam
        static bool led_state = false;
        led_state = !led_state;
        gpio_set_level(LED_GPIO, led_state ? 1 : 0);
        break;
    }
    case ESP_HIDH_CLOSE_EVENT: {
        is_connected = false;
        gpio_set_level(LED_GPIO, 0);
        ESP_LOGW(TAG, "!!! Remote BLE da ngat ket noi. Se tu dong quet lai...");
        break;
    }
    default:
        break;
    }
}

static void scan_task(void *pvParameters)
{
    while (1) {
        if (!is_connected) {
            ESP_LOGI(TAG, "Bat dau luot quet tim Remote...");
            esp_ble_gap_start_scanning(5);
        }
        vTaskDelay(pdMS_TO_TICKS(8000));
    }
}

extern "C" void app_main(void)
{
    // Tat bffering de luon xuat log ngay lap tuc qua USB CDC
    setvbuf(stdout, NULL, _IONBF, 0);

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "  ESP32-C3 BLE HID HOST - KET NOI REMOTE BLUETOOTH");
    ESP_LOGI(TAG, "=================================================");

    // 1. Khoi tao NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Cau hinh GPIO LED
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(LED_GPIO, 0);

    // 3. Khoi tao BLE Host Client
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));

    // Setup BLE Security Manager Protocol
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

    // 4. Khoi tao ESP-HID Host
    esp_hidh_config_t config = {
        .callback = hidh_callback,
        .event_stack_size = 4096,
        .callback_arg = NULL,
    };
    ESP_ERROR_CHECK(esp_hidh_init(&config));

    // Thiet lap thong so quet GAP
    esp_ble_gap_set_scan_params(&ble_scan_params);

    // 5. Tao Task quet tim Remote BLE dinh ky
    xTaskCreate(scan_task, "ble_scan_task", 4096, NULL, 5, NULL);
}
