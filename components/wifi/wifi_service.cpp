#include "wifi_service.h"
#include "esp_check.h"
#include "esp_log.h"

namespace services {
static const char *TAG = "wifi-service";

WifiService::WifiService() : ServiceInterface(ServiceType::SERVICE_WIFI, "WifiService", 4096) {
}

esp_err_t WifiService::onStart() {
    ESP_LOGI(TAG, "WiFi service is starting.");
    this->wifiComponent = std::make_shared<wifi::WifiComponent>();
    const esp_err_t result = this->wifiComponent->init();
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize WifiComponent: %s", esp_err_to_name(result));
        return result;
    }
    return ESP_OK;
}

void WifiService::onStop() {
    if (this->wifiComponent) {
        this->wifiComponent->stop();
        this->wifiComponent.reset();
    }
    ESP_LOGI(TAG, "WiFi service stopped.");
}

} // namespace services
