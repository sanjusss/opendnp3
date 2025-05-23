#ifndef OPENDNP3_SHAREDTCPSERVERCHANNEL_H
#define OPENDNP3_SHAREDTCPSERVERCHANNEL_H

#include "channel/DNP3Channel.h"
#include "channel/ISharedTcpServerCallback.h"
#include "channel/SharedTcpServer.h"

#include <map>
#include <memory>
#include <list>

namespace opendnp3
{
class SharedTcpServerChannel final : public IChannel,
                                     public ISharedTcpServerCallback,
                                     public std::enable_shared_from_this<SharedTcpServerChannel>
{
public:
    explicit SharedTcpServerChannel(const Logger& logger,
                                    std::shared_ptr<exe4cpp::StrandExecutor> executor,
                                    std::shared_ptr<IResourceManager> manager,
                                    std::shared_ptr<SharedTcpServer> server);
    ~SharedTcpServerChannel() override;

    void Shutdown() override;

    LinkStatistics GetStatistics() override;

    opendnp3::LogLevels GetLogFilters() const override;

    void SetLogFilters(const opendnp3::LogLevels& filters) override;

    std::shared_ptr<IMaster> AddMaster(const std::string& id,
                                       std::shared_ptr<ISOEHandler> SOEHandler,
                                       std::shared_ptr<IMasterApplication> application,
                                       const MasterStackConfig& config) override;

    std::shared_ptr<IOutstation> AddOutstation(const std::string& id,
                                               std::shared_ptr<ICommandHandler> commandHandler,
                                               std::shared_ptr<IOutstationApplication> application,
                                               const OutstationStackConfig& config) override;

    void OnConnect(std::shared_ptr<IOHandler> iohandler, const std::string& remoteAddr) override;
    void OnDisconnect(std::shared_ptr<IOHandler> iohandler, const std::string& remoteAddr) override;

private:
    void ShutdownImpl();
    void DoEachOutstation(std::function<void(std::shared_ptr<IOutstation>)> action);
    void DoEachIOHandler(std::function<void(std::shared_ptr<IOHandler>, const std::string &)> action);

private:
    Logger logger;
    const std::shared_ptr<exe4cpp::StrandExecutor> executor;
    std::shared_ptr<IResourceManager> manager;
    std::shared_ptr<ResourceManager> resources;
    std::shared_ptr<SharedTcpServer> server;
    opendnp3::LogLevels logLevels;

    bool hasShutdown = false;
    std::map<std::shared_ptr<IOHandler>, std::string> iohandlers;
    std::list<std::weak_ptr<IOutstation>> outstations;
};
} // namespace opendnp3

#endif // OPENDNP3_SHAREDTCPSERVERCHANNEL_H