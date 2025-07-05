/**********************************************
   File:   session_demo.h

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
#include "net/net_session.h"
#include "net/nonblock_conn.h"
#include <mutex>
#include <list>
#include <optional>
#include <assert.h>

namespace bongo {

/*******************************************************************************
 *   Dummies
 */
class DummyNetSession : public NetSession {
public:
    DummyNetSession(NonBlockConnection* conn) : NetSession(conn) {}
};

class DummyNetSessionFactory : public NetSessionFactory {
public:
    NetSession* makeSession(NonBlockConnection* conn) override { return new DummyNetSession(conn); }
};

/*******************************************************************************
 *   Echo
 */
class EchoNetSession : public NetSession {
public:
    EchoNetSession(NonBlockConnection* conn) : NetSession(conn) { }
    int onRead(SessionsQueue*) override;
};
    
class EchoNetSessionFactory : public NetSessionFactory {
public:
    EchoNetSessionFactory() : NetSessionFactory() {}
    NetSession* makeSession(NonBlockConnection* conn) override { return new EchoNetSession(conn); }
};

/*******************************************************************************
 *   BigWriter
 */
class BigWriterNetSession : public NetSession {
public:
    BigWriterNetSession(NonBlockConnection* conn) : NetSession(conn) {}
    int init() override;
    int onRead(SessionsQueue*) override;

private:
    const size_t BIG_SIZE = 100 * 1024 * 1024;
};
    
class BigWriterNetSessionFactory : public NetSessionFactory {
public:
    NetSession* makeSession(NonBlockConnection* conn) override { return new BigWriterNetSession(conn); }
};

/*******************************************************************************
 *   ReqRespSession
 * 
 *   Protocol.
 *   Request: 4 bytes length + N bytes command
 *   Response: 4 bytes length + N bytes message
 *   
 */

struct RequestDemo : public RequestBase {
    std::string command;
};

struct ResponseDemo : public ResponseBase {
    std::string data;
};

class ReqRespSession : public NetSession {
public:
    ReqRespSession(NonBlockConnection* conn)
     : NetSession(conn)
    {
        _headerSize = 4;
        _maxBodySize = 128;
    }

    int onRead(SessionsQueue*) override;

    std::optional<RequestBase*> getRequest() override;
    ProcessingStatus sendResponse(const ResponseBase& resp) override;

protected:
    virtual size_t parseMessageSize(Buffer header) override;

};
    
class ReqRespSessionFactory : public NetSessionFactory {
public:
    NetSession* makeSession(NonBlockConnection* conn) override {
        return new ReqRespSession(conn);
    }
};


} // namespace bongo
