/*
 * Copyright 2013-2022 Step Function I/O, LLC
 *
 * Licensed to Green Energy Corp (www.greenenergycorp.com) and Step Function I/O
 * LLC (https://stepfunc.io) under one or more contributor license agreements.
 * See the NOTICE file distributed with this work for additional information
 * regarding copyright ownership. Green Energy Corp and Step Function I/O LLC license
 * this file to you under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License. You may obtain
 * a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OPENDNP3_MEASUREMENTCONFIG_H
#define OPENDNP3_MEASUREMENTCONFIG_H

#include "opendnp3/app/MeasurementInfo.h"
#include "opendnp3/app/MeasurementTypes.h"
#include "opendnp3/gen/PointClass.h"

namespace opendnp3
{

// All entries have this information
template<class Info, class MeasurementType> struct StaticConfig
{
    typename Info::static_variation_t svariation = Info::DefaultStaticVariation;
    MeasurementType defaultValue;
};

template<class Info, class MeasurementType> struct EventConfig : StaticConfig<Info, MeasurementType>
{
    PointClass clazz = PointClass::Class1;
    typename Info::event_variation_t evariation = Info::DefaultEventVariation;
};

template<class Info, class MeasurementType> struct DeadbandConfig : EventConfig<Info, MeasurementType>
{
    typename Info::value_t deadband = 0;
};

struct BinaryConfig : public EventConfig<BinaryInfo, Binary>
{
};

struct DoubleBitBinaryConfig : public EventConfig<DoubleBitBinaryInfo, DoubleBitBinary>
{
};

struct AnalogConfig : public DeadbandConfig<AnalogInfo, Analog>
{
};

struct CounterConfig : public DeadbandConfig<CounterInfo, Counter>
{
};

struct FrozenCounterConfig : public DeadbandConfig<FrozenCounterInfo, FrozenCounter>
{
};

struct BOStatusConfig : public EventConfig<BinaryOutputStatusInfo, BinaryOutputStatus>
{
};

struct AOStatusConfig : public DeadbandConfig<AnalogOutputStatusInfo, AnalogOutputStatus>
{
};

struct OctetStringConfig : public EventConfig<OctetStringInfo, OctetString>
{
};

struct TimeAndIntervalConfig : public StaticConfig<TimeAndIntervalInfo, TimeAndInterval>
{
};

struct SecurityStatConfig
{
};

} // namespace opendnp3

#endif
