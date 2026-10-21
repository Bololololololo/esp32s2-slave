#include "status_led.h"

#include "status_led.h"
#include <stdio.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "rgb_led";

#define BLINK_GPIO CONFIG_BLINK_GPIO

#ifndef CONFIG_BLINK_LED_STRIP
#error "Please select LED strip backend in menuconfig"
#endif

namespace utils {

StatusLED::StatusLED() {
  ESP_LOGI(TAG, "Configuring status LED...");

  /* LED strip initialization with the GPIO and pixels number*/
  led_strip_config_t strip_config = {
      .strip_gpio_num = BLINK_GPIO,
      .max_leds = 1,                 // at least one LED on board
      .led_model = LED_MODEL_WS2812, // LED strip model
      .color_component_format =
          LED_STRIP_COLOR_COMPONENT_FMT_GRB, // color component format
      .flags = {
          .invert_out = 0,
      }};

#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
  led_strip_rmt_config_t rmt_config = {
      .clk_src = RMT_CLK_SRC_DEFAULT,
      .resolution_hz = (10 * 1000 * 1000), // 10MHz
      .mem_block_symbols = 0,              // use default size
      .flags = {
          .with_dma = false,
      }};
  ESP_ERROR_CHECK(
      led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
#elif CONFIG_BLINK_LED_STRIP_BACKEND_SPI
  led_strip_spi_config_t spi_config = {
      .spi_bus = SPI2_HOST,
      .flags.with_dma = true,
  };
  ESP_ERROR_CHECK(
      led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
#else
#error "unsupported LED strip backend"
#endif
  /* Set all LED off to clear all pixels */
  led_strip_clear(led_strip);
}

StatusLED::~StatusLED() {}

void StatusLED::setColor(StatusLEDColor led_color) {
  switch (led_color) {
  case StatusLEDColor::OFF:
    led_strip_clear(led_strip);
    break;
  case StatusLEDColor::RED:
    led_strip_set_pixel(led_strip, 0, 5, 0, 0);
    break;
  case StatusLEDColor::GREEN:
    led_strip_set_pixel(led_strip, 0, 0, 5, 0);
    break;
  case StatusLEDColor::BLUE:
    led_strip_set_pixel(led_strip, 0, 0, 0, 5);
    break;
  case StatusLEDColor::YELLOW:
    led_strip_set_pixel(led_strip, 0, 5, 5, 0);
    break;
  }
  led_strip_refresh(led_strip);
}
} // namespace utils
