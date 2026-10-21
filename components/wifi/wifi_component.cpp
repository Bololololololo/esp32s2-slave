#include "wifi_component.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_random.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "message_router.h"
#include "nvs_flash.h"
#include <cstdio>
#include <cstring>

using namespace messageRouter;

namespace wifi {
namespace {

static const char *TAG = "wifi";

static int hexValue(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}

static bool decodeFormValue(const char *value, size_t valueLength, char *output,
                            size_t outputSize) {
  size_t outputLength = 0;
  for (size_t i = 0; i < valueLength; ++i) {
    char decoded = value[i];
    if (decoded == '+') {
      decoded = ' ';
    } else if (decoded == '%') {
      if (i + 2 >= valueLength) {
        return false;
      }
      const int high = hexValue(value[i + 1]);
      const int low = hexValue(value[i + 2]);
      if (high < 0 || low < 0) {
        return false;
      }
      decoded = static_cast<char>((high << 4) | low);
      i += 2;
    }

    if (decoded == '\0' || outputLength + 1 >= outputSize) {
      return false;
    }
    output[outputLength++] = decoded;
  }
  output[outputLength] = '\0';
  return true;
}

static bool getFormField(const char *body, const char *fieldName, char *output,
                         size_t outputSize) {
  const size_t fieldNameLength = std::strlen(fieldName);
  const char *field = body;
  while (field && *field) {
    const char *end = std::strchr(field, '&');
    if (!end) {
      end = field + std::strlen(field);
    }

    const char *equals =
        static_cast<const char *>(std::memchr(field, '=', end - field));
    if (equals && static_cast<size_t>(equals - field) == fieldNameLength &&
        std::strncmp(field, fieldName, fieldNameLength) == 0) {
      return decodeFormValue(equals + 1, end - equals - 1, output, outputSize);
    }
    field = *end ? end + 1 : nullptr;
  }
  return false;
}

} // namespace

WifiComponent::WifiComponent()
    : ComponentInterface(ComponentId::COMPONENT_ID_WIFI) {}

esp_err_t WifiComponent::init() {
  if (m_initialized) {
    return ESP_ERR_INVALID_STATE;
  }

  esp_err_t ret = MessageRouter::getInstance()->subscribe(
      shared_from_this(), ComponentId::COMPONENT_ID_LVGL_DISPLAY);
  if (ret != ESP_OK) {
    return ret;
  }
  m_subscribed = true;

  ret = nvs_flash_init();
  if (ret != ESP_OK) {
    return ret;
  }
  ret = esp_netif_init();
  if (ret != ESP_OK) {
    return ret;
  }
  ret = esp_event_loop_create_default();
  if (ret != ESP_OK) {
    return ret;
  }

  m_stationNetif = esp_netif_create_default_wifi_sta();
  m_accessPointNetif = esp_netif_create_default_wifi_ap();
  if (!m_stationNetif || !m_accessPointNetif) {
    return ESP_FAIL;
  }

  const wifi_init_config_t initConfig = WIFI_INIT_CONFIG_DEFAULT();
  ret = esp_wifi_init(&initConfig);
  if (ret != ESP_OK) {
    return ret;
  }
  ret = esp_wifi_set_storage(WIFI_STORAGE_FLASH);
  if (ret != ESP_OK) {
    return ret;
  }
  ret = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                            wifiEventHandler, this,
                                            &m_wifiEventInstance);
  if (ret != ESP_OK) {
    return ret;
  }
  ret = esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, ipEventHandler, this, &m_ipEventInstance);
  if (ret != ESP_OK) {
    return ret;
  }

  m_initialized = true;
  ret = esp_wifi_set_mode(WIFI_MODE_STA);
  if (ret != ESP_OK) {
    return ret;
  }
  wifi_config_t stationConfig{};
  ret = esp_wifi_get_config(WIFI_IF_STA, &stationConfig);
  if (ret != ESP_OK) {
    return ret;
  }

  if (stationConfig.sta.ssid[0] != '\0') {
    ESP_LOGI(TAG, "Saved station credentials found; connecting");
    return applyMode("station");
  }

  ESP_LOGI(TAG, "No saved station credentials; starting the setup portal");
  return startProvisioningPortal();
}

