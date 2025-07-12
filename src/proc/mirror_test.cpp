/**********************************************
   File:   mirror_test.cpp

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
#include "mirror_test.h"
#include "utils/log.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <experimental/scope>

namespace bongo {

const std::string MirrorSession::HeaderDelimiter = "\r\n";

/***********************************************************
 *   Session
 */
MirrorSession::MirrorSession() {
    _headerSize = 4;  // By default it supports a fixed-size header protocol
}

void MirrorSession::setHeaderDelimiter() {
    _headerSize = 0;
    _headerDelimiter = HeaderDelimiter;
}

ProcessingStatus MirrorSession::sendResponse(const ResponseBase& response) {
    const MirrorResponse& resp = dynamic_cast<const MirrorResponse&>(response);
    return _headerSize > 0 ? sendResponseFixedHeader(resp) :
                             sendResponseVariableHeader(resp);
}

ProcessingStatus MirrorSession::sendResponseFixedHeader(const MirrorResponse& resp) {
    uint32_t len = resp.output.length();
    size_t serializedSize = sizeof(len) + resp.output.length();
    Buffer dest = _writeBuf.getAvailable(serializedSize);

    memcpy(dest.ptr, &len, sizeof(len));
    memcpy(dest.ptr + sizeof(len), resp.output.data(), len);

    _writeBuf.update(serializedSize);
    return ProcessingStatus::Ok;
}

ProcessingStatus MirrorSession::sendResponseVariableHeader(const MirrorResponse& resp) {
    const std::string output = makeMirrorPacketWithVarHeader(resp.output);
    Buffer dest = _writeBuf.getAvailable(output.length());
    memcpy(dest.ptr, output.data(), output.length());
    _writeBuf.update(output.length());
    return ProcessingStatus::Ok;
}

size_t MirrorSession::parseMessageSize(Buffer header) {
    size_t _ = 0;
    return _headerSize > 0 ? parseMessageSizeFixedHeader(header) :
                             parseMessageSizeVariableHeader(header, _);
}

size_t MirrorSession::parseMessageSizeFixedHeader(Buffer header) {
    uint32_t size = -1;
    assert(header.size >= sizeof(size));
    memcpy(&size, header.ptr, sizeof(size));
    return size;
}

size_t MirrorSession::parseMessageSizeVariableHeader(Buffer header, size_t& headerSize) {
    headerSize = 0;

    // N\r\n
    const std::string_view haystack(header.ptr, header.size);
    size_t endOfValuePos = haystack.find(HeaderDelimiter);
    if (endOfValuePos == std::string_view::npos) {
        // TODO: Invalid protocol. Kill session.
        return 0;
    }

    header.ptr[endOfValuePos] = '\0';
    auto restoreHeader = std::experimental::scope_exit([&]() {
        header.ptr[endOfValuePos] = HeaderDelimiter[0];
    });

    char* endptr = nullptr;
    const char* str = header.ptr;
    long val = strtol(str, &endptr, 10);
    if (endptr == str || *endptr != '\0' || val < 0) {
        // TODO: Invalid protocol. Kill session.
        return 0;
    }

    headerSize = endptr - header.ptr + HeaderDelimiter.length();
    return val;
}

std::optional<RequestBase*> MirrorSession::parseMessage(const InputMessagePtr& msg) {
    MirrorRequest* req = new MirrorRequest;
    req->input.assign(msg->body.data(), msg->body.size());
    return req;
}

std::string MirrorSession::makeMirrorPacketWithVarHeader(const std::string& str) {
    const uint32_t len = str.length();
    char buffer[64];
    snprintf(buffer, sizeof(buffer)-1, "%u", len);

    std::string result = buffer;
    result += HeaderDelimiter;
    result += str;

    return result;
}

// For testing purposes. Executed on the test thread.
std::string MirrorSession::parseOutput(Buffer buf, size_t& size) {
    size_t headerSize = 0;
    size_t bodySize = parseMessageSizeVariableHeader(buf, headerSize);
    size = headerSize + bodySize;
    assert(headerSize + bodySize <= buf.size);
    std::string result(buf.ptr + headerSize, bodySize);
    return result;
}

/***********************************************************
 *   Processor
 */
ProcessingStatus MirrorProcessor::processRequest(SessionBase* session, RequestBase* request) {
    MirrorRequest* req = dynamic_cast<MirrorRequest*>(request);
    assert(req);

    MirrorResponse resp;
    resp.output = req->input;

    assert(session);
    return session->sendResponse(resp);
}

} // namespace bongo
