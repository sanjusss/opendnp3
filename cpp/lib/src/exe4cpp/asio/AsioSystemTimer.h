/*
 * Copyright (c) 2018, Automatak LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef EXE4CPP_ASIO_ASIOSYSTEMTIMER_H
#define EXE4CPP_ASIO_ASIOSYSTEMTIMER_H

#include "exe4cpp/ITimer.h"

#include "AsioHeader.h"

namespace exe4cpp
{

class AsioSystemTimer final : public exe4cpp::ITimer
{
    friend class BasicExecutor;
    friend class StrandExecutor;

public:
    AsioSystemTimer(const std::shared_ptr<ASIO::io_context>& io_context) :
        io_context{io_context},
        impl{*io_context}
    {}

    // Uncopyable
    AsioSystemTimer(const AsioSystemTimer&) = delete;
    AsioSystemTimer& operator=(const AsioSystemTimer&) = delete;

    static std::shared_ptr<AsioSystemTimer> create(const std::shared_ptr<ASIO::io_context>& io_context)
    {
        return std::make_shared<AsioSystemTimer>(io_context);
    }

    virtual void cancel() override
    {
        ASIO_ERROR ec;
        impl.cancel(ec);
    }

    virtual steady_time_t expires_at() override
    {
        return std::chrono::steady_clock::now() + (impl.expires_at() - std::chrono::system_clock::now());
    }

private:
    const std::shared_ptr<ASIO::io_context> io_context;
    ASIO::basic_waitable_timer<std::chrono::system_clock> impl;
};

}

#endif
