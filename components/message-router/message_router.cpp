#include "message_router.h"
#include "esp_check.h"
#include <cstring>
#include <esp_log.h>

static const char *TAG = "message-router";

namespace messageRouter {

static const char *messageLogPayload(const Message &message) {
    // static const char *const sensitivePrefix = "wifi_status:portal|";
    // if (std::strncmp(message.payload, sensitivePrefix,
    // std::strlen(sensitivePrefix)) == 0) {
    //     return "wifi_status:portal|[credentials redacted]";
    // }
    return message.payload;
}

static void printSubscribersDb() {
    std::lock_guard<std::recursive_mutex> lock(MessageRouter::getInstance()->getDbMutex());
    ESP_LOGI(TAG, "------------------------------------------------------------");
    ESP_LOGI(TAG, "Current Subscribers Database:");
    auto &subscribersDB = MessageRouter::getInstance()->getSubscribersDB();
    for (const auto &entry : subscribersDB) {
        ComponentId sourceId = entry.first;
        const auto &subscribers = entry.second;

        ESP_LOGI(TAG, "Source Component ID: %s", componentIdToString[sourceId].c_str());
        for (const auto &subscriber : subscribers) {
            ESP_LOGI(TAG, "  Subscriber Component ID: %s", componentIdToString[subscriber->getComponentId()].c_str());
        }
    }
    ESP_LOGI(TAG, "------------------------------------------------------------");
}

void MessageRouter::routerTask(void *pvParameters) {
    Message receivedMsg;
    auto *router = static_cast<MessageRouter *>(pvParameters);
    QueueHandle_t &xRouterQueue = router->xRouterQueue;

    for (;;) {
        // Block indefinitely until a message arrives (no unnecessary wakeups)
        if (xQueueReceive(xRouterQueue, &receivedMsg, portMAX_DELAY) == pdPASS) {
            ESP_LOGI(TAG, "Received message %s: id %s", messageLogPayload(receivedMsg),
                     componentIdToString[receivedMsg.id].c_str());
            {
                // Acquire lock and copy subscribers to avoid holding lock during
                // callback
                std::vector<std::shared_ptr<ComponentInterface>> subscribers_copy;
                std::vector<std::shared_ptr<ComponentInterface>> wildcard_subscribers_copy;

                {
                    std::lock_guard<std::recursive_mutex> lock(router->getDbMutex());
                    auto &subscribersDB = router->getSubscribersDB();

                    // Single lookup: use find() iterator instead of find() + operator[]
                    auto it = subscribersDB.find(receivedMsg.id);
                    if (it != subscribersDB.end()) {
                        subscribers_copy = it->second; // Make a copy while holding the lock
                    }

                    // Also deliver to wildcard subscribers (COMPONENT_ID_ALL)
                    if (receivedMsg.id != COMPONENT_ID_ALL) {
                        auto wildcard_it = subscribersDB.find(COMPONENT_ID_ALL);
                        if (wildcard_it != subscribersDB.end()) {
                            wildcard_subscribers_copy = wildcard_it->second; // Copy
                        }
                    }
                } // Release lock before calling messageHandler

                // Deliver to specific subscribers outside lock (exclude publisher)
                for (const auto &subscriber : subscribers_copy) {
                    // Don't deliver to the publisher itself
                    if (subscriber->getComponentId() != receivedMsg.id) {
                        subscriber->messageHandler(receivedMsg);
                    }
                }

                // Deliver to wildcard subscribers outside lock (exclude publisher)
                for (const auto &subscriber : wildcard_subscribers_copy) {
                    // Don't deliver to the publisher itself
                    if (subscriber->getComponentId() != receivedMsg.id) {
                        subscriber->messageHandler(receivedMsg);
                    }
                }

                if (subscribers_copy.empty() && wildcard_subscribers_copy.empty()) {
                    ESP_LOGW(TAG, "No subscribers found for message id: %d", receivedMsg.id);
                }
            }
        }
    }
}

MessageRouter::MessageRouter(_cons) {
    // Create a FreeRTOS queue for message routing
    this->xRouterQueue = xQueueCreate(10, sizeof(Message));
    ESP_ERROR_CHECK(this->xRouterQueue ? ESP_OK : ESP_ERR_NO_MEM);

    ESP_LOGI(TAG, "Message router task created");
    const BaseType_t taskCreated = xTaskCreate(routerTask, "MessageRouterTask", 8192, this, 5, &this->xRouterTask);
    ESP_ERROR_CHECK(taskCreated == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
}

esp_err_t MessageRouter::publish(const Message &message) {
    ESP_LOGI(TAG, "Publishing message %s: id %s", messageLogPayload(message), componentIdToString[message.id].c_str());
    if (xQueueSend(xRouterQueue, &message, portMAX_DELAY) != pdPASS) {
        ESP_LOGE(TAG, "Failed to send message to router queue");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t MessageRouter::subscribe(std::shared_ptr<ComponentInterface> subscriber, ComponentId sourceComponentId) {
    ESP_LOGI(TAG, "Component %s subscribed to messages from: %s",
             componentIdToString[subscriber->getComponentId()].c_str(), componentIdToString[sourceComponentId].c_str());

    if (subscriber != nullptr) {
        std::lock_guard<std::recursive_mutex> lock(dbMutex);
        subscribersDB[sourceComponentId].push_back(subscriber);
    }

    // printSubscribersDb();
    return ESP_OK;
}

esp_err_t MessageRouter::unsubscribe(std::shared_ptr<ComponentInterface> subscriber, ComponentId sourceComponentId) {
    ESP_LOGI(TAG, "Component %s unsubscribed from messages from: %s",
             componentIdToString[subscriber->getComponentId()].c_str(), componentIdToString[sourceComponentId].c_str());

    if (subscriber != nullptr) {
        std::lock_guard<std::recursive_mutex> lock(dbMutex);
        auto it = subscribersDB.find(sourceComponentId);
        if (it != subscribersDB.end()) {
            auto &subscribers = it->second;
            subscribers.erase(std::remove(subscribers.begin(), subscribers.end(), subscriber), subscribers.end());
        }
    }

    // printSubscribersDb();
    return ESP_OK;
}

} // namespace messageRouter