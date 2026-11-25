#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_comp.h"
#include "service_interface.h"
#include <memory>

namespace services {

class MqttService : public ServiceInterface {
  public:
    MqttService();
    ~MqttService() override {
        stop();
    }

  private:
    esp_err_t onStart() override;
    void onStop() override;

    std::shared_ptr<MqttComponent> mqttComponent;
};

} // namespace services
