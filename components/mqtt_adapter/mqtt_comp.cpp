#include "mqtt_comp.h"
#include "esp_log.h"
#include "message_router.h"
#include "mqtt_pub.h"

using namespace messageRouter;
static const char *TAG = "mqtt-component";

MqttComponent::MqttComponent() : ComponentInterface(ComponentId::COMPONENT_ID_MQTT) {
}

esp_err_t MqttComponent::init() {
    const auto self = shared_from_this();
    esp_err_t ret = MessageRouter::getInstance()->subscribe(self);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to subscribe to messages addressed to itself: %s", esp_err_to_name(ret));
    }

#ifdef CONFIG_ENABLE_SERVICE_WIFI
    ret = MessageRouter::getInstance()->subscribe(self, ComponentId::COMPONENT_ID_WIFI);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to subscribe to messages from WiFi component: %s", esp_err_to_name(ret));
    }
#endif

#ifdef CONFIG_PROXIMITY_SENSOR
    ret = MessageRouter::getInstance()->subscribe(self, ComponentId::COMPONENT_ID_PROXIMITY_SENSOR);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to subscribe to messages from Proximity Sensor component: %s", esp_err_to_name(ret));
    }
#endif

    return ESP_OK;
}

void MqttComponent::stop() {
    MessageRouter::getInstance()->unsubscribe(shared_from_this());
#ifdef CONFIG_ENABLE_SERVICE_WIFI
    MessageRouter::getInstance()->unsubscribe(shared_from_this(), ComponentId::COMPONENT_ID_WIFI);
#endif
#ifdef CONFIG_PROXIMITY_SENSOR
    MessageRouter::getInstance()->unsubscribe(shared_from_this(), ComponentId::COMPONENT_ID_PROXIMITY_SENSOR);
#endif
}

esp_err_t MqttComponent::messageHandler(const Message &msg) {
    ESP_LOGI(TAG, "Received message from component %d: %s", msg.sender, msg.payload);

#ifdef CONFIG_ENABLE_SERVICE_WIFI
    if (msg.sender == ComponentId::COMPONENT_ID_WIFI) {
        {
            if (std::strncmp(msg.payload, "wifi_status:", 12) == 0) {
                if (std::strncmp(msg.payload + 12, "disabled", 3) == 0) {
                    //
                } else if (std::strncmp(msg.payload + 12, "connecting", 10) == 0) {
                    //
                } else if (std::strncmp(msg.payload + 12, "connected", 5) == 0) {
                    mqtt_app_start();
                }
            }
        }
    }
#endif

#ifdef CONFIG_ENABLE_SERVICE_SENSORS
    if (msg.sender == ComponentId::COMPONENT_ID_PROXIMITY_SENSOR) {
        if (std::strncmp(msg.payload, "proximity_distance/", 19) == 0) {
            // Handle proximity distance message
            ESP_LOGI(TAG, "Proximity distance: %s", msg.payload + 19);
            return mqtt_queue_payload(msg.payload + 19);
        } else {
            ESP_LOGI(TAG, "BANANANA MUFFIN!");
        }
    } else {
        ESP_LOGI(TAG, "component id sender UNKNOWN");
    }
#endif

    (void)msg;
    return ESP_ERR_NOT_SUPPORTED;
}
