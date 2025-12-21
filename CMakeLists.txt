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

# ===== Задание 1.7 =====
add_executable(task17
  1.7/main.c
  1.7/uthread.c
)
target_include_directories(task17 PRIVATE 1.7)

# ===== Задание 2.3 =====
add_executable(task23_mutex
  2.3/mutex/main_mtx.c
  2.3/mutex/threads.c
  2.3/mutex/storage.c
)

target_include_directories(task23_mutex PRIVATE 2.3/mutex)
target_link_libraries(task23_mutex PRIVATE pthread)

add_executable(task23_spin
  2.3/spin/main_spin.c
  2.3/spin/threads.c
  2.3/spin/storage.c
)

target_include_directories(task23_spin PRIVATE 2.3/spin)
target_link_libraries(task23_spin PRIVATE pthread)

add_executable(task23_rw
  2.3/rw/main_rw.c
  2.3/rw/threads.c
  2.3/rw/storage.c
)

target_include_directories(task23_rw PRIVATE 2.3/rw)
target_link_libraries(task23_rw PRIVATE pthread)

# ===== Задание 2.4 =====
add_executable(task24_spin
  2.4/spin/main_spin.c
  2.4/spin/spinlock.c
)
target_include_directories(task24_spin PRIVATE 2.4)
target_link_libraries(task24_spin PRIVATE pthread)

add_executable(task24_mutex
  2.4/mutex/main_mutex.c
  2.4/mutex/mutex.c
)
target_include_directories(task24_mutex PRIVATE 2.4)
target_link_libraries(task24_mutex PRIVATE pthread)
