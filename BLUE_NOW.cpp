#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

// NimBLE BLE Headers (NimBLE hoạt động cực mượt & chuẩn 100% trên ESP32-C3)
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"

static const char *TAG = "ESP32C3_NIMBLE_REMOTE";
#define LED_GPIO GPIO_NUM_8

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_DISC: {
        struct ble_hs_adv_fields fields;
        int rc = ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
        if (rc == 0) {
            char name[32] = {0};
            if (fields.name != NULL && fields.name_len > 0) {
                snprintf(name, sizeof(name), "%.*s", fields.name_len, (char *)fields.name);
            }
            
            // In danh sach thiet bi BLE phat hien duoc
            ESP_LOGI(TAG, "[BLE DISCOVER] %-20s | MAC: %02x:%02x:%02x:%02x:%02x:%02x | RSSI: %d",
                     name[0] ? name : "(Khong ten)",
                     event->disc.addr.val[5], event->disc.addr.val[4], event->disc.addr.val[3],
                     event->disc.addr.val[2], event->disc.addr.val[1], event->disc.addr.val[0],
                     event->disc.rssi);

            // Tự động phát hiện Remote Bluetooth
            if (name[0] && (strstr(name, "Remote") || strstr(name, "BRL") || strstr(name, "TV") || strstr(name, "G10") || strstr(name, "Voice"))) {
                ESP_LOGI(TAG, "-------------------------------------------------");
                ESP_LOGI(TAG, ">>> PHAT HIEN REMOTE BLUETOOTH PHU HOP: [%s] <<<", name);
                ESP_LOGI(TAG, "-------------------------------------------------");
                
                // Nhap nhay LED xac nhan va bat sang LED
                for (int i = 0; i < 6; i++) {
                    gpio_set_level(LED_GPIO, i % 2);
                    vTaskDelay(pdMS_TO_TICKS(80));
                }
                gpio_set_level(LED_GPIO, 1);
            }
        }
        return 0;
    }
    case BLE_GAP_EVENT_DISC_COMPLETE:
        ESP_LOGI(TAG, "Hoan tat luot quet NimBLE BLE.");
        return 0;
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

    // Quet BLE liên tuc
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
