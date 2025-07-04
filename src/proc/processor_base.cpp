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
#include "message.h"
#include "utils/log.h"
#include "utils/pipe_queue.h"
#include <experimental/scope>

#include <assert.h>

namespace bongo {

void ProcessorBase::run() {
    while (auto optionalSession = _itemsQueue->pop()) {
        if (!optionalSession) {
            break;
        }

        LOG_TRACE << "ProcessorBase::run: pop Session";

        auto item = optionalSession.value();
        if (item == nullptr) {
            LOG_TRACE << "ProcessorBase::run: BAD Session";
            break;
        }
    
        processSession(item);
    }
}

void ProcessorBase::processSession(ProcessorItem* item) {
    MessageType resultType = MessageType::ItemReleased;
    ProcessingStatus status = ProcessingStatus::Failed;

    LOG_TRACE << "ProcessorBase::processSession: start loop";
    while (auto optionalRequest = item->getRequest()) {
        LOG_TRACE << "ProcessorBase::processSession: loop step";
        if (!optionalRequest) {
            break;
        }

        assert(optionalRequest.value());
        auto cleanup = std::experimental::scope_exit([&]() {
            delete optionalRequest.value();
        });

        LOG_TRACE << "ProcessorBase::processSession: pop Request";
        if (_stats) {
            _stats->processedCount++;
        }

        status = processRequest(item, optionalRequest.value());
        if (status != ProcessingStatus::Ok) {
            break;
        }
    }

    if (item->failed() || !item->hasRequest() || status != ProcessingStatus::Ok) {
        // Notify the network thread that item is released.
        MessageBase* msg = new MessageBase(resultType, item);
        PipeQueue::write(item->getPipe(), &msg);
    } else {
        // Return the item to the queue for further processing.
        _itemsQueue->push(item);
    }
}

} // namespace bongo
