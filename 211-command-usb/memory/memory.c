#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "hardware/regs/addressmap.h"
#include "pico/stdlib.h"

#include "memory.h"
#include "device.h"
#include "command.h"

int main(void);

uint32_t data_variable = 100;
uint32_t bss_variable;

// печать строки данных для функции
static void fw_row_func(const char *obj_name, uintptr_t address)
{
    // адрес функции со сброшенным признаком Thumb
    uint16_t *addr = (uint16_t *)(address & ~1u);

    printf("%-15s 0x%08x 0x%04x\n",
           obj_name, (unsigned)address, *(uint16_t *)addr);
}

// печать строки данных для структуры
static void fw_row_struct(const char *obj_name, uintptr_t address, bool is_struct_item)
{
    printf(is_struct_item ? "- %-13s 0x%08x\n" : "%-15s 0x%08x\n",
           obj_name, (unsigned)address);
}

// печать строки данных для переменной
static void fw_row_var(const char *obj_name, uintptr_t address)
{
    printf("%-15s 0x%08x %u\n",
           obj_name, (unsigned)address, (unsigned)*(uint32_t *)address);
}

// печать строки данных для константы
static void fw_row_const(const char *obj_name, uintptr_t address)
{
    printf("%-15s 0x%08x %s\n",
           obj_name, (unsigned)address, (char *)address);
}

void fw_info(void)
{
    // считаем вызов: data_variable и bss_variable на единицу больше
    data_variable++;
    bss_variable++;

    // адреса функций со сброшенным признаком Thumb
    uint16_t *main_code = (uint16_t *)((uintptr_t)main & ~1u);

    // локальная переменная и блок из кучи
    uint32_t stack_variable = 1946;

    uint32_t *heap_variable = malloc(sizeof(uint32_t));
    if (heap_variable != NULL)
    {
        *heap_variable = 1951;
    }

    // шапка: объект, адрес, значение
    printf("%-15s %-10s %s\n", "object", "address", "value");

    // main, fw_info  — адрес с признаком Thumb и два байта по сброшенному адресу
    fw_row_func("main", (uintptr_t)main);
    fw_row_func("fw_info", (uintptr_t)fw_info);
    // commands       — адрес массива
    fw_row_struct("commands", (uintptr_t)&commands, false);
    // обработчики    — имя команды и адрес обработчика, строкой на команду
    for (uint i = 0; i < command_count; i++)
    {
        fw_row_struct(commands[i].name, (uintptr_t)commands[i].handler, true);
    }
    // константы      — адрес и значение строк паспорта из device.h
    fw_row_const("DEVICE_PROJECT", (uintptr_t)DEVICE_PROJECT); // wrong
    fw_row_const("DEVICE_BOARD", (uintptr_t)DEVICE_BOARD);     // wrong
    // data_variable  — адрес и значение, секция .data
    fw_row_var("data_variable", (uintptr_t)&data_variable);
    // bss_variable   — адрес и значение, секция .bss
    fw_row_var("bss_variable", (uintptr_t)&bss_variable);
    // stack_variable — адрес и значение
    fw_row_var("stack_variable", (uintptr_t)&stack_variable);
    // heap_variable  — адрес и значение
    fw_row_var("heap_variable", (uintptr_t)heap_variable); // wrong

    // возвращаем блок кучи
    free(heap_variable);
}

extern char __flash_binary_start;
extern char __flash_binary_end;
extern char __boot2_start__;
extern char __boot2_end__;
extern char __etext;
extern char __data_start__;
extern char __data_end__;
extern char __bss_start__;
extern char __bss_end__;
extern char __HeapLimit;
extern char __StackBottom;
extern char __StackTop;

static void row(const char *name, uintptr_t start, uintptr_t end)
{
    printf("%-10s 0x%08x 0x%08x %8u\n",
           name, (unsigned)start, (unsigned)end, (unsigned)(end - start));
}

