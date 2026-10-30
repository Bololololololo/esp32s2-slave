set(SOURCES)
if(CONFIG_ENABLE_SERVICE_SENSORS)
    file(GLOB SOURCES "*.cpp")
endif()

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "include"
                       REQUIRES interfaces message_router esp_timer esp_driver_gpio
                       ${extra_args})

if(SOURCES)
    target_compile_options(${COMPONENT_LIB} PUBLIC
                           "$<$<COMPILE_LANGUAGE:C>:-Wno-designated-init>"
                           "$<$<COMPILE_LANGUAGE:CXX>:-Wno-missing-field-initializers>")
endif()