#ifndef _SERVICES_H_
#define _SERVICES_H_

#include "esp_err.h"
#include "service_interface.h"
#include <list>
#include <memory>
#include <unordered_map>

namespace services {

class ServiceManager {
  private:
    static std::unique_ptr<ServiceManager> instance;
    struct _cons {
        explicit _cons() = default;
    };

    std::unordered_map<ServiceType, std::unique_ptr<ServiceInterface>> servicesMap;

  public:
    ServiceManager(_cons);

    static std::unique_ptr<ServiceManager> instanceFactory() {
        return std::make_unique<ServiceManager>(_cons{});
    }

    static ServiceManager *getInstance() {
        /*
            If control enters the declaration concurrently while the variable is being initialized,
            the concurrent execution shall wait for completion of the initialization.
        */
        static std::unique_ptr<ServiceManager> instance{ServiceManager::instanceFactory()};

        return instance.get();
    }

    esp_err_t startService(const ServiceType &serviceType);
    esp_err_t stopService(const ServiceType &serviceType);
    esp_err_t isServiceRunning(const ServiceType &serviceType);
};

} // namespace services

#endif // _SERVICES_H_