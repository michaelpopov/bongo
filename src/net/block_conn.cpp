/**********************************************
   File:   block_conn.cpp

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

#include <experimental/scope>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>

namespace bongo {

/**************************************************************
 */
BlockListener::~BlockListener() {
    if (_fd >= 0) {
        close(_fd);
    }
}

int BlockListener::init() {
    _fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_fd < 0) {
        LOG_ERROR << "Failed to create a listener socket: " << strerror(errno);
        return -1;
    }

    int opt = 1;
    if (0 != setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        LOG_ERROR << "Failed to set listener socket reusable";
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(_port);    
    if (0 != inet_pton(AF_INET, _iface.c_str(), &server_addr.sin_addr)) {
        strncpy(_stats.serverIp, _iface.c_str(), sizeof(_stats.serverIp)-1);
    } else {
        return -1;
    }

    if (0 != bind(_fd, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
        LOG_ERROR << "Failed to bind a socket: " << strerror(errno);
        return -1;
    }

    if (0 != listen(_fd, _backlog)) {
        LOG_ERROR << "Failed to listen on a socket: " << strerror(errno);
        return -1;
    }

    _stats.ready = true;

    return 0;
}

ConnectionInfoResult BlockListener::accept_connection() {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char client_ip[INET_ADDRSTRLEN];

    int client_fd = accept(_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_fd == -1) {
        LOG_ERROR << "Failed to accept connection: " << strerror(errno);
        _stats.failCount++;
        return {};
    }

    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    _stats.connectCount++;

    return ConnectionInfo { client_fd, client_ip, client_addr.sin_port };
}

/**************************************************************
 */
BlockConnector::~BlockConnector() {
    if (_addr_info != nullptr) {
        freeaddrinfo(_addr_info);
    }
}

int BlockConnector::init() {
    char port_str[32];
    int n = snprintf(port_str, sizeof(port_str), "%d", _port);
    if (n < 1) {
        LOG_ERROR << "BlockConnector::init: Failed to convert port to string: " << _port;
        return -1;
    }

    struct addrinfo hints;
    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(_host.c_str(), port_str, &hints, &_addr_info);
    if (status != 0) {
        LOG_ERROR << "Failed to resolve host: " << gai_strerror(status);
        return -1;
    }

    _addr = _addr_info;
    _stats.ready = true;
    return 0;
}

void BlockConnector::print_resolve() const {
    std::cout << "Resolved for " << _host << ":" << _port << std::endl;
    int n = 1;
    for (struct addrinfo *p = _addr_info; p != NULL; p = p->ai_next, n++) {
        struct sockaddr_in* addr_in = (struct sockaddr_in*)p->ai_addr;
        char serverIp[INET_ADDRSTRLEN];
        inet_ntop(p->ai_family, &addr_in->sin_addr, serverIp, sizeof(serverIp));

        std::cout << "\t" << n << ") " << serverIp << std::endl;
    }

    std::cout << std::endl;
}

ConnectionInfoResult BlockConnector::make_connection() {
    int fd = -1;

    if (-1 == (fd = socket(_addr->ai_family, _addr->ai_socktype, _addr->ai_protocol))) {
        LOG_ERROR << "Failed to create a socket: " << strerror(errno);
        _stats.failCount++;
        return {};
    }

    if (0 != connect(fd, _addr->ai_addr, _addr->ai_addrlen)) {
        LOG_ERROR << "Failed to connect: " << strerror(errno);
        close(fd);
        _stats.failCount++;
        return {};
    }

    _stats.connectCount++;
    return ConnectionInfo { fd, _stats.serverIp, _port };
}

/**************************************************************
 */
BlockConnection::~BlockConnection() {
    if (_fd != -1) {
        close(_fd);
    }
}

int BlockConnection::readAll(char* buf, int size) {
    // LOG_TRACE << "BlockConnection::readAll: size=" << size;
    int offset = 0;
    while (offset < size) {
        int ret = read(_fd, buf + offset, size - offset);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }

            LOG_ERROR << "BlockConnection::readAll: Failed to read all: " << strerror(errno);
            _stats.failCount++;
            return -1;
        }

        offset += ret;
        _stats.readSize += ret;

        // LOG_TRACE << "BlockConnection::readAll: offset=" << offset;
    }

    return size;
}

int BlockConnection::writeAll(char* buf, int size) {
    int offset = 0;
    while (offset < size) {
        int ret = write(_fd, buf + offset, size - offset);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }

            LOG_ERROR << "BlockConnection::writeAll: Failed to write all: " << strerror(errno);
            _stats.failCount++;
            return -1;
        }

        offset += ret;
        _stats.writeSize += ret;
    }

    return size;
}

int BlockConnection::readSome(char* buf, int size) {
    for (;;) {
        int ret = read(_fd, buf, size);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }

            LOG_ERROR << "BlockConnection::readSome: Failed to read some: " << strerror(errno);
            _stats.failCount++;
            return -1;
        }

        _stats.readSize += ret;
        return ret;
    }

    return 0;
}

int BlockConnection::writeSome(char* buf, int size) {
    for (;;) {
        int ret = write(_fd, buf, size);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }

            LOG_ERROR << "BlockConnection::writeSome: Failed to write some: " << strerror(errno);
            _stats.failCount++;
            return -1;
        }

        _stats.writeSize += ret;
        return ret;
    }

    return 0;
}


} // namespace bongo
