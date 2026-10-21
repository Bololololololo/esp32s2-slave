#ifndef _SERVICE_INTERFACE_H_
#define _SERVICE_INTERFACE_H_

namespace services {
enum ServiceType {
    SERVICE_WIFI,
    SERVICE_BLUETOOTH,
    SERVICE_GPS,
    SERVICE_SENSOR,
    SERVICE_CAMERA,
    SERVICE_GUI,
    SERVICE_MSGR,
    SERVICE_MAX
};

enum ServiceState {
    SERVICE_STOPPED,
    SERVICE_RUNNING,
    SERVICE_PAUSED,
    SERVICE_ERROR
};

class ServiceInterface {
  public:
    ServiceInterface(ServiceType type) : serviceType(type) {
    }
    virtual ~ServiceInterface() = default;
    virtual void start() = 0;
    virtual void stop() = 0;

    ServiceType getServiceType() const {
        return serviceType;
    }
    ServiceState getServiceState() const {
        return state;
    }
    void setServiceState(ServiceState newState) {
        state = newState;
    }

  private:
    ServiceType serviceType;
    ServiceState state = SERVICE_STOPPED;
};
} // namespace services

#endif // _SERVICE_INTERFACE_H_