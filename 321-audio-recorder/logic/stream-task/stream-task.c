#include "stream-task.h"
#include "platform/platform.h"

#include <stddef.h>

static stream_state_t stream_state;
static stream_task_callback_t on_frame_func;
static uint8_t stream_bits;
static stream_header_t stream_header;
static uint32_t samples_total_count;

// запомнить функцию для кадров, состояние IDLE
void stream_task_init(stream_task_callback_t on_frame)
{
    on_frame_func = on_frame;
    stream_state = STREAM_STATE_IDLE;
}

bool stream_task_start(uint8_t bits)
{
    // поток уже идёт
    if (stream_state == STREAM_STATE_RUN)
    {
        return false;
    }

    // запомнить разрядность, номер кадра и итог — с нуля, состояние RUN
    stream_bits = bits;
    stream_header.number = 0;
    samples_total_count = 0;
    stream_state = STREAM_STATE_RUN;

    // platform не начал поток — false
    return platform_stream_start(stream_bits);
}

// остановить поток в platform, состояние IDLE
void stream_task_stop(void)
{
    platform_stream_stop();
    stream_state = STREAM_STATE_IDLE;
}

void stream_task_handle(void)
{
    // в состоянии RUN, если platform отдал буфер:
    if (stream_state == STREAM_STATE_RUN)
    {
        uint32_t samples;
        const uint8_t *buffer = platform_stream_next(&samples);

        if (buffer != NULL)
        {
            // заголовок — синхрослово STREAM_SYNC_WORD, номер, число отсчётов
            stream_header.sync_word = STREAM_SYNC_WORD;
            stream_header.count = (uint16_t)samples;

            // байтов — отсчётов × 2 при 16 битах, отсчётов при 8
            uint32_t bytes = (stream_bits == 16) ? samples * 2 : samples;

            if (on_frame_func != NULL)
            {
                // отдать заголовок и буфер функции, буфер вернуть platform
                on_frame_func(&stream_header, buffer, bytes);
                platform_stream_release();

                // номер, число кадров и отсчётов — вперёд
                stream_header.number++;
                samples_total_count += samples;
            }
        }
    }
}

stream_state_t stream_task_get_state(void)
{
    return stream_state;
}

uint8_t stream_task_get_bits(void)
{
    return stream_bits;
}

uint32_t stream_task_get_frames(void)
{
    return stream_header.number;
}

uint32_t stream_task_get_samples(void)
{
    return samples_total_count;
}

uint32_t stream_task_get_overruns(void)
{
    return platform_stream_overruns();
}
