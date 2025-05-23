#include "SharedDatabaseConfig.h"

namespace opendnp3
{
SharedDatabaseConfig::SharedDatabaseConfig(const DatabaseConfig& database) : DatabaseConfig(database) {}

template<class TConfig, class TMeas>
bool SharedDatabaseConfig::UpdateMeas(std::map<uint16_t, TConfig>& configMap, uint16_t index, const TMeas& meas)
{
    auto it = configMap.find(index);
    if (it == configMap.end())
    {
        return false;
    }

    it->second.defaultValue = meas;
    return true;
}

template<class TConfig>
bool SharedDatabaseConfig::ModifyFlags(std::map<uint16_t, TConfig>& configMap,
                                       uint16_t start,
                                       uint16_t stop,
                                       uint8_t flags)
{
    bool success = true;

    for (uint16_t i = start; i <= stop; ++i)
    {
        auto it = configMap.find(i);
        if (it != configMap.end())
        {
            it->second.defaultValue.flags = (Flags)flags;
        }
        else
        {
            success = false;
        }
    }

    return success;
}

bool SharedDatabaseConfig::Update(const Binary& meas, uint16_t index, EventMode mode)
{
    return UpdateMeas(this->binary_input, index, meas);
}

bool SharedDatabaseConfig::Update(const DoubleBitBinary& meas, uint16_t index, EventMode mode)
{
    return UpdateMeas(this->double_binary, index, meas);
}

bool SharedDatabaseConfig::Update(const Analog& meas, uint16_t index, EventMode mode)
{
    return UpdateMeas(this->analog_input, index, meas);
}

bool SharedDatabaseConfig::Update(const Counter& meas, uint16_t index, EventMode mode)
{
    return UpdateMeas(this->counter, index, meas);
}

bool SharedDatabaseConfig::FreezeCounter(uint16_t index, bool clear, EventMode mode)
{
    auto itCounter = this->counter.find(index);
    if (itCounter == this->counter.end())
    {
        return false;
    }

    auto& oldValue = itCounter->second.defaultValue;
    auto itFrozenCounter = this->frozen_counter.find(index);
    if (itFrozenCounter == this->frozen_counter.end())
    {
        this->frozen_counter[index] = FrozenCounterConfig{
            (decltype(FrozenCounterConfig::svariation))itCounter->second.svariation,
            FrozenCounter{oldValue.value, oldValue.flags}, itCounter->second.clazz,
            (decltype(FrozenCounterConfig::evariation))itCounter->second.evariation, itCounter->second.deadband};
    }
    else
    {
        itFrozenCounter->second.defaultValue = FrozenCounter{oldValue.value, oldValue.flags};
    }

    if (clear)
    {
        oldValue.value = 0;
        oldValue.time = DNPTime{};
    }

    return true;
}

bool SharedDatabaseConfig::Update(const BinaryOutputStatus& meas, uint16_t index, EventMode mode)
{
    return UpdateMeas(this->binary_output_status, index, meas);
}

bool SharedDatabaseConfig::Update(const AnalogOutputStatus& meas, uint16_t index, EventMode mode)
{
    return UpdateMeas(this->analog_output_status, index, meas);
}

bool SharedDatabaseConfig::Update(const OctetString& meas, uint16_t index, EventMode mode)
{
    return UpdateMeas(this->octet_string, index, meas);
}

bool SharedDatabaseConfig::Update(const TimeAndInterval& meas, uint16_t index)
{
    return UpdateMeas(this->time_and_interval, index, meas);
}

bool SharedDatabaseConfig::Modify(FlagsType type, uint16_t start, uint16_t stop, uint8_t flags)
{
    switch (type)
    {
    case FlagsType::DoubleBinaryInput:
        return ModifyFlags(this->double_binary, start, stop, flags);
    case FlagsType::Counter:
        return ModifyFlags(this->counter, start, stop, flags);
    case FlagsType::FrozenCounter:
        return ModifyFlags(this->frozen_counter, start, stop, flags);
    case FlagsType::AnalogInput:
        return ModifyFlags(this->analog_input, start, stop, flags);
    case FlagsType::BinaryOutputStatus:
        return ModifyFlags(this->binary_output_status, start, stop, flags);
    case FlagsType::AnalogOutputStatus:
        return ModifyFlags(this->analog_output_status, start, stop, flags);
    case FlagsType::BinaryInput:
        return ModifyFlags(this->binary_input, start, stop, flags);

    default:
        return false;
    }
}

} // namespace opendnp3