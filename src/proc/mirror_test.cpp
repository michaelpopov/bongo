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

namespace bongo {

/***********************************************************
 *   Session
 */
MirrorSession::MirrorSession() {
    _headerSize = 4;
}

ProcessingStatus MirrorSession::sendResponse(const ResponseBase& response) {
    const MirrorResponse& resp = dynamic_cast<const MirrorResponse&>(response);

    uint32_t len = resp.output.length();
    size_t serializedSize = sizeof(len) + resp.output.length();
    Buffer dest = _writeBuf.getAvailable(serializedSize);

    memcpy(dest.ptr, &len, sizeof(len));
    memcpy(dest.ptr + sizeof(len), resp.output.data(), len);

    _writeBuf.update(serializedSize);
    return ProcessingStatus::Ok;
}

size_t MirrorSession::parseMessageSize(Buffer header) { 
    uint32_t size = -1;
    assert(header.size >= sizeof(size));
    memcpy(&size, header.ptr, sizeof(size));
    return size;
}

std::optional<RequestBase*> MirrorSession::parseMessage(const InputMessagePtr& msg) {
    MirrorRequest* req = new MirrorRequest;
    req->input.assign(msg->body.data(), msg->body.size());
    return req;
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
