#include "proximity_sensor_comp.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "message_router.h"
#include <stdio.h>

#define TRIG_PIN GPIO_NUM_5
#define ECHO_PIN GPIO_NUM_6
#define TAG "HC_SR04"

using namespace messageRouter;

namespace sensors {

ProximitySensorComponent::ProximitySensorComponent() : ComponentInterface(COMPONENT_ID_PROXIMITY_SENSOR) {
    MessageRouter::getInstance()->subscribe(std::make_shared<ProximitySensorComponent>(*this));
}

void ProximitySensorComponent::init() {
    ESP_LOGI(TAG, "Initializing proximity sensor...");
    // Configure Trigger pin as output
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << TRIG_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Configure Echo pin as input with an optional pull-down
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << ECHO_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpio_config(&io_conf);

    // Ensure trigger starts low
    gpio_set_level(TRIG_PIN, 0);

    ESP_LOGI(TAG, "Proximity sensor initialized.");
}

void ProximitySensorComponent::deinit() {
    // Deinitialize the proximity sensor hardware here
}

esp_err_t ProximitySensorComponent::messageHandler(const Message &msg) {
    // Handle incoming messages related to the proximity sensor here
    return ESP_OK;
}

float ProximitySensorComponent::measureDistance() {
    // 1. Send a 10 µs HIGH pulse to the TRIG pin
    gpio_set_level(TRIG_PIN, 1);
    esp_rom_delay_us(10);
    gpio_set_level(TRIG_PIN, 0);

    // 2. Wait for the ECHO pin to go HIGH (with a 30ms timeout safeguard)
    int64_t start_time = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 0) {
        if ((esp_timer_get_time() - start_time) > 30000) {
            return -1.0f; // Timeout / No object detected
        }
    }

    // 3. Measure how long the ECHO pin stays HIGH
    int64_t echo_start = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 1) {
        if ((esp_timer_get_time() - echo_start) > 30000) {
            return -1.0f; // Out of range
        }
    }
    int64_t echo_end = esp_timer_get_time();

    int64_t pulse_duration_us = echo_end - echo_start;

    // 4. Calculate distance: Speed of sound is ~0.0343 cm/µs.
    // Divide by 2 because the wave travels out and back.
    float distance_cm = (pulse_duration_us * 0.0343f) / 2.0f;

    ESP_LOGI(TAG, "Measured distance: %.2f cm", distance_cm);
    return distance_cm;
}

} // namespace sensors