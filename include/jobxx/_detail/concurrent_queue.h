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

#if !defined(_guard_JOBXX_DETAIL_CONCURRENT_QUEUE_H)
#define _guard_JOBXX_DETAIL_CONCURRENT_QUEUE_H
#pragma once

#include <mutex>
#include <deque>

namespace jobxx
{

    namespace _detail
    {

        struct task;
        
        class concurrent_queue
        {
        public:
            inline void push_back(_detail::task* task);
            inline _detail::task* pop_front();
            inline bool maybe_empty() const;

        private:
            // FIXME: temporary "just works" data-structure to be
            // replaced by "lock-free" structure
            mutable std::mutex _lock;
            std::deque<_detail::task*> _queue;
        };

        void concurrent_queue::push_back(_detail::task* task)
        {
            std::lock_guard<std::mutex> _(_lock);
            _queue.push_back(task);
        }

        _detail::task* concurrent_queue::pop_front()
        {
            std::lock_guard<std::mutex> _(_lock);
            if (!_queue.empty())
            {
                _detail::task* item = _queue.front();
                _queue.pop_front();
                return item;
            }
            else
            {
                return nullptr;
            }
        }

        bool concurrent_queue::maybe_empty() const
        {
            std::lock_guard<std::mutex> _(_lock);
            return _queue.empty();
        }
    }

}

#endif // defined(_guard_JOBXX_DETAIL_CONCURRENT_QUEUE_H)
