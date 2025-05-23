#ifndef OPENDNP3_ISHAREDTCPSERVERCALLBACK_H
#define OPENDNP3_ISHAREDTCPSERVERCALLBACK_H

#include "channel/IOHandler.h"

namespace opendnp3
{
class ISharedTcpServerCallback
{
public:
    virtual ~ISharedTcpServerCallback() = default;

    virtual void OnConnect(std::shared_ptr<IOHandler> iohandler, const std::string& remoteAddr) = 0;
    
    virtual void OnDisconnect(std::shared_ptr<IOHandler> iohandler, const std::string& remoteAddr) = 0;
};
} // namespace opendnp3

#endif // OPENDNP3_ISHAREDTCPSERVERCALLBACK_H