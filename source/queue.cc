
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
    work_all();
    delete _impl;
}

void jobxx::queue::wait_job_actively(job const& awaited)
{
    while (!awaited.complete())
    {
        work_one();
        // FIXME: back-off/sleep if work_one has no work
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
        // keep looping
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

	std::unique_lock<std::mutex> task_lock;
	tasks.push_back(item);
}

jobxx::_detail::task* jobxx::_detail::queue::pull_task()
{
	std::unique_lock<std::mutex> task_lock;
	if (!tasks.empty())
	{
		_detail::task* item = tasks.front();
		tasks.pop_front();
		return item;
	}
	else
	{
		return nullptr;
	}
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
