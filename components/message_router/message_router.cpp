#include "message_router.h"
#include "esp_check.h"
#include <algorithm>
#include <cstring>
#include <esp_log.h>
#include <utility>

static const char *TAG = "message_router";

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
    ESP_LOGI(TAG, "Current Sender Subscriptions:");
    auto &subscribersDB = MessageRouter::getInstance()->getSourceSubscribersDB();
    for (const auto &entry : subscribersDB) {
        ComponentId sender = entry.first;
        const auto &subscribers = entry.second;

        ESP_LOGI(TAG, "Sender Component ID: %s", componentIdToString[sender].c_str());
        for (const auto &subscriber : subscribers) {
            ESP_LOGI(TAG, "  Subscriber Component ID: %s", componentIdToString[subscriber->getComponentId()].c_str());
        }
    }
    ESP_LOGI(TAG, "Current Recipient Subscriptions:");
    auto &recipientSubscribersDB = MessageRouter::getInstance()->getRecipientSubscribersDB();
    for (const auto &entry : recipientSubscribersDB) {
        ESP_LOGI(TAG, "Message recipient: %s", componentIdToString[entry.first].c_str());
        for (const auto &subscriber : entry.second) {
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
            ESP_LOGI(TAG, "Received message %s: from %s to %s", messageLogPayload(receivedMsg),
                     componentIdToString[receivedMsg.sender].c_str(),
                     componentIdToString[receivedMsg.recipient].c_str());
            {
                // Copy subscribers while locked, then call handlers without holding the mutex.
                std::vector<std::shared_ptr<ComponentInterface>> subscribers_to_notify;
                std::vector<std::shared_ptr<ComponentInterface>> wildcard_subscribers_copy;

                {
                    std::lock_guard<std::recursive_mutex> lock(router->getDbMutex());
                    auto &sourceSubscribersDB = router->getSourceSubscribersDB();
                    auto &recipientSubscribersDB = router->getRecipientSubscribersDB();

                    auto sourceIt = sourceSubscribersDB.find(receivedMsg.sender);
                    if (sourceIt != sourceSubscribersDB.end()) {
                        for (const auto &subscriber : sourceIt->second) {
                            if (subscriber->getComponentId() != receivedMsg.sender &&
                                std::find(subscribers_to_notify.begin(), subscribers_to_notify.end(), subscriber) ==
                                    subscribers_to_notify.end()) {
                                subscribers_to_notify.push_back(subscriber);
                            }
                        }
                    }

                    if (receivedMsg.recipient != COMPONENT_ID_ALL) {
                        auto recipientIt = recipientSubscribersDB.find(receivedMsg.recipient);
                        if (recipientIt != recipientSubscribersDB.end()) {
                            for (const auto &subscriber : recipientIt->second) {
                                if (std::find(subscribers_to_notify.begin(), subscribers_to_notify.end(), subscriber) ==
                                    subscribers_to_notify.end()) {
                                    subscribers_to_notify.push_back(subscriber);
                                }
                            }
                        }
                    }

                    auto wildcardIt = sourceSubscribersDB.find(COMPONENT_ID_ALL);
                    if (wildcardIt != sourceSubscribersDB.end()) {
                        for (const auto &subscriber : wildcardIt->second) {
                            if (subscriber->getComponentId() != receivedMsg.sender &&
                                std::find(subscribers_to_notify.begin(), subscribers_to_notify.end(), subscriber) ==
                                    subscribers_to_notify.end()) {
                                wildcard_subscribers_copy.push_back(subscriber);
                            }
                        }
                    }
                }

                for (const auto &subscriber : subscribers_to_notify) {
                    subscriber->messageHandler(receivedMsg);
                }

                for (const auto &subscriber : wildcard_subscribers_copy) {
                    subscriber->messageHandler(receivedMsg);
                }

                if (subscribers_to_notify.empty() && wildcard_subscribers_copy.empty()) {
                    ESP_LOGW(TAG, "No subscribers for message from %d to %d",
                             static_cast<int>(receivedMsg.sender), static_cast<int>(receivedMsg.recipient));
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
    ESP_LOGI(TAG, "Publishing message %s: from %s to %s", messageLogPayload(message),
             componentIdToString[message.sender].c_str(), componentIdToString[message.recipient].c_str());
    if (xQueueSend(xRouterQueue, &message, portMAX_DELAY) != pdPASS) {
        ESP_LOGE(TAG, "Failed to send message to router queue");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t MessageRouter::subscribe(std::shared_ptr<ComponentInterface> subscriber) {
    if (subscriber == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Component %s subscribed to messages addressed to itself",
             componentIdToString[subscriber->getComponentId()].c_str());

    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    auto &subscribers = recipientSubscribersDB[subscriber->getComponentId()];
    if (std::find(subscribers.begin(), subscribers.end(), subscriber) == subscribers.end()) {
        subscribers.push_back(std::move(subscriber));
    }
    return ESP_OK;
}

esp_err_t MessageRouter::subscribe(std::shared_ptr<ComponentInterface> subscriber, ComponentId sender) {
    if (subscriber == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Component %s subscribed to messages from: %s",
             componentIdToString[subscriber->getComponentId()].c_str(), componentIdToString[sender].c_str());

    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    auto &subscribers = sourceSubscribersDB[sender];
    if (std::find(subscribers.begin(), subscribers.end(), subscriber) == subscribers.end()) {
        subscribers.push_back(std::move(subscriber));
    }

    return ESP_OK;
}

esp_err_t MessageRouter::unsubscribe(std::shared_ptr<ComponentInterface> subscriber) {
    if (subscriber == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Component %s unsubscribed from messages addressed to itself",
             componentIdToString[subscriber->getComponentId()].c_str());

    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    auto it = recipientSubscribersDB.find(subscriber->getComponentId());
    if (it != recipientSubscribersDB.end()) {
        auto &subscribers = it->second;
        subscribers.erase(std::remove(subscribers.begin(), subscribers.end(), subscriber), subscribers.end());
        if (subscribers.empty()) {
            recipientSubscribersDB.erase(it);
        }
    }
    return ESP_OK;
}

esp_err_t MessageRouter::unsubscribe(std::shared_ptr<ComponentInterface> subscriber, ComponentId sender) {
    if (subscriber == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Component %s unsubscribed from messages from: %s",
             componentIdToString[subscriber->getComponentId()].c_str(), componentIdToString[sender].c_str());

    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    auto it = sourceSubscribersDB.find(sender);
    if (it != sourceSubscribersDB.end()) {
        auto &subscribers = it->second;
        subscribers.erase(std::remove(subscribers.begin(), subscribers.end(), subscriber), subscribers.end());
        if (subscribers.empty()) {
            sourceSubscribersDB.erase(it);
        }
    }
    return ESP_OK;
}

} // namespace messageRouter