project(qupoints VERSION 0.1.0)

set (THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

add_executable(qupoints main_qup.cpp)

target_link_libraries(qupoints PRIVATE Threads::Threads)

install (TARGETS qupoints RUNTIME DESTINATION bin)