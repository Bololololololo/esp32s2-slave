/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hw069.h"
#include "message_router.h"
#include "mqtt.h"
#include "sdkconfig.h"
#include "service_manager.h"
#include "status_led.h"
#include <stdio.h>

using namespace utils;
using namespace services;

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

    /* Start the services */
    ServiceManager *serviceManager = ServiceManager::getInstance();
#ifdef CONFIG_ENABLE_SERVICE_MSGR
    ESP_ERROR_CHECK(serviceManager->startService(ServiceType::SERVICE_MSGR));
#endif
#ifdef CONFIG_ENABLE_SERVICE_WIFI
    ESP_ERROR_CHECK(serviceManager->startService(ServiceType::SERVICE_WIFI));
#endif
#ifdef CONFIG_ENABLE_SERVICE_SENSORS
    ESP_ERROR_CHECK(serviceManager->startService(ServiceType::SERVICE_SENSORS));
#endif

    /* Start the MQTT component */
    std::shared_ptr<MqttComponent> mqttComponent = std::make_shared<MqttComponent>();
    ESP_ERROR_CHECK(mqttComponent->init());

    ESP_LOGI(TAG, "Initializing HW-069 (TM1637) display...");
    tm1637_init();

    uint16_t counter = 0;
    while (1) {
        tm1637_display_number(counter, (counter % 2 == 0));
        counter = (counter + 1) % 10000;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
