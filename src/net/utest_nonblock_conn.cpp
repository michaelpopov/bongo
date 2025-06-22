/**********************************************
   File:   utest_nonblock_conn.cpp

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
#include "nonblock_conn.h"
#include "block_conn.h"
#include "utils/log.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <chrono>
#include <thread>

using namespace bongo;

TEST(NONBLOCK_CONN, CtorDtor) {
    NonBlockNet net;
    ASSERT_EQ(0, net.itemsCount());
    auto s = net.stats();
    ASSERT_FALSE(s.ready);
    ASSERT_EQ(0, s.acceptedCount);
    ASSERT_EQ(0, s.connectedCount);
    ASSERT_EQ(0, s.count());
}

TEST(NONBLOCK_CONN, NetInit) {
    NonBlockNet net;
    int ret = net.init();
    ASSERT_EQ(0, ret);
    auto s = net.stats();
    ASSERT_TRUE(s.ready);
    ASSERT_EQ(1, s.pipesCount);
}

TEST(NONBLOCK_CONN, ListenerBasic) {
    NonBlockNet net;
    int ret = net.init();

    NetOperation op {
        .name = "ListenerTest",
        .ip = "127.0.0.1",
        .port = 8888,
        .factory = nullptr,
    };
    ret = net.startListen(op);
    ASSERT_EQ(0, ret);
    auto s = net.stats();
    ASSERT_EQ(2, s.count());
}

TEST(NONBLOCK_CONN, ListenerBlockConnect) {
    // TempLogLevel tll{"DEBUG"};

    const std::string IP = "127.0.0.1";
    const int PORT = 8888;

    NonBlockNet net;
    int ret = net.init();

    std::thread t([&]() {
        NetOperation op { .name = "ListenTest", .ip = IP, .port = PORT, .factory = std::make_shared<EchoNetSessionFactory>() };
        ret = net.startListen(op);
        ASSERT_EQ(0, ret);
        net.run(100);
    });

    for (int i=0; i < 10; i++) {
        if (net.stats().listenersCount > 0) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    ASSERT_GT(net.stats().listenersCount, 0);

    auto foo = [&]() {
        BlockConnector connector(IP, PORT);
        ret = connector.init();
        ASSERT_EQ(0, ret);
        auto conn_info = connector.make_connection();
        ASSERT_TRUE(conn_info);

        const size_t COUNT = 16 * 1024;
        std::vector<uint64_t> data(COUNT);
        for (size_t i=0; i < COUNT; i++) {
            data[i] = i;
        }

        BlockConnection conn(conn_info->fd);

        std::thread t2([&]() {
            for (const auto val: data) {
                int ret = conn.writeAll((char*)&val, sizeof(val));
                ASSERT_EQ(sizeof(val), ret);
                // std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        });

        for (const auto val: data) {
            uint64_t num = 0;
            ret = conn.readAll((char*)&num, sizeof(num));
            ASSERT_EQ(sizeof(val), ret);
            ASSERT_EQ(val, num);
        }

        t2.join();
    };

    const size_t WORKERS_COUNT = 3;
    std::vector<std::thread> workers;
    for (size_t j = 0; j < WORKERS_COUNT; j++) {
        workers.emplace_back(std::thread(foo));
    }

    for (auto& w: workers) {
        w.join();
    }

    for (int i=0; i < 10; i++) {
        if (net.stats().connectionsCount > 0) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    ASSERT_GT(net.stats().connectionsCount, 0);

    net.stop();
    t.join();
}

TEST(NONBLOCK_CONN, ListenerBlockConnectBig) {
    // TempLogLevel tll{"DEBUG"};

    const std::string IP = "127.0.0.1";
    const int PORT = 8888;

    NonBlockNet net;
    int ret = net.init();

    std::thread t([&]() {
        const size_t SIZE = 8 * 1024 * 1024;
        NetOperation op { .name = "ListenTest", .ip = IP, .port = PORT, .factory = std::make_shared<EchoNetSessionFactory>(SIZE) };
        ret = net.startListen(op);
        ASSERT_EQ(0, ret);
        net.run(100);
    });

    for (int i=0; i < 10; i++) {
        if (net.stats().listenersCount > 0) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    ASSERT_GT(net.stats().listenersCount, 0);

    auto foo = [&]() {
        BlockConnector connector(IP, PORT);
        ret = connector.init();
        ASSERT_EQ(0, ret);
        auto conn_info = connector.make_connection();
        ASSERT_TRUE(conn_info);

        const size_t COUNT = 16 * 1024 * 1024;
        std::vector<uint64_t> data(COUNT);
        for (size_t i=0; i < COUNT; i++) {
            data[i] = i;
        }

        BlockConnection conn(conn_info->fd);

        std::thread t2([&]() {
            char* ptr = reinterpret_cast<char*>(data.data());
            size_t sz = data.size() * sizeof(data[0]);
            int ret = conn.writeAll(ptr, sz);
            ASSERT_EQ(sz, ret);
        });

        std::vector<uint64_t> check(COUNT);
        char* ptr = reinterpret_cast<char*>(check.data());
        size_t sz = check.size() * sizeof(check[0]);
        int ret = conn.readAll(ptr, sz);
        ASSERT_EQ(sz, ret);

        for (size_t i = 0; i < COUNT; i++) {
            ASSERT_EQ(data[i], check[i]);
        }

        t2.join();
    };

    const size_t WORKERS_COUNT = 1;
    std::vector<std::thread> workers;
    for (size_t j = 0; j < WORKERS_COUNT; j++) {
        workers.emplace_back(std::thread(foo));
    }

    for (auto& w: workers) {
        w.join();
    }

    for (int i=0; i < 10; i++) {
        if (net.stats().connectionsCount > 0) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    ASSERT_GT(net.stats().connectionsCount, 0);

    net.stop();
    t.join();
}

TEST(NONBLOCK_CONN, ConnectBlockListener) {
    // TempLogLevel tll{"DEBUG"};

    const std::string IP = "127.0.0.1";
    const int PORT = 8888;

    NonBlockNet net;
    int ret = net.init();
    ASSERT_EQ(0, ret);

    std::thread block_thread([&]() {
        LOG_TRACE << "Start listener";
        BlockListener listener(IP, PORT);
        int ret = listener.init();
        ASSERT_EQ(0, ret);

        LOG_TRACE << "Accept connection";
        auto conn_info = listener.accept_connection();
        ASSERT_TRUE(conn_info);

        LOG_TRACE << "Connection ready";
        BlockConnection conn(conn_info->fd);

        LOG_TRACE << "Read size";
        size_t size = 0;
        ret = conn.readAll((char*)&size, sizeof(size));
        ASSERT_EQ(sizeof(size), ret);

        LOG_TRACE << "Going to read: " << size;
    
        // size_t n = 0;
        size_t received_size = 0;
        std::vector<char> buffer(16 * 1024 * 1024);
        //char ch = 'A';
    
        while (received_size < size) {
            // TempLogLevel tll{"ERROR"};

            size_t sz = std::min(buffer.size(), size - received_size);
            // LOG_TRACE << "Receiving: " << sz << "   " << n++;
            int ret = conn.readAll(buffer.data(), sz);
            ASSERT_EQ(ret, sz);

            /*for (size_t i = 0; i < sz; i++, ch++) {
                if (ch > 'Z') {
                    ch = 'A';
                }
                ASSERT_EQ(ch, buffer[i]);
            }*/

            received_size += ret;

            LOG_TRACE << "Received so far: " << received_size;
        }

        LOG_TRACE << "Write size: " << size;
        ret = conn.writeAll((char*)&size, sizeof(size));
        ASSERT_EQ(sizeof(size), ret);

        LOG_TRACE << "Blocking thread finished";
    });

    // Let accept() start on the block_thread.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::thread t([&]() {
        NetOperation op { .name = "ConnectTest", .ip = IP, .port = PORT, .factory = std::make_shared<BigWriterNetSessionFactory>() };
        ret = net.startConnect(op);
        ASSERT_EQ(0, ret);
        net.run(100);
    });

    block_thread.join();
    net.stop();
    t.join();
}
