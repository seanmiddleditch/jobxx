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

#if !defined(_guard_JOBXX_CONCURRENT_QUEUE_H)
#define _guard_JOBXX_CONCURRENT_QUEUE_H
#pragma once

#include <mutex>
#include <deque>

namespace jobxx
{

    template <typename Value>
    class concurrent_queue
    {
    public:
        using value_type = Value;

        template <typename InsertValue> inline void push_back(InsertValue&& task);
        inline value_type pop_front();
        inline bool maybe_empty() const;

    private:
        // FIXME: temporary "just works" data-structure to be
        // replaced by "lock-free" structure
        mutable std::mutex _lock;
        std::deque<value_type> _queue;
    };

    template <typename Value>
    template <typename InsertValue>
    void concurrent_queue<Value>::push_back(InsertValue&& task)
    {
        std::lock_guard<std::mutex> _(_lock);
        _queue.push_back(std::forward<InsertValue>(task));
    }

    template <typename Value>
    auto concurrent_queue<Value>::pop_front() -> value_type
    {
        std::lock_guard<std::mutex> _(_lock);
        if (!_queue.empty())
        {
            value_type item = _queue.front();
            _queue.pop_front();
            return item;
        }
        else
        {
            return nullptr;
        }
    }

    template <typename Value>
    bool concurrent_queue<Value>::maybe_empty() const
    {
        std::lock_guard<std::mutex> _(_lock);
        return _queue.empty();
    }
    
}

#endif // defined(_guard_JOBXX_CONCURRENT_QUEUE_H)
