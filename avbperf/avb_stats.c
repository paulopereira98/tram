/* 
 * Copyright (C) 2022 Paulo Pereira, Uminho <github.com/paulopereira98>
 */

#include <sys/timerfd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "avb_stats.h"

#define NSEC_PER_SEC		1000000000ULL
#define NSEC_PER_MSEC		1000000ULL

void avb_stats_init(avb_stats_t *self)
{
    self->head = NULL;
    self->dropped_packets = 0;
}

static void avb_stats_delete_recursive(avb_stats_node_t *node)
{
    if(node->next != NULL)
        avb_stats_delete_recursive(node->next);
    free(node);
}   

void avb_stats_clear(avb_stats_t *self)
{
    avb_stats_delete_recursive(self->head);
}

void avb_stats_add_dropped(avb_stats_t *self, uint8_t count)
{
    self->dropped_packets += count;
}
int avb_stats_push(avb_stats_t *self, struct timespec *arrival, struct timespec *presentation)
{
    avb_stats_node_t *node = malloc(sizeof(avb_stats_node_t));

    if(!node)
        return -1;

    node->arrival = *arrival;
    node->presentation = *presentation;

    node->next = self->head;
    self->head = node;

    return 0;
}

int avb_stats_get_first(avb_stats_t *self, avb_stats_node_t *first)
{
    avb_stats_node_t *node_ptr;
    node_ptr = self->head;

    if (node_ptr == NULL)
        return -1;

    // find bottom entry
    while(node_ptr->next)
        node_ptr = node_ptr->next;

    memcpy(first, node_ptr, sizeof(avb_stats_node_t));

    return 0;
}

int avb_stats_get_last(avb_stats_t *self, avb_stats_node_t *last)
{
    if (self->head == NULL)
        return -1;

    memcpy(last, self->head, sizeof(avb_stats_node_t));

    return 0;
}

int avb_stats_get_cap_time(avb_stats_t *self, avb_stats_node_t *node, stream_settings_t *set, 
                                                                    struct timespec *cap_time)
{
    // capture = presentation - transit - hw_latency
    uint64_t capture_ns;

    if( !(self||node||set||cap_time) )
        return -1;
    capture_ns = (node->presentation.tv_nsec * NSEC_PER_SEC) + node->presentation.tv_nsec;
    capture_ns -= set->max_transit_time * NSEC_PER_MSEC;

    // latency is in samples
    capture_ns -= ((float)set->hw_latency / set->sample_rate) * NSEC_PER_SEC;

    cap_time->tv_sec = capture_ns / NSEC_PER_SEC;
	cap_time->tv_nsec = capture_ns % NSEC_PER_SEC;

    return 0;
} 

int avb_stats_print_stats(FILE* file, avb_stats_t *self, stream_settings_t *stats)
{
    //TODO: calculate statistics from aquired data
    // fprintf(file, "n");
    return 0;
}
