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
    ASSERT_EQ(0, net.sessionsCount());
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
