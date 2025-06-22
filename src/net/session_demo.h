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
#include "session_base.h"
#include "nonblock_conn.h"
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
    EchoNetSession(NonBlockConnection* conn, size_t size) : NetSession(conn) { _size = size; }
    int onRead() override;
};
    
class EchoNetSessionFactory : public NetSessionFactory {
public:
    EchoNetSessionFactory(size_t size = 1024) : NetSessionFactory(), _size(size) {}
    NetSession* makeSession(NonBlockConnection* conn) override { return new EchoNetSession(conn, _size); }
private:
    size_t _size;
};

/*******************************************************************************
 *   BigWriter
 */
class BigWriterNetSession : public NetSession {
public:
    BigWriterNetSession(NonBlockConnection* conn) : NetSession(conn) {}
    int init() override;
    int onRead() override;

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

struct Request {
    std::string command;
};

struct Response {
    std::string data;
};

class ReqRespSession : public NetSession {
public:
    ReqRespSession(NonBlockConnection* conn, PipeQueue* pipeQueue, SessionsQueue* sessionsQueue)
     : NetSession(conn), _pipeQueue(pipeQueue), _sessionsQueue(sessionsQueue)
    {
        assert(_pipeQueue);
        assert(_sessionsQueue);
        _headerSize = 4;
        _maxBodySize = 128;
    }

    int onRead() override;
    std::optional<Request> getRequest();
    int sendResponse(const Response& resp);
    PipeQueue* getPipeQueue() { return _pipeQueue; }

protected:
    virtual size_t parseMessageSize(Buffer header) override;

private:
    PipeQueue* _pipeQueue;
    SessionsQueue* _sessionsQueue;
};
    
class ReqRespSessionFactory : public NetSessionFactory {
public:
    ReqRespSessionFactory(PipeQueue* pipeQueue, SessionsQueue* sessionsQueue)
      : _pipeQueue(pipeQueue),
        _sessionsQueue(sessionsQueue) {}

    NetSession* makeSession(NonBlockConnection* conn) override {
        return new ReqRespSession(conn, _pipeQueue, _sessionsQueue);
    }

private:
    PipeQueue* _pipeQueue;
    SessionsQueue* _sessionsQueue;
};


} // namespace bongo
