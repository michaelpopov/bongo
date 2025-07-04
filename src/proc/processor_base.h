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
#include "processor_item.h"
#include "utils/thread_queue.h"
#include <atomic>

namespace bongo {

using ItemsQueue = ThreadQueue<ProcessorItem*>;

struct ProcessorStats {
    std::atomic<size_t> processedCount;
};

class ProcessorBase {
public:
    ProcessorBase(ItemsQueue* itemsQueue, ProcessorStats* stats = nullptr)
      : _itemsQueue(itemsQueue), _stats(stats) {}

    void run();

protected:
    virtual ProcessingStatus processRequest(ProcessorItem* /*item*/, RequestBase* /*request*/) {
        return ProcessingStatus::Failed;
    }

private:
    ItemsQueue* _itemsQueue;
    ProcessorStats* _stats;

private:
    void processSession(ProcessorItem* item);

};


} // namespace bongo
