#include "sensors_service.h"
#include "esp_log.h"

using namespace sensors;

namespace services {

SensorsService::SensorsService() : ServiceInterface(SERVICE_SENSORS) {
    proximitySensorComponent = std::make_unique<ProximitySensorComponent>();
}

void SensorsService::start() {
    ESP_LOGI("SensorsService", "Starting Sensors Service...");
    setServiceState(SERVICE_RUNNING);
    proximitySensorComponent->init();

    // Span measurement task to continuously measure distance in the background
    xTaskCreate(
        [](void *arg) {
            SensorsService *service = static_cast<SensorsService *>(arg);
            while (service->getServiceState() == SERVICE_RUNNING) {
                float distance = service->proximitySensorComponent->measureDistance();
                if (distance < 0.0f) {
                    ESP_LOGW("SensorsService", "Out of range.");
                } else {
                    ESP_LOGI("SensorsService", "Measured Distance: %.2f cm", distance);
                }
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            vTaskDelete(nullptr);
        },
        "MeasureTask", 2048, this, 5, &measureTaskHandle);
    ESP_LOGI("SensorsService", "Sensors Service started.");
}

void SensorsService::stop() {
    ESP_LOGI("SensorsService", "Stopping Sensors Service...");
    setServiceState(SERVICE_STOPPED);
    proximitySensorComponent->deinit();
    ESP_LOGI("SensorsService", "Sensors Service stopped.");
}

} // namespace services