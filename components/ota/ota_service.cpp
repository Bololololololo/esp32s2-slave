#include "ota_service.h"
#include "esp_log.h"

namespace services {

OTAService::OTAService() : ServiceInterface(SERVICE_OTA, "OTAService", 8192) {
}

esp_err_t OTAService::onStart() {
    ESP_LOGI("OTAService", "Starting OTA Service...");
    auto notifier = std::make_shared<OTAUpdateNotifier>(xTaskGetCurrentTaskHandle());
    otaComponent = std::make_shared<OTAComponent>(notifier);
    const esp_err_t result = otaComponent->init();
    if (result != ESP_OK) {
        ESP_LOGE("OTAService", "Failed to initialize OTA component: %s", esp_err_to_name(result));
        notifier->disable();
        otaComponent->deinit();
        otaComponent.reset();
        return result;
    }
    ESP_LOGI("OTAService", "OTA Service started.");
    return ESP_OK;
}

void OTAService::run() {
    while (!isStopRequested()) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (isStopRequested()) {
            break;
        }

        static uint64_t delayRetry = 10000; // 10 seconds
        while (m_otaChecked == false) {
            // Wait for WiFi to be connected
            const esp_err_t result = otaComponent->check_ota_update();
            if (result != ESP_OK) {
                ESP_LOGE("OTAService", "OTA update check failed: %s", esp_err_to_name(result));
            } else {
                ESP_LOGI("OTAService", "OTA update check completed successfully.");
                m_otaChecked = true;
            }

            delayRetry *= 2; // Exponential backoff
            ESP_LOGI("OTAService", "Retrying OTA update check in %llu s...", delayRetry / 1000);
            vTaskDelay(pdMS_TO_TICKS(delayRetry));
        }
    }
}

void OTAService::onStop() {
    ESP_LOGI("OTAService", "Stopping OTA Service...");
    if (otaComponent) {
        otaComponent->deinit();
        otaComponent.reset();
    }
    ESP_LOGI("OTAService", "OTA Service stopped.");
}

} // namespace services