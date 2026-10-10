#pragma once

#define SRAM_SIZE (264 * 1024)
#define ROM_SIZE (16 * 1024)
#define VECTOR_TABLE 0x10000100

void mem_info(void);
void fw_info(void);
void boot_info(void);