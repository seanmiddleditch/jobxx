// jobxx - C++ lightweight task library.
//
// This is free and unencumbered software released into the public domain.
// 
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non - commercial, and by any
// means.
// 
// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
// 
// For more information, please refer to <http://unlicense.org/>
//
// Authors:
//   Sean Middleditch <sean.middleditch@gmail.com>

#if !defined(_guard_JOBXX_PARK_H)
#define _guard_JOBXX_PARK_H
#pragma once

#include "spinlock.h"
#include "predicate.h"
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace jobxx
{

    class parking_lot;

    class parkable
    {
    public:
        parkable() = default;

        parkable(parkable const&) = delete;
        parkable& operator=(parkable const&) = delete;

        static parkable& thid_thread();

        void park_until(parking_lot& lot, predicate pred);
        void park_until(parking_lot& lot, parking_lot& lot2, predicate pred);

    private:
        bool _unpark();

        // FIXME: we can make this more efficient on some platforms.
        // Linux, Win8+, etc. can sleep on the atomic's value/address (futexes).
        std::mutex _lock;
        std::condition_variable _cond;
        std::atomic<bool> _parked = false;

        friend parking_lot;
    };

    class parking_spot
    {
    public:
        parking_spot() = default;

        parking_spot(parking_spot const&) = delete;
        parking_spot& operator=(parking_spot const&) = delete;

    private:
        parkable* _thread = nullptr;
        parking_spot* _next = this;
        parking_spot* _prev = this;

        friend parking_lot;
    };

    class parking_lot
    {
    public:
        parking_lot() = default;
        ~parking_lot() { unpark_all(); }

        parking_lot(parking_lot const&) = delete;
        parking_lot& operator=(parking_lot const&) = delete;

        bool unpark_one();
        void unpark_all();

    private:
        void _link(parking_spot& spot, parkable& thread);
        void _unlink(parking_spot& spot);
        
        spinlock _lock;
        parking_spot _parked;

        friend parkable;
    };

}

#endif // defined(_guard_JOBXX_PARK_H)
