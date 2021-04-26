cmake_minimum_required(VERSION 3.19)
project(Semaforos_en_el_buzon_circular C)

set(CMAKE_C_STANDARD 11)

add_executable(Semaforos_en_el_buzon_circular
        initializer.c producer.c consumer.c finalizer.c)
