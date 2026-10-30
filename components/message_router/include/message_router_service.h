#ifndef _MESSAGE_ROUTER_SERVICE_H_
#define _MESSAGE_ROUTER_SERVICE_H_

#include "message_router.h"
#include "service_interface.h"
#include <memory>

using namespace messageRouter;

namespace services {
class MessageRouterService : public ServiceInterface {
  public:
    MessageRouterService() : ServiceInterface(ServiceType::SERVICE_MSGR) {
    }
    ~MessageRouterService() override = default;

    void start() override;
    void stop() override;

  private:
    std::unique_ptr<MessageRouter> messageRouter;
};

} // namespace services

#endif // _MESSAGE_ROUTER_SERVICE_H_
