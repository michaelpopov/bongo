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
#include "nonblock_conn.h"
#include "utils/log.h"
#include <assert.h>
#include <string.h>
#include <iostream>

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

size_t InputMessagesQueue::empty() const {
    const std::unique_lock<std::mutex> lock(_mutex);
    return _queue.empty();
}

/*******************************************************************************
 *   NetSession
 */
void NetSession::processReadBufferData() {
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

        LOG_TRACE << "NetSession::process:  size=" << size;

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
        _readBuf.used(sizeof(uint32_t) + size);
    }
}

} // namespace bongo

