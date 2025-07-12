/**********************************************
   File:   utest_http.cpp

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
#include "http_test.h"
#include "notification_base.h"
#include "utils/pipe_queue.h"
#include "utils/log.h"
#include <experimental/scope>
#include "gtest/gtest.h"
#include <assert.h>

using namespace bongo;

TEST(SESSION, HttpBasic) {
    TempLogLevel tll{"DEBUG"};

    HttpSession* session = nullptr;
    auto cleanupSession = std::experimental::scope_exit([&]() { delete session; });

    NotificationQueue pipeQueue;
    auto pipeQueueRet = pipeQueue.init();
    ASSERT_EQ(0, pipeQueueRet.first);

    HttpSingleThreadPool pool;
    pool.start();
    SessionsQueue* sessionsQueue = pool.sessionsQueue();

    session = new HttpSession;
    session->setPipe(pipeQueue.getWriteFd());
    ASSERT_EQ(SessionState::Released, session->state());

    const std::vector<std::string> inputs = {
        "GET /index.html HTTP/1.1\r\n\r\n",
        "GET /index.html HTTP/1.1\r\nContent-Length: 13\r\n\r\nHello, world!",
    };

    const std::string& output = HttpSession::getSimpleHttpResponse();

    for (const auto& inputStr: inputs) {
        NotificationBase* msg = nullptr;
        auto cleanupMsg = std::experimental::scope_exit([&]() { delete msg; });

        Buffer readBuffer = session->getReadBuffer(inputStr.length());
        memcpy(readBuffer.ptr, inputStr.data(), inputStr.length());
        session->updateReadBuffer(inputStr.length());

        session->onRead(sessionsQueue);
        ASSERT_EQ(SessionState::InProcessing, session->state());

        msg = pipeQueue.next();
        ASSERT_NE(nullptr, msg);
        ASSERT_EQ(NotificationType::SessionReleased, msg->type());
        ASSERT_EQ(session, msg->session());

        session->setState(SessionState::Released);

        Buffer writeBuffer = session->getDataForWriting();
        ASSERT_EQ(output.length(), writeBuffer.size);
        ASSERT_EQ(0, memcmp(output.data(), writeBuffer.ptr, writeBuffer.size));
        session->completedWriting(writeBuffer.size);
    }
}

TEST(SESSION, HttpMultiRequest) {
    TempLogLevel tll{"DEBUG"};

    NotificationBase* msg = nullptr;
    auto cleanupMsg = std::experimental::scope_exit([&]() { delete msg; });
    HttpSession* session = nullptr;
    auto cleanupSession = std::experimental::scope_exit([&]() { delete session; });

    NotificationQueue pipeQueue;
    auto pipeQueueRet = pipeQueue.init();
    ASSERT_EQ(0, pipeQueueRet.first);

    HttpSingleThreadPool pool;
    pool.start();
    SessionsQueue* sessionsQueue = pool.sessionsQueue();

    session = new HttpSession;
    session->setPipe(pipeQueue.getWriteFd());
    ASSERT_EQ(SessionState::Released, session->state());

    const std::vector<std::string> inputs = {
        "GET /index.html HTTP/1.1\r\n\r\n",
        "GET /index.html HTTP/1.1\r\nContent-Length: 13\r\n\r\nHello, world!",
    };

    const std::string& output = HttpSession::getSimpleHttpResponse();

    for (const auto& inputStr: inputs) {
        Buffer readBuffer = session->getReadBuffer(inputStr.length());
        memcpy(readBuffer.ptr, inputStr.data(), inputStr.length());
        session->updateReadBuffer(inputStr.length());
    }

    session->onRead(sessionsQueue);
    ASSERT_EQ(SessionState::InProcessing, session->state());

    msg = pipeQueue.next();
    ASSERT_NE(nullptr, msg);
    ASSERT_EQ(NotificationType::SessionReleased, msg->type());
    ASSERT_EQ(session, msg->session());

    session->setState(SessionState::Released);

    Buffer writeBuffer = session->getDataForWriting();
    ASSERT_EQ(2 * output.length(), writeBuffer.size);

    size_t offset = 0;
    for (size_t i = 0; i < inputs.size(); i++) {
        ASSERT_EQ(0, memcmp(output.data(), writeBuffer.ptr + offset, output.length()));
        offset += output.length();
    }

    session->completedWriting(writeBuffer.size);
}
