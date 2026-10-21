#ifndef COMPONENT_INTERFACE_H
#define COMPONENT_INTERFACE_H

#include "componentIds.h"
#include "esp_err.h"
#include <cstring>

struct Message {
    ComponentId id;
    char payload[256];

    Message() = default;
    // Constructor that accepts a std::string variable
    Message(ComponentId id, const std::string& str) : id(id) {
        std::strncpy(payload, str.c_str(), sizeof(payload) - 1);
        payload[sizeof(payload) - 1] = '\0'; // Guarantee null-termination
    }
};

class ComponentInterface {
  private:
    ComponentId componentId;

  public:
    ComponentInterface(ComponentId id) : componentId(id) {
    }

    ComponentId getComponentId() const {
        return componentId;
    }

    virtual ~ComponentInterface() = default;
    virtual esp_err_t messageHandler(const Message &msg) = 0;
};

#endif // COMPONENT_INTERFACE_H