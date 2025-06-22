/**********************************************
   File:   utest_config.cpp

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

#include "config.h"
#include "gtest/gtest.h"

using namespace passio::a1::client;

TEST(CLIENT_CONFIG, Ctor) {
    Config config;
    ASSERT_EQ(-1, config.port());
    ASSERT_EQ('\0', *config.host());
}

TEST(CLIENT_CONFIG, Short) {
    Config config;
    const char* argv[] = {
        "client",
        "-h", "localhost",
        "-p", "12345",
    };
    int argc = sizeof(argv) / sizeof(*argv);
    int ret = config.init(argc, argv);
    ASSERT_EQ(0, ret);

    ASSERT_EQ(12345, config.port());
    ASSERT_EQ(std::string("localhost"), std::string(config.host()));
}

TEST(CLIENT_CONFIG, Long) {
    Config config;
    const char* argv[] = {
        "client",
        "--host", "localhost",
        "--port", "12345",
    };
    int argc = sizeof(argv) / sizeof(*argv);
    int ret = config.init(argc, argv);
    ASSERT_EQ(0, ret);

    ASSERT_TRUE(config.valid());

    ASSERT_EQ(12345, config.port());
    ASSERT_EQ(std::string("localhost"), std::string(config.host()));
}

TEST(CLIENT_CONFIG, LongAssign) {
    Config config;
    const char* argv[] = {
        "client",
        "--host=localhost",
        "--port=12345",
    };
    int argc = sizeof(argv) / sizeof(*argv);
    int ret = config.init(argc, argv);
    ASSERT_EQ(0, ret);

    ASSERT_TRUE(config.valid());

    ASSERT_EQ(12345, config.port());
    ASSERT_EQ(std::string("localhost"), std::string(config.host()));
}
