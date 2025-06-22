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

/*********************************************************************************
 You will need to create your own NetSession-inherrited class and 
 NetSessionFactory-inherrited class. Then you provide the "factory" class
 as part of operations startListen and startConnect. On accept a new NetSession
 is created using the factory and registered for operations. The same happens
 on completion of a connect operation.

 NetSession state machine
    1) Reading - there is not enough data in the input buffer to complete parsing
       of a command. Network thread can delete a NetSession object in this state.
    2) InputQueued - there is a command ready for execution. NetSession object
       is in the input queue waiting to be picked up for command execution.
    3) Executing - a working thread picked up the object from the input queue and
       executing "next command" of the NetSession.
    4) OutputQueued - a working thread placed the object into an output queue.
    5) Writing - a network thread picked up the object from the output queue.
       Network thread can delete the object in this state.

    Reading -> Reading
    Reading -> deleted
    Reading -> InputQueued -> Executing -> OutputQueued -> Writing
    Writing -> Writing
    Writing -> deleted
    Writing -> Reading
 */

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
    Reading,
    InputQueued,
    Executing,
    OutputQueued,
    Writing,
};

class NetSession {
public:
    NetSession(NonBlockConnection* conn) : _conn(conn) {}
    virtual ~NetSession() = default;

    virtual int init() { return 0; }

    NetSessionState state() const { return _state; }
    void setState(NetSessionState state) { _state = state; }

    size_t getSize() const { return _size; }
    void   setSize(size_t size) { _size = size; }

    Buffer getReadBuffer(size_t size) { return _readBuf.getAvailable(size); }
    void updateReadBuffer(size_t size) { _readBuf.update(size); }

    Buffer getWriteBuffer(size_t size) { return _writeBuf.getAvailable(size); }

    Buffer getDataForWriting() { return _writeBuf.getData(); }
    void completedWriting(size_t size) { _writeBuf.used(size); }

    virtual int onRead() { return 0; }
    virtual int onWrite() { return 0; }

    NonBlockConnection* connection() const { return _conn; }

    int  getPipe() const { return _pipeFd; }
    void setPipe(int fd) { _pipeFd = fd; }

    NonBlockConnection* parent() const { return _conn; }

    void appendForWriting(DataBuffer& buffer) { _writeBuf.append(buffer); }

protected:
    // By default the session is an ingress session.
    // It starts with waiting for the initial data packet from the other side.
    NetSessionState _state = NetSessionState::Reading;

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
