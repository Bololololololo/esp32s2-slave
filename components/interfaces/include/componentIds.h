#pragma once

#include <map>
#include <string>

#ifdef CONFIG_MSGR_TESTS
enum ComponentId {
    COMPONENT_ID_LVGL_DISPLAY = 0,
    COMPONENT_ID_LVGL_INPUT,       // 1
    COMPONENT_ID_WIFI,             // 2
    COMPONENT_ID_BLE,              // 3
    COMPONENT_ID_OTA,              // 4
    COMPONENT_ID_STORAGE,          // 5
    COMPONENT_ID_STATUS_LED,       // 6
    COMPONENT_ID_MQTT,             // 7
    COMPONENT_ID_PROXIMITY_SENSOR, // 8
    COMPONENT_ID_TEST_SUB,
    COMPONENT_ID_TEST_SUB1,
    COMPONENT_ID_TEST_SUB2,
    COMPONENT_ID_TEST_PUB,
    COMPONENT_ID_ALL, // Wildcard: subscribe to messages from all components
    COMPONENT_ID_MAX
};

inline std::map<ComponentId, std::string> componentIdToString = {{COMPONENT_ID_LVGL_DISPLAY, "LVGL Display"},
                                                                 {COMPONENT_ID_LVGL_INPUT, "LVGL Input"},
                                                                 {COMPONENT_ID_WIFI, "WiFi"},
                                                                 {COMPONENT_ID_BLE, "BLE"},
                                                                 {COMPONENT_ID_OTA, "OTA"},
                                                                 {COMPONENT_ID_STORAGE, "Storage"},
                                                                 {COMPONENT_ID_STATUS_LED, "Status LED"},
                                                                 {COMPONENT_ID_MQTT, "MQTT"},
                                                                 {COMPONENT_ID_PROXIMITY_SENSOR, "Proximity Sensor"},
                                                                 {COMPONENT_ID_TEST_SUB, "Test Subscriber"},
                                                                 {COMPONENT_ID_TEST_SUB1, "Test Subscriber 1"},
                                                                 {COMPONENT_ID_TEST_SUB2, "Test Subscriber 2"},
                                                                 {COMPONENT_ID_TEST_PUB, "Test Publisher"},
                                                                 {COMPONENT_ID_ALL, "All Components (Wildcard)"}};
#else
enum ComponentId {
    COMPONENT_ID_LVGL_DISPLAY = 0,
    COMPONENT_ID_LVGL_INPUT,       // 1
    COMPONENT_ID_WIFI,             // 2
    COMPONENT_ID_BLE,              // 3
    COMPONENT_ID_OTA,              // 4
    COMPONENT_ID_STORAGE,          // 5
    COMPONENT_ID_STATUS_LED,       // 6
    COMPONENT_ID_MQTT,             // 7
    COMPONENT_ID_PROXIMITY_SENSOR, // 8
    COMPONENT_ID_ALL,              // Wildcard: subscribe to messages from all components
    COMPONENT_ID_MAX
};

inline std::map<ComponentId, std::string> componentIdToString = {{COMPONENT_ID_LVGL_DISPLAY, "LVGL Display"},
                                                                 {COMPONENT_ID_LVGL_INPUT, "LVGL Input"},
                                                                 {COMPONENT_ID_WIFI, "WiFi"},
                                                                 {COMPONENT_ID_BLE, "BLE"},
                                                                 {COMPONENT_ID_OTA, "OTA"},
                                                                 {COMPONENT_ID_STORAGE, "Storage"},
                                                                 {COMPONENT_ID_STATUS_LED, "Status LED"},
                                                                 {COMPONENT_ID_MQTT, "MQTT"},
                                                                 {COMPONENT_ID_PROXIMITY_SENSOR, "Proximity Sensor"},
                                                                 {COMPONENT_ID_ALL, "All Components (Wildcard)"}};
#endif
