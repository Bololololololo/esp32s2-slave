#pragma once

#include "service_interface.h"
#include "wifi_component.h"
#include <memory>

namespace services {
class WifiService : public ServiceInterface {
  public:
    WifiService();
    ~WifiService() override {
        stop();
    }

  private:
    esp_err_t onStart() override;
    void onStop() override;

    std::shared_ptr<wifi::WifiComponent> wifiComponent;
};

} // namespace services
