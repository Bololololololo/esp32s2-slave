#include "message_router_service.h"
#include "esp_log.h"
#include <cstdio>

static const char *TAG = "msgr-service";

namespace services {

void MessageRouterService::start() {
    // Start the Message Router service
    ESP_LOGI(TAG, "Message router service starting");
    this->setServiceState(SERVICE_RUNNING);
    this->messageRouter = MessageRouter::instanceFactory();
    ESP_LOGI(TAG, "Message router service started successfully");
    // Message router is now initialized and running the router task
}

void MessageRouterService::stop() {
    // Stop the Message Router service
    this->setServiceState(SERVICE_STOPPED);
    // Clean up the message router
    this->messageRouter.reset();
}

} // namespace services
