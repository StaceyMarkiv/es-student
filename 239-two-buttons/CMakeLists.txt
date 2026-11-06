cmake_minimum_required(VERSION 3.15)

set(PICO_BOARD pico)
set(PICO_PLATFORM rp2040)

include(pico_sdk_import.cmake)

project(239_two_buttons)

pico_sdk_init()

add_compile_options(-Wall)

# железо прибора: единственное место проекта с SDK
add_library(platform STATIC
    platform/platform.c
)
# путь к заголовкам — папка проекта, platform подключает pico_stdlib как PRIVATE;
# через этот путь logic находит и firmware.h
target_include_directories(platform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(platform PRIVATE
    pico_stdlib
)

# логика прибора: задачи и команды, без SDK
add_library(logic STATIC
    # исходники api, led-task, button-task и pi-task
    logic/api/api.c
    logic/api/api-callbacks.c
    logic/button-task/button-task.c
    logic/led-task/led-task.c
    logic/pi-task/pi-task.c
)
# путь к заголовкам — папка logic
target_include_directories(logic PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    logic
)
# stdio_text_protocol — PUBLIC, остальные библиотеки — PRIVATE
target_link_libraries(logic PUBLIC
    stdio-text-protocol     # подключение библиотеки протокола (она нужна пользователям библиотеки, и те получают её вместе с библиотекой)
)
target_link_libraries(logic PRIVATE
    device                  # подключение библиотеки паспорта устройства
    logging                 # подключение библиотеки журнала
    profiling               # подключение библиотеки профилирования
    systime                 # подключение библиотеки работы со временем
)

add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}                 # подключение заголовочных файлов в основной папке проекта
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    device                  # подключение библиотеки паспорта устройства
    logging                 # подключение библиотеки журнала
    profiling               # подключение библиотеки профилирования
    stdio-text-protocol     # подключение библиотеки протокола
    systime                 # подключение библиотеки работы со временем
    platform
    logic
)

add_subdirectory(../libs ${CMAKE_BINARY_DIR}/libs)      # подключение папок библиотеки

# выключатель профилирования: 0 — вызовов секундомеров в образе нет
target_compile_definitions(profiling PUBLIC
    PROFILING_ENABLED=1
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)    # направляет поток вывода в USB
pico_enable_stdio_uart(${PROJECT_NAME} 0)   # выключает UART

pico_add_extra_outputs(${PROJECT_NAME})
