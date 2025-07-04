/**********************************************
   File:   utest_session.cpp

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
#include "mirror_test.h"
#include "message.h"
#include "utils/pipe_queue.h"
#include "utils/log.h"
#include <experimental/scope>
#include "gtest/gtest.h"
#include <assert.h>

using namespace bongo;

TEST(SESSION, MirrorBasic) {
    TempLogLevel tll{"DEBUG"};

    MirrorSession* session = nullptr;
    auto cleanupSession = std::experimental::scope_exit([&]() { delete session; });

    PipeQueue pipeQueue;
    auto pipeQueueRet = pipeQueue.init();
    ASSERT_EQ(0, pipeQueueRet.first);

    MirrorSingleThreadPool pool;
    pool.start();
    SessionsQueue* sessionsQueue = pool.sessionsQueue();

    session = new MirrorSession;
    session->setPipe(pipeQueue.writeFd());
    ASSERT_EQ(SessionState::Released, session->state());

    std::vector<std::string> inputs = {
        "Hello, world!",
        "",
    };

    for (const auto& inputStr: inputs) {
        MessageBase* msg = nullptr;
        auto cleanupMsg = std::experimental::scope_exit([&]() { delete msg; });

        const uint32_t len = inputStr.length();
        const size_t dataSize = sizeof(uint32_t) + len;
        Buffer readBuffer = session->getReadBuffer(dataSize);
        memcpy(readBuffer.ptr, &len, sizeof(len));
        memcpy(readBuffer.ptr + sizeof(len), inputStr.data(), len);
        session->updateReadBuffer(dataSize);

        session->onRead(sessionsQueue);
        ASSERT_EQ(SessionState::InProcessing, session->state());

        void* ptr = nullptr;
        pipeQueue.read(ptr);
        msg = static_cast<MessageBase*>(ptr);
        ASSERT_NE(nullptr, msg);
        ASSERT_EQ(MessageType::SessionReleased, msg->type());
        ASSERT_EQ(session, msg->session());

        session->setState(SessionState::Released);

        Buffer writeBuffer = session->getDataForWriting();
        uint32_t checkLength = 0;
        memcpy(&checkLength, writeBuffer.ptr, sizeof(checkLength));
        std::string outputStr(writeBuffer.ptr + sizeof(checkLength), writeBuffer.size - sizeof(checkLength));
        ASSERT_EQ(len, checkLength);
        ASSERT_EQ(inputStr, outputStr);
        session->completedWriting(writeBuffer.size);
    }
}

TEST(SESSION, MirrorMultiRequest) {
    TempLogLevel tll{"DEBUG"};

    MessageBase* msg = nullptr;
    auto cleanupMsg = std::experimental::scope_exit([&]() { delete msg; });
    MirrorSession* session = nullptr;
    auto cleanupSession = std::experimental::scope_exit([&]() { delete session; });

    PipeQueue pipeQueue;
    auto pipeQueueRet = pipeQueue.init();
    ASSERT_EQ(0, pipeQueueRet.first);

    MirrorSingleThreadPool pool;
    pool.start();
    SessionsQueue* sessionsQueue = pool.sessionsQueue();

    session = new MirrorSession;
    session->setPipe(pipeQueue.writeFd());
    ASSERT_EQ(SessionState::Released, session->state());

    std::vector<std::string> inputs = {
        "Hello, world!",
        "",
    };

    for (const auto& inputStr: inputs) {
        const uint32_t len = inputStr.length();
        const size_t dataSize = sizeof(uint32_t) + len;
        Buffer readBuffer = session->getReadBuffer(dataSize);
        memcpy(readBuffer.ptr, &len, sizeof(len));
        memcpy(readBuffer.ptr + sizeof(len), inputStr.data(), len);
        session->updateReadBuffer(dataSize);
    }

    session->onRead(sessionsQueue);
    ASSERT_EQ(SessionState::InProcessing, session->state());

    void* ptr = nullptr;
    pipeQueue.read(ptr);
    msg = static_cast<MessageBase*>(ptr);
    ASSERT_NE(nullptr, msg);
    ASSERT_EQ(MessageType::SessionReleased, msg->type());
    ASSERT_EQ(session, msg->session());

    session->setState(SessionState::Released);

    Buffer writeBuffer = session->getDataForWriting();
    size_t offset = 0;
    for (const auto& inputStr: inputs) {
        const uint32_t len = inputStr.length();

        uint32_t checkLength = 0;
        memcpy(&checkLength, writeBuffer.ptr + offset, sizeof(checkLength));
        ASSERT_EQ(len, checkLength);

        size_t dataOffset = offset + sizeof(checkLength);
        std::string outputStr(writeBuffer.ptr + dataOffset, checkLength);
        ASSERT_EQ(inputStr, outputStr);

        offset += sizeof(checkLength) + checkLength;
    }

    session->completedWriting(writeBuffer.size);
}