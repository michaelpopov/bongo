/**********************************************
   File:   utest_full_cycle.cpp

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
#include "proc/processor_base.h"
#include "proc/notification_base.h"
#include "proc/thread_pool.h"
#include "session_demo.h"
#include "net/block_conn.h"
#include "utils/log.h"
#include "gtest/gtest.h"
#include <atomic>
#include <chrono>
#include <iostream>

using namespace bongo;

class Processor : public ProcessorBase {
public:
    Processor(SessionsQueue* queue, ProcessorStats* stats = nullptr) : ProcessorBase(queue, stats) {}

protected:
    ProcessingStatus processRequest(SessionBase* session, RequestBase* request) override {
        RequestDemo* req = dynamic_cast<RequestDemo*>(request);
        assert(req);

        // That's processing: just copy request command to response data.
        ResponseDemo resp;
        resp.data = req->command;

        assert(session);
        return session->sendResponse(resp);
    }
};

TEST(FULL_CYCLE, ProcessorsControl) {
    // TempLogLevel tll{"DEBUG"};

    const size_t COUNT = 4;
    SessionsQueue sessionsQueue;

    auto processorFunc = [](SessionsQueue* sessionsQueue) {
        Processor processor(sessionsQueue);
        processor.run();
    };

    std::vector<std::thread> workingThreads;
    for (size_t i = 0; i < COUNT; i++) {
        workingThreads.emplace_back(std::thread{processorFunc, &sessionsQueue});
    }

    sessionsQueue.shutdown();
    for (auto& t: workingThreads) {
        t.join();
    }
}

TEST(FULL_CYCLE, WorkingThreadsPool) {
    const size_t COUNT = 4;
    ThreadPool<Processor> pool(COUNT);
    pool.start();
    pool.stop();
}

TEST(FULL_CYCLE, RequestResponse) {
    // TempLogLevel tll{"DEBUG"};

    const std::string IP = "127.0.0.1";
    const int PORT = 8888;
    const size_t COUNT = 4;
    const size_t REQUEST_COUNT = 1; //10;

    ThreadPool<Processor> pool(COUNT);

    NonBlockNet net;
    int ret = net.init();

    SessionsQueue* sessionsQueue = pool.sessionsQueue();
    net.setSessionsQueue(sessionsQueue);

    std::thread t([&]() {
        NetOperation op {
            .name = "FullCycleTest",
            .ip = IP,
            .port = PORT,
            .factory = std::make_shared<ReqRespSessionFactory>(),
        };
        ret = net.startListen(op);
        ASSERT_EQ(0, ret);
        net.run(100);
    });

    net.waitListenerReady();
    ASSERT_GT(net.stats().listenersCount, 0);

    pool.start();

    BlockConnector connector(IP, PORT);
    ret = connector.init(); ASSERT_EQ(0, ret);
    auto conn_info = connector.make_connection(); ASSERT_TRUE(conn_info);
    BlockConnection conn(conn_info->fd);

    char buf[16];
    RequestDemo req;
    req.command = "hello";

    for (size_t i = 0; i < REQUEST_COUNT; i++) {
        uint32_t size = req.command.size();
        char* data = req.command.data();
        size_t fullSize = sizeof(size) + size;
        memcpy(buf, &size, sizeof(size));
        memcpy(buf+sizeof(size), data, size);

        //auto start_time = std::chrono::high_resolution_clock::now();
        ret = conn.writeAll(buf, fullSize); ASSERT_EQ(fullSize, ret);
        ret = conn.readAll(buf, fullSize); ASSERT_EQ(fullSize, ret);
        //auto end_time = std::chrono::high_resolution_clock::now();
        //auto duration = end_time - start_time;
        //auto us = std::chrono::duration_cast<std::chrono::microseconds>(duration);
        //std::cout << std::endl << "Duration microseconds: " << us << std::endl << std::endl;

        uint32_t checkSize;
        memcpy(&checkSize, buf, sizeof(size));
        ASSERT_EQ(size, checkSize);
        ASSERT_EQ(req.command, std::string(buf + sizeof(size), size));
    }

    ASSERT_EQ(REQUEST_COUNT, pool.stats().processedCount.load());
    pool.stop();

    net.stop();
    t.join();
}

// using namespace bongo;
