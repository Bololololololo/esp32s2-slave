idf_component_register(SRCS "wifi_component.cpp" "wifi_service.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES interfaces message-router esp_event esp_netif esp_wifi
                             nvs_flash esp_http_server)
