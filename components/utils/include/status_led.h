/*
 * This header file declares functions for controlling a status LED.
 * The LED can be configured to blink using either a GPIO or an addressable LED strip.
 * The implementation details are in the corresponding source file.
 */

#ifndef STATUS_LED_H
#define STATUS_LED_H

#include "led_strip.h"

namespace utils
{
    enum class StatusLEDColor
    {
        OFF = 0,
        RED,
        GREEN,
        BLUE,
        YELLOW,
    };

    class StatusLED
    {
    public:
        StatusLED();
        ~StatusLED();

        void setColor(StatusLEDColor led_color);

    private:
        led_strip_handle_t led_strip{NULL};
    };
}

#endif // STATUS_LED_H
