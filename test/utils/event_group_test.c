/*
 * Copyright (C) 2026 SpacemiT (Hangzhou) Technology Co. Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "event_group.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#define SCHEDULE_EVENT         (1 << 0)
#define AUDIO_INPUT_READY      (1 << 1)
#define AUDIO_OUTPUT_READY     (1 << 2)

void* sensor_thread(void* arg) {
    struct event_group* eg = (struct event_group*)arg;
    sleep(1);
    printf("[Sensor] Audio input ready\n");
    event_group_set_bit(eg, AUDIO_INPUT_READY);

    sleep(2);
    printf("[Sensor] Schedule event\n");
    event_group_set_bit(eg, SCHEDULE_EVENT);

    return NULL;
}

int main() {
    struct event_group* eg = event_group_create();
    pthread_t tid;
    pthread_create(&tid, NULL, sensor_thread, eg);

    while (1) {
        unsigned char bits =  event_group_wait(eg,
            AUDIO_INPUT_READY | SCHEDULE_EVENT | AUDIO_OUTPUT_READY,
            AUDIO_INPUT_READY | SCHEDULE_EVENT | AUDIO_OUTPUT_READY, 0, 0xFFFF);
        if (bits & AUDIO_INPUT_READY) {
            printf("[Main] Processing audio input\n");
        }
        if (bits & SCHEDULE_EVENT) {
            printf("[Main] Handling scheduled task\n");
        }
        if (bits & AUDIO_OUTPUT_READY) {
            printf("[Main] Outputting audio\n");
        }
    }

    pthread_join(tid, NULL);
    event_group_delete(eg);
    return 0;
}
