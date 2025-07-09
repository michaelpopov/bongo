/**********************************************
   File:   data_buffer.h

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
#include <vector>

struct Buffer {
    char* ptr;
    size_t size;
};

class DataBuffer {
public:
    DataBuffer(size_t size = 1024) { _data.resize(size); }
    Buffer getData();
    Buffer getAvailable(size_t requested_size);
    void update(size_t increment_size);
    void used(size_t used_size);
    void release();
    void swap(DataBuffer& buffer) { _data.swap(buffer._data); }
    void append(DataBuffer& buffer);
    size_t size() const { return _size - _offset; }

    void testPrint() const;

private:
    size_t _offset = 0;
    size_t _size = 0;
    std::vector<char> _data;
};
