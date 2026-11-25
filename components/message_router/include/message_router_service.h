#pragma once

#include "message_router.h"
#include "service_interface.h"
#include <memory>

using namespace messageRouter;

namespace services {
class MessageRouterService : public ServiceInterface {
  public:
    MessageRouterService() : ServiceInterface(ServiceType::SERVICE_MSGR, "MessageRouterSvc", 3072) {
    }
    ~MessageRouterService() override {
        stop();
    }

  private:
    esp_err_t onStart() override;
    void onStop() override;
};

} // namespace services
