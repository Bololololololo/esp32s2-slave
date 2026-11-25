#include "message_router_service.h"
#include "esp_log.h"
#include <cstdio>

namespace services {
static const char *TAG = "msgr-service";

esp_err_t MessageRouterService::onStart() {
    ESP_LOGI(TAG, "Message router service is starting.");
    MessageRouter::getInstance();
    return ESP_OK;
}

void MessageRouterService::onStop() {
    ESP_LOGI(TAG, "Message router service stopped.");
}

} // namespace services