void WifiComponent::stop() {
  if (m_subscribed) {
    MessageRouter::getInstance()->unsubscribe(
        shared_from_this(), ComponentId::COMPONENT_ID_LVGL_DISPLAY);
    m_subscribed = false;
  }

  stopPortalServer();

  if (m_wifiEventInstance) {
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                          m_wifiEventInstance);
    m_wifiEventInstance = nullptr;
  }
  if (m_ipEventInstance) {
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                          m_ipEventInstance);
    m_ipEventInstance = nullptr;
  }
  if (m_wifiStarted) {
    esp_wifi_stop();
    m_wifiStarted = false;
  }
  if (m_initialized) {
    esp_wifi_deinit();
    m_initialized = false;
  }
  if (m_accessPointNetif) {
    esp_netif_destroy_default_wifi(m_accessPointNetif);
    m_accessPointNetif = nullptr;
  }
  if (m_stationNetif) {
    esp_netif_destroy_default_wifi(m_stationNetif);
    m_stationNetif = nullptr;
  }
  m_stationConnectRequested = false;
  m_provisioning = false;
}

esp_err_t WifiComponent::messageHandler(const Message &msg) {
  if (msg.id != ComponentId::COMPONENT_ID_LVGL_DISPLAY) {
    return ESP_ERR_INVALID_ARG;
  }

  if (std::strncmp(msg.payload, "wifi:", 5) != 0) {
    return ESP_ERR_NOT_SUPPORTED;
  }
  const esp_err_t ret = applyMode(msg.payload + 5);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Could not apply Wi-Fi mode command: %s",
             esp_err_to_name(ret));
  }
  return ret;
}

esp_err_t WifiComponent::applyMode(const char *mode) {
  if (std::strcmp(mode, "disabled") == 0) {
    m_stationConnectRequested = false;
    m_provisioning = false;
    m_connected = false;
    stopPortalServer();
    if (m_wifiStarted) {
      const esp_err_t ret = esp_wifi_stop();
      if (ret != ESP_OK) {
        return ret;
      }
      m_wifiStarted = false;
    }
    ESP_LOGI(TAG, "Wi-Fi disabled");
    publishStatus("disabled");
    return ESP_OK;
  }

  if (std::strcmp(mode, "status") == 0) {
    if (m_provisioning) {
      char status[128];
      std::snprintf(status, sizeof(status), "portal|%s|%s", m_provisioningSsid,
                    m_provisioningPassword);
      publishStatus(status);
    } else if (m_connected) {
      publishStatus("connected");
    } else if (m_stationConnectRequested) {
      publishStatus("connecting");
    } else {
      publishStatus("disabled");
    }
    return ESP_OK;
  }

  if (std::strcmp(mode, "provision") == 0 || std::strcmp(mode, "ap") == 0) {
    return startProvisioningPortal();
  }

  if (std::strcmp(mode, "station") != 0) {
    ESP_LOGW(TAG, "Ignoring unknown Wi-Fi mode command");
    return ESP_ERR_INVALID_ARG;
  }

  wifi_config_t stationConfig{};
  esp_err_t ret =
      esp_wifi_set_mode(m_provisioning ? WIFI_MODE_APSTA : WIFI_MODE_STA);
  if (ret != ESP_OK) {
    return ret;
  }
  ret = esp_wifi_get_config(WIFI_IF_STA, &stationConfig);
  if (ret != ESP_OK) {
    return ret;
  }
  if (stationConfig.sta.ssid[0] == '\0') {
    ESP_LOGI(
        TAG,
        "Station credentials are not configured; starting the setup portal");
    return startProvisioningPortal();
  }

  char savedSsid[sizeof(stationConfig.sta.ssid) + 1]{};
  char savedPassword[sizeof(stationConfig.sta.password) + 1]{};
  std::memcpy(savedSsid, stationConfig.sta.ssid,
              sizeof(stationConfig.sta.ssid));
  std::memcpy(savedPassword, stationConfig.sta.password,
              sizeof(stationConfig.sta.password));
  return connectToNetwork(savedSsid, savedPassword);
}

