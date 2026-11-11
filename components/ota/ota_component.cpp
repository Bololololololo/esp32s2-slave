#include "ota_component.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_wifi.h"
#include "esp_wifi_remote.h"
#include "message_router.h"

static const char *TAG = "ota_update";

using namespace messageRouter;

namespace ota {

OTAComponent::OTAComponent(std::shared_ptr<OTAUpdateNotifier> updateNotifier)
    : ComponentInterface(COMPONENT_ID_OTA), updateNotifier(std::move(updateNotifier)) {
    std::strncpy(server_cert_pem,
                 "-----BEGIN CERTIFICATE-----\n"
                 "MIIDETCCAfmgAwIBAgIUV5BWU/C+tuaOn9eYaxFCVmTZ3rswDQYJKoZIhvcNAQEL\n"
                 "BQAwGDEWMBQGA1UEAwwNMTkyLjE2OC4xLjEyODAeFw0yNjExMDYxNDAwMTdaFw0y\n"
                 "NzExMDYxNDAwMTdaMBgxFjAUBgNVBAMMDTE5Mi4xNjguMS4xMjgwggEiMA0GCSqG\n"
                 "SIb3DQEBAQUAA4IBDwAwggEKAoIBAQCxWqs7zCyAX3U7n5/JUgn2ujG7w9Gib2Rp\n"
                 "XdJFM9oMXUYQleJrxo0ZOPDyZo2jXyF1HK2QOjbyYSVQe0J0UzVQ2Oryk8PtQ7C+\n"
                 "M5155ARN2pYsN2YjFtZBQJRQoKwAhRkkncqF6qv+SmP61Qyefj13k2TkD7qEH8yW\n"
                 "u1ehYslQnKUiKw3BVdL5eBdGf3oYpoq/crAwbkG8yhqdWgzH1kqJVx+qGxqnoZc0\n"
                 "cW+4uRFOROm4nxo8rtj4kFBK1ct/NIHS/QM/FDiiXl8dmwMxFHljihpiWIPp1M7/\n"
                 "O6jFFQtiVGsd/bBUxfkl0e2BxnKbIZBopGZM2Sm9ADhSwNlDF6YBAgMBAAGjUzBR\n"
                 "MB0GA1UdDgQWBBSJA3uDLCQAX9erlXGSK11sL4hHHjAfBgNVHSMEGDAWgBSJA3uD\n"
                 "LCQAX9erlXGSK11sL4hHHjAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUA\n"
                 "A4IBAQCE8hYCL8rLZGlsfYno83HGxOFjnrgtwTNciqDGSsiKyQXvoo/WqZaPygKj\n"
                 "IL7obqUDHCr/gwds/pwFQ/95traBa/VaxMedot3RosSf7xOUKBgiGiqGYRrydx16\n"
                 "EApX+p/WhoGgQ32+AQIainkj/gVB8TpCgC+lMNKM8SK8s06ootzQLxuU9GigVEaS\n"
                 "4SUepbAq5DdlcQymAAtlAPicLl0cYt6v7sMzCXNtxyG3r4zCdiaNLBu87JdSbObn\n"
                 "wIUXqAsZzgu2IkTMZpHDRqNyR2q6n2WetxGV/0eDk3VTQaYZcK0b7hisF2VEshEW\n"
                 "1rl46m9wWUEW0xuVMb8ytbJSgJze\n"
                 "-----END CERTIFICATE-----\n",
                 2500); // Ensure null-termination

    config = {
        .url = "",                   // Replace with your OTA update URL
        .cert_pem = server_cert_pem, // Pass CA certificate if using HTTPS/TLS
        .timeout_ms = 10000,
    };
}

esp_err_t OTAComponent::init() {
    if (subscribed) {
        return ESP_ERR_INVALID_STATE;
    }
    const auto self = shared_from_this();
    const esp_err_t result = MessageRouter::getInstance()->subscribe(self);
    if (result == ESP_OK) {
        subscribed = true;
    }
    if (result != ESP_OK) {
        return result;
    }

    const esp_err_t wifiResult = MessageRouter::getInstance()->subscribe(self, COMPONENT_ID_WIFI);
    if (wifiResult != ESP_OK) {
        MessageRouter::getInstance()->unsubscribe(self);
        subscribed = false;
        return wifiResult;
    }
    wifiStatusSubscribed = true;
    return ESP_OK;
}

void OTAComponent::deinit() {
    const auto self = shared_from_this();
    if (subscribed) {
        MessageRouter::getInstance()->unsubscribe(self);
        subscribed = false;
    }
    if (wifiStatusSubscribed) {
        MessageRouter::getInstance()->unsubscribe(self, COMPONENT_ID_WIFI);
        wifiStatusSubscribed = false;
    }
    updateNotifier->disable();
}

esp_err_t OTAComponent::messageHandler(const Message &msg) {
#ifdef CONFIG_ENABLE_SERVICE_WIFI
    if (msg.sender == ComponentId::COMPONENT_ID_WIFI && std::strncmp(msg.payload, "wifi_status:connected", 21) == 0) {
        updateNotifier->notify();
        return ESP_OK;
    }
#endif

    if (msg.recipient == ComponentId::COMPONENT_ID_OTA && std::strncmp(msg.payload, "ota_update:check", 16) == 0) {
        updateNotifier->notify();
        return ESP_OK;
    }

    return ESP_ERR_INVALID_ARG;
}

std::string OTAComponent::fetch_version_info() {
    ESP_LOGI(TAG, "Fetching version info from server...");

    char buffer[256];
    int read_len = 0;

    config.url = "https://192.168.1.128:8070/version.txt";

    ESP_LOGI(TAG, "Printing server certificate:\n%s", config.cert_pem);
    ESP_LOGI(TAG, "Printing OTA URL: %s", config.url);

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize ESP HTTP client");
        return "";
    }

