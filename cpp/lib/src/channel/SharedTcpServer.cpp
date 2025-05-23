#include "channel/SharedTcpServer.h"

#include "channel/TCPServer.h"
#include "channel/TCPSocketIOHandler.h"
#include "logging/LogMacros.h"
#ifdef OPENDNP3_USE_TLS
#include "channel/tls/TLSServer.h"
#endif

#include "opendnp3/channel/IChannelListener.h"

namespace opendnp3
{
class ChannelWather final : public std::enable_shared_from_this<ChannelWather>, public IChannelListener
{
public:
    void OnStateChange(ChannelState state) override final
    {
        if (state != ChannelState::CLOSED)
        {
            return;
        }

        auto iohandler = this->iohandler.lock();
        if (iohandler)
        {
            callback->OnDisconnect(iohandler, remoteAddr);
        }
    }

    ISharedTcpServerCallback* callback;
    std::weak_ptr<IOHandler> iohandler;
    std::string remoteAddr;
};

class IServer
{
public:
    virtual ~IServer() = default;
    virtual void Start() = 0;
};

class SharedTcpLinstener final : public TCPServer, public IServer
{
public:
    SharedTcpLinstener(const Logger& logger,
                       const std::shared_ptr<exe4cpp::StrandExecutor>& executor,
                       const IPEndpoint& endpoint,
                       ISharedTcpServerCallback* callback,
                       ASIO_ERROR& ec)
        : TCPServer(logger, executor, endpoint, ec), callback(callback)
    {
    }

    void OnShutdown() override {}

    void AcceptConnection(uint64_t sessionid,
                          const std::shared_ptr<exe4cpp::StrandExecutor>& executor,
                          ASIO::ip::tcp::socket socket) override
    {
        auto watcher = std::make_shared<ChannelWather>();
        try
        {
            watcher->remoteAddr = socket.remote_endpoint().address().to_string() + ":"
                + std::to_string(socket.remote_endpoint().port());
        }
        catch (...)
        {
            watcher->remoteAddr = "unknown";
        }

        auto iohandler = std::make_shared<TCPSocketIOHandler>(logger, watcher, executor, std::move(socket));
        watcher->callback = this->callback;
        watcher->iohandler = iohandler;

        FORMAT_LOG_BLOCK(this->logger, flags::INFO, "Receive link from %s.", watcher->remoteAddr.c_str());
        this->callback->OnConnect(iohandler, watcher->remoteAddr);
    }

    void Start() override
    {
        this->StartAccept();
    }

private:
    ISharedTcpServerCallback* callback;
};

#ifdef OPENDNP3_USE_TLS
class SharedTlsLinstener final : public TLSServer, public IServer
{
public:
    SharedTlsLinstener(const Logger& logger,
                       const std::shared_ptr<exe4cpp::StrandExecutor>& executor,
                       const IPEndpoint& endpoint,
                       const TLSConfig& config,
                       ISharedTcpServerCallback* callback,
                       ASIO_ERROR& ec)
        : TLSServer(logger, executor, endpoint, config, ec), callback(callback)
    {
    }

private:
    bool AcceptConnection(uint64_t sessionid, const ASIO::ip::tcp::endpoint& remote) override
    {
        return true;
    }
    bool VerifyCallback(uint64_t sessionid, bool preverified, ASIO::ssl::verify_context& ctx) override
    {
        return preverified;
    }
    void AcceptStream(uint64_t sessionid,
                      const std::shared_ptr<exe4cpp::StrandExecutor>& executor,
                      std::shared_ptr<ASIO::ssl::stream<ASIO::ip::tcp::socket>> stream) override
    {
        auto watcher = std::make_shared<ChannelWather>();
        try
        {
            auto&& socket = stream->lowest_layer();
            watcher->remoteAddr = socket.remote_endpoint().address().to_string() + ":"
                + std::to_string(socket.remote_endpoint().port());
        }
        catch (...)
        {
            watcher->remoteAddr = "unknown";
        }

        auto iohandler = std::make_shared<TCPSocketIOHandler>(logger, watcher, executor, std::move(stream));
        watcher->callback = this->callback;
        watcher->iohandler = iohandler;

        FORMAT_LOG_BLOCK(this->logger, flags::INFO, "Receive link from %s.", watcher->remoteAddr.c_str());
    }
    void OnShutdown() override {}

    void Start() override
    {
        this->StartAccept();
    }

private:
    ISharedTcpServerCallback* callback;
};
#endif

SharedTcpServer::SharedTcpServer(const Logger& logger,
                                 const std::shared_ptr<exe4cpp::StrandExecutor>& executor,
                                 const IPEndpoint& endpoint,
                                 std::shared_ptr<IResourceManager> manager)
    : logger(logger.detach(logger.get_id() + " local " + endpoint.address + ":" + std::to_string(endpoint.port))),
      executor(executor),
      manager(manager)
{
}

SharedTcpServer::SharedTcpServer(const Logger& logger,
                                 const std::shared_ptr<exe4cpp::StrandExecutor>& executor,
                                 const IPEndpoint& endpoint,
                                 std::shared_ptr<IResourceManager> manager,
                                 ASIO_ERROR& ec)
    : SharedTcpServer(logger, executor, endpoint, manager)
{
    this->listener = std::make_shared<SharedTcpLinstener>(this->logger, executor, endpoint, this, ec);
}

#ifdef OPENDNP3_USE_TLS
SharedTcpServer::SharedTcpServer(const Logger& logger,
                                 const std::shared_ptr<exe4cpp::StrandExecutor>& executor,
                                 const IPEndpoint& endpoint,
                                 const TLSConfig& tlsConfig,
                                 std::shared_ptr<IResourceManager> manager,
                                 ASIO_ERROR& ec)
    : SharedTcpServer(logger, executor, endpoint, manager)
{
    this->listener = std::make_shared<SharedTlsLinstener>(this->logger, executor, endpoint, tlsConfig, this, ec);
}

#endif
SharedTcpServer::~SharedTcpServer()
{
    this->ShutdownImpl();
}

void SharedTcpServer::Shutdown()
{
    this->executor->post([self = shared_from_this()]() {
        self->ShutdownImpl();
        if (self->manager)
        {
            self->manager->Detach(self);
            self->manager.reset();
        }
    });
}

void SharedTcpServer::SetCallback(std::weak_ptr<ISharedTcpServerCallback> callback)
{
    this->executor->post([callback, self = shared_from_this()]() {
        self->callback = callback;
        auto server = std::dynamic_pointer_cast<IServer>(self->listener);
        if (server)
        {
            server->Start();
        }
    });
}

void SharedTcpServer::OnConnect(std::shared_ptr<IOHandler> iohandler, const std::string& remoteAddr)
{
    FORMAT_LOG_BLOCK(this->logger, flags::WARN, "Get connection from %s.", remoteAddr.c_str());
    auto callback = this->callback.lock();
    if (!callback)
    {
        return;
    }

    callback->OnConnect(iohandler, remoteAddr);
}

void SharedTcpServer::OnDisconnect(std::shared_ptr<IOHandler> iohandler, const std::string& remoteAddr)
{
    FORMAT_LOG_BLOCK(this->logger, flags::WARN, "Disconnect to %s.", remoteAddr.c_str());
    auto callback = this->callback.lock();
    if (!callback)
    {
        return;
    }

    callback->OnDisconnect(iohandler, remoteAddr);
}

void SharedTcpServer::ShutdownImpl()
{
    if (this->listener)
    {
        this->listener->Shutdown();
        this->listener.reset();
    }
}
} // namespace opendnp3