esp_err_t WifiComponent::startProvisioningPortal() {
  if (m_provisioning && m_httpServer) {
    return ESP_OK;
  }

  uint8_t mac[6];
  esp_err_t ret = esp_read_mac(mac, ESP_MAC_WIFI_STA);
  if (ret != ESP_OK) {
    return ret;
  }
  std::snprintf(m_provisioningSsid, sizeof(m_provisioningSsid),
                "ESP32S2-Setup-%02X%02X", mac[4], mac[5]);

  uint8_t randomPassword[8];
  esp_fill_random(randomPassword, sizeof(randomPassword));
  for (size_t i = 0; i < sizeof(randomPassword); ++i) {
    std::snprintf(m_provisioningPassword + i * 2, 3, "%02x", randomPassword[i]);
  }

  wifi_config_t accessPointConfig{};
  std::strncpy(reinterpret_cast<char *>(accessPointConfig.ap.ssid),
               m_provisioningSsid, sizeof(accessPointConfig.ap.ssid) - 1);
  std::strncpy(reinterpret_cast<char *>(accessPointConfig.ap.password),
               m_provisioningPassword,
               sizeof(accessPointConfig.ap.password) - 1);
  accessPointConfig.ap.ssid_len = std::strlen(m_provisioningSsid);
  accessPointConfig.ap.channel = 1;
  accessPointConfig.ap.max_connection = 4;
  accessPointConfig.ap.authmode = WIFI_AUTH_WPA2_PSK;

  m_stationConnectRequested = false;
  m_connected = false;
  m_provisioning = true;
  ret = esp_wifi_set_mode(WIFI_MODE_AP);
  if (ret != ESP_OK) {
    return ret;
  }
  ret = esp_wifi_set_config(WIFI_IF_AP, &accessPointConfig);
  if (ret != ESP_OK) {
    return ret;
  }
  if (!m_wifiStarted) {
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
      return ret;
    }
    m_wifiStarted = true;
  }

  char portalStatus[128];
  std::snprintf(portalStatus, sizeof(portalStatus), "portal|%s|%s",
                m_provisioningSsid, m_provisioningPassword);
  publishStatus(portalStatus);
  ret = startPortalServer();
  if (ret != ESP_OK) {
    return ret;
  }
  ESP_LOGI(TAG, "Provisioning portal ready: SSID=%s URL=http://192.168.4.1/",
           m_provisioningSsid);
  return ESP_OK;
}

esp_err_t WifiComponent::connectToNetwork(const char *ssid,
                                          const char *password) {
  const size_t ssidLength = ssid ? std::strlen(ssid) : 0;
  const size_t passwordLength = password ? std::strlen(password) : 0;
  if (ssidLength == 0 || ssidLength > 32 ||
      (passwordLength > 0 && passwordLength < 8) || passwordLength > 63) {
    return ESP_ERR_INVALID_ARG;
  }

  wifi_config_t stationConfig{};
  std::memcpy(stationConfig.sta.ssid, ssid, ssidLength);
  std::memcpy(stationConfig.sta.password, password, passwordLength);
  stationConfig.sta.threshold.authmode =
      password[0] == '\0' ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;

  const bool keepPortal = m_provisioning;
  m_stationConnectRequested = false;
  m_connected = false;
  esp_err_t ret =
      esp_wifi_set_mode(keepPortal ? WIFI_MODE_APSTA : WIFI_MODE_STA);
  if (ret != ESP_OK) {
    return ret;
  }
  ret = esp_wifi_set_config(WIFI_IF_STA, &stationConfig);
  if (ret != ESP_OK) {
    return ret;
  }
  m_stationConnectRequested = true;

  if (!m_wifiStarted) {
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
      return ret;
    }
    m_wifiStarted = true;
  } else {
    ret = esp_wifi_connect();
    if (ret != ESP_OK) {
      return ret;
    }
  }
  ESP_LOGI(TAG, "Connecting to configured Wi-Fi network");
  publishStatus("connecting");
  return ESP_OK;
}

void WifiComponent::publishStatus(const char *status) {
  const esp_err_t ret = MessageRouter::getInstance()->publish(Message(
      ComponentId::COMPONENT_ID_WIFI, "wifi_status:" + std::string(status)));
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to publish Wi-Fi status: %s", esp_err_to_name(ret));
  }
}

esp_err_t WifiComponent::startPortalServer() {
  if (m_httpServer) {
    return ESP_OK;
  }

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 4;
  esp_err_t ret = httpd_start(&m_httpServer, &config);
  if (ret != ESP_OK) {
    m_httpServer = nullptr;
    return ret;
  }

  const httpd_uri_t getUri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = portalGetHandler,
      .user_ctx = this,
  };
  ret = httpd_register_uri_handler(m_httpServer, &getUri);
  if (ret != ESP_OK) {
    stopPortalServer();
    return ret;
  }

  const httpd_uri_t postUri = {
      .uri = "/connect",
      .method = HTTP_POST,
      .handler = portalPostHandler,
      .user_ctx = this,
  };
  ret = httpd_register_uri_handler(m_httpServer, &postUri);
  if (ret != ESP_OK) {
    stopPortalServer();
  }
  return ret;
}

