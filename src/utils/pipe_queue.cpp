/**********************************************
   File:   pipe_queue.cpp

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

#include "pipe_queue.h"
#include "log.h"

PipeQueue::~PipeQueue() {
    for (int i = 0; i < 2; i++) {
        if (pipefd[i] != -1) {
            close(pipefd[i]);
        }
    }
}

PipeQueue::PipeResult PipeQueue::init() {
    int ret = pipe(pipefd);
    if (ret < 0) {
        LOG_ERROR << "PipeQueue::init: failed pipe(): " << strerror(errno);
        return PipeResult{ ret, errno };
    }

    return PipeResult{ 0, 0 };
}

PipeQueue::PipeResult PipeQueue::read(void*& ptr) {
    int ret = ::read(pipefd[0], &ptr, sizeof(ptr));
    if (ret != sizeof(ptr)) {
        return PipeResult{ -1, errno };
    }

    return PipeResult{ 0, 0 };
}

PipeQueue::PipeResult PipeQueue::write(void* ptr) {
    int ret = ::write(pipefd[1], ptr, sizeof(ptr));
    if (ret != sizeof(ptr)) {
        return PipeResult{ ret, errno };
    }

    return PipeResult{ 0, 0 };
}
