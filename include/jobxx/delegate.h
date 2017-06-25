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

#if !defined(_guard_JOBXX_DELEGATE_H)
#define _guard_JOBXX_DELEGATE_H
#pragma once

namespace jobxx
{

    class delegate
    {
    public:
        template <typename FunctionT>
        /*implicit*/ delegate(FunctionT&& func)
        {
            _thunk = [](void* f){ (*static_cast<FunctionT*>(f))(); }
            _function = &func;
            // FIXME: won't work with types that overload operator&,
            // but I don't want to pull in <memory> if I can avoid it
        }

        explicit operator bool() const { return _thunk != nullptr; }

        void operator()() { _thunk(_function); }

    private:
        void(*_thunk)(void*) = nullptr;
        void* _function = nullptr;
    };

}

#endif // defined(_guard_JOBXX_DELEGATE_H)