void WifiComponent::stopPortalServer() {
  if (m_httpServer) {
    const esp_err_t ret = httpd_stop(m_httpServer);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to stop provisioning web server: %s",
               esp_err_to_name(ret));
    }
    m_httpServer = nullptr;
  }
}

void WifiComponent::wifiEventHandler(void *arg, esp_event_base_t eventBase,
                                     int32_t eventId, void *eventData) {
  auto *self = static_cast<WifiComponent *>(arg);
  if (eventBase != WIFI_EVENT) {
    return;
  }

  if (eventId == WIFI_EVENT_STA_START && self->m_stationConnectRequested) {
    const esp_err_t ret = esp_wifi_connect();
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to start station connection: %s",
               esp_err_to_name(ret));
    }
  } else if (eventId == WIFI_EVENT_STA_DISCONNECTED &&
             self->m_stationConnectRequested) {
    self->m_connected = false;
    const auto *event = static_cast<wifi_event_sta_disconnected_t *>(eventData);
    ESP_LOGW(TAG, "Station disconnected (reason=%d); retrying",
             event ? event->reason : -1);
    self->publishStatus("connecting");
    const esp_err_t ret = esp_wifi_connect();
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to retry station connection: %s",
               esp_err_to_name(ret));
    }
  }
}

void WifiComponent::ipEventHandler(void *arg, esp_event_base_t eventBase,
                                   int32_t eventId, void *) {
  auto *self = static_cast<WifiComponent *>(arg);
  if (eventBase != IP_EVENT || eventId != IP_EVENT_STA_GOT_IP) {
    return;
  }

  ESP_LOGI(TAG, "Connected to Wi-Fi");
  self->m_connected = true;
  self->publishStatus("connected");
  if (self->m_provisioning) {
    self->m_provisioning = false;
    self->stopPortalServer();
    const esp_err_t ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Connected but failed to close provisioning AP: %s",
               esp_err_to_name(ret));
    } else {
      ESP_LOGI(TAG, "Provisioning AP stopped");
    }
  }
}

esp_err_t WifiComponent::portalGetHandler(httpd_req_t *request) {
  static const char page[] =
      "<!doctype html><html><head><meta name=\"viewport\" "
      "content=\"width=device-width,initial-scale=1\">"
      "<title>Wi-Fi setup</title></head><body><h1>Wi-Fi setup</h1>"
      "<form method=\"post\" action=\"/connect\">"
      "<label>Network name (SSID)<br><input name=\"ssid\" maxlength=\"32\" "
      "required></label><br><br>"
      "<label>Password<br><input name=\"password\" type=\"password\" "
      "maxlength=\"63\"></label><br><br>"
      "<button type=\"submit\">Connect</button></form></body></html>";
  httpd_resp_set_type(request, "text/html");
  return httpd_resp_send(request, page, HTTPD_RESP_USE_STRLEN);
}

esp_err_t WifiComponent::portalPostHandler(httpd_req_t *request) {
  if (request->content_len <= 0 || request->content_len > 512) {
    httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Invalid form size");
    return ESP_OK;
  }

  char body[513];
  size_t received = 0;
  while (received < static_cast<size_t>(request->content_len)) {
    const int result = httpd_req_recv(request, body + received,
                                      request->content_len - received);
    if (result <= 0) {
      httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "Could not read form");
      return ESP_FAIL;
    }
    received += result;
  }
  body[received] = '\0';

  char ssid[33];
  char password[64];
  if (!getFormField(body, "ssid", ssid, sizeof(ssid)) ||
      !getFormField(body, "password", password, sizeof(password)) ||
      ssid[0] == '\0' || (password[0] != '\0' && std::strlen(password) < 8)) {
    httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST,
                        "SSID or password is invalid");
    return ESP_OK;
  }

  auto *self = static_cast<WifiComponent *>(request->user_ctx);
  const esp_err_t ret = self->connectToNetwork(ssid, password);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Could not apply station configuration: %s",
             esp_err_to_name(ret));
    httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "Could not start Wi-Fi connection");
    return ESP_OK;
  }

  static const char response[] =
      "<!doctype html><html><body><h1>Connecting</h1><p>Keep this page open "
      "while the device connects.</p></body></html>";
  httpd_resp_set_type(request, "text/html");
  return httpd_resp_send(request, response, HTTPD_RESP_USE_STRLEN);
}

} // namespace wifi
