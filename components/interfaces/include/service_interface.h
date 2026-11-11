#ifndef _SERVICE_INTERFACE_H_
#define _SERVICE_INTERFACE_H_

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <atomic>
#include <cstdint>

namespace services {
enum ServiceType {
    SERVICE_WIFI,
    SERVICE_BLUETOOTH,
    SERVICE_GPS,
    SERVICE_SENSOR,
    SERVICE_CAMERA,
    SERVICE_GUI,
    SERVICE_MSGR,
    SERVICE_SENSORS,
    SERVICE_OTA,
    SERVICE_MAX
};

enum ServiceState {
    SERVICE_STOPPED,
    SERVICE_RUNNING,
    SERVICE_PAUSED,
    SERVICE_ERROR,
    SERVICE_STARTING
};

class ServiceInterface {
  public:
    ServiceInterface(ServiceType type, const char *taskName, uint32_t taskStackSize, UBaseType_t taskPriority = 5)
        : serviceType(type), taskName(taskName), taskStackSize(taskStackSize), taskPriority(taskPriority) {
    }
    virtual ~ServiceInterface() {
        if (taskStoppedSemaphore != nullptr) {
            vSemaphoreDelete(taskStoppedSemaphore);
        }
        if (startupSemaphore != nullptr) {
            vSemaphoreDelete(startupSemaphore);
        }
    }

    esp_err_t start() {
        if (taskActive.load()) {
            if (taskStoppedSemaphore == nullptr || xSemaphoreTake(taskStoppedSemaphore, 0) != pdTRUE) {
                return ESP_ERR_INVALID_STATE;
            }
            taskActive.store(false);
        }

        if (taskStoppedSemaphore == nullptr) {
            taskStoppedSemaphore = xSemaphoreCreateBinary();
            if (taskStoppedSemaphore == nullptr) {
                setServiceState(SERVICE_ERROR);
                return ESP_ERR_NO_MEM;
            }
        }
        if (startupSemaphore == nullptr) {
            startupSemaphore = xSemaphoreCreateBinary();
            if (startupSemaphore == nullptr) {
                setServiceState(SERVICE_ERROR);
                return ESP_ERR_NO_MEM;
            }
        }
        xSemaphoreTake(taskStoppedSemaphore, 0);
        xSemaphoreTake(startupSemaphore, 0);

        stopRequested.store(false);
        taskActive.store(true);
        setServiceState(SERVICE_STARTING);
        ESP_LOGI(taskName, "Creating service worker");
        TaskHandle_t worker = nullptr;
        const BaseType_t result =
            xTaskCreate(taskEntry, taskName, taskStackSize, this, taskPriority, &worker);
        if (result != pdPASS) {
            taskActive.store(false);
            setServiceState(SERVICE_ERROR);
            ESP_LOGE(taskName, "Failed to create service worker");
            return ESP_ERR_NO_MEM;
        }
        taskHandle.store(worker);
        ESP_LOGI(taskName, "Service worker created; releasing startup gate");
        xTaskNotifyGive(worker);
        ESP_LOGI(taskName, "Waiting for service initialization");
        if (xSemaphoreTake(startupSemaphore, portMAX_DELAY) != pdTRUE) {
            return ESP_FAIL;
        }
        ESP_LOGI(taskName, "Service initialization returned: %s", esp_err_to_name(startupResult));
        return startupResult;
    }

    esp_err_t stop() {
        if (!taskActive.load()) {
            return ESP_OK;
        }

        stopRequested.store(true);
        TaskHandle_t worker = taskHandle.load();
        if (worker != nullptr) {
            xTaskNotifyGive(worker);
        }
        if (xSemaphoreTake(taskStoppedSemaphore, portMAX_DELAY) != pdTRUE) {
            return ESP_FAIL;
        }
        taskActive.store(false);
        return ESP_OK;
    }

    ServiceType getServiceType() const {
        return serviceType;
    }
    ServiceState getServiceState() const {
        return state.load();
    }
    void setServiceState(ServiceState newState) {
        state.store(newState);
    }

  protected:
    virtual esp_err_t onStart() = 0;
    virtual void run() {
        waitForStop();
    }
    virtual void onStop() = 0;

    bool waitForStop(TickType_t timeout = portMAX_DELAY) {
        ulTaskNotifyTake(pdTRUE, timeout);
        return stopRequested.load();
    }

    bool isStopRequested() const {
        return stopRequested.load();
    }

  private:
    static void taskEntry(void *arg) {
        auto *service = static_cast<ServiceInterface *>(arg);
        ESP_LOGI(service->taskName, "Service worker entered; waiting on startup gate");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ESP_LOGI(service->taskName, "Service worker beginning initialization");
        const esp_err_t result = service->onStart();
        service->startupResult = result;
        if (result == ESP_OK) {
            service->setServiceState(SERVICE_RUNNING);
        } else {
            ESP_LOGE(service->taskName, "Service startup failed: %s", esp_err_to_name(result));
            service->setServiceState(SERVICE_ERROR);
        }
        xSemaphoreGive(service->startupSemaphore);
        if (result == ESP_OK && !service->isStopRequested()) {
            service->run();
        }

        service->onStop();
        if (result == ESP_OK && service->getServiceState() != SERVICE_ERROR) {
            service->setServiceState(SERVICE_STOPPED);
        }
        service->taskHandle.store(nullptr);
        xSemaphoreGive(service->taskStoppedSemaphore);
        vTaskDelete(nullptr);
    }

    ServiceType serviceType;
    const char *taskName;
    uint32_t taskStackSize;
    UBaseType_t taskPriority;
    std::atomic<ServiceState> state{SERVICE_STOPPED};
    std::atomic<TaskHandle_t> taskHandle{nullptr};
    std::atomic<bool> taskActive{false};
    SemaphoreHandle_t taskStoppedSemaphore = nullptr;
    SemaphoreHandle_t startupSemaphore = nullptr;
    esp_err_t startupResult = ESP_FAIL;
    std::atomic<bool> stopRequested{false};
};
} // namespace services

#endif // _SERVICE_INTERFACE_H_