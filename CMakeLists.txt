cmake_minimum_required(VERSION 3.0)

project(jobxx)
enable_testing()

set(JOBXX_PUBLIC_HEADERS
    include/jobxx/job.h
    include/jobxx/queue.h
    include/jobxx/task.h
)
set(JOBXX_SOURCES
    source/job.cc
    source/queue.cc
    source/task.cc
)

set(JOBXX_FILES ${JOBXX_PUBLIC_HEADERS} ${JOBXX_SOURCES})

add_library(jobxx ${JOBXX_FILES})   
target_include_directories(jobxx PUBLIC "include")
set_property(TARGET jobxx PROPERTY CXX_STANDARD 14)