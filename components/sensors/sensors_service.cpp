#include "sensors_service.h"
#include "esp_log.h"
#include "message_router.h"

using namespace sensors;
using namespace messageRouter;

#define MEASURE_DELAY 1000

namespace services {
static const char *TAG = "sensors-service";

SensorsService::SensorsService() : ServiceInterface(SERVICE_SENSORS, "SensorsService", 4096) {
    proximitySensorComponent = std::make_unique<ProximitySensorComponent>();
}

esp_err_t SensorsService::onStart() {
    ESP_LOGI(TAG, "Sensors service is starting.");
    proximitySensorComponent->init();
    return ESP_OK;
}

void SensorsService::run() {
    while (!waitForStop(pdMS_TO_TICKS(MEASURE_DELAY))) {
        float distance = proximitySensorComponent->measureDistance();
        if (distance < 0.0f) {
            ESP_LOGW(TAG, "Out of range.");
        } else {
            MessageRouter::getInstance()->publish(Message(COMPONENT_ID_PROXIMITY_SENSOR, COMPONENT_ID_MQTT,
                                                          "proximity_distance/" + std::to_string(distance)));
            ESP_LOGI(TAG, "Measured Distance: %.2f cm", distance);
        }
    }
}

void SensorsService::onStop() {
    proximitySensorComponent->deinit();
    ESP_LOGI(TAG, "Sensors Service stopped.");
}

} // namespace services