#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define TM1637_CLK_PIN GPIO_NUM_7
#define TM1637_DIO_PIN GPIO_NUM_15
#define TAG "HW069_TM1637"

// Segment mapping for digits 0-9
static const uint8_t digit_map[] = {
    0x3f, // 0
    0x06, // 1
    0x5b, // 2
    0x4f, // 3
    0x66, // 4
    0x6d, // 5
    0x7d, // 6
    0x07, // 7
    0x7f, // 8
    0x6f  // 9
};

// Low-level bit-bang helpers
static void tm1637_start(void) {
    gpio_set_direction(TM1637_DIO_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(TM1637_CLK_PIN, 1);
    gpio_set_level(TM1637_DIO_PIN, 1);
    esp_rom_delay_us(5);
    gpio_set_level(TM1637_DIO_PIN, 0);
    esp_rom_delay_us(5);
    gpio_set_level(TM1637_CLK_PIN, 0);
}

static void tm1637_stop(void) {
    gpio_set_direction(TM1637_DIO_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(TM1637_CLK_PIN, 0);
    gpio_set_level(TM1637_DIO_PIN, 0);
    esp_rom_delay_us(5);
    gpio_set_level(TM1637_CLK_PIN, 1);
    gpio_set_level(TM1637_DIO_PIN, 1);
    esp_rom_delay_us(5);
}

static void tm1637_write_byte(uint8_t b) {
    gpio_set_direction(TM1637_DIO_PIN, GPIO_MODE_OUTPUT);
    for (int i = 0; i < 8; ++i) {
        gpio_set_level(TM1637_CLK_PIN, 0);
        gpio_set_level(TM1637_DIO_PIN, (b >> i) & 0x01);
        esp_rom_delay_us(5);
        gpio_set_level(TM1637_CLK_PIN, 1);
        esp_rom_delay_us(5);
    }

    // ACK cycle
    gpio_set_level(TM1637_CLK_PIN, 0);
    gpio_set_direction(TM1637_DIO_PIN, GPIO_MODE_INPUT);
    esp_rom_delay_us(5);
    gpio_set_level(TM1637_CLK_PIN, 1);
    esp_rom_delay_us(5);
    gpio_set_level(TM1637_CLK_PIN, 0);
}

void tm1637_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << TM1637_CLK_PIN) | (1ULL << TM1637_DIO_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    gpio_set_level(TM1637_CLK_PIN, 1);
    gpio_set_level(TM1637_DIO_PIN, 1);
}

void tm1637_display_number(uint16_t num, bool show_colon) {
    uint8_t digits[4];
    digits[0] = digit_map[(num / 1000) % 10];
    digits[1] = digit_map[(num / 100) % 10];
    digits[2] = digit_map[(num / 10) % 10];
    digits[3] = digit_map[num % 10];

    // Toggle colon bit on 2nd digit if colon is supported on display
    if (show_colon) {
        digits[1] |= 0x80;
    }

    // 1. Data command setting (Auto-increment address mode)
    tm1637_start();
    tm1637_write_byte(0x40);
    tm1637_stop();

    // 2. Address command setting (Start at digit 0)
    tm1637_start();
    tm1637_write_byte(0xC0);
    for (int i = 0; i < 4; i++) {
        tm1637_write_byte(digits[i]);
    }
    tm1637_stop();

    // 3. Display control command (Turn display ON, max brightness = 0x88 | 0x07)
    tm1637_start();
    tm1637_write_byte(0x8f);
    tm1637_stop();
}
