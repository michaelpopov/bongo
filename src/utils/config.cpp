/**********************************************
   File:   config.cpp

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
#include <unistd.h>
#include <iostream>
#include <getopt.h>

static struct option opts[] = {
    { "host",  1, 0, 'h' },
    { "port",  1, 0, 'p' },
    { "log",   1, 0, 'l' },
    { "batch", 0, 0, 'b' },
    { 0,       0, 0,  0 },
};
    
static const char* config_str = "h:p:l:b";

static const char* usage_str = "Usage:\n"
"    client -p <port> -h <host>\n"
"    client --port <port> --host <host>\n";

int Config::init(int argc, const char** argv) {
    optind = 1;
    int option_index = 1;

    for (;;) {
        int c = getopt_long(argc, const_cast<char**>(argv), config_str, opts, &option_index);
        if (c < 0) {
            break;
        }

        switch (c) {
            case 'b':
                _interactive = false;
                break;

            case 'h':
                _host = optarg;
                break;
            
            case 'p': {
                int n = sscanf(optarg, "%d", &_port);
                if (n != 1) {
                    return -1;
                }
                break;
            }
            
            case 'l':
                if (0 != setLogLevel(optarg)) {
                    return -1;
                }
                break;

            default:
                return -1;
        }
    }

    return valid() ? 0 : -1;
}

bool Config::valid() const {
    return _port > 0 && !_host.empty();
}

const char* Config::usage() {
    return usage_str;
}

int Config::setLogLevel(const char* arg) {
    if (0 == strcasecmp(arg, "DEBUG")) {
        _logLevel = LL_DEBUG;
    } else  if (0 == strcasecmp(arg, "TRACE")) {
        _logLevel = LL_TRACE;
    } else  if (0 == strcasecmp(arg, "INFO")) {
        _logLevel = LL_INFO;
    } else  if (0 == strcasecmp(arg, "WARN")) {
        _logLevel = LL_WARN;
    } else  if (0 == strcasecmp(arg, "ERROR")) {
        _logLevel = LL_ERROR;
    } else  if (0 == strcasecmp(arg, "CRITICAL")) {
        _logLevel = LL_CRITICAL;
    } else {
        return -1;
    }

    return 0;
}
