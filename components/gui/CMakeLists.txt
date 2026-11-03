set(SOURCES)
if(CONFIG_ENABLE_SERVICE_GUI_DIGITAL_DISPLAY)
    list(APPEND SOURCES "digital_display/hw069.c")
endif()

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "digital_display/include"
                       REQUIRES esp_timer esp_driver_gpio
                       ${extra_args})
