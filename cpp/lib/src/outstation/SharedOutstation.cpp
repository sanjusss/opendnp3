#include "outstation/SharedOutstation.h"

#include "logging/LogMacros.h"
#include "outstation/OutstationStack.h"

#include <vector>

namespace opendnp3
{
SharedOutstation::SharedOutstation(const Logger& logger,
                                   const std::shared_ptr<exe4cpp::StrandExecutor>& executor,
                                   const std::shared_ptr<ICommandHandler>& commandHandler,
                                   const std::shared_ptr<IOutstationApplication>& application,
                                   const std::shared_ptr<IResourceManager>& manager,
                                   const OutstationStackConfig& config)
    : logger(logger),
      executor(executor),
      commandHandler(commandHandler),
      application(application),
      manager(manager),
      resources(ResourceManager::Create()),
      config(config),
      database(config.database)
{
    this->config.database = DatabaseConfig{};
}

SharedOutstation::~SharedOutstation()
{
    this->executor->block_until([this]() { this->ShutdownImpl(); });
}

bool SharedOutstation::Enable()
{
    if (hasShutdown)
    {
        return false;
    }

    return this->executor->return_from<bool>([self = shared_from_this()]() {
        if (self->enabled)
        {
            return true;
        }

        self->enabled = true;
        self->DoEach([](std::shared_ptr<IOutstation> outstation) { outstation->Enable(); });
        return true;
    });
}

bool SharedOutstation::Disable()
{
    if (hasShutdown)
    {
        return false;
    }
    return this->executor->return_from<bool>([self = shared_from_this()]() {
        if (self->enabled == false)
        {
            return true;
        }

        self->enabled = false;
        self->DoEach([](std::shared_ptr<IOutstation> outstation) { outstation->Disable(); });
        return true;
    });
}

void SharedOutstation::Shutdown()
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

StackStatistics SharedOutstation::GetStackStatistics()
{
    if (hasShutdown)
    {
        return {};
    }
    return this->executor->return_from<StackStatistics>([self = shared_from_this()]() {
        StackStatistics sum;
        self->DoEach(
            [&sum](std::shared_ptr<IOutstation> outstation) { sum.Merge(outstation->GetStackStatistics()); });
        return sum;
    });
}

void SharedOutstation::SetLogFilters(const LogLevels& filters)
{
    if (hasShutdown)
    {
        return;
    }

    this->executor->post([self = shared_from_this(), filters]() {
        self->logLevels = filters;
        self->DoEach([filters](std::shared_ptr<IOutstation> outstation) { outstation->SetLogFilters(filters); });
    });
}

void SharedOutstation::SetRestartIIN()
{
    if (hasShutdown)
    {
        return;
    }

    this->executor->post([self = shared_from_this()]() {
        self->DoEach([](std::shared_ptr<IOutstation> outstation) { outstation->SetRestartIIN(); });
    });
}

void SharedOutstation::Apply(const Updates& updates)
{
    if (hasShutdown || updates.IsEmpty())
    {
        return;
    }

    this->executor->post([updates, self = shared_from_this()]() {
        updates.Apply(self->database);
        self->DoEach([&updates](std::shared_ptr<IOutstation> outstation) { outstation->Apply(updates); });
    });
}

void SharedOutstation::OnConnect(std::shared_ptr<IOHandler> iohandler, const std::string& remoteAddr)
{
    if (hasShutdown)
    {
        return;
    }

    auto config = this->config;
    config.database = this->database;
    auto stack = OutstationStack::Create(this->logger.detach(this->logger.get_id() + "-" + remoteAddr), this->executor,
                                         commandHandler, application, iohandler, this->resources, config);
    auto create = [iohandler, stack, route = Addresses(config.link.RemoteAddr, config.link.LocalAddr),
                   self = this->shared_from_this()]() {
        auto add = [iohandler, stack, route, self]() -> bool { return iohandler->AddContext(stack, route); };

        return self->executor->return_from<bool>(add) ? stack : nullptr;
    };

    auto out = this->resources->Bind<OutstationStack>(create);
    if (out)
    {
        FORMAT_LOG_BLOCK(this->logger, flags::WARN, "Create outstation for %s successfully.", remoteAddr.c_str());
        this->outstations[iohandler] = out;
        out->SetLogFilters(this->logLevels);
        if (this->enabled)
        {
            out->Enable();
        }
    }
    else
    {
        FORMAT_LOG_BLOCK(this->logger, flags::ERR, "Create outstation for %s failed.", remoteAddr.c_str());
        iohandler->Shutdown();
    }
}

void SharedOutstation::OnDisconnect(std::shared_ptr<IOHandler> iohandler, const std::string& remoteAddr)
{
    auto i = this->outstations.find(iohandler);
    if (i == this->outstations.end())
    {
        return;
    }

    auto out = i->second;
    this->outstations.erase(i);
    out->Shutdown();
    FORMAT_LOG_BLOCK(this->logger, flags::WARN, "Release outstation for %s.", remoteAddr.c_str());
}

void SharedOutstation::ShutdownImpl()
{
    if (hasShutdown)
    {
        return;
    }

    hasShutdown = true;
    DoEach([](std::shared_ptr<IOutstation> outstation) { outstation->Shutdown(); });
    this->outstations.clear();
    this->resources->Shutdown();
    this->resources.reset();
}

void SharedOutstation::DoEach(std::function<void(std::shared_ptr<IOutstation>)> action)
{
    // Prevents instances from destructuring during traversal.
    std::vector<std::shared_ptr<IOutstation>> outs;
    for (auto i = this->outstations.begin(); i != this->outstations.end(); i++)
    {
        outs.push_back(i->second);
    }

    for (auto i = outs.begin(); i != outs.end(); i++)
    {
        action(*i);
    }
}
} // namespace opendnp3