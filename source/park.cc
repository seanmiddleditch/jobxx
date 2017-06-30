
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

void jobxx::parkable::park(parking_lot& lot)
{
    // we can't be parked again if we're already parked
    bool expected = false;
    if (!_parking.compare_exchange_strong(expected, true, std::memory_order_acquire))
    {
        return;
    }

    // link into the parking lot(s) that we want to be
    // awoken by. note that our parked state is not
    // guaranteed to still be true by the end of this
    // process, so _wait must deal with that.
    if (!lot._park(*this))
    {
        _parking = false;
        return;
    }

    {
        std::unique_lock<std::mutex> lock(_lock);
        _cond.wait(lock, [&parked = _parking]()
        {
            return !parked;
        });
    }
}

bool jobxx::parkable::_unpark()
{
    // signal a thread to awaken _if_ it's currently parked.
    bool expected = true;
    bool const awoken = _parking.compare_exchange_strong(expected, false, std::memory_order_release);
    if (awoken)
    {
        // the lock is held to avoid a race; condition_variable
        // conditions can _only_ be modified under the lock used
        // to wait to avoid a race condition. by holding the lock,
        // we ensure that the condition_variable cannot be actively
        // querying its condition at the time we signal it, and
        // that it either hasn't queried yet or that it's for-sure
        // blocking and waiting for the notify.
        // FIXME: we can make this more efficient on some
        // platforms.
        std::lock_guard<std::mutex> _(_lock);
        _cond.notify_one();
    }
    return awoken;
}

bool jobxx::parking_lot::_park(parkable& thread)
{
    std::lock_guard<spinlock> _(_lock);

    // if the parking lot is already closed, we cannot park
    if (_closed)
    {
        return false;
    }

    // this is how we know to wake the thread
    if (_tail != nullptr)
    {
        _tail->_next = &thread;
        _tail = &thread;
    }
    else
    {
        _head = _tail = &thread;
    }

    return true;
}

bool jobxx::parking_lot::unpark_one()
{
    std::lock_guard<spinlock> _(_lock);

    while (_head != nullptr)
    {
        parkable* thread = _head;
        _head = _head->_next;
        if (_head == nullptr)
        {
            _tail = nullptr;
        }

        thread->_next = nullptr;

        // keep looping until we awaken a thread;
        // a thread may already be unparked by another
        // thread even though it was still in our queue.
        if (thread->_unpark())
        {
            return true;
        }
    }

    return false;
}

void jobxx::parking_lot::close()
{
    std::lock_guard<spinlock> _(_lock);

    // we can only modify this under the lock; signals
    // that no new threds can be parked, and any
    // parked threads that awaken must unpark
    _closed = true;

    // tell all currently-parked threads to awaken
    parkable* thread = _head;
    while (thread != nullptr)
    {
        parkable* next = thread->_next;
        thread->_next = nullptr;
        thread->_unpark();
        thread = next;
    }
    _head = _tail = nullptr;
}
