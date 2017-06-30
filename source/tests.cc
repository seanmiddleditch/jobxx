
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

#include <thread>
#include <atomic>
#include <array>

// test the general queue/task/job system _without_ threads
bool basic_test_one(jobxx::queue& queue)
{
    int num = 0x1337c0de;
    int num2 = 0x600df00d;

    jobxx::job job = queue.create_job([&num, &num2](jobxx::context& ctx)
    {
        // spawn a task in the job (with no task context)
        ctx.spawn_task([&num](){ num = 0xdeadbeef; });

        // spawn a task in the job (with task context)
        ctx.spawn_task([&num2](jobxx::context& ctx)
        {
            num2 = 0xdeadbeee;

            ctx.spawn_task([&num2](){ ++num2; });
        });
    });
    queue.wait_job_actively(job);

    if (num != 0xdeadbeef || num2 != 0xdeadbeef)
    {
        return false;
    }

    return true;
}

bool basic_test()
{
    // execute the test 10 times in naive hopes of catching races
    // FIXME: do this smarter
    for (int i = 0; i < 10; ++i)
    {
        jobxx::queue queue;
        if (!basic_test_one(queue))
        {
            return false;
        }
    }
    return true;
}

// test background threads and the main thread actively working together
bool thread_test_one()
{
    jobxx::queue queue;

    std::array<std::thread, 2> workers{
        std::thread([&queue](){ queue.work_forever(); }),
        std::thread([&queue](){ queue.work_forever(); })
    };

    std::atomic<int> counter = 0;
    for (int inc = 1; inc != 5; ++inc)
    {
        for (int ji = 0; ji != 1000; ++ji)
        {
            queue.spawn_task([&counter, inc](){ counter += inc; });
        }
    }

    while (counter != (1000 + 2000 + 3000 + 4000))
    {
        queue.work_all();
    }


    queue.close();
    for (auto& worker : workers)
    {
        worker.join();
    }

    return true;
}

bool thread_test()
{
    // execute the test 10 times in naive hopes of catching races
    // FIXME: do this smarter
    for (int i = 0; i < 10; ++i)
    {
        if (!thread_test_one())
        {
            return false;
        }
    }
    return true;
}

// test background threads working while the main thread does not execute tasks
bool inactive_wait_thread_test_one()
{
    jobxx::queue queue;

    std::array<std::thread, 2> workers{
        std::thread([&queue](){ queue.work_forever(); }),
        std::thread([&queue](){ queue.work_forever(); })
    };

    std::atomic<int> counter = 0;
    constexpr int target = 10000;
    for (int i = 0; i != target; ++i)
    {
        queue.spawn_task([&counter](){ counter += 1; });
    }

    // do _not
    while (counter != target)
    {
        std::this_thread::yield();
    }


    queue.close();
    for (auto& worker : workers)
    {
        worker.join();
    }

    return true;
}

bool inactive_wait_thread_test()
{
    // execute the test 10 times in naive hopes of catching races
    // FIXME: do this smarter
    for (int i = 0; i < 10; ++i)
    {
        if (!inactive_wait_thread_test_one())
        {
            return false;
        }
    }
    return true;
}

int main()
{
    return !(basic_test() && thread_test() && inactive_wait_thread_test());
}
