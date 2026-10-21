#include "wifi_service.h"
#include "esp_check.h"
#include "esp_log.h"
#include <cstdio>

static const char *TAG = "wifi-service";

namespace services {

void WifiService::start() {
    // Start the WiFi service
    ESP_LOGI(TAG, "WiFi service starting");
    this->wifiComponent = std::make_shared<WifiComponent>();
    ESP_ERROR_CHECK(this->wifiComponent->init());
    this->setServiceState(SERVICE_RUNNING);
    ESP_LOGI(TAG, "WiFi service started successfully");
}

void WifiService::stop() {
    if (this->wifiComponent) {
        this->wifiComponent->stop();
        this->wifiComponent.reset();
    }
    this->setServiceState(SERVICE_STOPPED);
}

} // namespace services
