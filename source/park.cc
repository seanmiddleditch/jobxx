
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
    std::atomic<int> _state = -2;

};

jobxx::park_result jobxx::park::_park(park* first, predicate first_pred, park* second, predicate second_pred)
{
    thread_local thread_state local_thread;
    thread_state& thread = local_thread; // can't capture thread_local variables in lambdas

    // we can't be parked again if we're already parked
    int expected = -1;
    if (!thread._state.compare_exchange_strong(expected, -1, std::memory_order_acquire))
    {
        return park_result::failure;
    }

    // link into the park(s) that we want to be
    // awoken by. note that our parked state is not
    // guaranteed to still be true by the end of this
    // process, so _wait must deal with that.
    parked_node first_node;
    first_node._id = 0;
    first_node._thread = &thread;
    first->_link(first_node);

    if (first_pred && first_pred())
    {
        first->_unlink(first_node);
        return park_result::first;
    }

    parked_node second_node;
    if (second != nullptr)
    {
        first_node._id = 1;
        second_node._thread = &thread;
        second->_link(second_node);

        if (second_pred && second_pred())
        {
            // we may have been unparked by the prior condition if it triggered after its predicate but before now.
            if (thread._state.compare_exchange_strong(expected, first_node._id, std::memory_order_seq_cst))
            {
                first->_unlink(first_node);
                second->_unlink(second_node);
                return park_result::second;
            }
        }
    }

    // we check the predicate after parking to avoid a race condition.
    // (1) the event may be triggered before parking.
    // (2) the event may be triggered after parking but before sleeping.
    // (3) the event may be triggered after sleeping.
    // the thread state and unpark logic will catch the second two.
    // the predicate is intended to catch the first. if the predicate
    // itself were checked before parking, then there would be a gap
    // of time before checking the predicate and parking in which the
    // event could be triggered and effectively lost.
    {
        std::unique_lock<std::mutex> lock(thread._lock);
        thread._cond.wait(lock, [&thread](){ return thread._state.load() == -1; });
    }

    // determine whom unlocked us, and reset our state back to its default.
    // note that the state will be either 0 or 1, which indicates which node
    // unparked this thread; it maps to the park_result values.
    int const old_state = thread._state.exchange(-2, std::memory_order::memory_order_seq_cst);

    // unlink from both parks, because we very possibly were only
    // unlinked by one of them, and we can't leave either with a
    // dangling reference to a node.
    first->_unlink(first_node);
    if (second != nullptr)
    {
        second->_unlink(second_node);
    }

    return static_cast<park_result>(old_state);
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
        // unpark operation even though it was still
        // in our queue, so we cannot assume that its
        // presence means we unlocked it.
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
    int expected = -1;
    bool const awoken = thread._state.compare_exchange_strong(expected, 0, std::memory_order_release);
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
