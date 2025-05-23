#include "channel/SharedTcpServerChannel.h"

#include "logging/LogMacros.h"
#include "outstation/SharedOutstation.h"

#include "opendnp3/ErrorCodes.h"

#include <vector>

namespace opendnp3
{
SharedTcpServerChannel::SharedTcpServerChannel(const Logger& logger,
                                               std::shared_ptr<exe4cpp::StrandExecutor> executor,
                                               std::shared_ptr<IResourceManager> manager,
                                               std::shared_ptr<SharedTcpServer> server)
    : logger(logger),
      executor(std::move(executor)),
      manager(std::move(manager)),
      resources(ResourceManager::Create()),
      server(std::move(server)),
      logLevels(levels::ALL)
{
}

SharedTcpServerChannel::~SharedTcpServerChannel()
{
    this->executor->block_until([this]() { this->ShutdownImpl(); });
}

void SharedTcpServerChannel::Shutdown()
{
    this->executor->block_until([self = shared_from_this()]() {
        self->ShutdownImpl();
        if (self->manager)
        {
            self->manager->Detach(self);
            self->manager.reset();
        }
    });
}

LinkStatistics SharedTcpServerChannel::GetStatistics()
{
    return this->executor->return_from<LinkStatistics>([self = shared_from_this()]() {
        LinkStatistics sum;
        self->DoEachIOHandler(
            [&sum](std::shared_ptr<IOHandler> iohanlder, const std::string&) { sum.Merge(iohanlder->Statistics()); });
        return sum;
    });
}

opendnp3::LogLevels SharedTcpServerChannel::GetLogFilters() const
{
    return this->executor->return_from<opendnp3::LogLevels>(
        [self = shared_from_this()]() -> opendnp3::LogLevels { return self->logLevels; });
}

void SharedTcpServerChannel::SetLogFilters(const opendnp3::LogLevels& filters)
{
    this->executor->post([self = shared_from_this(), filters]() {
        self->logLevels = filters;
        self->DoEachOutstation(
            [filters](std::shared_ptr<IOutstation> outstation) { outstation->SetLogFilters(filters); });
    });
}

std::shared_ptr<IMaster> SharedTcpServerChannel::AddMaster(const std::string& id,
                                                           std::shared_ptr<ISOEHandler> SOEHandler,
                                                           std::shared_ptr<IMasterApplication> application,
                                                           const MasterStackConfig& config)
{
    throw DNP3Error(Error::NO_MASTER_SUPPORT);
}

std::shared_ptr<IOutstation> SharedTcpServerChannel::AddOutstation(const std::string& id,
                                                                   std::shared_ptr<ICommandHandler> commandHandler,
                                                                   std::shared_ptr<IOutstationApplication> application,
                                                                   const OutstationStackConfig& config)
{
    if (hasShutdown)
    {
        return nullptr;
    }

    return this->executor->return_from<std::shared_ptr<IOutstation>>([&, this]() -> std::shared_ptr<IOutstation> {
        auto create = [&, this]() {
            return std::make_shared<SharedOutstation>(this->logger.detach(this->logger.get_id() + " " + id),
                                                      this->executor, commandHandler, application, manager, config);
        };
        auto outstation = this->resources->Bind<IOutstation>(create);
        if (outstation == nullptr)
        {
            return outstation;
        }

        outstation->SetLogFilters(this->logLevels);
        this->outstations.push_back(outstation);
        auto callback = std::dynamic_pointer_cast<ISharedTcpServerCallback>(outstation);
        if (callback == nullptr)
        {
            return nullptr;
        }

        this->DoEachIOHandler([callback](std::shared_ptr<IOHandler> iohanlder, const std::string& remoteAddr) {
            callback->OnConnect(iohanlder, remoteAddr);
        });
        return outstation;
    });
}

void SharedTcpServerChannel::OnConnect(std::shared_ptr<IOHandler> iohandler, const std::string& remoteAddr)
{
    this->DoEachOutstation([iohandler, &remoteAddr](std::shared_ptr<IOutstation> outstation) {
        auto callback = std::dynamic_pointer_cast<ISharedTcpServerCallback>(outstation);
        if (callback)
        {
            callback->OnConnect(iohandler, remoteAddr);
        }
    });

    this->iohandlers[iohandler] = remoteAddr;
}

void SharedTcpServerChannel::OnDisconnect(std::shared_ptr<IOHandler> iohandler, const std::string& remoteAddr)
{
    auto i = this->iohandlers.find(iohandler);
    if (i == this->iohandlers.end())
    {
        return;
    }

    this->iohandlers.erase(i);
    this->DoEachOutstation([iohandler, &remoteAddr](std::shared_ptr<IOutstation> outstation) {
        auto callback = std::dynamic_pointer_cast<ISharedTcpServerCallback>(outstation);
        if (callback)
        {
            callback->OnDisconnect(iohandler, remoteAddr);
        }
    });

    iohandler->Shutdown();
}

void SharedTcpServerChannel::ShutdownImpl()
{
    if (this->hasShutdown)
    {
        return;
    }
    else
    {
        this->hasShutdown = true;
    }

    this->server->Shutdown();
    this->server.reset();
    this->DoEachOutstation([](std::shared_ptr<IOutstation> outstation) { outstation->Shutdown(); });
    this->outstations.clear();
    this->DoEachIOHandler([](std::shared_ptr<IOHandler> iohandler, const std::string&) { iohandler->Shutdown(); });
    this->iohandlers.clear();
    this->resources->Shutdown();
    this->resources.reset();
}

void SharedTcpServerChannel::DoEachOutstation(std::function<void(std::shared_ptr<IOutstation>)> action)
{
    std::vector<std::shared_ptr<IOutstation>> outs;
    for (auto i = this->outstations.begin(); i != this->outstations.end();)
    {
        auto out = i->lock();
        if (out)
        {
            outs.push_back(out);
            ++i;
        }
        else
        {
            i = this->outstations.erase(i);
        }
    }

    for (auto i = outs.begin(); i != outs.end(); ++i)
    {
        action(*i);
    }
}

void SharedTcpServerChannel::DoEachIOHandler(std::function<void(std::shared_ptr<IOHandler>, const std::string&)> action)
{
    std::vector<std::pair<std::shared_ptr<IOHandler>, std::string>> iohandlers;
    for (auto i = this->iohandlers.begin(); i != this->iohandlers.end(); ++i)
    {
        iohandlers.emplace_back(i->first, i->second);
    }

    for (auto i = iohandlers.begin(); i != iohandlers.end(); ++i)
    {
        action(i->first, i->second);
    }
}

} // namespace opendnp3