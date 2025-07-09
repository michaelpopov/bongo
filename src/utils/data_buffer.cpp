/**********************************************
   File:   data_buffer.cpp

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

#include "data_buffer.h"
#include <assert.h>
#include <string.h>
#include <iostream>

Buffer DataBuffer::getData() {
    return Buffer {
        .ptr = _data.data() + _offset,
        .size = _size - _offset,
    };
}

Buffer DataBuffer::getAvailable(size_t requested_size) {
    if (_size + requested_size > _data.size()) {
        _data.resize(_size + requested_size);
    }

    return Buffer {
        .ptr = _data.data() + _size,
        .size = _data.size() - _size,
    };
}

void DataBuffer::update(size_t increment_size) {
    assert(_size + increment_size <= _data.size());
    _size += increment_size;
}

void DataBuffer::used(size_t used_size) {
    assert(_offset + used_size <= _size);

    _offset += used_size;
    if (_offset == _size) {
        _offset = _size = 0;
        return;
    }
}

void DataBuffer::release() {
    if (_size == 0 || _offset == 0) {
        return;
    }

    memmove(_data.data(), _data.data() + _offset, _size - _offset);
    _size -= _offset;
    _offset = 0;
}

void DataBuffer::append(DataBuffer& buffer) {
    Buffer src = buffer.getData();
    if (src.size == 0) {
        return;
    }

    Buffer buf = getAvailable(src.size);
    memcpy(buf.ptr, src.ptr, src.size);
    update(src.size);
}

void DataBuffer::testPrint() const {
    std::cout << "DataBuffer:\t_size=" << _size 
        << "\t_offset=" << _offset 
        << "\tptr=" << (void*)_data.data()
        << std::endl;
}
