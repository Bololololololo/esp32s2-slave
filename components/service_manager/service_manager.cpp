#include "service_manager.h"
#include "esp_log.h"
#include "sdkconfig.h"
#ifdef CONFIG_ENABLE_SERVICE_MSGR
#include "message_router_service.h"
#endif
#ifdef CONFIG_ENABLE_SERVICE_SENSORS
#include "sensors_service.h"
#endif
#ifdef CONFIG_ENABLE_SERVICE_WIFI
#include "wifi_service.h"
#endif
#ifdef CONFIG_ENABLE_SERVICE_OTA
#include "ota_service.h"
#endif
#include <stdio.h>

static const char *TAG = "service-manager";

namespace services {
ServiceManager::ServiceManager(_cons) {
    // Constructor implementation - initialize all services
#ifdef CONFIG_ENABLE_SERVICE_MSGR
    this->servicesMap.emplace(ServiceType::SERVICE_MSGR, std::make_unique<MessageRouterService>());
#endif
#ifdef CONFIG_ENABLE_SERVICE_WIFI
    this->servicesMap.emplace(ServiceType::SERVICE_WIFI, std::make_unique<WifiService>());
#endif
#ifdef CONFIG_ENABLE_SERVICE_SENSORS
    this->servicesMap.emplace(ServiceType::SERVICE_SENSORS, std::make_unique<SensorsService>());
#endif
#ifdef CONFIG_ENABLE_SERVICE_OTA
    this->servicesMap.emplace(ServiceType::SERVICE_OTA, std::make_unique<OTAService>());
#endif
}

esp_err_t ServiceManager::startService(const ServiceType &serviceType) {
    auto it = this->servicesMap.find(serviceType);
    if (it != this->servicesMap.end()) {
        return it->second->start();
    }
    ESP_LOGE(TAG, "Service %d not found", static_cast<int>(serviceType));
    return ESP_ERR_NOT_FOUND;
}

esp_err_t ServiceManager::stopService(const ServiceType &serviceType) {
    auto it = this->servicesMap.find(serviceType);
    if (it != this->servicesMap.end()) {
        return it->second->stop();
    }
    ESP_LOGE(TAG, "Service %d not found", static_cast<int>(serviceType));
    return ESP_ERR_NOT_FOUND;
}

esp_err_t ServiceManager::isServiceRunning(const ServiceType &serviceType) {
    auto it = this->servicesMap.find(serviceType);
    if (it != this->servicesMap.end()) {
        if (it->second->getServiceState() == SERVICE_RUNNING) {
            return ESP_OK;
        }
        return ESP_ERR_INVALID_STATE;
    }
    ESP_LOGE(TAG, "Service %d not found", static_cast<int>(serviceType));
    return ESP_ERR_NOT_FOUND;
}
} // namespace services