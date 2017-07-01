
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

struct jobxx::park::thread_state
{
    thread_state() = default;

    thread_state(thread_state const&) = delete;
    thread_state& operator=(thread_state const&) = delete;

    // FIXME: we can make this more efficient on some platforms.
    // Linux, Win8+, etc. can sleep on the atomic's value/address (futexes).
    std::mutex _lock;
    std::condition_variable _cond;
    std::atomic<bool> _parked = false;
};

void jobxx::park::park_until(park* other, predicate pred)
{
    thread_local thread_state local_thread;
    thread_state& thread = local_thread; // can't capture thread_local variables in lambdas

    // we can't be parked again if we're already parked
    bool expected = false;
    if (!thread._parked.compare_exchange_strong(expected, true, std::memory_order_acquire))
    {
        return;
    }

    // link into the park(s) that we want to be
    // awoken by. note that our parked state is not
    // guaranteed to still be true by the end of this
    // process, so _wait must deal with that.
    parked_node node;
    node._thread = &thread;
    _link(node);

    parked_node other_node;
    if (other != nullptr)
    {
        other_node._thread = &thread;
        other->_link(other_node);
    }

    // we check the predicate after parking to 
    // avoid a race condition: if the predicate fails
    // for any reason, those reasons must result in an
    // eventual call to to unpark_one/unpark_all on the
    // park, so we check the predicate after parking 
    // but before sleeping. if the condition is
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

    // unlink from both parks, because we very possibly were only
    // unlinked by one of them, and we can't leave either with a
    // dangling reference to a node.
    _unlink(node);
    if (other != nullptr)
    {
        other->_unlink(other_node);
    }
}

bool jobxx::park::unpark_one()
{
    std::lock_guard<spinlock> _(_lock);

    while (_parked._next != &_parked)
    {
        parked_node* const node = _parked._next;
        _parked._next = _parked._next->_next;
        _parked._next->_prev = &_parked;

        node->_prev = node->_next = node;

        // keep looping until we awaken a thread;
        // a thread may already be unparked by another
        // thread even though it was still in our queue.
        if (_unpark(*node->_thread))
        {
            return true;
        }
    }

    return false;
}

void jobxx::park::unpark_all()
{
    std::lock_guard<spinlock> _(_lock);

    // tell all currently-parked threads to awaken
    parked_node* node = _parked._next;
    while (node != &_parked)
    {
        parked_node* const next = node->_next;
        node->_prev = node->_next = node;
        _unpark(*node->_thread);
        node = next;
    }
    _parked._prev = _parked._next = &_parked;
}

bool jobxx::park::_unpark(thread_state& thread)
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

void jobxx::park::_link(parked_node& node)
{
    std::lock_guard<spinlock> _(_lock);

    node._next = &_parked;
    node._prev = _parked._prev;
    node._prev->_next = &node;
    _parked._prev = &node;
}

void jobxx::park::_unlink(parked_node& node)
{
    std::lock_guard<spinlock> _(_lock);

    node._next->_prev = node._prev;
    node._prev->_next = node._next;
}
