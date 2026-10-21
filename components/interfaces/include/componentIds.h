#ifndef COMPONENT_IDS_H
#define COMPONENT_IDS_H

#include <map>
#include <string>

#ifdef CONFIG_MSGR_TESTS
enum ComponentId {
    COMPONENT_ID_LVGL_DISPLAY = 0,
    COMPONENT_ID_LVGL_INPUT,
    COMPONENT_ID_WIFI,
    COMPONENT_ID_BLE,
    COMPONENT_ID_STORAGE,
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
                                                                 {COMPONENT_ID_STORAGE, "Storage"},
                                                                 {COMPONENT_ID_TEST_SUB, "Test Subscriber"},
                                                                 {COMPONENT_ID_TEST_SUB1, "Test Subscriber 1"},
                                                                 {COMPONENT_ID_TEST_SUB2, "Test Subscriber 2"},
                                                                 {COMPONENT_ID_TEST_PUB, "Test Publisher"},
                                                                 {COMPONENT_ID_ALL, "All Components (Wildcard)"}};
#else
enum ComponentId {
    COMPONENT_ID_LVGL_DISPLAY = 0,
    COMPONENT_ID_LVGL_INPUT,
    COMPONENT_ID_WIFI,
    COMPONENT_ID_BLE,
    COMPONENT_ID_STORAGE,
    COMPONENT_ID_ALL, // Wildcard: subscribe to messages from all components
    COMPONENT_ID_MAX
};

inline std::map<ComponentId, std::string> componentIdToString = {{COMPONENT_ID_LVGL_DISPLAY, "LVGL Display"},
                                                                 {COMPONENT_ID_LVGL_INPUT, "LVGL Input"},
                                                                 {COMPONENT_ID_WIFI, "WiFi"},
                                                                 {COMPONENT_ID_BLE, "BLE"},
                                                                 {COMPONENT_ID_STORAGE, "Storage"},
                                                                 {COMPONENT_ID_ALL, "All Components (Wildcard)"}};
#endif

#endif // COMPONENT_IDS_H