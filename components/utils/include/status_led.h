/*
 * This header file declares functions for controlling a status LED.
 * The LED can be configured to blink using either a GPIO or an addressable LED strip.
 * The implementation details are in the corresponding source file.
 */

#ifndef STATUS_LED_H
#define STATUS_LED_H

#include "component_interface.h"
#include "led_strip.h"
#include <memory>

namespace utils {
enum class StatusLEDColor {
    OFF = 0,
    RED,
    GREEN,
    BLUE,
    YELLOW,
};

class StatusLED : public ComponentInterface, public std::enable_shared_from_this<StatusLED> {
  public:
    StatusLED();
    ~StatusLED();

    void setColor(StatusLEDColor led_color);

  private:
    esp_err_t messageHandler(const Message &msg) override;
    led_strip_handle_t led_strip{NULL};
    std::shared_ptr<StatusLED> messageSubscriber;
};
} // namespace utils

#endif // STATUS_LED_H
