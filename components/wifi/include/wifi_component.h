#ifndef WIFI_COMP_H
#define WIFI_COMP_H

#include "component_interface.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_netif.h"
#include <memory>

namespace wifi {

enum class WifiState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED
};

class WifiComponent : public ComponentInterface, public std::enable_shared_from_this<WifiComponent> {
  public:
    WifiComponent();
    esp_err_t init();
    void stop();

  private:
    esp_err_t messageHandler(const Message &msg) override;
    esp_err_t applyMode(const char *mode);
    esp_err_t startProvisioningPortal();
    esp_err_t connectToNetwork(const char *ssid, const char *password);
    esp_err_t startPortalServer();
    void stopPortalServer();
    void publishStatus(const char *status);

    static void wifiEventHandler(void *arg, esp_event_base_t eventBase, int32_t eventId, void *eventData);
    static void ipEventHandler(void *arg, esp_event_base_t eventBase, int32_t eventId, void *eventData);
    static esp_err_t portalGetHandler(httpd_req_t *request);
    static esp_err_t portalPostHandler(httpd_req_t *request);

    esp_netif_t *m_stationNetif{nullptr};
    esp_netif_t *m_accessPointNetif{nullptr};
    httpd_handle_t m_httpServer{nullptr};
    esp_event_handler_instance_t m_wifiEventInstance{nullptr};
    esp_event_handler_instance_t m_ipEventInstance{nullptr};
    bool m_wifiStarted{false};
    bool m_stationConnectRequested{false};
    bool m_provisioning{false};
    bool m_connected{false};
    bool m_initialized{false};
    bool m_subscribed{false};
    char m_provisioningSsid[33]{};
    char m_provisioningPassword[17]{};
};

} // namespace wifi

#endif // WIFI_COMP_H