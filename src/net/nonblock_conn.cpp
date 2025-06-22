/**********************************************
   File:   nonblock_conn.cpp

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
#include "message.h"
#include "utils/log.h"

#include <experimental/scope>

#include <assert.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>

namespace bongo {

/**************************************************
 *    NonBlockNet
 */
NonBlockBase::~NonBlockBase() {
    if (_fd != -1) {
        LOG_TRACE << "NonBlockBase::~NonBlockBase: close fd=" << _fd;
        close(_fd);
    }
}

void NonBlockBase::die() {
    _dead = true;
    if (_fd != -1) {
        LOG_TRACE << "NonBlockBase::~NonBlockBase: close fd=" << _fd;
        close(_fd);
        _fd = -1;
    }
}

/**************************************************
 *    NonBlockNet
 */
NonBlockNet::~NonBlockNet() {
    if (_fd != -1) {
        close(_fd);
    }

    for (auto& nb: _items) {
        clearItem(nb);
    }
    _items.clear();
}

int NonBlockNet::init(size_t slotsCount) {
    _evsvec.resize(slotsCount);

    _fd = epoll_create(slotsCount);
    if (_fd < 0) {
        LOG_CRITICAL << "NonBlockNet::init: failed to create epoll: " << strerror(errno);
        return -1;
    }

    auto [ret, err] = _pipe.init();
    if (ret != 0) {
        return -1;
    }

    NonBlockBase* nb = new NonBlockBase("PipeQueue", _pipe.readFd(), NonBlockFdType::PipeQueue);
    ret = registerFd(_pipe.readFd(), NetOpType::Read, nb);
    if (ret != 0) {
        LOG_ERROR << "onBlockNet::init: failed register pipe fd";
        delete nb;
        return -1;
    }

    _stats.ready = true;
    _keepRunning.store(true);

    return 0;
}

void NonBlockNet::deleteItem(NonBlockBase* nb) {
    assert(_items.find(nb) != _items.end());
    assert(_stats.count() == itemsCount());

    if (nb->type() == NonBlockFdType::Connection) {
        NonBlockConnection* conn = static_cast<NonBlockConnection*>(nb);
        NetSession* session = conn->session();
        switch (session->state()) {
            case NetSessionState::Reading:
            case NetSessionState::Writing:
                break; // keep going, let's remove this connection.
            default:
                nb->die();
                return; // we cannot delete connection when a session in this state.
        }
    }

    _items.erase(nb);
    clearItem(nb);
}

void NonBlockNet::clearItem(NonBlockBase* nb) {
    if (nb == nullptr) {
        return;
    }

    switch (nb->type()) {
        case NonBlockFdType::Connection: _stats.connectionsCount--; break;
        case NonBlockFdType::Connector: _stats.connectorsCount--; break;
        case NonBlockFdType::Listener: _stats.listenersCount--; break;
        case NonBlockFdType::PipeQueue: _stats.pipesCount--; break;
    }

    delete nb;
}

void NonBlockNet::addItem(NonBlockBase* nb) {
    assert(nb !=  nullptr);

    switch (nb->type()) {
        case NonBlockFdType::Connection: _stats.connectionsCount++; break;
        case NonBlockFdType::Connector: _stats.connectorsCount++; break;
        case NonBlockFdType::Listener: _stats.listenersCount++; break;
        case NonBlockFdType::PipeQueue: _stats.pipesCount++; break;
    }

    _items.insert(nb);
    assert(_stats.count() == itemsCount());
}

int NonBlockNet::setNonBlocking(int fd) {
    if (0 != fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK)) {
        LOG_ERROR << "NonBlockNet::setNonBlocking: failed to set non-blocking socket: " << strerror(errno);
        return -1;
    }

    LOG_TRACE << "NonBlockNet::setNonBlocking: fd=" << fd;
    return 0;
}

int NonBlockNet::registerFd(int fd, NetOpType opType, NonBlockBase* nb) {
    LOG_TRACE << "NonBlockNet::registerFd: fd=" << fd << "  " 
              << ((opType == NetOpType::Read) ? "EPOLLIN" : "(EPOLLOUT | EPOLLET)");

    int ret = setNonBlocking(fd);
    if (ret != 0) {
        return -1;
    }

    int flags = (opType == NetOpType::Read) ? EPOLLIN : (EPOLLOUT | EPOLLET);

    epoll_event ev;
    ev.data.ptr = nb;
    ev.events = /*EPOLLRDHUP |*/ flags;
    ret = epoll_ctl(_fd, EPOLL_CTL_ADD, fd, &ev);
    if (ret) {
        LOG_ERROR << "NonBlockNet::registerFd: failed to add epoll event: " << strerror(errno);
        return -1;
    }

    addItem(nb);
    return 0;
}

