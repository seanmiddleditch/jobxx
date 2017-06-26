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

#if !defined(_guard_JOBXX_DETAIL_QUEUE_H)
#define _guard_JOBXX_DETAIL_QUEUE_H
#pragma once

#include "jobxx/delegate.h"
#include "jobxx/concurrent_queue.h"
#include "jobxx/predicate.h"
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace jobxx
{

    class queue;

    namespace _detail
    {

        struct job;
        struct task;
        struct parking_lot;

        class parkable
        {
        public:
            parkable() = default;

            parkable(parkable const&) = delete;
            parkable& operator=(parkable const&) = delete;

        private:
            std::condition_variable _cond;
            parking_lot* _lot = nullptr;
            parkable* _prev = nullptr;
            parkable* _next = nullptr;

            friend parking_lot;
            friend jobxx::queue;
        };

        struct parking_lot
        {
            void park(parkable& thread, predicate pred);
            void unpark(parkable& thread);
            void unpark_one() { if (head != nullptr) { unpark(*head); } }
            void unpark_all();

            // FIXME: use a atomic linked list, though note that
            // we may need to keep a mutex to pair with the
            // condition_variable.
            std::mutex lock;
            parkable* head = nullptr;
        };

        struct queue
        {
            void spawn_task(delegate work, _detail::job* parent);
            _detail::task* pull_task();
            void execute(_detail::task& item);

            concurrent_queue<_detail::task*> tasks;
            parking_lot parked;
        };

    }

}

#endif // defined(_guard_JOBXX_DETAIL_QUEUE_H)
