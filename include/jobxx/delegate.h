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

#include <utility>
#include <type_traits>

namespace jobxx
{

    class delegate
    {
    public:
        static constexpr int max_size = sizeof(void*) * 3;
        static constexpr int max_alignment = alignof(double);

		delegate() = default;

		delegate(delegate&& rhs) = default;
		delegate& operator=(delegate&& rhs) = delete;

        template <typename FunctionT> /*implicit*/ delegate(FunctionT&& func) { _assign(std::forward<FunctionT>(func)); }

        explicit operator bool() const { return _thunk != nullptr; }

        void operator()() { _thunk(&_storage); }

    private:
		template <typename FunctionT> void _assign(FunctionT&& func);

        void(*_thunk)(void*) = nullptr;
		std::aligned_storage_t<max_size, max_alignment> _storage;
    };

	template <typename FunctionT>
	void delegate::_assign(FunctionT&& func)
	{
		static_assert(sizeof(FunctionT) <= max_size, "function too large for jobxx::delegate");
		static_assert(alignof(FunctionT) <= max_alignment, "function over-aligned for jobxx::delegate");
		static_assert(std::is_trivially_move_constructible_v<FunctionT>, "function not a trivially move-constructible as required by jobxx::delegate");
		static_assert(std::is_trivially_destructible_v<FunctionT>, "function not a trivially destructible as required by jobxx::delegate");

		_thunk = [](void* f){ (*static_cast<FunctionT*>(f))(); };
		new (&_storage) FunctionT(std::forward<FunctionT>(func));
	}

}

#endif // defined(_guard_JOBXX_DELEGATE_H)
