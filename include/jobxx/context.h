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

#if !defined(_guard_JOBXX_CONTEXT_H)
#define _guard_JOBXX_CONTEXT_H
#pragma once

#include "delegate.h"

namespace jobxx
{

    namespace _detail
    {
        struct job_impl;
        struct queue_impl;
    }

    enum class spawn_result;

    class context
    {
    public:
        explicit context(_detail::queue_impl& queue, _detail::job_impl* parent) : _queue(queue), _job(parent) {}

        context(context const&) = delete;
        context& operator=(context const&) = delete;

        spawn_result spawn_task(delegate&& work);

    private:
        _detail::queue_impl& _queue;
        _detail::job_impl* _job = nullptr;
    };

}

#endif // defined(_guard_JOBXX_CONTEXT_H)
