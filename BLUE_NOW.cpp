#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

// NimBLE BLE Headers
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"

static const char *TAG = "ESP32C3_NIMBLE_REMOTE";
#define LED_GPIO GPIO_NUM_8

static bool is_connected = false;

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_DISC: {
        struct ble_hs_adv_fields fields;
        int rc = ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
        char name[32] = {0};
        if (rc == 0 && fields.name != NULL && fields.name_len > 0) {
            snprintf(name, sizeof(name), "%.*s", fields.name_len, (char *)fields.name);
        }

        const uint8_t *addr = event->disc.addr.val;
        ESP_LOGI(TAG, "[BLE DISCOVER] %-20s | MAC: %02x:%02x:%02x:%02x:%02x:%02x | RSSI: %d dBm",
                 name[0] ? name : "(Khong ten)",
                 addr[5], addr[4], addr[3], addr[2], addr[1], addr[0],
                 event->disc.rssi);

        // Tu dong ket noi neu:
        // 1. Ten co chu Remote/TV/BRL/G10/Voice
        // 2. HOAC thiet bi dang o RAT GAN ESP32-C3 (RSSI mạnh > -50 dBm)
        if (!is_connected) {
            bool is_target_remote = (name[0] && (strstr(name, "Remote") || strstr(name, "BRL") || 
                                                strstr(name, "TV") || strstr(name, "G10") || strstr(name, "Voice")));
            bool is_very_close = (event->disc.rssi > -50); // Thiet bi ngay canh ESP32-C3

            if (is_target_remote || is_very_close) {
                ESP_LOGI(TAG, "-------------------------------------------------");
                ESP_LOGI(TAG, ">>> PHAT HIEN THIET BI GAN/REMOTE: MAC %02x:%02x:%02x:%02x:%02x:%02x (RSSI: %d dBm)! DANG KET NOI... <<<",
                         addr[5], addr[4], addr[3], addr[2], addr[1], addr[0], event->disc.rssi);
                ESP_LOGI(TAG, "-------------------------------------------------");

                ble_gap_disc_cancel();
                
                // Nhap nhay LED xac nhan va bat sang LED
                for (int i = 0; i < 6; i++) {
                    gpio_set_level(LED_GPIO, i % 2);
                    vTaskDelay(pdMS_TO_TICKS(80));
                }
                gpio_set_level(LED_GPIO, 1);
                is_connected = true;
            }
        }
        return 0;
    }
    case BLE_GAP_EVENT_CONNECT: {
        if (event->connect.status == 0) {
            ESP_LOGI(TAG, "✅ KET NOI BLUETOOTH THANH CONG!");
            is_connected = true;
            gpio_set_level(LED_GPIO, 1);
        } else {
            ESP_LOGE(TAG, "❌ KET NOI THAT BAI: %d", event->connect.status);
            is_connected = false;
            gpio_set_level(LED_GPIO, 0);
        }
        return 0;
    }
    case BLE_GAP_EVENT_DISCONNECT: {
        ESP_LOGW(TAG, "⚠️ NGAT KET NOI BLUETOOTH. DANG KHOI DONG LAI QUET...");
        is_connected = false;
        gpio_set_level(LED_GPIO, 0);
        return 0;
    }
    default:
        return 0;
    }
}

static void start_scan(void)
{
    struct ble_gap_disc_params disc_params;
    memset(&disc_params, 0, sizeof(disc_params));
    disc_params.filter_duplicates = 1;
    disc_params.passive = 0;

    int rc = ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &disc_params, ble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Loi khi goi ble_gap_disc: %d", rc);
    }
}

static void ble_on_sync(void)
{
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "  NimBLE Stack Da Dong Bo! Bat dau quet...");
    ESP_LOGI(TAG, "=================================================");
    start_scan();
}

static void nimble_host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

extern "C" void app_main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "  ESP32-C3 NimBLE BLE SCANNER & REMOTE CONNECT");
    ESP_LOGI(TAG, "=================================================");

    nvs_flash_init();

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(LED_GPIO, 0);

    nimble_port_init();
    ble_hs_cfg.sync_cb = ble_on_sync;
    nimble_port_freertos_init(nimble_host_task);
}
