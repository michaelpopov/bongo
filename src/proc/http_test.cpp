/**********************************************
   File:   http_test.cpp

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
#include "http_test.h"
#include "utils/log.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <experimental/scope>

namespace bongo {

const std::string HeaderDelimiter = "\r\n\r\n";
const std::string ContentLengthHeader = "Content-Length: ";
const std::string SimpleHttpResponse = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\nContent-Type: text/html\r\n\r\nHello World!";

/***********************************************************
 *   Session
 */
HttpSession::HttpSession() {
    _headerDelimiter = HeaderDelimiter;
}

ProcessingStatus HttpSession::sendResponse(const ResponseBase&) {
    Buffer dest = _writeBuf.getAvailable(SimpleHttpResponse.length());
    memcpy(dest.ptr, SimpleHttpResponse.data(), SimpleHttpResponse.length());
    _writeBuf.update(SimpleHttpResponse.length());
    return ProcessingStatus::Ok;
}

size_t HttpSession::parseMessageSize(Buffer header) {
    const std::string_view haystack(header.ptr, header.size);
    size_t headerStart = haystack.find(ContentLengthHeader);
    if (headerStart == std::string_view::npos) {
        return 0;
    }

    char* endptr = nullptr;
    const char* str = header.ptr + headerStart + ContentLengthHeader.length();
    long val = strtol(str, &endptr, 10);
    if (endptr == str || *endptr != '\r' || val < 0) {
        // TODO: Invalid protocol. Kill session.
        return 0;
    }

    return val;
}

std::optional<RequestBase*> HttpSession::parseMessage(const InputMessagePtr&) {
    RequestBase* req = new RequestBase;
    return req;
}

const std::string& HttpSession::getSimpleHttpResponse() {
    return SimpleHttpResponse;
}

/***********************************************************
 *   Processor
 */
ProcessingStatus HttpProcessor::processRequest(SessionBase* session, RequestBase* request) {
    assert(session);
    assert(request);

    ResponseBase resp;
    return session->sendResponse(resp);
}

} // namespace bongo
