/**********************************************
   File:   thread_queue.h

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
#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <optional>

template<typename T>
class ThreadQueue {
public:
    ThreadQueue() : _done(false) {}

    void push(T value) {
        std::unique_lock<std::mutex> lock(_mtx);
        _queue.push(std::move(value));
        _cv.notify_one();
    }

    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(_mtx);
        _cv.wait(lock, [&] { return !_queue.empty() || _done; });

        if (_queue.empty())
            return std::nullopt;

        T value = std::move(_queue.front());
        _queue.pop();
        return value;
    }

    void shutdown() {
        std::unique_lock<std::mutex> lock(_mtx);
        _done = true;
        _cv.notify_all();
    }

private:
    std::queue<T> _queue;
    std::mutex _mtx;
    std::condition_variable _cv;
    bool _done;
};
