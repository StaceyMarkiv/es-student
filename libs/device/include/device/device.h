#pragma once

#include <stdint.h>

#ifndef DEVICE_BOARD
#define DEVICE_BOARD "unknown"
#endif

// тип со сведениями о плате: тип платы, серийный номер, производитель, изделие и ревизия кристалла, версия SDK
typedef struct
{
    char *board;
    char *serial;
    uint32_t manufacturer;
    uint32_t part;
    uint32_t revision;
    char *pico_sdk;
} device_info_t;

void device_get_info(device_info_t *info);
