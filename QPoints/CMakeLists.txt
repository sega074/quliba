project(qpoints VERSION 0.1.0)

set (THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

add_executable(qpoints main_qp.cpp)

target_link_libraries(qpoints PRIVATE Threads::Threads)

install (TARGETS qpoints RUNTIME DESTINATION bin)