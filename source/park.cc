
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

void jobxx::parkable::park(parking_lot& lot, predicate pred)
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
    parking_spot spot;
    lot._link(spot, *this);

    // we check the predicate after parking the lot to 
    // avoid a race condition: if the predicate fails
    // for any reason, those reasons must result in an
    // eventual call to to unpark_one/unpark_all on the
    // lot, so we check the predicate after parking in
    // the lot but before sleeping. if the condition is
    // triggered after parking but before the sleep,
    // we'll be unparked (and the sleep will fail because
    // the parked flag is unset).
    if (pred && pred())
    {
        _parking = false;
        lot._unlink(spot);
        return;
    }

    {
        std::unique_lock<std::mutex> lock(_lock);
        _cond.wait(lock, [this](){ return !_parking.load(); });
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

void jobxx::parking_lot::_link(parking_spot& spot, parkable& thread)
{
    std::lock_guard<spinlock> _(_lock);

    spot._thread = &thread;

    // this is how we know to wake the thread
    if (_tail != nullptr)
    {
        _tail->_next = &spot;
        _tail = &spot;
    }
    else
    {
        _head = _tail = &spot;
    }
}

void jobxx::parking_lot::_unlink(parking_spot& spot)
{
    std::lock_guard<spinlock> _(_lock);

    if (_head == &spot)
    {
        _head = spot._next;
        if (_tail == &spot)
        {
            _tail = nullptr;
        }
    }
    else if (_head != nullptr)
    {
        for (parking_spot* iter = _head; iter->_next != nullptr; iter = iter->_next)
        {
            if (iter->_next == &spot)
            {
                iter->_next = spot._next;
                if (_tail == &spot)
                {
                    _tail = iter;
                }
                break;
            }
        }
    }
}

bool jobxx::parking_lot::unpark_one()
{
    std::lock_guard<spinlock> _(_lock);

    while (_head != nullptr)
    {
        parking_spot* const spot = _head;
        _head = _head->_next;
        if (_tail == spot)
        {
            _tail = nullptr;
        }

        spot->_next = nullptr;

        // keep looping until we awaken a thread;
        // a thread may already be unparked by another
        // thread even though it was still in our queue.
        if (spot->_thread->_unpark())
        {
            return true;
        }
    }

    return false;
}

void jobxx::parking_lot::unpark_all()
{
    std::lock_guard<spinlock> _(_lock);

    // tell all currently-parked threads to awaken
    parking_spot* spot = _head;
    while (spot != nullptr)
    {
        parking_spot* const next = spot->_next;
        spot->_next = nullptr;
        spot->_thread->_unpark();
        spot = next;
    }
    _head = _tail = nullptr;
}
