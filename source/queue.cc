
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
#include "jobxx/_detail/job.h"
#include "jobxx/_detail/queue.h"
#include "jobxx/_detail/task.h"

jobxx::queue::queue() : _impl(new _detail::queue) {}

jobxx::queue::~queue()
{
    work_all(); // finish any remaining tasks
    unpark_all(); // unpark any threads - FIXME: diallow re-parking
    delete _impl;
}

void jobxx::queue::wait_job_actively(job const& awaited)
{
    while (!awaited.complete())
    {
        work_one();
        // FIXME: park if work_one has no work, but only
        //  when we support parking on both a queue and a
        //  job. until then, back-off may be appropriate.
    }
}

bool jobxx::queue::work_one()
{
    _detail::task* item = _impl->pull_task();
    if (item != nullptr)
    {
        _impl->execute(*item);
        delete item;
        return true;
    }
    else
    {
        return false;
    }
}

void jobxx::queue::work_all()
{
    while (work_one())
    {
        // keep looping while there's work
    }
}

void jobxx::queue::park(predicate pred)
{
    _detail::parked_thread parked;

    // add thread to list of parked threads
    {
        std::lock_guard<std::mutex> _(_impl->parked.lock);

        parked.next = _impl->parked.head;
        _impl->parked.head = &parked;
        if (parked.next != nullptr)
        {
            parked.next->prev = &parked;
        }
    }

    // wait until a task is available or the predicate fires
    {
        std::unique_lock<std::mutex> lock(parked.lock);
        parked.signal.wait(lock, [this, &parked, &pred]()
        {
            return !_impl->tasks.maybe_empty() || (pred && pred());
        });
    }

    // remove thread from list of parked threads
    // FIXME: this is the race mentioned in spawn_task, and
    // we should instead here assume that we've been unparked
    // at this point, and repark ourselves if the condition
    // variable was "spuriously" woken.
    {
        std::lock_guard<std::mutex> _(_impl->parked.lock);

        if (parked.next != nullptr)
        {
            parked.next->prev = parked.prev;
        }
        if (parked.prev != nullptr)
        {
            parked.prev->next = parked.next;
        }
        if (&parked == _impl->parked.head)
        {
            _impl->parked.head = parked.next;
        }
    }
}

void jobxx::queue::unpark_all()
{
    std::lock_guard<std::mutex> _(_impl->parked.lock);
    for (_detail::parked_thread* parked = _impl->parked.head; parked != nullptr; parked = parked->next)
    {
        parked->signal.notify_one();
    }
}

jobxx::_detail::job* jobxx::queue::_create_job()
{
    return new _detail::job;
}

void jobxx::queue::spawn_task(delegate&& work)
{
    _impl->spawn_task(std::move(work), nullptr);
}

void jobxx::_detail::queue::spawn_task(delegate work, _detail::job* parent)
{
    if (parent != nullptr)
    {
        // increment the number of pending tasks
        // and if this is the first task, add a
        // reference so the job isn't deleted
        // before the task completes. we only do
        // this count on the first/last task to
        // avoid excessive reference counting.
        if (0 == parent->tasks++)
        {
            ++parent->refs;
        }
    }

    _detail::task* item = new _detail::task{std::move(work), parent};
    tasks.push_back(item);

    {
        // FIXME: this can result in the same thread being asked to unpark itself
        // twice while other threads remain parked. we need to unpark the thread
        // here and then wake it, and rely on the thread re-parking itself if
        // there's no work when it wakes up. will need to figure out how to do
        // that with condition_variable sanely, if possible.
        std::lock_guard<std::mutex> _(parked.lock);
        if (parked.head != nullptr)
        {
            parked.head->signal.notify_one();
        }
    }
}

jobxx::_detail::task* jobxx::_detail::queue::pull_task()
{
    return tasks.pop_front();
}

void jobxx::_detail::queue::execute(_detail::task& item)
{
    if (item.work)
    {
        context ctx(*this, item.parent);
        item.work(ctx);
    }

    if (item.parent != nullptr)
    {
        // decrement the number of outstanding
        // tasks. if this is the last task that
        // was pending, also remove the reference
        // count we added when the first task was
        // added, since there are no longer any
        // tasks referencing the job.
        if (item.parent != nullptr && 0 == --item.parent->tasks)
        {
            if (0 == --item.parent->refs)
            {
                delete item.parent;
            }
        }
    }
}
