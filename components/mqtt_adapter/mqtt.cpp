#include "mqtt.h"
#ifdef CONFIG_ENABLE_SERVICE_WIFI
#include "esp_log.h"
#include "message_router.h"
#include "mqtt_pub.h"

using namespace messageRouter;
#endif

MqttComponent::MqttComponent() : ComponentInterface(ComponentId::COMPONENT_ID_MQTT) {
}

esp_err_t MqttComponent::init() {
#ifdef CONFIG_ENABLE_SERVICE_WIFI
    esp_err_t ret = MessageRouter::getInstance()->subscribe(shared_from_this(), ComponentId::COMPONENT_ID_WIFI);
    if (ret != ESP_OK) {
        return ret;
    }
    m_subscribed = true;
#endif
    return ESP_OK;
}

void MqttComponent::stop() {
#ifdef CONFIG_ENABLE_SERVICE_WIFI
    if (m_subscribed) {
        MessageRouter::getInstance()->unsubscribe(shared_from_this(), ComponentId::COMPONENT_ID_WIFI);
        m_subscribed = false;
    }
#endif
}

esp_err_t MqttComponent::messageHandler(const Message &msg) {
#ifdef CONFIG_ENABLE_SERVICE_WIFI
    ESP_LOGI("MqttComponent", "Received message from component %d: %s", msg.id, msg.payload);

    if (msg.id != ComponentId::COMPONENT_ID_WIFI) {
        return ESP_ERR_INVALID_ARG;
    }

    if (std::strncmp(msg.payload, "wifi_status:", 12) != 0) {
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (std::strncmp(msg.payload + 12, "disabled", 3) == 0) {
        //
    } else if (std::strncmp(msg.payload + 12, "connecting", 10) == 0) {
        //
    } else if (std::strncmp(msg.payload + 12, "connected", 5) == 0) {
        mqtt_app_start();
    }
    return ESP_OK;
#else
    (void)msg;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}
