/**********************************************
   File:   utest_block_conn.cpp

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

#include "block_conn.h"
#include "utils/log.h"
#include "gtest/gtest.h"

#include <thread>

using namespace bongo;

TEST(BLOCK_CONN, ListenerCtor) {
    //TempLogLevel tll{"DEBUG"};
    BlockListener listener("127.0.0.1", 8888);
    int ret = listener.init();
    ASSERT_EQ(0, ret);
    ASSERT_EQ(0, listener.stats().connectCount);
    ASSERT_EQ(0, listener.stats().failCount);
    ASSERT_TRUE(listener.stats().ready);
}

TEST(BLOCK_CONN, ListenerConnector) {
    const int port = 8888;
    const std::string ip = "127.0.0.1";
    int ret = -1;

    BlockListener listener(ip, port);
    ret = listener.init();
    ASSERT_EQ(0, ret);

    BlockConnector connector(ip, port);
    ret = connector.init();
    ASSERT_EQ(0, ret);
    ASSERT_TRUE(connector.stats().ready);
    //connector.print_resolve();

    auto connect = [&]() {
        auto conn = connector.make_connection();
        ASSERT_TRUE(conn);
    };
    std::thread t(connect);

    auto conn = listener.accept_connection();
    ASSERT_TRUE(conn);

    ASSERT_EQ(1, listener.stats().connectCount);
    ASSERT_EQ(1, connector.stats().connectCount);

    t.join();
}

TEST(BLOCK_CONN, Connection) {
    //TempLogLevel tll{"DEBUG"};

    const int port = 8888;
    const std::string ip = "127.0.0.1";
    int ret = -1;

    BlockListener listener(ip, port);
    ret = listener.init();
    ASSERT_EQ(0, ret);
    ASSERT_TRUE(listener.stats().ready);

    BlockConnector connector(ip, port);
    ret = connector.init();
    ASSERT_EQ(0, ret);
    ASSERT_TRUE(connector.stats().ready);

    const std::array<uint32_t, 3> data = { 42, 43, 44 };

    auto connect = [&]() {
        auto conn_info = connector.make_connection();
        ASSERT_TRUE(conn_info);
        BlockConnection writer(conn_info->fd);
        for (const auto val: data) {
            int ret = writer.writeAll((char*)&val, sizeof(val));
            ASSERT_EQ(sizeof(val), ret);
        }
    };
    std::thread t(connect);

    auto conn_info = listener.accept_connection();
    ASSERT_TRUE(conn_info);

    ASSERT_EQ(1, listener.stats().connectCount);
    ASSERT_EQ(1, connector.stats().connectCount);

    BlockConnection reader(conn_info->fd);
    for (const auto val: data) {
        uint32_t num;
        int ret = reader.readAll((char*)&num, sizeof(num));
        ASSERT_EQ(sizeof(num), ret);
        ASSERT_EQ(val, num);
    }

    t.join();
}




