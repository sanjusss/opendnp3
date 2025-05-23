#ifndef OPENDNP3_SHAREDTCPSERVER_H
#define OPENDNP3_SHAREDTCPSERVER_H

#include "IResourceManager.h"
#include "channel/ISharedTcpServerCallback.h"

#include "opendnp3/channel/IListener.h"
#include "opendnp3/channel/IPEndpoint.h"
#ifdef OPENDNP3_USE_TLS
#include "opendnp3/channel/TLSConfig.h"
#endif

#include <memory>

namespace opendnp3
{
class SharedTcpServer final : public std::enable_shared_from_this<SharedTcpServer>,
                              public IListener,
                              public ISharedTcpServerCallback,
                              private Uncopyable
{
private:
    SharedTcpServer(const Logger& logger,
                    const std::shared_ptr<exe4cpp::StrandExecutor>& executor,
                    const IPEndpoint& endpoint,
                    std::shared_ptr<IResourceManager> manager);

public:
    SharedTcpServer(const Logger& logger,
                    const std::shared_ptr<exe4cpp::StrandExecutor>& executor,
                    const IPEndpoint& endpoint,
                    std::shared_ptr<IResourceManager> manager,
                    ASIO_ERROR& ec);
#ifdef OPENDNP3_USE_TLS
    SharedTcpServer(const Logger& logger,
                    const std::shared_ptr<exe4cpp::StrandExecutor>& executor,
                    const IPEndpoint& endpoint,
                    const TLSConfig& tlsConfig,
                    std::shared_ptr<IResourceManager> manager,
                    ASIO_ERROR& ec);
#endif
    ~SharedTcpServer() override;

    void Shutdown() override;
    void SetCallback(std::weak_ptr<ISharedTcpServerCallback> callback);

private:
    void OnConnect(std::shared_ptr<IOHandler> iohandler, const std::string& remoteAddr) override;
    void OnDisconnect(std::shared_ptr<IOHandler> iohandler, const std::string& remoteAddr) override;

    void ShutdownImpl();

private:
    Logger logger;
    std::shared_ptr<exe4cpp::StrandExecutor> executor;
    std::shared_ptr<IResourceManager> manager;

    std::shared_ptr<IListener> listener;
    std::weak_ptr<ISharedTcpServerCallback> callback;
};
} // namespace opendnp3

#endif // OPENDNP3_SHAREDTCPSERVER_H