#ifndef QSO_QUEUE_H
#define QSO_QUEUE_H

#include <stdint.h>
#include "qso-input.h"

#define QUEUE_SIZE 128
#define SEEN_SIZE 1024

static qso_event queue[QUEUE_SIZE];

static unsigned int queue_head;
static unsigned int queue_tail;
static unsigned int queue_count;

static uint64_t seen_ids[SEEN_SIZE];
static unsigned int seen_next;

static int queue_push(qso_event event)
{
    for (unsigned int i = 0; i < SEEN_SIZE; i++)
        if (seen_ids[i] == event.id)
            return 0;

    if (queue_count == QUEUE_SIZE)
        return -1;

    queue[queue_tail] = event;
    queue_tail = (queue_tail + 1) % QUEUE_SIZE;
    queue_count++;

    seen_ids[seen_next] = event.id;
    seen_next = (seen_next + 1) % SEEN_SIZE;

    return 1;
}

static int queue_pop(qso_event *event)
{
    if (!queue_count)
        return 0;

    *event = queue[queue_head];

    queue_head = (queue_head + 1) % QUEUE_SIZE;
    queue_count--;

    return 1;
}

#endif
