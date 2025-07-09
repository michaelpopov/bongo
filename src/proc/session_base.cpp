/**********************************************
   File:   session_base.cpp

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
#include "session_base.h"
#include "notification_base.h"
#include "utils/log.h"
#include <assert.h>
#include <string.h>
#include <experimental/scope>
#include <string_view>

namespace bongo {

/*******************************************************************************
 *   InputMessagesQueue
 */
InputMessagePtr InputMessagesQueue::pop() {
    const std::unique_lock<std::mutex> lock(_mutex);
    if (_queue.empty()) {
        return nullptr;
    }

    auto result = _queue.front();
    _queue.pop();
    return result;
}

void InputMessagesQueue::push(InputMessagePtr& imsg) {
    const std::unique_lock<std::mutex> lock(_mutex);
    _queue.push(imsg);
}

bool InputMessagesQueue::empty() const {
    const std::unique_lock<std::mutex> lock(_mutex);
    return _queue.empty();
}

/*******************************************************************************
 *   SessionBase
 */
void SessionBase::processReadBufferData() {
    if (_headerSize > 0) {
        processReadBufferDataFixedHeader();
    } else {
        processReadBufferDataVariableHeader();
    }
}

void SessionBase::processReadBufferDataFixedHeader() {
    for (;;) {
        // Get size of the request
        Buffer src = _readBuf.getData();
        if (src.size < _headerSize) {
            break;
        }

        // Light parsing to get body size from the message header.
        uint32_t size = parseMessageSize(src);
        if (size > _maxBodySize) {
            // TODO: KILL SESSION
            return;
        }

        // Check is there a whole request body in the buffer
        if (src.size < size + _headerSize) {
            break;
        }

        InputMessagePtr msg = new InputMessage;
        msg->header.resize(_headerSize);
        memcpy(msg->header.data(), src.ptr, _headerSize);
        msg->body.resize(size);
        memcpy(msg->body.data(), src.ptr + _headerSize, size);
        _inputQueue.push(msg);

        // After getting a whole request release space in the buffer
        _readBuf.used(_headerSize + size);        
    }
}

void SessionBase::processReadBufferDataVariableHeader() {
    for (;;) {
        // Get size of the request
        Buffer src = _readBuf.getData();
        std::string_view haystack(src.ptr, src.size);
        const size_t pos = haystack.find(_headerDelimiter);
        if (pos == std::string_view::npos) {
            if (src.size > _maxHeaderSize) {
                // TODO: Bad input. Kill session.
            }

            break;
        }

        // Light parsing to get body size from the message header.
        uint32_t size = parseMessageSize(src);
        if (size > _maxBodySize) {
            // TODO: KILL SESSION
            return;
        }

        // Check is there a whole request body in the buffer
        size_t bodyStartPos = pos + _headerDelimiter.length();
        if (src.size < size + bodyStartPos) {
            break;
        }

        InputMessagePtr msg = new InputMessage;
        msg->header.resize(bodyStartPos);
        memcpy(msg->header.data(), src.ptr, bodyStartPos);
        msg->body.resize(size);
        memcpy(msg->body.data(), src.ptr + bodyStartPos, size);
        _inputQueue.push(msg);

        // After getting a whole request release space in the buffer
        _readBuf.used(bodyStartPos + size);        
    }
}

std::optional<RequestBase*> SessionBase::getRequest() {
    InputMessagePtr msg = _inputQueue.pop();
    if (msg == nullptr) {
        return {};
    }

    auto cleanup = std::experimental::scope_exit([&]() {
        if (msg != nullptr) {
            delete msg;
        }
    });

    return parseMessage(msg);
}

int SessionBase::onRead(SessionsQueue* queue) {
    processReadBufferData();

    // If the session is already in processing state, let processor to handle it.
    if (_state != SessionState::Released) {
        return 0;
    }

    // If the session is in the Released state, let's pass it to processing.
    if (!_inputQueue.empty()) {
        _state = SessionState::InProcessing;
        queue->push(this);
    }

    return 0;
}

} // namespace bongo
