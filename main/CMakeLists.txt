idf_component_register(SRCS "main.cpp"
                       INCLUDE_DIRS "."
                       EMBED_TXTFILES "certs/cert.pem"
                       PRIV_REQUIRES utils service_manager message_router mqtt_adapter mqtt gui ota)