int NonBlockNet::modifyFd(int fd, NetOpType opType, NonBlockBase* nb) {
    LOG_TRACE << "NonBlockNet::modifyFd: fd=" << fd << "  " 
              << ((opType == NetOpType::Read) ? "EPOLLIN" : "(EPOLLOUT | EPOLLET)");
    assert(nb->type() == NonBlockFdType::Connection);

    int flags = (opType == NetOpType::Read) ? EPOLLIN : (EPOLLOUT | EPOLLET);

    epoll_event ev;
    ev.data.ptr = nb; 
    ev.events = /*EPOLLRDHUP |*/ flags;
    int ret = epoll_ctl(_fd, EPOLL_CTL_MOD, fd, &ev);
    if (ret) {
        LOG_ERROR << "NonBlockNet::modifyFd: failed to modify epoll event: " << strerror(errno);
        return -1;
    }   

    LOG_TRACE << "NonBlockNet::modifyFd: type " << ((opType == NetOpType::Read) ? "EPOLLIN" : "EPOLLOUT");
    assert(_stats.count() == itemsCount());
    return 0;
}

void NonBlockNet::unregisterFd(int fd) {
    epoll_event ev;
    bzero(&ev, sizeof(ev));
    int ret = epoll_ctl(_fd, EPOLL_CTL_DEL, fd, &ev);
    if (ret) {
        LOG_ERROR << "NonBlockNet::unregisterFd: failed to unregister epoll event: " << strerror(errno);
        return;
    }   

    assert(_stats.count() == itemsCount());
}

int NonBlockNet::startListen(const NetOperation& op) {
    int fd = -1;
    NonBlockListener* nb = nullptr;
    int result = -1;

    auto cleanup = std::experimental::scope_exit([&]() {
        if (result != 0) {
            if (nb == nullptr && fd != -1) {
                close(fd);
            }

            clearItem(nb);
        }
    });

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        LOG_ERROR << "NonBlockNet::startListen: failed to create socket: " << strerror(errno);
        return result;
    }

    int on = 1;
    int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (ret != 0) {
        LOG_ERROR << "NonBlockNet::startListen: failed to set socket resuable: " << strerror(errno);
        return result;
    }

    sockaddr_in bindaddr;
    bindaddr.sin_family= AF_INET;
    bindaddr.sin_port = htons(op.port);
    if (op.ip.empty() || op.ip[0] == '*') {
        bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        bindaddr.sin_addr.s_addr = inet_addr(op.ip.c_str()); 
        if (bindaddr.sin_addr.s_addr == INADDR_NONE) {
            LOG_ERROR << "NonBlockNet::startListen: failed to convert IP address: " << op.ip;
            return result;
        }   
    }

    ret = bind(fd, (struct sockaddr *) &bindaddr, sizeof(struct sockaddr_in));
    if (ret != 0) {
        LOG_ERROR << "NonBlockNet::startListen: failed to bind: " << strerror(errno);
        return result;
    }

    ret = listen(fd, SOMAXCONN);
    if (ret != 0) {
        LOG_ERROR << "NonBlockNet::startListen: failed to listen: " << strerror(errno);
        return result;
    }

    nb = new NonBlockListener(op.name, fd, op.factory);
    ret = registerFd(fd, NetOpType::Read, nb);
    if (ret != 0) {
        LOG_ERROR << "NonBlockNet::startListen: failed to register";
        return result;
    }

    result = 0;
    return result;
}

