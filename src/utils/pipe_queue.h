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

class PipeQueue {
public:
    using PipeResult = std::pair<int, int>; // ret, errno

    ~PipeQueue();

    PipeResult init();
    int readFd() const { return pipefd[0]; }
    int writeFd() const { return pipefd[1]; }

    PipeResult read(void*& ptr);
    PipeResult write(void* ptr);

    static PipeResult write(int fd, void* ptr);

private:
    int pipefd[2] = { -1, -1 };
};