void mem_info(void)
{
    // шапка таблицы: область, начало, конец, размер
    printf("%-10s %-10s %-10s %-10s\n", "area", "start", "end", "size");

    // flash — XIP_BASE и PICO_FLASH_SIZE_BYTES
    row("flash", (uintptr_t)XIP_BASE, (uintptr_t)(XIP_BASE + PICO_FLASH_SIZE_BYTES));
    // sram — базовый адрес из SDK, размер из datasheet
    row("sram", (uintptr_t)SRAM_BASE, (uintptr_t)(SRAM_BASE + SRAM_SIZE));
    // rom — базовый адрес из SDK, размер из datasheet
    row("rom", (uintptr_t)ROM_BASE, (uintptr_t)(ROM_BASE + ROM_SIZE));

    // image — от __flash_binary_start до __flash_binary_end
    row("image", (uintptr_t)&__flash_binary_start, (uintptr_t)&__flash_binary_end);
    // free  — от __flash_binary_end до конца флеш-памяти
    row("free", (uintptr_t)&__flash_binary_end, (uintptr_t)(XIP_BASE + PICO_FLASH_SIZE_BYTES));
    // boot2 — от __boot2_start__ до __boot2_end__
    row("boot2", (uintptr_t)&__boot2_start__, (uintptr_t)&__boot2_end__);
    // text  — от __boot2_end__ до __etext: код и константы
    row("text", (uintptr_t)&__boot2_end__, (uintptr_t)&__etext);

    // data flash — хранение .data, от __etext, длиной с .data
    row("data flash", (uintptr_t)&__etext, (uintptr_t)(&__etext + (&__data_end__ - &__data_start__)));
    // data ram   — работа .data, от __data_start__ до __data_end__
    row("data ram", (uintptr_t)&__data_start__, (uintptr_t)&__data_end__);
    // bss        — от __bss_start__ до __bss_end__
    row("bss", (uintptr_t)&__bss_start__, (uintptr_t)&__bss_end__);
    // heap       — от __bss_end__ до __HeapLimit
    row("heap", (uintptr_t)&__bss_end__, (uintptr_t)&__HeapLimit);
    // stack      — от __StackBottom до __StackTop
    row("stack", (uintptr_t)&__StackBottom, (uintptr_t)&__StackTop);

    printf("\ntotal\n");

    // итог: образ во флеш и из чего он сложился
    unsigned boot_size = (unsigned)((uintptr_t)&__boot2_end__ - (uintptr_t)&__boot2_start__);
    unsigned text_size = (unsigned)((uintptr_t)&__etext - (uintptr_t)&__boot2_end__);
    unsigned data_size = (unsigned)((uintptr_t)&__data_end__ - (uintptr_t)&__data_start__);
    unsigned flash_image_total = boot_size + text_size + data_size;
    printf("%-10s %8u = boot2 %u + text %u + data %u\n", "flash image", flash_image_total, boot_size, text_size, data_size);

    // итог: свободно во флеш-памяти из всего её объёма
    printf("%-10s %8u of %u\n", "flash free", PICO_FLASH_SIZE_BYTES - flash_image_total, PICO_FLASH_SIZE_BYTES);

    // итог: занято в ОЗУ — .data и .bss
    unsigned bss_size = (unsigned)((uintptr_t)&__bss_end__ - (uintptr_t)&__bss_start__);
    unsigned ram_used = data_size + bss_size;
    printf("%-10s %8u = data %u + bss %u\n", "ram used", ram_used, data_size, bss_size);

    // итог: свободно в ОЗУ — под кучу и под стек
    unsigned heap_size = (unsigned)((uintptr_t)&__HeapLimit - (uintptr_t)&__bss_end__);
    unsigned stack_size = (unsigned)((uintptr_t)&__StackTop - (uintptr_t)&__StackBottom);
    printf("%-10s %8u for heap and %u for stack\n", "ram free", heap_size, stack_size);
}