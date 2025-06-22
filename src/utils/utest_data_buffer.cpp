/**********************************************
   File:   utest_data_buffer.cpp

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
#include "log.h"
#include "gtest/gtest.h"

using namespace bongo;

TEST(DATA_BUFFER, Basics) {
    const size_t count = 4096;
    const size_t step = 16;
    const size_t data_buffer_size = 16 * 1024;

    DataBuffer buf(data_buffer_size);

    // First request should resize the buffer and get its beginning.
    Buffer b1 = buf.getAvailable(count);
    ASSERT_NE(nullptr, b1.ptr);
    ASSERT_EQ(data_buffer_size, b1.size);
    
    // Update moves size of the buffer to "count".
    memset(b1.ptr, 'A', count);
    buf.update(count);

    // Next request to get availabe should bring buffer right after b1.
    Buffer b2 = buf.getAvailable(step);
    size_t s1 = (size_t)b1.ptr;
    size_t s2 = (size_t)b2.ptr;
    ASSERT_EQ(b1.ptr + count, b2.ptr) << "\n\n  s1=" << s1 << "\n  s2=" << s2 << "\n\n";
    ASSERT_EQ(data_buffer_size - count, b2.size);
    // Without calling update, we practically discarding this buffer.

    // Get data in pieces. Let the last piece not used.
    size_t i = 0;
    for ( ; i < count-step; i += step) {
        Buffer b3 = buf.getData();
        ASSERT_EQ(b3.ptr, b1.ptr + i);
        ASSERT_EQ(b3.size, count - i);
        buf.used(step);
    }

    // Get the last piece. Must empty the buffer.
    Buffer b4 = buf.getData();
    ASSERT_EQ(b4.ptr, b1.ptr + i);
    ASSERT_EQ(b4.size, step);
    buf.used(step);

    // Next available request should get the same buffer as b1.
    Buffer b5 = buf.getAvailable(count);
    ASSERT_EQ(b1.ptr, b5.ptr);
    ASSERT_EQ(b1.size, b5.size);
}