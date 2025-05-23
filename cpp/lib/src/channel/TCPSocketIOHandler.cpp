#include "channel/TCPSocketIOHandler.h"

#include "channel/TCPSocketChannel.h"
#ifdef OPENDNP3_USE_TLS
#include "channel/tls/TLSStreamChannel.h"
#endif

namespace opendnp3
{
TCPSocketIOHandler::TCPSocketIOHandler(const Logger& logger,
                                       std::shared_ptr<IChannelListener> listener,
                                       std::shared_ptr<exe4cpp::StrandExecutor> executor,
                                       ASIO::ip::tcp::socket socket)
    : IOHandler(logger, false, listener),
      executor(std::move(executor)),
      socket(std::make_unique<ASIO::ip::tcp::socket>(std::move(socket)))
{
}
#ifdef OPENDNP3_USE_TLS
TCPSocketIOHandler::TCPSocketIOHandler(const Logger& logger,
                                       std::shared_ptr<IChannelListener> listener,
                                       std::shared_ptr<exe4cpp::StrandExecutor> executor,
                                       std::shared_ptr<ASIO::ssl::stream<ASIO::ip::tcp::socket>> sslStream)
    : IOHandler(logger, false, listener), executor(std::move(executor)), sslStream(std::move(sslStream))
{
}
#endif
void TCPSocketIOHandler::ShutdownImpl() {}

void TCPSocketIOHandler::BeginChannelAccept()
{
    if (socket)
    {
        auto channel = TCPSocketChannel::Create(executor, std::move(*socket));
        socket.reset();
        this->OnNewChannel(channel);
    }

#ifdef OPENDNP3_USE_TLS
    if (sslStream)
    {
        auto channel = TLSStreamChannel::Create(executor, sslStream);
        sslStream.reset();
        this->OnNewChannel(channel);
    }
#endif
}

void TCPSocketIOHandler::SuspendChannelAccept() {}

void TCPSocketIOHandler::OnChannelShutdown() {}
} // namespace opendnp3