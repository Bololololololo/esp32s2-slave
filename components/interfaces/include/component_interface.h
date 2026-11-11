#ifndef COMPONENT_INTERFACE_H
#define COMPONENT_INTERFACE_H

#include "componentIds.h"
#include "esp_err.h"
#include <cstring>
#include <string>

struct Message {
    ComponentId sender;
    ComponentId recipient;
    char payload[256];

    Message() : sender(COMPONENT_ID_ALL), recipient(COMPONENT_ID_ALL), payload{} {
    }

    // Two-argument messages retain sender-based broadcast semantics.
    Message(ComponentId sender, const std::string &str) : sender(sender), recipient(COMPONENT_ID_ALL) {
        std::strncpy(payload, str.c_str(), sizeof(payload) - 1);
        payload[sizeof(payload) - 1] = '\0';
    }

    Message(ComponentId sender, ComponentId recipient, const std::string &str) : sender(sender), recipient(recipient) {
        std::strncpy(payload, str.c_str(), sizeof(payload) - 1);
        payload[sizeof(payload) - 1] = '\0';
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