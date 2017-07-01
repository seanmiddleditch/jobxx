
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
#include <mutex>
#include <condition_variable>

struct jobxx::parking_lot::parkable
{
    parkable() = default;

    parkable(parkable const&) = delete;
    parkable& operator=(parkable const&) = delete;

    // FIXME: we can make this more efficient on some platforms.
    // Linux, Win8+, etc. can sleep on the atomic's value/address (futexes).
    std::mutex _lock;
    std::condition_variable _cond;
    std::atomic<bool> _parked = false;
};

void jobxx::parking_lot::park_until(parking_lot* other_lot, predicate pred)
{
    thread_local parkable local_thread;
    parkable& thread = local_thread; // can't capture thread_local variables in lambdas

    // we can't be parked again if we're already parked
    bool expected = false;
    if (!thread._parked.compare_exchange_strong(expected, true, std::memory_order_acquire))
    {
        return;
    }

    // link into the parking lot(s) that we want to be
    // awoken by. note that our parked state is not
    // guaranteed to still be true by the end of this
    // process, so _wait must deal with that.
    node spot;
    _link(spot, thread);

    parking_lot::node spot2;
    if (other_lot != nullptr)
    {
        other_lot->_link(spot2, thread);
    }

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
        thread._parked = false;
    }
    else
    {
        std::unique_lock<std::mutex> lock(thread._lock);
        thread._cond.wait(lock, [&thread](){ return !thread._parked.load(); });
    }

    // unlink from both lot, because we very possibly were only
    // unlinked by one of them, and we can't leave either with a
    // dangling reference to a node.
    _unlink(spot);
    if (other_lot != nullptr)
    {
        other_lot->_unlink(spot2);
    }
}

bool jobxx::parking_lot::unpark_one()
{
    std::lock_guard<spinlock> _(_lock);

    while (_parked._next != &_parked)
    {
        node* const spot = _parked._next;
        _parked._next = _parked._next->_next;
        _parked._next->_prev = &_parked;

        spot->_prev = spot->_next = spot;

        // keep looping until we awaken a thread;
        // a thread may already be unparked by another
        // thread even though it was still in our queue.
        if (_unpark(*spot->_thread))
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
    node* spot = _parked._next;
    while (spot != &_parked)
    {
        node* const next = spot->_next;
        spot->_prev = spot->_next = spot;
        _unpark(*spot->_thread);
        spot = next;
    }
    _parked._prev = _parked._next = &_parked;
}

bool jobxx::parking_lot::_unpark(parkable& thread)
{
    // signal a thread to awaken _if_ it's currently parked.
    bool expected = true;
    bool const awoken = thread._parked.compare_exchange_strong(expected, false, std::memory_order_release);
    if (awoken)
    {
        // the lock is held to avoid a race; condition_variable
        // conditions can _only_ be modified under the lock used
        // to wait to avoid a race condition. by holding the lock,
        // we ensure that the condition_variable cannot be actively
        // querying its condition at the time we signal it, and
        // that it either hasn't queried yet or that it's for-sure
        // blocking and waiting for the notify.
        std::lock_guard<std::mutex> _(thread._lock);
        thread._cond.notify_one();
    }
    return awoken;
}

void jobxx::parking_lot::_link(node& spot, parkable& thread)
{
    std::lock_guard<spinlock> _(_lock);

    spot._thread = &thread;
    spot._next = &_parked;
    spot._prev = _parked._prev;
    spot._prev->_next = &spot;
    _parked._prev = &spot;
}

void jobxx::parking_lot::_unlink(node& spot)
{
    std::lock_guard<spinlock> _(_lock);

    spot._next->_prev = spot._prev;
    spot._prev->_next = spot._next;
}
