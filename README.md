# jobxx

## License

Copyright (c) 2017 Sean Middleditch <sean.middleditch@gmail.com>

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>

## Summary

Simple task and job scheduling library for C++. The library aims for
reasonably good performance, low overhead, light memory usage, and for
suitability for games and soft-real-time uses.

## Documentation

### Design

jobxx aims to be as light-weight as possible while retaining strong
performance. To this end, the basic primitives of the library are
few and focused on minimalism.

The core concepts of jobxx are *jobs*, *tasks*, *queues*, and *threads*.
Of the four, jobxx only directly represents the first three; threads
are provided by `std::thread` or the application.

The *task* is the lowest-level primitive of the core concepts. A task
represents a unit of work. Tasks have no return values nor error states.
If applications wish to communicate results from a task, they must use
an external mechanism such as `std::future`. jobxx tasks are best suited
for small discrete chunks of work with no failure state or individual
results, though of course having a task mutate some shared state (with
the appropriate care for thread-safety) is a common use case. For instance,
spawning a number of tasks to mutate a large array - where each task
operates on a distinct subrange of the array - is an excellent case.

A *job* is a collection of tasks. jobxx allows users to create a job
that spawns 0 or more tasks. The job maintains a completion state which
is unset while the job has 1 or more tasks that have not yet been
completed. Tasks spawned by a job may themselves spawn more tasks as
part of the same job, allowing a job to represent the completion state
of an entire tree of tasks, sub-tasks, and continuations. An application
can either poll a job's completion state or block on its completion,
working on tasks (if available) while waiting, or putting the thread to
sleep until the job is ready.

Scheduling tasks is performed by a *queue*. A queue is essentially
a list of tasks that have been spawned and are ready to execute. Any
number of threads may poll a queue for tasks to execute. Additionally,
queues contain a mechanism for *parking* threads; this is a way for
a thread to sleep/block until a particular queue has work available. The
parking approach allows for task creation to be efficient (no need to
signal the OS if there are no sleeping threads) and is extensible for
future needs, such as parking a thread on a job until it completes (not
yet supported).

### API

The two primary points of the api are `jobxx::queue` and `jobxx::job`.

#### `jobxx::queue`

A `jobxx::queue` is used to spawn tasks, execute spawned tasks, to
create `job` instances, and to wait for jobs to complete.

##### `queue::spawn_task(delegate work) -> void`

Create a new task encapsulating the `work` to be performed. The task
is put into a pending task queue and will be executed when a thread
calls `queue::work_one`.

##### `queue::create_job(initializer : (context&) -> void) -> job`

Creates a new `job` instance and then invokes `initializer` with
a `context` object. Tasks spawned via this context will be added
to the returned `job` as child tasks.

#### `jobxx::job`

A `jobxx::job` represents the completion state of a set of tasks.
The job can be queried to see if all tasks spawned for the job have
been fully executed.

##### `job::complete() const -> bool`

Returns `true` if there are no outstanding tasks associated with the
job.

##### `job::operator bool() const`

Same as `job::complete`.

#### `jobxx::context`

A context allows for spawning tasks as part of a `job`.

##### `context::spawn_task(delegate work) -> void`

As `queue::spawn_task`, except that the spawned task will be associated
with the context's job.

#### `jobxx::delegate`

A `delegate` is very similar to `std::function` with two primary
differences. First, a `delegate` is guaranteed to never allocate. It is
a compile-time error to attempt to store a function object (or lambda, or
other invokable) into a `delegate` if it is too large or overly aligned.

Second, a `delegate` can wrap an invokable with one of two different
potential signatures: `() -> void` or `(context&) -> void`. This allows
for convenience when needing to construct a task which has no need for a
`context` while still allowing for tasks which do need a `context`.

Note: the current incarnation of `delegate` only works for function
objects which are trivially move-constructible and trivially destructible.
This limitation is planned to be lifted in future releases.

##### `delegate::delegate(function: () -> void)`

Constructs a delegate wrapping `function`.

##### `delegate::delegate(function: (context&) -> void)`

Constructs a delegate wrapping `function`.

##### `delegate::operator bool() const`

Returns `true` if the delegate has been created with a function, or `false`
for a default-constructed (empty) delegate.

##### `delegate::operator()(ctx: context&) -> void`

Executes the stored function, passing `ctx` to it if the stored function
takes a `context&` parameter. It is undefined behavior to call this
operator on a `delegate` with no stored function.

#### `jobxx::predicate`

A `predicate` is simple wrapper for a function reference of signature
`() -> bool`. Note that it is only a _reference_ type, meaning that it
does not take ownership of a function object used to construct it.

##### `predicate::predicate(pred: () -> bool)`

Creates a predicate wrapping the given `pred` invokable.

##### `predicate::operator bool() const`

Returns `true` if the predicate was constructed with a function, or `false`
if the predicate has no function reference.

##### `predicate::operator()() -> bool`

Invokes the reference function and returns its result. It is undefined
behavior to call this operator on a `predicate` with no stored
function reference.

