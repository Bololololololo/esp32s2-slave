#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <stdlib.h>

namespace utils {
// Linker symbols generated for main/certs/cert.pem
extern const char certificate_pem_start[] asm("_binary_cert_pem_start");
extern const char certificate_pem_end[] asm("_binary_cert_pem_end");

const char *TAG = "NvsHelpers";

class NvsHelpers {
  public:
    static void init_nvs_with_reset_on_failure(void) {
        esp_err_t err = nvs_flash_init();

        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            // Partition structure changed or ran out of pages; attempt recovery erase
            ESP_LOGW("NVS", "NVS partition needs erasing due to version mismatch or full pages.");
            ESP_ERROR_CHECK(nvs_flash_erase());
            err = nvs_flash_init();
        }

        if (err != ESP_OK) {
            ESP_LOGE("NVS", "Critical NVS initialization failure: %s (0x%x)", esp_err_to_name(err), err);

            // Give the UART log buffer time to transmit before restarting
            vTaskDelay(pdMS_TO_TICKS(500));

            // Trigger a software reset of the ESP32 board
            esp_restart();
        }

        ESP_LOGI("NVS", "NVS initialized successfully.");
    }

    // 1. Overload for 32-bit Signed Integers (passed by reference)
    static esp_err_t read_nvs_value(const char *key, int32_t &out_value) {
        nvs_handle_t handle;
        esp_err_t err = nvs_open("storage", NVS_READONLY, &handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
            return err;
        }

        err = nvs_get_i32(handle, key, &out_value);
        nvs_close(handle);
        return err;
    }

    // 2. Overload for Null-Terminated Strings
    static esp_err_t read_nvs_value(const char *key, char *out_buf, size_t max_len) {
        nvs_handle_t handle;
        esp_err_t err = nvs_open("storage", NVS_READONLY, &handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
            return err;
        }

        size_t required_len = max_len;
        err = nvs_get_str(handle, key, out_buf, &required_len);
        nvs_close(handle);
        return err;
    }

    // 3. Overload for Binary Blobs (structs/raw bytes)
    static esp_err_t read_nvs_value(const char *key, void *out_data, size_t *io_len) {
        nvs_handle_t handle;
        ESP_LOGI(TAG, "Reading binary blob from NVS: key='%s', expected size=%zu bytes", key, *io_len);
        esp_err_t err = nvs_open("storage", NVS_READONLY, &handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
            return err;
        }

        err = nvs_get_blob(handle, key, out_data, io_len);
        nvs_close(handle);
        return err;
    }

    // 1. Overload for 32-bit Signed Integers
    static esp_err_t write_nvs_value(const char *key, int32_t value) {
        nvs_handle_t handle;
        esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
            return err;
        }

        err = nvs_set_i32(handle, key, value);
        if (err == ESP_OK) {
            err = nvs_commit(handle);
        }

        nvs_close(handle);
        return err;
    }

    // 2. Overload for Null-Terminated Strings
    static esp_err_t write_nvs_value(const char *key, const char *value) {
        nvs_handle_t handle;
        esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
            return err;
        }

        ESP_LOGI(TAG, "Writing string to NVS: key='%s', value='%s'", key, value);
        err = nvs_set_str(handle, key, value);
        if (err == ESP_OK) {
            err = nvs_commit(handle);
        }

        ESP_LOGI(TAG, "NVS write operation completed with status: %s", esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }

    // 3. Overload for Binary Blobs (e.g., structs, raw byte arrays)
    static esp_err_t write_nvs_value(const char *key, const void *data, size_t length) {
        nvs_handle_t handle;
        esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
            return err;
        }

        err = nvs_set_blob(handle, key, data, length);
        if (err == ESP_OK) {
            err = nvs_commit(handle);
        }

        nvs_close(handle);
        return err;
    }

    static void check_and_store_embedded_certificate(void) {
        // Calculate exact size of the embedded file
        size_t cert_len = certificate_pem_end - certificate_pem_start;

        ESP_LOGI("CERT", "Found embedded certificate of size: %u bytes", cert_len);

        char check_buf[16] = {0};
        size_t check_buf_len = sizeof(check_buf);
        esp_err_t err = read_nvs_value("root_ca", check_buf, &check_buf_len);

        if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGI("NVS", "Certificate not found in NVS. Writing from embedded repo file...");

            // Use your binary blob overload to write the repo file into NVS
            esp_err_t err = write_nvs_value("root_ca", certificate_pem_start, cert_len);
            if (err != ESP_OK) {
                ESP_LOGE("NVS", "Failed to write certificate to NVS: %s", esp_err_to_name(err));
            }
        } else {
            ESP_LOGI("NVS", "Certificate already exists in NVS. No action needed. Error code: %s",
                     esp_err_to_name(err));
        }
    }

    static void print_all_nvs_entries(void) {
        ESP_LOGI(TAG, "--- Starting NVS Flash Dump ---");

        nvs_iterator_t it = NULL;
        esp_err_t err = nvs_entry_find("nvs", NULL, NVS_TYPE_ANY, &it);

        if (err != ESP_OK || it == NULL) {
            ESP_LOGI(TAG, "No NVS entries found in partition.");
            return;
        }

        size_t count = 0;
        while (err == ESP_OK && it != NULL) {
            nvs_entry_info_t info;
            nvs_entry_info(it, &info);
            count++;

            ESP_LOGI(TAG, "[%u] Namespace: '%s' | Key: '%s' | Type: %d", (unsigned int)count, info.namespace_name,
                     info.key, info.type);

            // Open the specific namespace to read the actual value based on its type
            nvs_handle_t handle;
            if (nvs_open(info.namespace_name, NVS_READONLY, &handle) == ESP_OK) {

                if (info.type == NVS_TYPE_I32) {
                    int32_t val = 0;
                    if (nvs_get_i32(handle, info.key, &val) == ESP_OK) {
                        ESP_LOGI(TAG, "      Value (i32): %ld", (long)val);
                    }
                } else if (info.type == NVS_TYPE_U32) {
                    uint32_t val = 0;
                    if (nvs_get_u32(handle, info.key, &val) == ESP_OK) {
                        ESP_LOGI(TAG, "      Value (u32): %lu", (unsigned long)val);
                    }
                } else if (info.type == NVS_TYPE_STR) {
                    size_t required_size = 0;
                    if (nvs_get_str(handle, info.key, NULL, &required_size) == ESP_OK) {
                        char *val_buf = (char *)malloc(required_size);
                        if (val_buf) {
                            if (nvs_get_str(handle, info.key, val_buf, &required_size) == ESP_OK) {
                                ESP_LOGI(TAG, "      Value (str): %s", val_buf);
                            }
                            free(val_buf);
                        }
                    }
                } else if (info.type == NVS_TYPE_BLOB) {
                    ESP_LOGI(TAG, "      Value (blob): [Binary Data]");
                } else {
                    ESP_LOGI(TAG, "      Value: [Other supported type]");
                }

                nvs_close(handle);
            }

            // Advance to the next entry (passes address of iterator in newer IDF versions)
            err = nvs_entry_next(&it);
        }

        ESP_LOGI(TAG, "--- NVS Dump Complete (Total items: %u) ---", (unsigned int)count);
    }
};

} // namespace utils
