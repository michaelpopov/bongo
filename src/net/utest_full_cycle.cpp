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

#include "session_demo.h"
#include "message.h"
#include "block_conn.h"
#include "utils/log.h"
#include "gtest/gtest.h"
#include <atomic>
#include <chrono>
#include <iostream>

using namespace bongo;

struct ProcessorStats {
    std::atomic<size_t> processedCount;
};

class Processor {
public:
    Processor(SessionsQueue* sessionsQueue, ProcessorStats* stats = nullptr)
      : _sessionsQueue(sessionsQueue), _stats(stats) {}

    void run() {
        while (auto optionalSession = _sessionsQueue->pop()) {
            if (!optionalSession) {
                break;
            }

            LOG_TRACE << "Processor::run: pop Session";

            ReqRespSession* session = dynamic_cast<ReqRespSession*>(optionalSession.value());
            if (session == nullptr) {
                LOG_TRACE << "Processor::run: BAD Session";
            }
            assert(session);
            processSession(session);
        }
    }

private:
    SessionsQueue* _sessionsQueue;
    ProcessorStats* _stats;

private:
    void processSession(ReqRespSession* session) {
        MessageType resultType = MessageType::SessionReleased;

        LOG_TRACE << "Processor::processSession: start loop";
        while (auto optionalRequest = session->getRequest()) {
            LOG_TRACE << "Processor::processSession: loop step";
            if (!optionalRequest) {
                break;
            }

            LOG_TRACE << "Processor::processSession: pop Request";
            if (_stats) {
                _stats->processedCount++;
            }

            Request& req = optionalRequest.value();

            Response resp { .data = req.command };
            int ret = session->sendResponse(resp);
            if (ret != 0) {
                // Session couldn't send all response data,
                // let's break processing of the session for now
                resultType = MessageType::MoreData;
                break;
            }
        }

        MessageBase* msg = new MessageBase(resultType, session);
        auto pipeQueue = session->getPipeQueue();
        pipeQueue->write(&msg);
    }
};

template <typename ProcessorType>
class WorkingThreadsPool {
public:
    WorkingThreadsPool(size_t size) : _size(size) {}
    ~WorkingThreadsPool() {
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
    WorkingThreadsPool<Processor> pool(COUNT);
    pool.start();
    pool.stop();
}

TEST(FULL_CYCLE, RequestResponse) {
    //TempLogLevel tll{"DEBUG"};

    const std::string IP = "127.0.0.1";
    const int PORT = 8888;
    const size_t COUNT = 4;
    const size_t REQUEST_COUNT = 10;

    WorkingThreadsPool<Processor> pool(COUNT);

    NonBlockNet net;
    int ret = net.init();

    std::thread t([&]() {
        NetOperation op {
            .name = "FullCycleTest",
            .ip = IP,
            .port = PORT,
            .factory = std::make_shared<ReqRespSessionFactory>(net.getPipeQueue(), pool.sessionsQueue()),
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

    for (size_t i = 0; i < REQUEST_COUNT; i++) {
        char buf[16];
        Request req { .command = "hello" };
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
