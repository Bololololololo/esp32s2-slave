#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void tm1637_display_number(uint16_t num, bool show_colon);
void tm1637_init(void);

#ifdef __cplusplus
}
#endif
