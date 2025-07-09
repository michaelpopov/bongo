/**********************************************
   File:   utest_pipe_queue.cpp

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
#include "gtest/gtest.h"

struct TestData {
    int aaa;
    int bbb;
};

using TestPipeQueue = PipeQueue<TestData>;

TEST(UTILS, PipeQueueBasic) {
    TestPipeQueue pipeQueue;
    const auto res = pipeQueue.init();
    ASSERT_EQ(0, res.first);
    ASSERT_EQ(0, res.second);
}

TEST(UTILS, PipeQueueDataTransfer) {
    TestPipeQueue pipeQueue;
    auto res = pipeQueue.init();
    ASSERT_EQ(0, res.first);
    ASSERT_EQ(0, res.second);

    int fd = pipeQueue.getWriteFd();
    ASSERT_NE(-1, fd);

    std::vector<TestData> data = {
        { .aaa = 1, .bbb = 2 },
        { .aaa = 3, .bbb = 4 },
        { .aaa = 5, .bbb = 6 },
        { .aaa = 7, .bbb = 8 },
    };

    for (size_t k = 0; k < 10; k++) {
      for (size_t i = 0; i < data.size(); i++) {
          void* ptr = &data[i];
          res = writePipeFd(fd, &ptr);
          ASSERT_EQ(0, res.first);
          ASSERT_EQ(0, res.second);
      }
      
      for (size_t i = 0; i < data.size() / 2; i++) {
          for (size_t j = 0; j < data.size() / 2; j++) {
              TestData* ptr = pipeQueue.next();
              size_t index = i * 2 + j;
              ASSERT_EQ(&data[index], ptr);
          }
      }
    }
}