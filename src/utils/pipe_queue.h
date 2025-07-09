/**********************************************
   File:   pipe_queue.h

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
#include <utility>
#include <vector>

using PipeResult = std::pair<int, int>; // ret, errno

size_t readPipeData(int fd, void** buf, size_t bufSize);
PipeResult initPipeFds(int* fds);
void closePipeFds(int* fds);
PipeResult writePipeFd(int fd, void* ptr);

template <typename T, size_t Size=128>
class PipeQueue {
public:
    PipeQueue(size_t size = 0) {
        if (size == 0) {
            size = Size;
        }
        _data.resize(size);
    }

    ~PipeQueue() {
        closePipeFds(_pipefd);
    }

    PipeResult init() {
        return initPipeFds(_pipefd);
    }

    T* next() {
        if (_pos == _count) {
            _pos = 0;
            _count = readPipeData(_pipefd[0], _data.data(), _data.size());
        }

        if (_pos == _count) {
            return nullptr;
        }

        return (T*)_data[_pos++];
    }

    int getReadFd() const { return _pipefd[0]; }
    int getWriteFd() const { return _pipefd[1]; }

private:
    size_t _pos = 0;
    size_t _count = 0;
    std::vector<void*> _data;
    int _pipefd[2] = { -1, -1 };
};
