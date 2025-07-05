/**********************************************
   File:   thread_pool.h

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
#include "processor_base.h"
#include <thread>
#include <vector>

namespace bongo {

template <typename ProcessorType, size_t Size = 0>
class ThreadPool {
public:
    ThreadPool(size_t size = Size) : _size(size) {
        if (_size == 0) {
            _size = std::thread::hardware_concurrency();
        }
    }

    ~ThreadPool() {
        stop();
    }

    void start() {
        auto processorFunc = [](SessionsQueue* sessionsQueue, ProcessorStats* stats) {
            ProcessorType processor(sessionsQueue, stats);
            processor.run();
        };

        for (size_t i = 0; i < _size; i++) {
            _threads.emplace_back(std::thread{processorFunc, &_sessionsQueue, &_stats});
        }
    }

    void stop() {
        _sessionsQueue.shutdown();
        for (auto& t: _threads) {
            t.join();
        }
        _threads.clear();
    }

    SessionsQueue* sessionsQueue() { return &_sessionsQueue; }
    const ProcessorStats& stats() const { return _stats; }

private:
    size_t _size;
    std::vector<std::thread> _threads;
    SessionsQueue _sessionsQueue;
    ProcessorStats _stats;
};

} // namespace bongo