int NonBlockNet::startConnect(const NetOperation& op) {
    int fd = -1;
    NonBlockConnector* nb = nullptr;
    int result = -1;

    auto cleanup = std::experimental::scope_exit([&]() {
        if (result != 0) {
            if (nb == nullptr && fd != -1) {
                close(fd);
            }

            clearItem(nb);
        }
    });

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        LOG_ERROR << "NonBlockNet::startConnect: failed create socket: " << strerror(errno);
        return result;
    }

    int ret = setNonBlocking(fd);
    if (ret != 0) {
        return result;
    }

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(op.port);
    sa.sin_addr.s_addr = inet_addr(op.ip.c_str()); 

    ret = connect(fd, (struct sockaddr*)&sa, sizeof(sa));
    if (ret == 0) {
        LOG_TRACE << "NonBlockNet::startConnect: connection ready.";
        ret = on_connect(fd, op);
        if (ret != 0) {
            return -1;
        }
    } else {
        if (errno != EINPROGRESS) {
            LOG_ERROR << "OpRet::Failed: failed connect: " << strerror(errno);
            return result;
        }

        LOG_TRACE << "NonBlockNet::startConnect: register fd.";
        nb = new NonBlockConnector(op.name, fd, op.factory);
        ret = registerFd(fd, NetOpType::Write, nb);
        if (ret != 0) {
            LOG_ERROR << "NonBlockNet::startConnect: failed to register";
            return result;
        }
    }

    result = 0;
    return result;
}

void NonBlockNet::on_accept(NonBlockListener* listener) {
    while (_keepRunning.load()) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
    
        int fd = accept(listener->fd(), (struct sockaddr *)&client_addr, &client_addr_len);
        if (fd == -1 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
            break;
        }

        if (fd == -1) {
            LOG_ERROR << "NonBlockNet::on_accept: failed to accept connection: " << strerror(errno);
            deleteItem(listener);
            return;
        }

        NonBlockConnection* nb = new NonBlockConnection(this, listener->name(), fd);
        int ret = nb->setSession(listener->factory());
        if (ret != 0) {
            LOG_ERROR << "NonBlockNet::on_accept: failed to set session for connection " << listener->name();
            close(fd);
            delete nb;
            return;
        }

        ret = registerFd(fd, NetOpType::Read, nb);
        if (ret != 0) {
            LOG_ERROR << "NonBlockNet::on_accept: failed to register connection";
            clearItem(nb);
            return;
        }

        LOG_TRACE << "NonBlockNet::on_accept: accepted new connection for " << nb->name();
        onRead(nb);
    }
}

void NonBlockNet::on_connect(NonBlockConnector* connector) {
    LOG_TRACE << "NonBlockNet::on_connect: enter.";
    bool result = false;

    auto cleanup = [&]() {
        if (connector != nullptr) {
            if (result) {
                connector->releaseFd();
            }

            deleteItem(connector);
            connector = nullptr;
        }
    };

    auto cleanup_on_exit = std::experimental::scope_exit(cleanup);

    NonBlockConnection* nb = new NonBlockConnection(this, connector->name(), connector->fd());
    int ret = nb->setSession(connector->factory());
    if (ret != 0) {
        LOG_ERROR << "NonBlockNet::on_accept: failed to set session for connection " << connector->name();
        clearItem(nb);
        return;
    }

    nb->session()->setState(NetSessionState::Writing);
    ret = modifyFd(nb->fd(), NetOpType::Write, nb);
    if (ret != 0) {
        LOG_ERROR << "NonBlockNet::on_connect: failed to modify fd";
        clearItem(nb);
        return;
    }

    result = true;
    cleanup();
    addItem(nb);
    onWrite(nb);
}

int NonBlockNet::on_connect(int fd, const NetOperation& op) {
    NonBlockConnection* nb = new NonBlockConnection(this, op.name, fd);
    int ret = nb->setSession(op.factory);
    if (ret != 0) {
        LOG_ERROR << "NonBlockNet::on_connect: failed to set session for connection " << op.name;
        close(fd);
        delete nb;
        return -1;
    }

    ret = registerFd(nb->fd(), NetOpType::Write, nb);
    if (ret != 0) {
        LOG_ERROR << "NonBlockNet::on_connect: failed to register fd";
        clearItem(nb);
        return -1;
    }

    onWrite(nb);

    return 0;
}

