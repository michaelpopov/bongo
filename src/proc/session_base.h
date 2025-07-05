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
#include "utils/data_buffer.h"
#include "utils/thread_queue.h"
#include <optional>
#include <mutex>
#include <vector>
#include <queue>

namespace bongo {

class SessionBase;
using SessionsQueue = ThreadQueue<SessionBase*>;

struct InputMessage {
    std::vector<char> header;
    std::vector<char> body;
};

using InputMessagePtr = InputMessage*;

class InputMessagesQueue {
public:
    InputMessagePtr pop();
    void push(InputMessagePtr& imsg);
    bool empty() const;

private:
    using Queue = std::queue<InputMessagePtr>; // TODO: make it lock-less list

private:
    mutable std::mutex _mutex;    
    Queue _queue;
};

enum class SessionState {
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

class SessionBase {
public:
    virtual ~SessionBase() = default;

    SessionState state() const { return _state; }
    virtual void setState(SessionState state) { _state = state; }

    virtual int init() { return 0; }

    Buffer getReadBuffer(size_t size) { return _readBuf.getAvailable(size); }
    void updateReadBuffer(size_t size) { _readBuf.update(size); }

    Buffer getWriteBuffer(size_t size) { return _writeBuf.getAvailable(size); }

    Buffer getDataForWriting() { return _writeBuf.getData(); }
    void completedWriting(size_t size) { _writeBuf.used(size); }
    void appendForWriting(DataBuffer& buffer) { _writeBuf.append(buffer); }

    virtual int onRead(SessionsQueue* session);
    virtual int onWrite() { return 0; }

    virtual std::optional<RequestBase*> getRequest();
    virtual bool hasRequest() const { return _inputQueue.empty(); }
    virtual ProcessingStatus sendResponse(const ResponseBase& /*response*/) { return ProcessingStatus::Failed; }
    virtual bool failed() const { return true; }

    int  getPipe() const { return _pipeFd; }
    void setPipe(int fd) { _pipeFd = fd; }

protected:
    SessionState _state = SessionState::Released;
    DataBuffer _readBuf{1024};
    DataBuffer _writeBuf{1024 * 16};
    InputMessagesQueue _inputQueue;

protected: // Support for the fixed-size header protocol
    size_t _headerSize = 0;
    size_t _maxBodySize = 1024;
    virtual size_t parseMessageSize(Buffer header) { (void)header; return 0; }

protected:
    // Process data in a read buffer, convert it to an input message if possible,
    // put this input message into the queue.
    // Base implementation supports fixed-size header protocol.
    virtual void processReadBufferData();
    virtual std::optional<RequestBase*> parseMessage(const InputMessagePtr&) { return {}; }

private:
    int _pipeFd = -1;
};

} // namespace bongo
