idf_component_register(SRCS "mqtt_comp.cpp" "mqtt_pub.cpp" "mqtt_service.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES interfaces message_router
                    PRIV_REQUIRES mqtt)
