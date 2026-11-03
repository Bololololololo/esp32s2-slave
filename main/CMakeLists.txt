idf_component_register(SRCS "main.cpp"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES utils service_manager message_router mqtt_adapter mqtt gui)
