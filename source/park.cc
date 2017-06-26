
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

#include "jobxx/park.h"

void jobxx::parking_lot::park(parkable& thread, predicate pred)
{
    {
        std::lock_guard<std::mutex> _(_lock);

        thread._next = _head;
        thread._lot = this;
        _head = &thread;
        if (thread._next != nullptr)
        {
            thread._next->_prev = &thread;
        }
    }

    // wait until a task is available or the predicate fires
    {
        // FIXME: I believe this is a race condition now; we could
        // park the thread, add a job and signal the condition, then
        // start waiting. the lack in question needs to be taken or
        // used by the job posting, which we want to avoid of course,
        // so this likely just needs to be rethough.
        std::unique_lock<std::mutex> lock(this->_lock);
        thread._cond.wait(lock, [&thread, &pred, this](){ return thread._lot != this || pred(); });
    }

    unpark(thread);
}

void jobxx::parking_lot::unpark(parkable& thread)
{
    // remove thread from list of parked threads
    // FIXME: this is the race mentioned in spawn_task, and
    // we should instead here assume that we've been unparked
    // at this point, and repark ourselves if the condition
    // variable was "spuriously" woken.
    {
        std::lock_guard<std::mutex> _(_lock);

        thread._lot = nullptr;
        if (thread._next != nullptr)
        {
            thread._next->_prev = thread._prev;
        }
        if (thread._prev != nullptr)
        {
            thread._prev->_next = thread._next;
        }
        if (&thread == _head)
        {
            _head = thread._next;
        }
    }

    thread._cond.notify_one();
}

void jobxx::parking_lot::unpark_all()
{
    std::lock_guard<std::mutex> _(_lock);
    for (parkable* parked = _head; parked != nullptr; parked = parked->_next)
    {
        parked->_cond.notify_one();
    }
}
