/**********************************************
   File:   block_conn.h

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

#pragma once
#include <string>
#include <optional>

struct sockaddr_in;
struct addrinfo;

namespace bongo {

static constexpr int DefaultBacklog = 5;
static constexpr size_t AddressIpLength = 32;

class BlockConnection;
class BlockConnector;
class BlockListener;

struct ConnectionInfo {
    int fd;
    std::string addr;
    int port;
};
using ConnectionInfoResult = std::optional<ConnectionInfo>;

/**************************************************************
 */
class BlockConnection {
public:
    struct Stats {
        size_t readSize = 0;
        size_t writeSize = 0;
        size_t failCount = 0;
    };

public:
    BlockConnection(int fd) : _fd(fd) {}
    ~BlockConnection();

    int readAll(char* buf, int size);
    int writeAll(char* buf, int size);

    int readSome(char* buf, int size);
    int writeSome(char* buf, int size);

    const Stats& stats() const { return _stats; }

private:
    int _fd;
    Stats _stats;
};

/**************************************************************
 */
class BlockConnector {
public:
    struct Stats {
        bool ready = false;
        char serverIp[AddressIpLength];
        size_t connectCount = 0;
        size_t failCount = 0;
    };

public:
    BlockConnector(const std::string& host, int port)
        : _host(host), _port(port)
    {}
    ~BlockConnector();

    int init();
    ConnectionInfoResult make_connection();

    const Stats& stats() const { return _stats; }
    void print_resolve() const;

private:
    const std::string _host;
    const int _port;
    struct addrinfo *_addr_info = nullptr;
    struct addrinfo *_addr = nullptr;
    Stats _stats;
};

/**************************************************************
 */
class BlockListener {
public:
    struct Stats {
        bool ready = false;
        char serverIp[AddressIpLength];
        size_t connectCount = 0;
        size_t failCount = 0;
    };

public:
    BlockListener(const std::string& iface, int port, int backlog = DefaultBacklog)
        : _iface(iface), _port(port), _backlog(backlog)
    {}

    ~BlockListener();

    int init();
    ConnectionInfoResult accept_connection();

    const Stats& stats() const { return _stats; }

private:
    const std::string _iface;
    const int _port;
    const int _backlog;
    int _fd = -1;
    Stats _stats;
};

} // namespace bongo
