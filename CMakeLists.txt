cmake_minimum_required(VERSION 3.0)

project(jobxx)
enable_testing()

set(JOBXX_PUBLIC_HEADERS
    include/jobxx/context.h
    include/jobxx/delegate.h
    include/jobxx/job.h
	include/jobxx/predicate.h
    include/jobxx/queue.h
)
set(JOBXX_PRIVATE_HEADERS
    include/jobxx/_detail/concurrent_queue.h
    include/jobxx/_detail/job.h
    include/jobxx/_detail/queue.h
    include/jobxx/_detail/task.h
)
set(JOBXX_SOURCES
    source/context.cc
    source/job.cc
    source/queue.cc
)
set(JOBXX_TESTS
    source/tests.cc
)

set(JOBXX_FILES ${JOBXX_PUBLIC_HEADERS} ${JOBXX_PRIVATE_HEADERS} ${JOBXX_SOURCES})

add_library(jobxx ${JOBXX_FILES})   
target_include_directories(jobxx PUBLIC "include")
source_group("Header Files\\_detail" FILES ${JOBXX_PRIVATE_HEADERS})
set_property(TARGET jobxx PROPERTY CXX_STANDARD 14)

add_executable(jobxx_tests ${JOBXX_TESTS})
set_property(TARGET jobxx_tests PROPERTY CXX_STANDARD 14)
target_link_libraries(jobxx_tests jobxx)
add_test(jobxx_tests jobxx_tests)
