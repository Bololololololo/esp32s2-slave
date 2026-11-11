#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ota_component.h"
#include "service_interface.h"
#include <memory>

using namespace ota;

namespace services {

class OTAService : public ServiceInterface {
  public:
    OTAService();
    ~OTAService() override {
        stop();
    }

  private:
    esp_err_t onStart() override;
    void run() override;
    void onStop() override;

    std::shared_ptr<OTAComponent> otaComponent;
    bool m_otaChecked{false};
};

} // namespace services
