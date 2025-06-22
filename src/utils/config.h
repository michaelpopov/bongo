/**********************************************
   File:   config.h

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
#include "log.h"
#include <string>

namespace passio::a1::client {

class Config {
public:
    int init(int argc, const char** argv);

    int port() const { return _port; }
    const char* host() const { return _host.c_str(); }
    LogLevel logLevel() const { return _logLevel; }
    bool interactive() const { return _interactive; }

    bool valid() const;

    static const char* usage();

private:
    int _port = -1;
    std::string _host;
    LogLevel _logLevel = LL_INFO;
    bool _interactive = true;

private:
    int setLogLevel(const char* arg);
};

} // namespace passio::a1::client
