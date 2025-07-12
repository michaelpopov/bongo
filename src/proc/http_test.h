/**********************************************
   File:   http_test.h

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
#include "notification_base.h"
#include "utils/pipe_queue.h"
#include "thread_pool.h"
#include <string_view>

namespace bongo {

/***************************
 * Session
 */
class HttpSession : public SessionBase {
public:
    HttpSession();
    static const std::string& getSimpleHttpResponse();

protected:
    ProcessingStatus sendResponse(const ResponseBase& response) override;
    size_t parseMessageSize(Buffer header) override;
    std::optional<RequestBase*> parseMessage(const InputMessagePtr& msg) override;
};

/***************************
 * Processor
 */
class HttpProcessor : public ProcessorBase {
public:
    HttpProcessor(SessionsQueue* sessionsQueue, ProcessorStats* stats = nullptr)
      : ProcessorBase(sessionsQueue, stats) {}

protected:
    ProcessingStatus processRequest(SessionBase* session, RequestBase* request) override;
};

/***************************
 * ThreadPool
 */
using HttpSingleThreadPool = ThreadPool<HttpProcessor, 1>;

} // namespace bongo
