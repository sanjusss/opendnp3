#ifndef OPENDNP3_SHAREDDATABASECONFIG_H__
#define OPENDNP3_SHAREDDATABASECONFIG_H__

#include "opendnp3/outstation/DatabaseConfig.h"
#include "opendnp3/outstation/IUpdateHandler.h"

#include <memory>

namespace opendnp3
{
class SharedDatabaseConfig final : public DatabaseConfig,
                                   public IUpdateHandler,
                                   public std::enable_shared_from_this<SharedDatabaseConfig>
{
public:
    SharedDatabaseConfig(const DatabaseConfig& database);

    // ------ IUpdateHandler interface ------
    bool Update(const Binary& meas, uint16_t index, EventMode mode) override;
    bool Update(const DoubleBitBinary& meas, uint16_t index, EventMode mode) override;
    bool Update(const Analog& meas, uint16_t index, EventMode mode) override;
    bool Update(const Counter& meas, uint16_t index, EventMode mode) override;
    bool FreezeCounter(uint16_t index, bool clear, EventMode mode) override;
    bool Update(const BinaryOutputStatus& meas, uint16_t index, EventMode mode) override;
    bool Update(const AnalogOutputStatus& meas, uint16_t index, EventMode mode) override;
    bool Update(const OctetString& meas, uint16_t index, EventMode mode) override;
    bool Update(const TimeAndInterval& meas, uint16_t index) override;
    bool Modify(FlagsType type, uint16_t start, uint16_t stop, uint8_t flags) override;

private:
    template<class TConfig, class TMeas>
    bool UpdateMeas(std::map<uint16_t, TConfig>& configMap, uint16_t index, 
                   const TMeas& meas);

    template<class TConfig>
    bool ModifyFlags(std::map<uint16_t, TConfig>& configMap, 
                    uint16_t start, uint16_t stop, uint8_t flags);
};
} // namespace opendnp3

#endif // OPENDNP3_SHAREDDATABASECONFIG_H__