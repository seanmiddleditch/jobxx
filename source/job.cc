
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

#include "jobxx/job.h"
#include "jobxx/task.h"

namespace jobxx
{
   
    struct job::impl
    {
        std::atomic<int> refs = 0;
        std::atomic<int> tasks = 0;
    };

    job& job::operator=(job&& rhs)
    {
        if (this != &rhs)
        {
            _dec_ref();

            _impl = rhs._impl;
            rhs._impl = nullptr;

            _add_ref();
        }

        return *this;
    }

    bool job::complete() const
    {
        return _impl == nullptr || _impl->tasks == 0;
    }

    void job::_add_ref()
    {
        if (_impl != nullptr)
        {
            ++_impl->refs;
        }
    }

    void job::_dec_ref()
    {
        if (_impl != nullptr)
        {
            if (--_impl->refs == 0)
            {
                delete _impl;
            }
        }
    }

    void job::_add_task()
    {
        // create impl if it doesn't yet exist
        // e.g. for a default-constructed job
        // or a moved-from job
        if (_impl == nullptr)
        {
            _impl = new impl;
        }

        // increment the number of pending tasks
        // and if this is the first task, add a
        // reference so the job isn't deleted
        // before the task completes. we only do
        // this count on the first/last task to
        // avoid excessive reference counting.
        if (0 == _impl->tasks++)
        {
            ++_impl->refs;
        }
    }

    void job::_complete_task()
    {
        // decrement the number of outstanding
        // tasks. if this is the last task that
        // was pending, also remove the reference
        // count we added when the first task was
        // added, since there are no longer any
        // tasks referencing the job.
        if (_impl != nullptr && 0 == --_impl->tasks)
        {
            _dec_ref();
        }
    }

}
