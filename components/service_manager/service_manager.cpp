#include "service_manager.h"
#include "esp_log.h"
#include "message_router_service.h"
#include "wifi_service.h"
#include <stdio.h>

static const char *TAG = "service-manager";

namespace services {
ServiceManager::ServiceManager(_cons) {
  // Constructor implementation - initialize all services
  this->servicesMap.emplace(ServiceType::SERVICE_MSGR,
                            std::make_unique<MessageRouterService>());
  this->servicesMap.emplace(ServiceType::SERVICE_WIFI,
                            std::make_unique<WifiService>());
}

esp_err_t ServiceManager::startService(const ServiceType &serviceType) {
  auto it = this->servicesMap.find(serviceType);
  if (it != this->servicesMap.end()) {
    it->second->start();
    return ESP_OK;
  }
  ESP_LOGE(TAG, "Service %d not found", static_cast<int>(serviceType));
  return ESP_ERR_NOT_FOUND;
}

esp_err_t ServiceManager::stopService(const ServiceType &serviceType) {
  auto it = this->servicesMap.find(serviceType);
  if (it != this->servicesMap.end()) {
    it->second->stop();
    return ESP_OK;
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