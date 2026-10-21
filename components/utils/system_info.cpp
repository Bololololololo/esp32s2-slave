#include "system_info.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_private/esp_clk.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "system_info.h"

static const char *TAG = "sys_info";

namespace utils {
using namespace std;

SystemInfo::SystemInfo() {
    ESP_LOGI(TAG, "Initializing system info");
}

SystemInfo::~SystemInfo() {
    ESP_LOGI(TAG, "Deinitializing system info");
}

uint32_t SystemInfo::getCpuFreqMHz(void) {
    return esp_clk_cpu_freq() / 1000000;
}

size_t SystemInfo::getTotalFreeDRAMSizeKB(void) {
    return esp_get_free_heap_size() / 1024;
}

size_t SystemInfo::getFreeInternalDRAMSizeKB(void) {
    return heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL) / 1024;
}

size_t SystemInfo::getTotalPsramSize(void) {
    return esp_psram_get_size() / 1024;
}

size_t SystemInfo::getTotalInternalSizeKB(void) {
    return heap_caps_get_total_size(MALLOC_CAP_INTERNAL) / 1024;
}
} // namespace utils