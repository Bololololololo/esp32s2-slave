#ifndef _SENSORS_SERVICE_H_
#define _SENSORS_SERVICE_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "proximity_sensor_comp.h"
#include "service_interface.h"
#include <memory>

using namespace sensors;

namespace services {

class SensorsService : public ServiceInterface {
  public:
    SensorsService();
    ~SensorsService() override {
        stop();
    }

  private:
    esp_err_t onStart() override;
    void run() override;
    void onStop() override;

    std::unique_ptr<ProximitySensorComponent> proximitySensorComponent;
};

} // namespace services

#endif // _SENSORS_SERVICE_H_