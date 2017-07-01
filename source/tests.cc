
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
#include <vector>

// test utilities and helpers
namespace
{

    class worker_pool
    {
    public:
        explicit worker_pool(int threads)
        {
            for (int i = 0; i < threads; ++i)
            {
                _threads.emplace_back([this](){ _queue.work_forever(); });
            }
        }

        jobxx::queue& queue() { return _queue; }

        ~worker_pool()
        {
            _queue.close();
            for (auto& thread : _threads)
            {
                thread.join();
            }
        }

    private:
        jobxx::queue _queue;
        std::vector<std::thread> _threads;
    };

    static bool execute(bool(*test)(), int times = 1)
    {
        for (int i = 0; i < times; ++i)
        {
            if (!test())
            {
                return false;
            }
        }
        return true;
    }

    template <typename ContextT, typename FunctionT>
    static void spawn_n(ContextT& context, int count, FunctionT&& func)
    {
        for (int i = 0; i < count; ++i)
        {
            context.spawn_task(func);
        }
    }

}

// our tests
namespace
{

    // test the general queue/task/job system _without_ threads
    static bool basic_test()
    {
        jobxx::queue queue;

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

    // test background threads and the main thread actively working together
    static bool thread_test()
    {
        worker_pool pool(4);

        std::atomic<int> counter = 0;
        for (int inc = 1; inc != 5; ++inc)
        {
            spawn_n(pool.queue(), 1000, [&counter, inc](){ counter += inc; });
        }

        while (counter != (1000 + 2000 + 3000 + 4000))
        {
            pool.queue().work_all();
        }

        return true;
    }

    // test background threads working while the main thread does not execute tasks
    static bool inactive_wait_thread_test()
    {
        worker_pool pool(4);

        std::atomic<int> counter = 0;
        constexpr int target = 16;
        spawn_n(pool.queue(), target, [&counter]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            counter += 1;
        });

        // do _not_ wait actively here
        while (counter != target)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        return true;
    }

    static bool multi_queue_job_test()
    {
        worker_pool pool(2);

        std::atomic<int> counter = 0;
        constexpr int target = 16;

        jobxx::job job = pool.queue().create_job([&counter, &pool, target](auto& ctx)
        {
            spawn_n(ctx, target, [&counter]()
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                counter += 1;
            });
        });

        // wait for the job on a queue that will never run work for it
        jobxx::queue queue;
        queue.wait_job_actively(job);

        return true;
    }

}

int main()
{
    return !(
        execute(&basic_test, 10) &&
        execute(&thread_test) &&
        execute(&inactive_wait_thread_test) &&
        execute(&multi_queue_job_test)
    );
}
