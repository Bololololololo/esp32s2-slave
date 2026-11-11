#include "sensors_service.h"
#include "esp_log.h"

using namespace sensors;

namespace services {

SensorsService::SensorsService() : ServiceInterface(SERVICE_SENSORS, "SensorsService", 4096) {
    proximitySensorComponent = std::make_unique<ProximitySensorComponent>();
}

esp_err_t SensorsService::onStart() {
    ESP_LOGI("SensorsService", "Starting Sensors Service...");
    proximitySensorComponent->init();
    return ESP_OK;
}

void SensorsService::run() {
    while (!waitForStop(pdMS_TO_TICKS(100))) {
        float distance = proximitySensorComponent->measureDistance();
        if (distance < 0.0f) {
            ESP_LOGW("SensorsService", "Out of range.");
        } else {
            ESP_LOGI("SensorsService", "Measured Distance: %.2f cm", distance);
        }
    }
}

void SensorsService::onStop() {
    proximitySensorComponent->deinit();
    ESP_LOGI("SensorsService", "Sensors Service stopped.");
}

} // namespace services