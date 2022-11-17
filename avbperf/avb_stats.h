/* 
 * Copyright (C) 2022 Paulo Pereira, Uminho <github.com/paulopereira98>
 */

#ifndef _AVB_STATS_H_
#define _AVB_STATS_H_

#include <sys/timerfd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "avb_aaf.h"

typedef struct avb_stats_data_node{

    struct timespec arrival;
    struct timespec presentation;
    struct avb_stats_data_node *next;

}avb_stats_data_node_t;

typedef struct{
    uint64_t min, max, mean, std, count;
}avb_stats_stat_t;

typedef struct {

    avb_stats_data_node_t *stack_head;
    uint32_t dropped_packets;

    avb_stats_stat_t delta_cap;
    avb_stats_stat_t travel;

}avb_stats_handler_t;


void avb_stats_handler_init(avb_stats_handler_t *self);

void avb_stats_stat_init(avb_stats_stat_t *self);

void avb_stats_clear(avb_stats_handler_t *self);

void avb_stats_add_dropped(avb_stats_handler_t *self, uint8_t count);

int avb_stats_push(avb_stats_handler_t *self, struct timespec *arrival, struct timespec *presentation);

int avb_stats_get_first(avb_stats_handler_t *self, avb_stats_data_node_t *first);

int avb_stats_get_last(avb_stats_handler_t *self, avb_stats_data_node_t *last);

int avb_stats_get_cap_time(avb_stats_data_node_t *node, stream_settings_t *set, 
                                                                    struct timespec *cap_time);

int avb_stats_process_stats(avb_stats_handler_t *self, stream_settings_t *set);

void avb_stats_print_stats(FILE* file, avb_stats_handler_t *self);

#endif // _AVB_STATS_H_
