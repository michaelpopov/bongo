/**********************************************
   File:   non_block.h

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
#include "session_base.h"
#include "utils/pipe_queue.h"
#include "utils/thread_queue.h"
#include <string>
#include <vector>
#include <atomic>
#include <unordered_set>
#include <sys/epoll.h>

namespace bongo {

enum class NonBlockFdType {
    Listener,
    Connector,
    Connection,
    PipeQueue,
};

class NonBlockBase {
public:
    NonBlockBase(const std::string& name, int fd, NonBlockFdType type)
        : _name(name), _type(type), _fd(fd) {}
    virtual ~NonBlockBase();

    const std::string& name() const { return _name; }
    NonBlockFdType type() const { return _type; }

    int fd() const { return _fd; }
    void releaseFd() { _fd = -1; }

    bool dead() const { return _dead; }
    void die();

protected:
    const std::string _name;
    const NonBlockFdType _type;
    int _fd = -1;
    bool _dead = false;
};

class NonBlockListener : public NonBlockBase, public NetSessionFactoryOwner {
public:
    NonBlockListener(const std::string& name, int fd, NetSessionFactoryPtr factory)
      : NonBlockBase(name, fd, NonBlockFdType::Listener),
        NetSessionFactoryOwner(factory) { }
};

class NonBlockConnector : public NonBlockBase, public NetSessionFactoryOwner {
public:
    NonBlockConnector(const std::string& name, int fd, NetSessionFactoryPtr factory)
      : NonBlockBase(name, fd, NonBlockFdType::Connector),
        NetSessionFactoryOwner(factory) { }
};

class NonBlockNet;

class NonBlockConnection : public NonBlockBase {
public:
    NonBlockConnection(NonBlockNet* parent, const std::string& name, int fd)
      : NonBlockBase(name, fd, NonBlockFdType::Connection), _parent(parent) {}

    // Assuming this object can be deleted on the network thread and the session
    // in the right state, which might be not quite right. Keep thinking...
    virtual ~NonBlockConnection() { delete _session; }

    NetSession* session() const { return _session; }
    int setSession(NetSessionFactoryPtr factory);

    void writeData();

private:
    NonBlockNet* _parent;
    NetSession* _session = nullptr;
};

struct NetOperation {
    const std::string name;
    const std::string ip;
    int port;
    NetSessionFactoryPtr factory;
};

enum class NetOpType { Read, Write };
using SessionsQueue = ThreadQueue<NetSession*>;

class NonBlockNet {
    friend class NonBlockConnection;
public:
    virtual ~NonBlockNet();

    int init(size_t slotsCount = 1024);

    int startListen(const NetOperation& op);
    int startConnect(const NetOperation& op);

    int  run(int time_ms);
    void stop();
    int  step(int time_ms);

    int pipeFd() const { return _pipe.writeFd(); }

    void waitListenerReady(size_t listenersCount = 1, size_t loopCount = 10, int sleepLenMs = 50);

public:
    struct Stats {
        bool ready = false;
        bool running = false;
        size_t acceptedCount = 0;
        size_t connectedCount = 0;
        size_t connectionsCount = 0;
        size_t listenersCount = 0;
        size_t connectorsCount = 0;
        size_t pipesCount = 0;
        size_t count() const { return connectionsCount + listenersCount + connectorsCount + pipesCount; }
    };

    Stats stats() const { return _stats; }
    size_t itemsCount() const { return _items.size(); }

    void setSessionsQueue(SessionsQueue* queue) { _queue = queue; }
    PipeQueue* getPipeQueue() { return &_pipe; }

private:
    int _fd = -1;
    std::vector<epoll_event> _evsvec;
    std::unordered_set<NonBlockBase*> _items;
    Stats _stats;
    std::atomic<bool> _keepRunning = false;
    PipeQueue _pipe;
    SessionsQueue* _queue = nullptr;

private:
    void on_accept(NonBlockListener* listener);
    void on_connect(NonBlockConnector* connector);
    int  on_connect(int fd, const NetOperation& op);
    void onRead(NonBlockConnection* connection);
    void onWrite(NonBlockConnection* connection);
    void on_error(NonBlockBase* nb);

    int  registerFd(int fd, NetOpType opType, NonBlockBase* nb);
    int  modifyFd(int fd, NetOpType opType, NonBlockBase* nb);
    void unregisterFd(int fd);

    int setNonBlocking(int fd);

    void deleteItem(NonBlockBase* nb);
    void clearItem(NonBlockBase* nb);
    void addItem(NonBlockBase* nb);

    void processPipe();
};

} // namespace bongo
