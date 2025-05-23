#ifndef OPENDNP3_SHAREDOUTSTATION_H
#define OPENDNP3_SHAREDOUTSTATION_H

#include "IResourceManager.h"
#include "ResourceManager.h"
#include "channel/ISharedTcpServerCallback.h"
#include "outstation/SharedDatabaseConfig.h"

#include "opendnp3/outstation/ICommandHandler.h"
#include "opendnp3/outstation/IOutstation.h"
#include "opendnp3/outstation/IOutstationApplication.h"
#include "opendnp3/outstation/OutstationStackConfig.h"

#include <map>
#include <memory>
#include <mutex>

namespace opendnp3
{
class SharedOutstation final : public IOutstation,
                               public ISharedTcpServerCallback,
                               public std::enable_shared_from_this<SharedOutstation>
{
public:
    explicit SharedOutstation(const Logger& logger,
                              const std::shared_ptr<exe4cpp::StrandExecutor>& executor,
                              const std::shared_ptr<ICommandHandler>& commandHandler,
                              const std::shared_ptr<IOutstationApplication>& application,
                              const std::shared_ptr<IResourceManager>& manager,
                              const OutstationStackConfig& config);
    ~SharedOutstation() override;

    // Implement IOutstation interface
    bool Enable() override;
    bool Disable() override;
    void Shutdown() override;
    StackStatistics GetStackStatistics() override;
    void SetLogFilters(const LogLevels& filters) override;
    void SetRestartIIN() override;
    void Apply(const Updates& updates) override;

    // Implement ISharedTcpServerCallback interface
    void OnConnect(std::shared_ptr<IOHandler> iohandler, const std::string& remoteAddr) override;
    void OnDisconnect(std::shared_ptr<IOHandler> iohandler, const std::string& remoteAddr) override;

private:
    void ShutdownImpl();
    void DoEach(std::function<void(std::shared_ptr<IOutstation>)> action);

private:
    Logger logger;
    std::shared_ptr<exe4cpp::StrandExecutor> executor;
    std::shared_ptr<ICommandHandler> commandHandler;
    std::shared_ptr<IOutstationApplication> application;
    std::shared_ptr<IResourceManager> manager;
    std::shared_ptr<ResourceManager> resources;
    OutstationStackConfig config;
    SharedDatabaseConfig database;

    bool hasShutdown = false;
    std::map<std::shared_ptr<IOHandler>, std::shared_ptr<IOutstation>> outstations;

    bool enabled = false;
    LogLevels logLevels = levels::ALL;
};

} // namespace opendnp3

#endif // OPENDNP3_SHAREDOUTSTATION_H