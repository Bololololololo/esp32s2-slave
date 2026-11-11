#include "message_router_service.h"
#include "esp_log.h"
#include <cstdio>

static const char *TAG = "msgr-service";

namespace services {

esp_err_t MessageRouterService::onStart() {
    ESP_LOGI(TAG, "Message router service starting");
    MessageRouter::getInstance();
    ESP_LOGI(TAG, "Message router service started successfully");
    return ESP_OK;
}

void MessageRouterService::onStop() {
}

} // namespace services
