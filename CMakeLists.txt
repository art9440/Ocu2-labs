cmake_minimum_required(VERSION 3.16)
project(ocu2labs C)


set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Общие флаги
add_compile_options(-Wall -Wextra -O0 -g)
add_definitions(-D_GNU_SOURCE)

# ===== Задание 1.6 =====
add_executable(task16
  1.6/main.c
  1.6/mythread.c
)
target_include_directories(task16 PRIVATE 1.6)
target_link_libraries(task16 PRIVATE pthread)


# ===== Задание 2.3 =====
add_executable(task23
  2.3/mutex/main_mtx.c
  2.3/mutex/threads.c
  2.3/mutex/storage.c
)

target_include_directories(task23 PRIVATE 2.3)
target_link_libraries(task23 PRIVATE pthread)