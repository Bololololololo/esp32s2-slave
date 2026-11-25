#pragma once

#include "esp_err.h"

void mqtt_app_start(void);
esp_err_t mqtt_queue_payload(const char *payload);
