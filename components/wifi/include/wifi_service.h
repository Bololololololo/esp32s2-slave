#ifndef _WIFI_SERVICE_H_
#define _WIFI_SERVICE_H_

#include "service_interface.h"
#include "wifi_component.h"
#include <memory>

using namespace wifi;

namespace services {
class WifiService : public ServiceInterface {
  public:
    WifiService() : ServiceInterface(ServiceType::SERVICE_WIFI) {
    }
    ~WifiService() override = default;

    void start() override;
    void stop() override;

  private:
    std::shared_ptr<WifiComponent> wifiComponent;
};

} // namespace services

#endif // _WIFI_SERVICE_H_
