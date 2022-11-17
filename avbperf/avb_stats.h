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

typedef struct avb_stats_node{

    struct timespec arrival;
    struct timespec presentation;
    struct avb_stats_node *next;

}avb_stats_node_t;

typedef struct {

    avb_stats_node_t *head;
    uint32_t dropped_packets;

}avb_stats_t;


void avb_stats_init(avb_stats_t *self);

void avb_stats_clear(avb_stats_t *self);

void avb_stats_add_dropped(avb_stats_t *self, uint8_t count);

int avb_stats_push(avb_stats_t *self, struct timespec *arrival, struct timespec *presentation);

int avb_stats_get_first(avb_stats_t *self, avb_stats_node_t *first);

int avb_stats_get_last(avb_stats_t *self, avb_stats_node_t *last);

int avb_stats_get_cap_time(avb_stats_t *self, avb_stats_node_t *node, stream_settings_t *set, 
                                                                    struct timespec *cap_time);

int avb_stats_print_stats(FILE* file, avb_stats_t *self, stream_settings_t *set);

#endif // _AVB_STATS_H_
