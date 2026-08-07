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
static uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;

static int on_disc_chrs(uint16_t conn_h, const struct ble_gatt_error *error, const struct ble_gatt_chr *chr, void *arg)
{
    if (error->status == 0 && chr != NULL) {
        uint16_t uuid16 = ble_uuid_u16(&chr->uuid.u);
        if (uuid16 == 0x2A4D || uuid16 == 0x2A22) { // 0x2A4D = HID Report, 0x2A22 = Boot Keyboard Input
            ESP_LOGI(TAG, ">>> PHAT HIEN NÚT BẤM REMOTE (UUID: 0x%04X, Val Handle: %d)", uuid16, chr->val_handle);
            
            // Đăng ký nhận Notification/Indication từ nút bấm
            uint8_t notify_buf[2] = {0x01, 0x00};
            ble_gattc_write_flat(conn_h, chr->val_handle + 1, notify_buf, 2, NULL, NULL);
        }
    }
    return 0;
}

static int on_disc_svc(uint16_t conn_h, const struct ble_gatt_error *error, const struct ble_gatt_svc *svc, void *arg)
{
    if (error->status == 0 && svc != NULL) {
        uint16_t uuid16 = ble_uuid_u16(&svc->uuid.u);
        if (uuid16 == 0x1812) { // 0x1812 = HID Service
            ESP_LOGI(TAG, ">>> PHAT HIEN BLE HID SERVICE (0x1812). Dang kham pha nut bam...");
            ble_gattc_disc_all_chrs(conn_h, svc->start_handle, svc->end_handle, on_disc_chrs, NULL);
        }
    }
    return 0;
}

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_DISC: {
        struct ble_hs_adv_fields fields;
        int rc = ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
        char name[32] = {0};
        bool is_hid_uuid = false;

        if (rc == 0) {
            if (fields.name != NULL && fields.name_len > 0) {
                snprintf(name, sizeof(name), "%.*s", fields.name_len, (char *)fields.name);
            }
            if (fields.uuids16 != NULL && fields.num_uuids16 > 0) {
                for (int i = 0; i < fields.num_uuids16; i++) {
                    if (fields.uuids16[i].value == 0x1812 || fields.uuids16[i].value == 0x180F) {
                        is_hid_uuid = true;
                    }
                }
            }
        }

        const uint8_t *addr = event->disc.addr.val;
        ESP_LOGI(TAG, "[BLE DISCOVER] %-20s | MAC: %02x:%02x:%02x:%02x:%02x:%02x | RSSI: %d dBm %s",
                 name[0] ? name : "(Khong ten)",
                 addr[5], addr[4], addr[3], addr[2], addr[1], addr[0],
                 event->disc.rssi,
                 is_hid_uuid ? "<=== [THIET BI HID/REMOTE]" : "");

        if (!is_connected) {
            bool is_target_remote = (name[0] && (strstr(name, "Remote") || strstr(name, "BRL") || 
                                                strstr(name, "TV") || strstr(name, "G10") || strstr(name, "Voice")));
            bool is_very_close = (event->disc.rssi > -75);

            if (is_hid_uuid || is_target_remote || is_very_close) {
                ESP_LOGI(TAG, "-------------------------------------------------");
                ESP_LOGI(TAG, ">>> DA TIM THAY REMOTE! DANG THUC HIEN KET NOI TOI MAC %02x:%02x:%02x:%02x:%02x:%02x... <<<",
                         addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
                ESP_LOGI(TAG, "-------------------------------------------------");

                ble_gap_disc_cancel();

                ble_addr_t peer_addr;
                memcpy(peer_addr.val, event->disc.addr.val, 6);
                peer_addr.type = event->disc.addr.type;

                struct ble_gap_conn_params conn_params;
                memset(&conn_params, 0, sizeof(conn_params));
                conn_params.scan_itvl = 0x0010;
                conn_params.scan_window = 0x0010;
                conn_params.itvl_min = 12;
                conn_params.itvl_max = 24;
                conn_params.latency = 0;
                conn_params.supervision_timeout = 400;

                int res = ble_gap_connect(BLE_OWN_ADDR_PUBLIC, &peer_addr, 10000, &conn_params, ble_gap_event, NULL);
                if (res != 0) {
                    ESP_LOGE(TAG, "Loi ble_gap_connect: %d", res);
                }
                is_connected = true;
            }
        }
        return 0;
    }
    case BLE_GAP_EVENT_CONNECT: {
        if (event->connect.status == 0) {
            ESP_LOGI(TAG, "=================================================");
            ESP_LOGI(TAG, "  ✅ KET NOI BLUETOOTH VOI REMOTE THANH CONG!");
            ESP_LOGI(TAG, "=================================================");
            is_connected = true;
            conn_handle = event->connect.conn_handle;

            // Nhap nhay LED xac nhan va bat sang LED 8
            for (int i = 0; i < 8; i++) {
                gpio_set_level(LED_GPIO, i % 2);
                vTaskDelay(pdMS_TO_TICKS(80));
            }
            gpio_set_level(LED_GPIO, 1);

            // Kham pha GATT Services de dang ky nhan tin hieu nut bam Remote
            ble_gattc_disc_all_svcs(conn_handle, on_disc_svc, NULL);
        } else {
            ESP_LOGE(TAG, "❌ KET NOI THAT BAI: %d. Tu dong quet lai...", event->connect.status);
            is_connected = false;
            gpio_set_level(LED_GPIO, 0);

            struct ble_gap_disc_params disc_params;
            memset(&disc_params, 0, sizeof(disc_params));
            disc_params.filter_duplicates = 1;
            disc_params.passive = 0;
            ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &disc_params, ble_gap_event, NULL);
        }
        return 0;
    }
    case BLE_GAP_EVENT_NOTIFY_RX: {
        ESP_LOGI(TAG, "\n🎯 [TIN HIEU PHIM BAM REMOTE] Nhan Notification! Do dai: %d bytes", event->notify_rx.om->om_len);
        ESP_LOG_BUFFER_HEX(TAG, event->notify_rx.om->om_data, event->notify_rx.om->om_len);

        // Dao trang thai LED GPIO 8 moi khi co phim bam
        static bool led_state = true;
        led_state = !led_state;
        gpio_set_level(LED_GPIO, led_state ? 1 : 0);
        return 0;
    }
    case BLE_GAP_EVENT_DISCONNECT: {
        ESP_LOGW(TAG, "⚠️ NGAT KET NOI BLUETOOTH. DANG KHOI DONG LAI QUET...");
        is_connected = false;
        gpio_set_level(LED_GPIO, 0);

        struct ble_gap_disc_params disc_params;
        memset(&disc_params, 0, sizeof(disc_params));
        disc_params.filter_duplicates = 1;
        disc_params.passive = 0;
        ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &disc_params, ble_gap_event, NULL);
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
