#pragma once

#include "stdio-text-protocol/stdio-text-protocol.h"
#include "stream-task/stream-task.h"
#include "telemetry-task/telemetry-task.h"

void api_handle(const command_t *command);

// печатает запись телеметрии
void api_print_telemetry(const telemetry_record_t *record);

// захват DMA
void print_dma_cap(const uint16_t *samples, uint32_t count, uint32_t time_us);

// отправка кадра
void api_send_frame(const stream_header_t *header, const uint8_t *data, uint32_t bytes);
