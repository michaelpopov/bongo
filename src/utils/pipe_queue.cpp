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
#include <assert.h>
#include <iostream>

size_t readPipeData(int fd, void** buf, size_t bufSize) {
    int ret = ::read(fd, buf, bufSize * sizeof(void*));
    if (ret < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR << "readPipeData: failed read(): " << strerror(errno);
        }
        return 0;
    }

    assert(ret % sizeof(void*) == 0);
    return ret / sizeof(void*);
}

PipeResult initPipeFds(int* fds) {
    int ret = pipe(fds);
    if (ret < 0) {
        LOG_ERROR << "initPipeFds: failed pipe(): " << strerror(errno);
        return PipeResult{ ret, errno };
    }

    return PipeResult{ 0, 0 };
}

void closePipeFds(int* fds) {
    for (int i = 0; i < 2; i++) {
        if (fds[i] != -1) {
            close(fds[i]);
        }
    }
}

PipeResult writePipeFd(int fd, void* ptr) {
    int ret = ::write(fd, ptr, sizeof(ptr));
    if (ret != sizeof(ptr)) {
        return PipeResult{ ret, errno };
    }

    return PipeResult{ 0, 0 };
}
