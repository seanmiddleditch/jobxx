
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

#include "jobxx/queue.h"
#include "jobxx/job.h"
#include "jobxx/_detail/task.h"

namespace jobxx
{

    struct queue::impl
    {

    };

    queue::queue() : _impl(new impl) {}
    queue::~queue()
    {
        work_all();
        delete _impl;
    }

    void queue::wait_job_actively(job const& awaited)
    {
        while (!awaited.complete())
        {
            work_one();
            // FIXME: back-off/sleep if work_one has no work
        }
    }

    bool queue::work_one()
    {
        return false;
    }

    void queue::work_all()
    {
        while (work_one())
        {
            // keep looping
        }
    }

    job queue::_spawn_job(delegate work)
    {
        job j;
        _spawn_task(std::move(work), &j);
        return j;
    }

    void queue::_spawn_task(delegate work, job* parent)
    {
        if (parent != nullptr)
        {
            parent->_add_task();
        }
        _detail::task item{std::move(work), parent};
        _execute(item);
    }

    void queue::_execute(_detail::task& item)
    {
        if (item.work)
        {
            item.work();
        }

        if (item.parent != nullptr)
        {
            item.parent->_complete_task();
        }
    }

}
