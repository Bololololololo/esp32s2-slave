file(GLOB SOURCES "*.cpp")

idf_component_register(SRCS "system_info.cpp" "status_led.cpp" "${SOURCES}"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES esp_psram driver led_strip esp_hw_support esp_system esp_driver_gpio
                       ${extra_args})

target_compile_options(${COMPONENT_LIB} PUBLIC
                       "$<$<COMPILE_LANGUAGE:C>:-Wno-designated-init>"
                       "$<$<COMPILE_LANGUAGE:CXX>:-Wno-missing-field-initializers>")