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

#ifndef OPENDNP3_ASIOTESTS_MOCKTCPPAIR_H
#define OPENDNP3_ASIOTESTS_MOCKTCPPAIR_H

#include "channel/TCPClient.h"
#include "mocks/MockIO.h"
#include "mocks/MockTCPClientHandler.h"
#include "mocks/MockTCPServer.h"

#include <dnp3mocks/MockLogHandler.h>

class MockTCPPair
{

public:
    MockTCPPair(std::shared_ptr<MockIO> io, uint16_t port, ASIO_ERROR ec = ASIO_ERROR());

    ~MockTCPPair();

    void Connect(size_t num = 1);

    bool NumConnectionsEqual(size_t num) const;

private:
    MockLogHandler log;
    std::shared_ptr<MockIO> io;
    uint16_t port;
    std::shared_ptr<MockTCPClientHandler> chandler;
    std::shared_ptr<opendnp3::TCPClient> client;
    std::shared_ptr<MockTCPServer> server;
};

#endif
