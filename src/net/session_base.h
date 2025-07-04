/**********************************************
   File:   session_base.h

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
#include <memory>
#include <queue>
#include <mutex>
#include <atomic>

namespace bongo {

class NonBlockConnection;

class NetSession;
using SessionsQueue = ThreadQueue<NetSession*>;

struct InputMessage {
    std::vector<char> header;
    std::vector<char> body;
};

using InputMessagePtr = InputMessage*;

class InputMessagesQueue {
public:
    InputMessagePtr pop();
    void push(InputMessagePtr& imsg);
    size_t empty() const;

private:
    using Queue = std::queue<InputMessagePtr>; // TODO: make it lock-less list

private:
    mutable std::mutex _mutex;    
    Queue _queue;
};

enum class NetSessionState {
    Released,
    InProcessing,
};

struct RequestBase {
    virtual ~RequestBase() = default;
};

struct ResponseBase {
    virtual ~ResponseBase() = default;
};

enum class ProcessingStatus {
    Ok,
    Failed,
    IncompleteDataSend,
};

class NetSession {
public:
    NetSession(NonBlockConnection* conn) : _conn(conn) {}
    virtual ~NetSession() = default;

    virtual int init() { return 0; }

    NetSessionState state() const { return _state; }
    virtual void setState(SessionsQueue*, NetSessionState state) { _state = state; }

    size_t getSize() const { return _size; }
    void   setSize(size_t size) { _size = size; }

    Buffer getReadBuffer(size_t size) { return _readBuf.getAvailable(size); }
    void updateReadBuffer(size_t size) { _readBuf.update(size); }

    Buffer getWriteBuffer(size_t size) { return _writeBuf.getAvailable(size); }

    Buffer getDataForWriting() { return _writeBuf.getData(); }
    void completedWriting(size_t size) { _writeBuf.used(size); }

    virtual int onRead(SessionsQueue*) { return 0; }
    virtual int onWrite() { return 0; }

    NonBlockConnection* connection() const { return _conn; }

    int  getPipe() const { return _pipeFd; }
    void setPipe(int fd) { _pipeFd = fd; }

    NonBlockConnection* parent() const { return _conn; }

    void appendForWriting(DataBuffer& buffer) { _writeBuf.append(buffer); }

    // Support for the session's request queue
    virtual std::optional<RequestBase*> getRequest() { return nullptr; }
    virtual bool hasRequest() const { return false; }
    virtual ProcessingStatus sendResponse(const ResponseBase& /*response*/) { return ProcessingStatus::Failed; }
    virtual bool failed() const { return true; }

protected:
    NetSessionState _state = NetSessionState::Released;

    NonBlockConnection* _conn;
    DataBuffer _readBuf{1024};
    DataBuffer _writeBuf{1024 * 16};

    size_t _size = 1024;

    int _pipeFd = -1;

protected: // PROTOCOL SUPPORT
    // We support only fixed-size header protocol. Live with this.
    // Set this value in the derived class. 
    // Use it in process() to get an input message fixed-size header.
    size_t _headerSize = 0;

    // Sanity check. Message body size cannot exceed this value.
    size_t _maxBodySize = 1024;

    // Pass header data to this function and get the size of the data message
    // that follows the fixed-size header on wire.
    // Override it in the derived class!
    // One more thing: this fixed-size header should be parsed at no cost.
    virtual size_t parseMessageSize(Buffer header) { (void)header; return 0; }

protected: // PROCESSING SUPPORT
    InputMessagesQueue _inputQueue;

    // Process data in a read buffer, convert it to an input message if possible,
    // put this input message into the queue.
    virtual void processReadBufferData();
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
