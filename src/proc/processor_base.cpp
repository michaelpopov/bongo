/**********************************************
   File:   processor_base.cpp

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
#include "processor_base.h"
#include "notification_base.h"
#include "utils/log.h"
#include "utils/pipe_queue.h"
#include <experimental/scope>
#include <assert.h>

namespace bongo {

void ProcessorBase::run() {
    while (auto optionalSession = _sessionsQueue->pop()) {
        if (!optionalSession) {
            break;
        }

        auto session = optionalSession.value();
        if (session == nullptr) {
            LOG_ERROR << "ProcessorBase::run: BAD Session";
            break;
        }
    
        processSession(session);
    }
}

void ProcessorBase::processSession(SessionBase* session) {
    NotificationType resultType = NotificationType::SessionReleased;
    ProcessingStatus status = ProcessingStatus::Failed;

    while (auto optionalRequest = session->getRequest()) {
        if (!optionalRequest) {
            break;
        }

        assert(optionalRequest.value());
        auto cleanup = std::experimental::scope_exit([&]() {
            delete optionalRequest.value();
        });

        if (_stats) {
            _stats->processedCount++;
        }

        status = processRequest(session, optionalRequest.value());
        if (status != ProcessingStatus::Ok) {
            break;
        }
    }

    if (session->failed() || !session->hasRequest() || status != ProcessingStatus::Ok) {
        // Notify the network thread that session is released.
        NotificationBase* msg = new NotificationBase(resultType, session);
        writePipeFd(session->getPipe(), &msg);
    } else {
        // Return the session to the queue for further processing.
        _sessionsQueue->push(session);
    }
}

} // namespace bongo
