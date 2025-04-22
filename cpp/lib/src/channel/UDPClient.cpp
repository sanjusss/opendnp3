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

#include "channel/UDPClient.h"

#include "channel/SocketHelpers.h"

#include <sstream>
#include <utility>

namespace opendnp3
{

UDPClient::UDPClient(const Logger& logger, const std::shared_ptr<exe4cpp::StrandExecutor>& executor)
    : condition(logger), executor(executor), socket(*executor->get_context()), resolver(*executor->get_context())
{
}

bool UDPClient::Cancel()
{
    if (this->canceled || !this->connecting)
    {
        return false;
    }

    ASIO_ERROR ec;
    socket.cancel(ec);
    resolver.cancel();
    this->canceled = true;
    return true;
}

bool UDPClient::Open(const IPEndpoint& localEndpoint, const IPEndpoint& remoteEndpoint, connect_callback_t callback)
{
    if (connecting || canceled)
        return false;

    this->connecting = true;

    ASIO_ERROR ec;
    SocketHelpers::BindToLocalAddress<ASIO::ip::udp>(localEndpoint.address, localEndpoint.port, this->socket, ec);

    if (ec)
    {
        return this->PostConnectError(callback, ec);
    }

    // Find remote address
    const auto address = ASIO::ip::address::from_string(remoteEndpoint.address, ec);
    auto self = this->shared_from_this();
    if (ec)
    {
        // Try DNS resolution instead
        auto cb = [self, callback](const ASIO_ERROR& ec, ASIO::ip::udp::resolver::iterator endpoints) {
            self->HandleResolveResult(callback, endpoints, ec);
        };

        std::stringstream portstr;
        portstr << remoteEndpoint.port;

        resolver.async_resolve(ASIO::ip::udp::resolver::query(remoteEndpoint.address, portstr.str()),
                               executor->wrap(cb));

        return true;
    }

    ASIO::ip::udp::endpoint asioRemoteEndpoint(address, remoteEndpoint.port);
    auto cb = [self, callback](const ASIO_ERROR& ec) {
        self->connecting = false;
        if (!self->canceled)
        {
            callback(self->executor, std::move(self->socket), ec);
        }
    };

    // On UDP sockets, connecting only sets the address used in future async_send.
    socket.async_connect(asioRemoteEndpoint, executor->wrap(cb));
    return true;
}

void UDPClient::HandleResolveResult(const connect_callback_t& callback,
                                    const ASIO::ip::udp::resolver::iterator& endpoints,
                                    const ASIO_ERROR& ec)
{
    if (ec)
    {
        this->PostConnectError(callback, ec);
    }
    else
    {
        // attempt a connection to each endpoint in the iterator until we connect
        auto cb = [self = shared_from_this(), callback](const ASIO_ERROR& ec,
                                                        ASIO::ip::udp::resolver::iterator endpoints) {
            self->connecting = false;
            if (!self->canceled)
            {
                callback(self->executor, std::move(self->socket), ec);
            }
        };

        ASIO::async_connect(this->socket, endpoints, this->condition, this->executor->wrap(cb));
    }
}

bool UDPClient::PostConnectError(const connect_callback_t& callback, const ASIO_ERROR& ec)
{
    auto cb = [self = shared_from_this(), ec, callback]() {
        self->connecting = false;
        if (!self->canceled)
        {
            callback(self->executor, std::move(self->socket), ec);
        }
    };
    executor->post(cb);
    return true;
}

} // namespace opendnp3