void NonBlockNet::onRead(NonBlockConnection* connection) {
    LOG_TRACE << "NonBlockNet::onRead for " << connection->name();

    NetSession* session = connection->session();
    session->setState(NetSessionState::Reading);
    size_t size = session->getSize();

    while (_keepRunning.load()) {
        Buffer buf = session->getReadBuffer(size);
        int ret = read(connection->fd(), buf.ptr, buf.size);
    
        // Interrupted by signal. Try again.
        if (ret < 0 && errno == EINTR) {
            LOG_TRACE << "NonBlockNet::onRead interrupt " << connection->name();
            continue;
        }

        // Failed connection. Remove the connection.
        if (ret < 0 && (errno != EWOULDBLOCK && errno != EAGAIN)) {
            LOG_TRACE << "NonBlockNet::onRead: failed to read from socket";
            deleteItem(connection);
            return;
        }

        if (ret <= 0) {
            LOG_TRACE << "NonBlockNet::onRead no more data " << connection->name();
            break;
        }

        size_t sz = (size_t)ret;
        session->updateReadBuffer(sz);
        LOG_TRACE << "NonBlockNet::onRead received " << sz << " Bytes " << connection->name();

        // If operation found data to fill the whole buffer, it means
        // there may be more data. Keep reading.
        if (sz == buf.size) {
            LOG_TRACE << "NonBlockNet::onRead completed buffer " << connection->name();
            continue;
        }

        // Now we completed reading all data. Start processing.
        break;
    }

    LOG_TRACE << "NonBlockNet::onRead finished reading " << connection->name();
    int ret = session->onRead();
    if (ret != 0) {
        LOG_TRACE << "NonBlockNet::onRead: finish connection: " << connection->name();
        deleteItem(connection);
    }
}

void NonBlockNet::onWrite(NonBlockConnection* connection) {
    LOG_TRACE << "NonBlockNet::onWrite for " << connection->name();

    NetSession* session = connection->session();

    while (_keepRunning.load()) {
        Buffer buf = session->getDataForWriting();
        if (buf.size == 0) {
            LOG_TRACE << "NonBlockNet::onWrite no data " << connection->name();
            break;
        }

        LOG_TRACE << "NonBlockNet::onWrite: buffer size " << buf.size;
        int ret = send(connection->fd(), buf.ptr, buf.size, MSG_NOSIGNAL);
        if (ret < 0 && errno == EINTR) {
            LOG_TRACE << "NonBlockNet::onWrite: interrupt";
            continue;
        }

        // Failed connection. Remove the connection.
        if (ret < 0 && (errno != EWOULDBLOCK && errno != EAGAIN)) {
            LOG_TRACE << "NonBlockNet::onWrite: failed to write to socket";
            deleteItem(connection);
            return;
        }

        // Switch to writing mode. If needed.
        if (ret < 0 && session->state() != NetSessionState::Writing) {
            LOG_TRACE << "NonBlockNet::onWrite modify fd to write " << connection->name();
            ret = modifyFd(connection->fd(), NetOpType::Write, connection);
            if (ret != 0) {
                LOG_TRACE << "NonBlockNet::onWrite: failed to modify to write: " << connection->name();
                deleteItem(connection);
            }

            session->setState(NetSessionState::Writing);

            return;
        }

        if (ret < 0) {
            LOG_TRACE << "NonBlockNet::onWrite: ret=-1 " << strerror(errno);
            return;
        }

        size_t sz = (size_t)ret;
        session->completedWriting(sz);

        LOG_TRACE << "NonBlockNet::onWrite written " << sz << " Bytes for " << connection->name();
    }

    /****************************************
     *   ret < 0 - something wrong happened on the session, close connection and delete it.
     *   ret == 0 - all good, session finished writing, let's read
     *   ret > 0. all good, but session will write more later, no reading yet.
     */
    int ret = session->onWrite();
    if (ret < 0) {
        LOG_TRACE << "NonBlockNet::onWrite: finish connection: " << connection->name();
        deleteItem(connection);
        return;
    }

    // Switch to reading mode. If needed.
    if (ret == 0 && session->state() != NetSessionState::Reading) {
        LOG_TRACE << "NonBlockNet::onWrite modify fd to read " << connection->name();
        // TODO: NOT GOOD. Session state and connection state is not the same!!!
        int ret = modifyFd(connection->fd(), NetOpType::Read, connection);
        if (ret != 0) {
            LOG_TRACE << "NonBlockNet::onWrite: failed to modify to write: " << connection->name();
            deleteItem(connection);
            return;
        }

        session->setState(NetSessionState::Reading);
    }
}

void NonBlockNet::on_error(NonBlockBase* nb) {
    LOG_TRACE << "NonBlockNet::on_error: finish connection: " << nb->name();

    int error = 0;
    socklen_t len = sizeof(error);

    int ret = getsockopt(nb->fd(), SOL_SOCKET, SO_ERROR, &error, &len);
    if (ret != 0) {
        LOG_ERROR << "NonBlockNet::on_error: failed to get socket error: " << strerror(errno);
    } else {
        LOG_TRACE << "NonBlockNet::on_error: socket error: " << strerror(error);
    }

    deleteItem(nb);
}

