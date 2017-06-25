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

#if !defined(_guard_JOBXX_QUEUE_H)
#define _guard_JOBXX_QUEUE_H
#pragma once

namespace jobxx
{

    class job;

    class queue
    {
    public:
        queue();
        ~queue();

        queue(queue const&) = delete;
        queue& operator=(queue const&) = delete;

        template <typename FunctionT> job spawn_job(FunctionT&& func);
        template <typename FunctionT> job& spawn_task(job& parent, FunctionT&& func);
        template <typename FunctionT> void spawn_task(FunctionT&& func);

        void wait_job_actively(job const& awaited);

        bool work_one();
        void work_all();

    private:
        struct impl;

        job _spawn_job(void(*)(void*), void*);
        void _spawn_task(void(*)(void*), void*, job*);

        impl* _impl = nullptr;
    };

    template <typename FunctionT>
    job queue::spawn_job(FunctionT&& func)
    {
        return _spawn_job([](void* d){ (*static_cast<FunctionT*>(d))(); }, std::addressof(func));
    }

    template <typename FunctionT>
    job& queue::spawn_task(job& parent, FunctionT&& func)
    {
        _spawn_task([](void* d){ (*static_cast<FunctionT*>(d))(); }, std::addressof(func), &parent);
        return parent;
    }

    template <typename FunctionT>
    void queue::spawn_task(FunctionT&& func)
    {
        _spawn_task([](void* d){ (*static_cast<FunctionT*>(d))(); }, std::addressof(func), nullptr);
    }

}

#endif // defined(_guard_JOBXX_QUEUE_H)
