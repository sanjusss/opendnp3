#ifndef OPENDNP3_TCPSOCKETIOHANDLER_H
#define OPENDNP3_TCPSOCKETIOHANDLER_H

#include "channel/IOHandler.h"

#include <memory>

namespace opendnp3
{
class TCPSocketIOHandler final : public IOHandler
{
public:
    TCPSocketIOHandler(const Logger& logger,
                       std::shared_ptr<IChannelListener> listener,
                       std::shared_ptr<exe4cpp::StrandExecutor> executor,
                       ASIO::ip::tcp::socket socket);
#ifdef OPENDNP3_USE_TLS
    TCPSocketIOHandler(const Logger& logger,
                       std::shared_ptr<IChannelListener> listener,
                       std::shared_ptr<exe4cpp::StrandExecutor> executor,
                       std::shared_ptr<ASIO::ssl::stream<ASIO::ip::tcp::socket>> sslStream);
#endif

private:
    void ShutdownImpl() final;
    void BeginChannelAccept() final;
    void SuspendChannelAccept() final;
    void OnChannelShutdown() final;

private:
    std::shared_ptr<exe4cpp::StrandExecutor> executor;
    std::unique_ptr<ASIO::ip::tcp::socket> socket;
#ifdef OPENDNP3_USE_TLS
    std::shared_ptr<ASIO::ssl::stream<ASIO::ip::tcp::socket>> sslStream;
#endif
};
} // namespace opendnp3

#endif // OPENDNP3_TCPSOCKETIOHANDLER_H