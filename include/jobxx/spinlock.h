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

#if !defined(_guard_JOBXX_SPINLOCK_H)
#define _guard_JOBXX_SPINLOCK_H
#pragma once

#include <atomic>

namespace jobxx
{

    class spinlock
    {
    public:
        spinlock() = default;

        spinlock(spinlock const&) = delete;
        spinlock& operator=(spinlock const&) = delete;

        inline void lock();
        inline void unlock();

    private:
        std::atomic<bool> _flag = false;
    };

    void spinlock::lock()
    {
        for (;;)
        {
            // spin waiting for the lock to be free. this spin is avoiding
            // invalidating the cacheline (since it's only reading).
            int spins = 0;
            while (_flag.load(std::memory_order_relaxed) == true)
            {
                // FIXME: there aren't any standard C++ ways to briefly relax the
                // CPU (e.g. Intel's RELAX instruction _mm_pause()) and I'm
                // not yet ready to start the CPU-specific optimizations.
            }

            // the lock is unlocked, so now we try to acquire it. another
            // thread may have already acquired it, though, so there's a
            // chance this will fail, and we'll need to spin more.
            if (_flag.exchange(true, std::memory_order_acquire) == /*old-value*/false)
            {
                // lock acquired
                break;
            }
        }
    }

    void spinlock::unlock()
    {
        _flag.store(false, std::memory_order_release);
    }

}

#endif // defined(_guard_JOBXX_SPINLOCK_H)
