
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
#include "jobxx/_detail/job.h"

namespace jobxx
{

	job::~job()
	{
		if (_impl != nullptr && 0 == --_impl->refs)
		{
			delete _impl;
		}
	}

    job& job::operator=(job&& rhs)
    {
        if (this != &rhs)
        {
			if (_impl != nullptr && 0 == --_impl->refs)
			{
				delete _impl;
			}

            _impl = rhs._impl;
            rhs._impl = nullptr;

			if (_impl != nullptr)
			{
				++_impl->refs;
			}
        }

        return *this;
    }

    bool job::complete() const
    {
        return _impl == nullptr || _impl->tasks == 0;
    }

	_detail::job* job::_get_impl()
	{
		// FIXME: this is racy if two threads try to spawn
		// tasks in an empty job at the same time.
		if (_impl == nullptr)
		{
			_impl = new _detail::job;
		}

		return _impl;
	}

}
