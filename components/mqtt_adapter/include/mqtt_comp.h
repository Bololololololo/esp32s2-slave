#pragma once

#include "component_interface.h"
#include "sdkconfig.h"
#include <memory>

class MqttComponent : public ComponentInterface, public std::enable_shared_from_this<MqttComponent> {
  public:
    MqttComponent();
    esp_err_t init();
    void stop();

  private:
    esp_err_t messageHandler(const Message &msg) override;
};
