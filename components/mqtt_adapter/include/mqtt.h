#ifndef _MQTT_H_
#define _MQTT_H_

#include "component_interface.h"
#include "sdkconfig.h"
#include <memory>

class MqttComponent : public ComponentInterface, public std::enable_shared_from_this<MqttComponent> {
  public:
    MqttComponent();
    esp_err_t init();
    void stop();

  private:
#ifdef CONFIG_ENABLE_SERVICE_WIFI
    bool m_subscribed{false};
#endif

    esp_err_t messageHandler(const Message &msg) override;
};

#endif // _MQTT_H_