/**********************************************
   File:   net_session.h

   Copyright 2025 Michael Popov

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 **********************************************/

#pragma once
#include "utils/thread_queue.h"
#include "utils/data_buffer.h"
#include "proc/session_base.h"
#include <memory>
#include <queue>
#include <mutex>
#include <atomic>

namespace bongo {

class NonBlockConnection;

class NetSession;
using SessionsQueue = ThreadQueue<SessionBase*>;

class NetSession : public SessionBase {
public:
    NetSession(NonBlockConnection* conn) : _conn(conn) {}
    virtual ~NetSession() = default;
    NonBlockConnection* connection() const { return _conn; }
protected:
    NonBlockConnection* _conn;
};

class NetSessionFactory {
public:
    virtual ~NetSessionFactory() = default;
    virtual NetSession* makeSession(NonBlockConnection* conn) = 0;
};

using NetSessionFactoryPtr = std::shared_ptr<NetSessionFactory>;

class NetSessionFactoryOwner {
public:
    NetSessionFactoryOwner(NetSessionFactoryPtr factory) : _factory(factory) {}
    virtual ~NetSessionFactoryOwner() = default;

    NetSessionFactoryPtr factory() const { return _factory; }

private:
    NetSessionFactoryPtr _factory;
};
   
} // namespace bongo
