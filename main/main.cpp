/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "message_router.h"
#include "sdkconfig.h"
#include "service_manager.h"
#include "status_led.h"
#include "wifi_service.h"
#include <stdio.h>

using namespace utils;
using namespace services;
using namespace wifi;

#define LOG_LEVEL ESP_LOG_INFO

static const char *TAG = "main";

extern "C" {
void app_main(void);
}

void app_main(void) {
  /* Set log level  */
  esp_log_level_set("*", LOG_LEVEL);

  /* Configure the peripheral according to the LED type */
  StatusLED status_led;
  status_led.setColor(utils::StatusLEDColor::RED);

  /* Start the WiFi service*/
  ServiceManager *serviceManager = ServiceManager::getInstance();
  ESP_ERROR_CHECK(serviceManager->startService(ServiceType::SERVICE_MSGR));
  ESP_ERROR_CHECK(serviceManager->startService(ServiceType::SERVICE_WIFI));

  /* Start the wifi provisioning */
  const esp_err_t ret = messageRouter::MessageRouter::getInstance()->publish(
      Message(COMPONENT_ID_LVGL_DISPLAY, "wifi:provision"));
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to publish Wi-Fi mode selection: %s",
             esp_err_to_name(ret));
  }

  status_led.setColor(utils::StatusLEDColor::GREEN);

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
