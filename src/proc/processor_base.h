/**********************************************
   File:   processor_base.h

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
#pragma once
#include "session_base.h"
#include <atomic>

namespace bongo {

struct ProcessorStats {
    std::atomic<size_t> processedCount;
};

class ProcessorBase {
public:
    ProcessorBase(ItemsQueue* sessionsQueue, ProcessorStats* stats = nullptr)
      : _sessionsQueue(sessionsQueue), _stats(stats) {}

    void run();

protected:
    virtual ProcessingStatus processRequest(SessionBase* /*session*/, RequestBase* /*request*/) {
        return ProcessingStatus::Failed;
    }

private:
    ItemsQueue* _sessionsQueue;
    ProcessorStats* _stats;

private:
    void processSession(SessionBase* session);

};


} // namespace bongo
