/**********************************************
   File:   mirror_test.h

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
#include "processor_base.h"
#include "thread_pool.h"

namespace bongo {

/***************************
 * Request / Response
 */
struct MirrorRequest : public RequestBase {
    std::string input;
};

struct MirrorResponse : public ResponseBase {
    std::string output;
};

/***************************
 * Session
 */
class MirrorSession : public SessionBase {
public:
    MirrorSession();
    ProcessingStatus sendResponse(const ResponseBase& response) override;
protected:
    size_t parseMessageSize(Buffer header) override;
    std::optional<RequestBase*> parseMessage(const InputMessagePtr& msg) override;
};

/***************************
 * Processor
 */
class MirrorProcessor : public ProcessorBase {
public:
    MirrorProcessor(SessionsQueue* sessionsQueue, ProcessorStats* stats = nullptr)
      : ProcessorBase(sessionsQueue, stats) {}

protected:
    ProcessingStatus processRequest(SessionBase* session, RequestBase* request) override;
};

/***************************
 * ThreadPool
 */
using MirrorSingleThreadPool = ThreadPool<MirrorProcessor, 1>;

} // namespace bongo
