cmake_minimum_required(VERSION 3.15)

set(PICO_BOARD pico)
set(PICO_PLATFORM rp2040)

include(pico_sdk_import.cmake)

project(231_firmware_template)

pico_sdk_init()

add_compile_options(-Wall)

add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/api/api.c                   # подключение модуля API
    ${CMAKE_CURRENT_SOURCE_DIR}/api/api-callbacks.c         # подключение внутренних функций модуля API
    ${CMAKE_CURRENT_SOURCE_DIR}/button-task/button-task.c   # подключение модуля кнопки
    ${CMAKE_CURRENT_SOURCE_DIR}/led-task/led-task.c         # подключение модуля управления светодиодом
    ${CMAKE_CURRENT_SOURCE_DIR}/pi-task/pi-task.c           # подключение модуля расчета ПИ
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}                 # подключение заголовочных файлов в основной папке проекта
    ${CMAKE_CURRENT_SOURCE_DIR}/api             # подключение заголовочных файлов модуля API
    ${CMAKE_CURRENT_SOURCE_DIR}/button-task     # подключение заголовочных файлов модуля кнопки
    ${CMAKE_CURRENT_SOURCE_DIR}/led-task        # подключение заголовочных файлов модуля управления светодиодом
    ${CMAKE_CURRENT_SOURCE_DIR}/pi-task         # подключение заголовочных файлов модуля расчета ПИ
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    device                  # подключение библиотеки паспорта устройства
    logging                 # подключение библиотеки журнала
    profiling               # подключение библиотеки профилирования
    stdio-text-protocol     # подключение библиотеки протокола
    systime                 # подключение библиотеки работы со временем
)

add_subdirectory(../libs ${CMAKE_BINARY_DIR}/libs)      # подключение папок библиотеки

# выключатель профилирования: 0 — вызовов секундомеров в образе нет
target_compile_definitions(profiling PUBLIC
    PROFILING_ENABLED=1
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)    # направляет поток вывода в USB
pico_enable_stdio_uart(${PROJECT_NAME} 0)   # выключает UART

pico_add_extra_outputs(${PROJECT_NAME})
