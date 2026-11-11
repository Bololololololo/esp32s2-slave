set(OTA_SRCS)
if(CONFIG_ENABLE_SERVICE_OTA)
    list(APPEND OTA_SRCS "ota_component.cpp" "ota_service.cpp")
endif()

idf_component_register(SRCS ${OTA_SRCS}
                    INCLUDE_DIRS "include"
                    REQUIRES  message_router esp_https_ota bootloader_support nvs_flash app_update esp_wifi)

if(OTA_SRCS)
    target_compile_options(${COMPONENT_LIB} PUBLIC
                           "$<$<COMPILE_LANGUAGE:C>:-Wno-designated-init>"
                           "$<$<COMPILE_LANGUAGE:CXX>:-Wno-missing-field-initializers>")
endif()