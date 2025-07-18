/**********************************************
   File:   notification_base.h

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
#include "utils/pipe_queue.h"

namespace bongo {

class SessionBase;

enum class NotificationType {
    SessionReleased,
    MoreData,
    PushData,
};

class NotificationBase {
public:
    NotificationBase(NotificationType type, SessionBase* session) : _type(type), _session(session) {}
    virtual ~NotificationBase() = default;

    NotificationType type() const { return _type; }
    SessionBase* session() const { return _session; }

private:
    NotificationType _type;
    SessionBase* _session;
};

/*******************************************************************************
 * DON'T BOTHER WITH THIS YET!!!
class MessagePushData : public NotificationBase {
public:
    MessagePushData(SessionBase* session, DataBuffer& buffer)
      : NotificationBase(NotificationType::PushData, session) {
        _buffer.swap(buffer);
    }

    DataBuffer& buffer() { return _buffer; }

private:
    DataBuffer _buffer;
};
 *********************************************************************************/

using NotificationQueue = PipeQueue<NotificationBase>;

} // namespace bongo
