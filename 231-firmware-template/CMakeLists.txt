cmake_minimum_required(VERSION 3.15)

set(PICO_BOARD pico)
set(PICO_PLATFORM rp2040)

include(pico_sdk_import.cmake)

project(231_firmware_template)

pico_sdk_init()

add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/api/api.c                                   # подключение модуля API
    ${CMAKE_CURRENT_SOURCE_DIR}/api/api-callbacks.c                         # подключение внутренних функций модуля API
    ${CMAKE_CURRENT_SOURCE_DIR}/led/led.c                                   # подключение модуля управления светодиодом
    ${CMAKE_CURRENT_SOURCE_DIR}/logging/log.c                               # подключение модуля журнала
    ${CMAKE_CURRENT_SOURCE_DIR}/device/device.c                             # подключение модуля паспорта устройства
    ${CMAKE_CURRENT_SOURCE_DIR}/profiling/profiling.c                       # подключение модуля профилирования
    ${CMAKE_CURRENT_SOURCE_DIR}/stdio-text-protocol/stdio-text-protocol.c   # подключение модуля протокола
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}                         # подключение заголовочных файлов в основной папке проекта
    ${CMAKE_CURRENT_SOURCE_DIR}/api                     # подключение заголовочных файлов модуля API
    ${CMAKE_CURRENT_SOURCE_DIR}/led                     # подключение заголовочных файлов модуля управления светодиодом
    ${CMAKE_CURRENT_SOURCE_DIR}/logging                 # подключение заголовочных файлов модуля журнала
    ${CMAKE_CURRENT_SOURCE_DIR}/device                  # подключение заголовочных файлов модуля паспорта устройства
    ${CMAKE_CURRENT_SOURCE_DIR}/profiling               # подключение заголовочных файлов модуля профилирования
    ${CMAKE_CURRENT_SOURCE_DIR}/stdio-text-protocol     # подключение заголовочных файлов модуля протокола
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    pico_unique_id
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    DEVICE_BOARD="${PICO_BOARD}"
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)            # направляет поток вывода в USB
pico_enable_stdio_uart(${PROJECT_NAME} 0)           # выключает UART

pico_add_extra_outputs(${PROJECT_NAME})
