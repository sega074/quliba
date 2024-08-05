project(qumpoints VERSION 0.1.0)

set (THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

add_executable(qumpoints main_qump.cpp)

target_link_libraries(qumpoints PRIVATE Threads::Threads)

install (TARGETS qumpoints RUNTIME DESTINATION bin)