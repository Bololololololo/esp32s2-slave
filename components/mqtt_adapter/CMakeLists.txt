idf_component_register(SRCS "mqtt.cpp" "mqtt_pub.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES interfaces message_router
                    PRIV_REQUIRES mqtt)
