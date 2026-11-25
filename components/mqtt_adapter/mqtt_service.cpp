#include "mqtt_service.h"
#include "esp_log.h"
#include "message_router.h"

using namespace messageRouter;

namespace services {
static const char *TAG = "mqtt-service";

MqttService::MqttService() : ServiceInterface(SERVICE_MQTT, "MqttService", 4096) {
}

esp_err_t MqttService::onStart() {
    ESP_LOGI(TAG, "MQTT service is starting.");
    mqttComponent = std::make_shared<MqttComponent>();
    const esp_err_t ret = mqttComponent->init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize MqttComponent: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

void MqttService::onStop() {
    if (mqttComponent) {
        mqttComponent->stop();
        mqttComponent.reset();
    }
    ESP_LOGI(TAG, "MQTT Service stopped.");
}

} // namespace services