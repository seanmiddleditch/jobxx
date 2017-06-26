
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
    do
    {
        // must wrap the link operation and is used for the
        // condition variable.
        std::unique_lock<std::mutex> lock(_lock);

        // we may have been unlinked before being scheduled, but
        // the predicate may have become false again before being
        // scheduled, so we have to relink.
        _link(thread);

        // the condition is either that we've been unparked (the
        // thread is no longer linked) or the predicate is true.
        // thread is unlinked when explicitly unparked to make sure
        // that unpark_one no longer considers the thread; the
        // predicate, which presumably was true at the time, may
        // become false again before the thread is scheduled for
        // execution, so it's possible to become unlinked while
        // the predicate is false.
        
        thread._cond.wait(lock, [&thread, &pred, this]()
        {
            return thread._lot != this || (pred && pred());
        });
    }
    while (pred && !pred());
}

void jobxx::parking_lot::unpark(parkable& thread)
{
    _unlink(thread);
    thread._cond.notify_one();
}

void jobxx::parking_lot::unpark_one()
{
    parkable* thread = nullptr;

    {
        std::lock_guard<std::mutex> _(_lock);
        if (!_queue.empty())
        {
            thread = _queue.front();
            _queue.pop_front();
        }
    }

    if (thread != nullptr)
    {
        // FIXME: _lot needs to be protected, maybe an atomic
        thread->_lot = nullptr;
        thread->_cond.notify_one();
    }
}

void jobxx::parking_lot::unpark_all()
{
    std::lock_guard<std::mutex> _(_lock);
    for (parkable* parked : _queue)
    {
        parked->_lot = nullptr;
        parked->_cond.notify_one();
    }
    _queue.clear();
}

void jobxx::parking_lot::_link(parkable& thread)
{
    if (thread._lot != this)
    {
        thread._lot = this;
        _queue.push_back(&thread);
    }
}

void jobxx::parking_lot::_unlink(parkable& thread)
{
    std::lock_guard<std::mutex> _(_lock);

    if (thread._lot == this)
    {
        auto it = std::find(_queue.begin(), _queue.end(), &thread);
        if (it != _queue.end())
        {
            _queue.erase(it);
        }
    }
}
