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

#include "channel/tls/TLSClient.h"

#include "channel/SocketHelpers.h"

#include "opendnp3/logging/LogLevels.h"

#include <utility>

namespace opendnp3
{

TLSClient::TLSClient(const Logger& logger,
                     const std::shared_ptr<exe4cpp::StrandExecutor>& executor,
                     std::string adapter,
                     const TLSConfig& config,
                     ASIO_ERROR& ec)
    : logger(logger),
      condition(logger),
      executor(executor),
      adapter(std::move(adapter)),
      ctx(logger, false, config, ec),
      resolver(*executor->get_context())
{
}

bool TLSClient::Cancel()
{
    if (this->canceled)
    {
        return false;
    }

    ASIO_ERROR ec;
    resolver.cancel();
    this->canceled = true;
    return true;
}

bool TLSClient::BeginConnect(const IPEndpoint& remote, const connect_callback_t& callback)
{
    if (canceled)
        return false;

    auto stream
        = std::make_shared<ASIO::ssl::stream<ASIO::ip::tcp::socket>>(*this->executor->get_context(), this->ctx.value);

    auto verify = [self = shared_from_this()](bool preverified, ASIO::ssl::verify_context& ctx) -> bool {
        self->LogVerifyCallback(preverified, ctx);
        return preverified;
    };

    ASIO_ERROR ec;
    stream->set_verify_callback(verify, ec);

    if (ec)
    {
        auto cb = [self = shared_from_this(), callback, stream, ec] {
            if (!self->canceled)
            {
                callback(self->executor, stream, ec);
            }
        };

        this->executor->post(cb);
        return true;
    }

    SocketHelpers::BindToLocalAddress<ASIO::ip::tcp>(this->adapter, 0, stream->lowest_layer(), ec);

    if (ec)
    {
        auto cb = [self = shared_from_this(), callback, stream, ec] {
            if (!self->canceled)
            {
                callback(self->executor, stream, ec);
            }
        };

        this->executor->post(cb);
        return true;
    }

    const auto address = ASIO::ip::address::from_string(remote.address, ec);
    auto self = this->shared_from_this();
    if (ec)
    {
        // Try DNS resolution instead
        auto cb = [self, callback, stream](const ASIO_ERROR& ec, ASIO::ip::tcp::resolver::iterator endpoints) {
            self->HandleResolveResult(callback, stream, endpoints, ec);
        };

        std::stringstream portstr;
        portstr << remote.port;

        resolver.async_resolve(ASIO::ip::tcp::resolver::query(remote.address, portstr.str()), executor->wrap(cb));

        return true;
    }

    ASIO::ip::tcp::endpoint remoteEndpoint(address, remote.port);
    auto cb = [self, stream, callback](const ASIO_ERROR& ec) { self->HandleConnectResult(callback, stream, ec); };

    stream->lowest_layer().async_connect(remoteEndpoint, executor->wrap(cb));
    return true;
}

void TLSClient::LogVerifyCallback(bool preverified, ASIO::ssl::verify_context& ctx)
{
    const int MAX_SUBJECT_NAME = 512;

    int depth = X509_STORE_CTX_get_error_depth(ctx.native_handle());

    // lookup the subject name
    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    char subjectName[MAX_SUBJECT_NAME];
    X509_NAME_oneline(X509_get_subject_name(cert), subjectName, MAX_SUBJECT_NAME);

    if (preverified)
    {
        FORMAT_LOG_BLOCK(this->logger, flags::INFO, "Verified certificate at depth: %d subject: %s", depth,
                         subjectName);
    }
    else
    {
        const int err = X509_STORE_CTX_get_error(ctx.native_handle());
        FORMAT_LOG_BLOCK(this->logger, flags::WARN, "Error verifying certificate at depth: %d subject: %s error: %d:%s",
                         depth, subjectName, err, X509_verify_cert_error_string(err));
    }
}

void TLSClient::HandleResolveResult(const connect_callback_t& callback,
                                    const std::shared_ptr<ASIO::ssl::stream<ASIO::ip::tcp::socket>>& stream,
                                    const ASIO::ip::tcp::resolver::iterator& endpoints,
                                    const ASIO_ERROR& ec)
{
    if (ec)
    {
        if (!this->canceled)
        {
            callback(this->executor, stream, ec);
        }
    }
    else
    {
        // attempt a connection to each endpoint in the iterator until we connect
        auto cb = [self = this->shared_from_this(), callback, stream](const ASIO_ERROR& ec,
                                                                      ASIO::ip::tcp::resolver::iterator endpoints) {
            self->HandleConnectResult(callback, stream, ec);
        };

        ASIO::async_connect(stream->lowest_layer(), endpoints, this->condition, this->executor->wrap(cb));
    }
}

void TLSClient::HandleConnectResult(const connect_callback_t& callback,
                                    const std::shared_ptr<ASIO::ssl::stream<ASIO::ip::tcp::socket>>& stream,
                                    const ASIO_ERROR& ec)
{
    if (ec)
    {
        if (!this->canceled)
        {
            callback(this->executor, stream, ec);
        }
    }
    else
    {
        auto cb = [self = shared_from_this(), callback, stream](const ASIO_ERROR& ec) {
            if (!self->canceled)
            {
                callback(self->executor, stream, ec);
            }
        };

        stream->async_handshake(ASIO::ssl::stream_base::client, executor->wrap(cb));
    }
}

} // namespace opendnp3
