/**********************************************
   File:   session_demo.cpp

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

#include "session_demo.h"
#include "net/nonblock_conn.h"
#include "utils/log.h"
#include <assert.h>
#include <string.h>
#include <iostream>
#include <experimental/scope>

namespace bongo {

/*******************************************************************************
 *   EchoNetSession
 */
int EchoNetSession::onRead(SessionsQueue*) {
    LOG_TRACE << "EchoNetSession::onRead enter";

    Buffer src = _readBuf.getData();
    if (src.size == 0) {
        LOG_TRACE << "EchoNetSession::onRead no data";
        return 0;
    }

    LOG_TRACE << "EchoNetSession::onRead data size " << src.size;

    Buffer dest = _writeBuf.getAvailable(src.size);
    memcpy(dest.ptr, src.ptr, src.size);

    _writeBuf.update(src.size);
    _readBuf.used(src.size);

    _conn->writeData();

    return 0;
}

/*******************************************************************************
 *   BigWriter
 */
int BigWriterNetSession::init() {
    const size_t target_buf_size = sizeof(size_t) + BIG_SIZE;
    Buffer buf = _writeBuf.getAvailable(target_buf_size);

    memcpy(buf.ptr, &BIG_SIZE, sizeof(size_t));

    char ch = 'A';
    for (size_t i = sizeof(size_t); i < target_buf_size; i++, ch++) {
        if (ch > 'Z') {
            ch = 'A';
        }
        buf.ptr[i] = ch;
    }

    _writeBuf.update(target_buf_size);

    buf = _writeBuf.getData();
    if (buf.size != target_buf_size) {
        LOG_ERROR << "BigWriterNetSession::init: failed to set a write buffer";
        return -1;
    }

    LOG_TRACE << "BigWriterNetSession::init: finished ok";
    return 0;
}

int BigWriterNetSession::onRead(SessionsQueue*) {
    Buffer src = _readBuf.getData();
    if (src.size < sizeof(size_t)) {
        LOG_TRACE << "BigWriterNetSession::onRead not enough data";
        return 0;
    }

    size_t val;
    memcpy(&val, src.ptr, sizeof(val));

    if (val != BIG_SIZE) {
        LOG_ERROR << "BigWriterNetSession::onRead invalid feedback";
    }

    LOG_TRACE << "BigWriterNetSession::onRead: finished ok";
    return -1;
}

/*******************************************************************************
 *   ReqRespSession
 */
std::optional<RequestBase*> ReqRespSession::parseMessage(const InputMessagePtr& msg) {
    if (msg->header.size() != sizeof(uint32_t)) {
        // TODO: KILL SESSION HERE!!!
        return {};
    }

    uint32_t size;
    memcpy(&size, msg->header.data(), sizeof(uint32_t));

    if (msg->body.size() != size) {
        // TODO: KILL SESSION HERE!!!
        return {};
    }

    RequestDemo* req = new RequestDemo;
    req->command.assign(msg->body.data(), msg->body.size());
    return req;
}

ProcessingStatus ReqRespSession::sendResponse(const ResponseBase& response) {
    const ResponseDemo& resp = dynamic_cast<const ResponseDemo&>(response);

    size_t serializedSize = sizeof(uint32_t) + resp.data.size();
    Buffer dest = _writeBuf.getAvailable(serializedSize);

    uint32_t size = resp.data.size();
    memcpy(dest.ptr, &size, sizeof(size));
    memcpy(dest.ptr + sizeof(size), resp.data.data(), size);

    _writeBuf.update(serializedSize);

    _conn->writeData();

    if (_writeBuf.size() > 0) {
        LOG_TRACE << "ReqRespSession::sendResponse: more data";
        return ProcessingStatus::IncompleteDataSend;
    }

    return ProcessingStatus::Ok;
}

size_t ReqRespSession::parseMessageSize(Buffer header) {
    assert(header.size >= sizeof(uint32_t));
    uint32_t size;
    memcpy(&size, header.ptr, sizeof(uint32_t));
    return size;
}

} // namespace bongo
