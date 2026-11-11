#pragma once

#include "component_interface.h"
#include "esp_http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <memory>
#include <mutex>

namespace ota {

#define FIRMWARE_VERSION CONFIG_FIRMWARE_VERSION

class OTAUpdateNotifier {
  public:
    explicit OTAUpdateNotifier(TaskHandle_t taskHandle) : taskHandle(taskHandle) {
    }

    void notify() {
        std::lock_guard<std::mutex> lock(mutex);
        if (taskHandle != nullptr) {
            xTaskNotifyGive(taskHandle);
        }
    }

    void disable() {
        std::lock_guard<std::mutex> lock(mutex);
        taskHandle = nullptr;
    }

  private:
    std::mutex mutex;
    TaskHandle_t taskHandle;
};

class OTAComponent : public ComponentInterface, public std::enable_shared_from_this<OTAComponent> {
  public:
    explicit OTAComponent(std::shared_ptr<OTAUpdateNotifier> updateNotifier);
    esp_err_t init();
    void deinit();

    esp_err_t check_ota_update();
    esp_err_t messageHandler(const Message &msg) override;

    esp_http_client_config_t config;

  private:
    std::shared_ptr<OTAUpdateNotifier> updateNotifier;
    bool subscribed{false};
    bool wifiStatusSubscribed{false};
    char server_cert_pem[2500]; // Adjust size as needed

    std::string fetch_version_info();
    esp_err_t run_ota_update();
    esp_err_t checkWifiConnection();
};

} // namespace ota