int  NonBlockNet::run(int time_ms) {
    _keepRunning.store(true);
    _stats.running = true;

    int ret = 0;
    while (_keepRunning.load()) {
        ret = step(time_ms);
        if (ret != 0) {
            break;
        }
    }

    _stats.running = false;
    return ret;
}

void NonBlockNet::stop() {
    _keepRunning.store(false);
}

int  NonBlockNet::step(int time_ms) {
    bool once = true;
    while (once) {
        once = false;

        int count = epoll_wait(_fd, _evsvec.data(), _evsvec.size(), time_ms);
        if (count < 0 && errno == EINTR) {
            continue;
        }

        if (count < 0) {
            LOG_ERROR << "NonBlockNet::step: failed epoll_wait: " << strerror(errno);
            return -1;
        }

        for (int i = 0; i < count; i++) {
            auto& ev = _evsvec[i];
            NonBlockBase* nb = static_cast<NonBlockBase*>(ev.data.ptr);
            int mask = ev.events;

            if (mask & (EPOLLERR | EPOLLHUP)) {
                on_error(nb);
                continue;
            }

            if (mask & EPOLLIN) {
                switch (nb->type()) {
                    case NonBlockFdType::Connection: {
                        NonBlockConnection* conn = dynamic_cast<NonBlockConnection*>(nb);
                        onRead(conn);
                        break;
                    }

                    case NonBlockFdType::Listener: {
                        NonBlockListener* listener = dynamic_cast<NonBlockListener*>(nb);
                        on_accept(listener);
                        break;
                    }

                    case NonBlockFdType::PipeQueue: {
                        processPipe();
                        break;
                    }

                    default: {
                        LOG_ERROR << "NonBlockNet::step: invalid socket type on read";
                        on_error(nb);
                        break;
                    }
                }
            }

            if (mask & EPOLLOUT) {
                switch (nb->type()) {
                    case NonBlockFdType::Connection: {
                        NonBlockConnection* conn = dynamic_cast<NonBlockConnection*>(nb);
                        onWrite(conn);
                        break;
                    }

                    case NonBlockFdType::Connector: {
                        NonBlockConnector* connector = dynamic_cast<NonBlockConnector*>(nb);
                        on_connect(connector);
                        break;
                    }

                    default: {
                        LOG_ERROR << "NonBlockNet::step: invalid socket type on write";
                        on_error(nb);
                        break;
                    }
                }
            }            
        }
    }

    return 0;
}

void NonBlockNet::processPipe() {
    for (;;) {
        void* ptr = nullptr;
        auto [ret, err] = _pipe.read(ptr); // TODO: make it buffered read in the pipe.
        if (ret != 0) {
            if (err == EINTR) {
                continue;
            }

            if (err == EWOULDBLOCK || err == EAGAIN) {
                break;
            }

            // TODO: make it FATAL.
            LOG_CRITICAL << "NonBlockNet::processPipe: failed to get ptr from pipe: " << strerror(err);
            return;
        }

        MessageBase* msg = static_cast<MessageBase*>(ptr);
        NetSession* session = msg->session();
        NonBlockConnection* conn = session->parent();
        if (conn->dead()) {
            deleteItem(conn);
            continue;
        }

        switch (msg->type()) {
            case MessageType::SessionReleased:
                // TODO: if session has an input message, put it on the pipe for execution.
                //       otherwise - do nothing????
                break;
            
            case MessageType::MoreData: {
                onWrite(conn);
                break;
            }

            default:
                assert(false);
                break;
        }
    }
}

void NonBlockNet::waitListenerReady(size_t listenersCount, size_t loopCount, int sleepLenMs) {
    for (size_t i=0; i < loopCount; i++) {
        if (stats().listenersCount >= listenersCount) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(sleepLenMs));
    }
}

/**************************************************
 *    NonBlockConnection
 */

void NonBlockConnection::writeData() {
    _parent->onWrite(this);
}

int NonBlockConnection::setSession(NetSessionFactoryPtr factory) {
    _session = factory->makeSession(this);
    _session->setPipe(_parent->pipeFd());
    return _session->init();
}

} // namespace bongo