    // Open connection and send HTTP GET request
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return "";
    }

    // Fetch response headers from the server
    int content_length = esp_http_client_fetch_headers(client);
    int status_code = esp_http_client_get_status_code(client);

    ESP_LOGI(TAG, "HTTP Status = %d, Content-Length = %d", status_code, content_length);

    if (status_code == 200) {
        // Read data stream chunk by chunk until EOF
        while (true) {
            read_len = esp_http_client_read(client, buffer, sizeof(buffer) - 1);
            if (read_len < 0) {
                ESP_LOGE(TAG, "Read failed from data stream");
                break;
            } else if (read_len == 0) {
                ESP_LOGI(TAG, "Finished reading file.");
                break;
            }

            // Null-terminate and process the received chunk
            buffer[read_len] = '\0';
            ESP_LOGI(TAG, "Data chunk: %s", buffer);
        }
    } else {
        ESP_LOGE(TAG, "Server returned non-200 status code: %d", status_code);
    }

    // Clean up and release client memory
    esp_http_client_cleanup(client);
    return std::string(buffer);
}

esp_err_t OTAComponent::check_ota_update() {
    ESP_LOGI(TAG, "Checking for OTA update...");

    std::string version_info = fetch_version_info();

    if (version_info.empty()) {
        ESP_LOGE(TAG, "Failed to fetch version info.");
        return ESP_FAIL;
    }

    if (FIRMWARE_VERSION != version_info) {
        ESP_LOGI(TAG, "New version available: %s. Current version: %s", version_info.c_str(), FIRMWARE_VERSION);
        return run_ota_update();
    } else {
        ESP_LOGI(TAG, "No new version available. Current version: %s", FIRMWARE_VERSION);
    }
    return ESP_OK;
}

esp_err_t OTAComponent::run_ota_update() {
    this->config.url = "https://192.168.1.128:8070/firmware.bin";

    esp_https_ota_config_t ota_config = {
        .http_config = &this->config,
    };

    ESP_LOGI(TAG, "Starting OTA update from URL: %s", this->config.url);

    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA Update successful. Restarting system...");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA Update failed with error: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t OTAComponent::checkWifiConnection() {
    wifi_ap_record_t ap_info;
    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Wi-Fi is connected to SSID: %s", ap_info.ssid);
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Wi-Fi is not connected. Error: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
}

} // namespace ota