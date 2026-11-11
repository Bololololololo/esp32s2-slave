/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "buttons.hpp"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hw069.h"
#include "message_router.h"
#include "mqtt.h"
#include "nvs_helpers.hpp"
#include "sdkconfig.h"
#include "service_manager.h"
#include "status_led.h"
#include <stdio.h>

#include "message_router.h"
using namespace messageRouter;

using namespace utils;
using namespace services;

#define LOG_LEVEL ESP_LOG_INFO

static const char *TAG = "main";

void log_firmware_version(void) {
    char version[32];
    size_t required_size = sizeof(version);
    const esp_app_desc_t *app_desc = esp_app_get_description();

    esp_err_t err = NvsHelpers::read_nvs_value("fw_vers", version, required_size);
    if (err == ESP_OK) {
        if (strcmp(version, app_desc->version) != 0) {
            ESP_LOGW("VERSION", "Firmware version mismatch! NVS: %s, Running: %s", version, app_desc->version);
            esp_err_t err = NvsHelpers::write_nvs_value("fw_vers", app_desc->version);
            if (err != ESP_OK) {
                ESP_LOGE("VERSION", "Failed to update firmware version in NVS: %s", esp_err_to_name(err));
            } else {
                ESP_LOGI("VERSION", "Firmware version updated successfully in NVS.");
            }
        }
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI("VERSION", "Firmware version not found in NVS.");
        esp_err_t err = NvsHelpers::write_nvs_value("fw_vers", app_desc->version);
        if (err != ESP_OK) {
            ESP_LOGE("VERSION", "Failed to write firmware version to NVS: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI("VERSION", "Firmware version written successfully to NVS: %s", app_desc->version);
        }
    } else {
        ESP_LOGE("VERSION", "Failed to read firmware version from NVS: %s", esp_err_to_name(err));
    }

    ESP_LOGI("VERSION", "Printing all NVS entries for debugging purposes:");
    NvsHelpers::print_all_nvs_entries();
}

extern "C" {
void app_main(void);
}

void app_main(void) {
    /* Set log level  */
    esp_log_level_set("*", LOG_LEVEL);

    /* Init nvs */
    NvsHelpers::init_nvs_with_reset_on_failure();
    NvsHelpers::check_and_store_embedded_certificate();

    /* Check if the running firmware is in a pending verification state and confirm it */
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;

    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        ESP_LOGI("OTA_VERIFY", "Current OTA state: %d", ota_state);
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI("OTA_VERIFY", "First boot of new OTA app. Marking app as valid...");
            esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();

            if (err == ESP_OK) {
                ESP_LOGI("OTA_VERIFY", "OTA update locked in successfully. Rollback cancelled.");
            } else {
                ESP_LOGE("OTA_VERIFY", "Failed to mark app valid: %s", esp_err_to_name(err));
            }
        }
    }

    /* save firmware version */
    log_firmware_version();

    /* Config buttons */
    // ButtonManager::Config config = {.pins = {GPIO_NUM_11, GPIO_NUM_12, GPIO_NUM_13, GPIO_NUM_14},
    //                                 .poll_interval_ms = 50};

    // Instantiate and start the button manager
    // static ButtonManager buttons(config);
    // buttons.init();
    // buttons.start();

    /* Configure the peripheral according to the LED type */
    StatusLED status_led;
    status_led.setColor(utils::StatusLEDColor::RED);

    /* Start the services */
    ServiceManager *serviceManager = ServiceManager::getInstance();
#ifdef CONFIG_ENABLE_SERVICE_MSGR
    ESP_ERROR_CHECK(serviceManager->startService(ServiceType::SERVICE_MSGR));
#endif
#ifdef CONFIG_ENABLE_SERVICE_WIFI
    ESP_ERROR_CHECK(serviceManager->startService(ServiceType::SERVICE_WIFI));
#endif
#ifdef CONFIG_ENABLE_SERVICE_SENSORS
//    ESP_ERROR_CHECK(serviceManager->startService(ServiceType::SERVICE_SENSORS));
#endif
#ifdef CONFIG_ENABLE_SERVICE_OTA
    ESP_ERROR_CHECK(serviceManager->startService(ServiceType::SERVICE_OTA));
#endif

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
