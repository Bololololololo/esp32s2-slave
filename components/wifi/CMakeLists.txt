set(WIFI_SRCS)
if(CONFIG_ENABLE_SERVICE_WIFI)
    list(APPEND WIFI_SRCS "wifi_component.cpp" "wifi_service.cpp")
endif()

idf_component_register(SRCS ${WIFI_SRCS}
                    INCLUDE_DIRS "include"
                    REQUIRES interfaces message_router esp_event esp_netif esp_wifi
                             nvs_flash esp_http_server)
