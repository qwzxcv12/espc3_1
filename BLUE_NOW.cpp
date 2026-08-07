#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "driver/gpio.h"

static const char *TAG = "ESP32C3_TEST";
#define LED_GPIO GPIO_NUM_8

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "   KHOI DONG MA CHAY THU ESP32-C3 (ESP-IDF)");
    ESP_LOGI(TAG, "=================================================");

    // Khoi tao NVS (Non-Volatile Storage)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // In thong tin phan cung Chip ESP32-C3
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    
    ESP_LOGI(TAG, "Chip model: ESP32-C3 RISC-V");
    ESP_LOGI(TAG, "So nhan CPU: %d", chip_info.cores);
    ESP_LOGI(TAG, "Tinh nang: %s%s",
            (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "Wi-Fi 2.4GHz " : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "Bluetooth LE (BLE) " : "");

    if (esp_flash_get_size(NULL, &flash_size) == ESP_OK) {
        ESP_LOGI(TAG, "Kich thuoc Flash: %lu MB", (unsigned long)(flash_size / (1024 * 1024)));
    }

    // Cau hinh GPIO cho den LED on-board (GPIO 8 tren ESP32-C3 DevKit)
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Bat dau nhap nhay LED va theo doi bo nho...");

    int count = 0;
    bool led_state = false;

    while (1) {
        led_state = !led_state;
        gpio_set_level(LED_GPIO, led_state ? 1 : 0);

        ESP_LOGI(TAG, "[%d] ESP32-C3 dang hoat dong tot! Free Heap: %lu bytes", 
                 ++count, (unsigned long)esp_get_free_heap_size());

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
