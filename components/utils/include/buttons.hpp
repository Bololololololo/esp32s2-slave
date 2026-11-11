#pragma once
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <array>

class ButtonManager {
  public:
    static constexpr const char *TAG = "ButtonManager";
    std::array<gpio_num_t, 4> pins_;
    TickType_t poll_interval_;
    std::array<int, 4> last_states_;

    struct Config {
        std::initializer_list<gpio_num_t> pins;
        uint32_t poll_interval_ms = 50;
    };

    explicit ButtonManager(const Config &config) : poll_interval_(pdMS_TO_TICKS(config.poll_interval_ms)) {
        last_states_.fill(1); // Default HIGH state due to active-low internal pull-ups
        // Copy pins from the initializer_list into the fixed-size std::array
        std::copy(config.pins.begin(), config.pins.end(), pins_.begin());
    }

    // Initialize GPIO configuration
    void init() {
        uint64_t pin_bit_mask = 0;
        for (auto pin : pins_) {
            pin_bit_mask |= (1ULL << pin);
        }

        gpio_config_t io_conf = {.pin_bit_mask = pin_bit_mask,
                                 .mode = GPIO_MODE_INPUT,
                                 .pull_up_en = GPIO_PULLUP_ENABLE,
                                 .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                 .intr_type = GPIO_INTR_DISABLE};

        gpio_config(&io_conf);
        ESP_LOGI(TAG, "C++ ButtonManager initialized across %zu pins.", pins_.size());
    }

    // Spawn the FreeRTOS background polling task
    void start() {
        xTaskCreate(task_thunk, "button_task", 16384, this, 10, nullptr);
    }

  private:
    // Static thunk to bridge FreeRTOS C-style callback to C++ member function
    static void task_thunk(void *arg) {
        static_cast<ButtonManager *>(arg)->poll_loop();
    }

    void poll_loop() {
        while (true) {
            size_t index = 0;
            for (auto pin : pins_) {
                int state = gpio_get_level(pin);

                // Falling edge: HIGH -> LOW (Button Pressed)
                if (state == 0 && last_states_[index] == 1) {
                    on_press(index, pin);
                }
                // Rising edge: LOW -> HIGH (Button Released)
                else if (state == 1 && last_states_[index] == 0) {
                    on_release(index, pin);
                }

                last_states_[index] = state;
                index++;
            }
            vTaskDelay(poll_interval_);
        }
    }

    void on_press(size_t index, gpio_num_t pin) {
        ESP_LOGI(TAG, "Button %u (GPIO %d) Pressed!", static_cast<unsigned int>(index + 1), static_cast<int>(pin));
        // Add custom application event handling logic here
    }

    void on_release(size_t index, gpio_num_t pin) {
        ESP_LOGI(TAG, "Button %u (GPIO %d) Released.", static_cast<unsigned int>(index + 1), static_cast<int>(pin));
    }
};
