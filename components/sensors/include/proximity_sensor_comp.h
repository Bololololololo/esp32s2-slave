#ifndef _PROXIMITY_SENSOR_COMP_H_
#define _PROXIMITY_SENSOR_COMP_H_

#include "component_interface.h"

namespace sensors {

class ProximitySensorComponent : public ComponentInterface {
  public:
    ProximitySensorComponent();
    ~ProximitySensorComponent() override = default;

    void init();
    void deinit();
    esp_err_t messageHandler(const Message &msg) override;

    float measureDistance();
};

} // namespace sensors

#endif // _PROXIMITY_SENSOR_COMP_H_