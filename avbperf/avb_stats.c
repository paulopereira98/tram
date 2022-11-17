/* 
 * Copyright (C) 2022 Paulo Pereira, Uminho <github.com/paulopereira98>
 */

#include <sys/timerfd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "avb_stats.h"

#define NSEC_PER_SEC		1000000000ULL
#define NSEC_PER_MSEC		1000000ULL

void avb_stats_handler_init(avb_stats_handler_t *self)
{
    self->stack_head = NULL;
    self->dropped_packets = 0;

    avb_stats_stat_init( &(self->delta_cap) );
    avb_stats_stat_init( &(self->travel) );
}

void avb_stats_stat_init(avb_stats_stat_t *self)
{
    self->min = (uint64_t)-1;
    self->max = 0;
    self->mean = 0;
    self->std = 0;
    self->count = 0;
}

static void avb_stats_delete_recursive(avb_stats_data_node_t *node)
{
    if(node->next != NULL)
        avb_stats_delete_recursive(node->next);
    free(node);
}   

void avb_stats_clear(avb_stats_handler_t *self)
{
    avb_stats_delete_recursive(self->stack_head);
}

void avb_stats_add_dropped(avb_stats_handler_t *self, uint8_t count)
{
    self->dropped_packets += count;
}
int avb_stats_push(avb_stats_handler_t *self, struct timespec *arrival, struct timespec *presentation)
{
    avb_stats_data_node_t *node = malloc(sizeof(avb_stats_data_node_t));

    if(!node)
        return -1;

    node->arrival = *arrival;
    node->presentation = *presentation;

    node->next = self->stack_head;
    self->stack_head = node;

    return 0;
}

int avb_stats_get_first(avb_stats_handler_t *self, avb_stats_data_node_t *first)
{
    avb_stats_data_node_t *node_ptr;
    node_ptr = self->stack_head;

    if (node_ptr == NULL)
        return -1;

    // find bottom entry
    while(node_ptr->next)
        node_ptr = node_ptr->next;

    memcpy(first, node_ptr, sizeof(avb_stats_data_node_t));

    return 0;
}

int avb_stats_get_last(avb_stats_handler_t *self, avb_stats_data_node_t *last)
{
    if (self->stack_head == NULL)
        return -1;

    memcpy(last, self->stack_head, sizeof(avb_stats_data_node_t));

    return 0;
}

int avb_stats_get_cap_time(avb_stats_data_node_t *node, stream_settings_t *set, 
                                                                    struct timespec *cap_time)
{
    // capture = presentation - transit - hw_latency
    uint64_t capture_ns;

    if( !(node||set||cap_time) )
        return -1;
    capture_ns = (node->presentation.tv_nsec * NSEC_PER_SEC) + node->presentation.tv_nsec;
    capture_ns -= set->max_transit_time * NSEC_PER_MSEC;

    // latency is in samples
    capture_ns -= avb_aaf_hwlatency_to_ns(set);

    cap_time->tv_sec = capture_ns / NSEC_PER_SEC;
	cap_time->tv_nsec = capture_ns % NSEC_PER_SEC;

    return 0;
} 

static uint64_t timespec_to_ns(struct timespec *cap_time)
{
    return (uint64_t)(cap_time->tv_nsec * NSEC_PER_SEC) + cap_time->tv_nsec;
}

static void process_stat(uint64_t data_arr[], uint64_t count, avb_stats_stat_t *stat)
{
    for(int i=0; i<count; i++)
    {
        if (data_arr[i] > stat->max)
            stat->max = data_arr[i];
        if (data_arr[i] < stat->min)
            stat->min = data_arr[i];

        (stat->mean) += data_arr[i]; //mean must be divided by count later
    }
    stat->count=count;
    //calc means
    stat->mean = stat->mean / count;

    // calc standard eviation.      std = sqrt( sum( (x-mean)^2 ) )
    for(int i=0; i<count; i++)
    {
        stat->std += pow(data_arr[i] - stat->mean ,2);
    }
    stat->std = sqrt(stat->std / count);
}

int avb_stats_process_stats(avb_stats_handler_t *self, stream_settings_t *set)
{

    //min, mean, max, std
    //delta_capture, travel

    struct timespec current_cap, next_cap;
    uint64_t current_cap_ns;
    avb_stats_data_node_t *node_ptr;

    uint64_t entry_count = 0;
    uint64_t *delta_cap_arr = NULL;
    uint64_t *travel_arr = NULL;
    uint64_t entry_index = 0;

    node_ptr = self->stack_head;
    if (node_ptr == NULL)
        return -1;

    // count number of entries
    while(node_ptr){
        entry_count++;
        node_ptr = node_ptr->next;
    }
    travel_arr = malloc( sizeof(uint64_t) * entry_count );
    delta_cap_arr = malloc( (sizeof(uint64_t) * entry_count) - 1 );


    // traverse the list
    node_ptr = self->stack_head;
    while(node_ptr){

        avb_stats_get_cap_time(node_ptr, set, &current_cap);
        current_cap_ns = timespec_to_ns(&current_cap);

        //travel = arrival - (capture + hw_latency)
        travel_arr[entry_index] = timespec_to_ns(&node_ptr->arrival);
        travel_arr[entry_index] -= (current_cap_ns + avb_aaf_hwlatency_to_ns(set));

        // if this is the last node, break. There is no next to calc the delta
        if( node_ptr->next == NULL)
            break;

        avb_stats_get_cap_time(node_ptr->next, set, &next_cap);

        //delta_capture = next-curernt
        delta_cap_arr[entry_index] = timespec_to_ns(&next_cap)-current_cap_ns;

        entry_index++;
        node_ptr = node_ptr->next;
    }

    process_stat(travel_arr     , entry_count   , &(self->travel)   );
    process_stat(delta_cap_arr  , entry_count-1 , &(self->delta_cap));

    return 0;
}

void avb_stats_print_stats(FILE* file, avb_stats_handler_t *self)
{
    fprintf(file, "Received packets: %lu\n", self->travel.count);
    fprintf(file, "Dropped packets : %u\n", self->dropped_packets);

    fprintf(file, "\nCapture deltas\n");
    fprintf(file, "min : %lu\n", self->delta_cap.min );
    fprintf(file, "max : %lu\n", self->delta_cap.max );
    fprintf(file, "mean: %lu\n", self->delta_cap.mean);
    fprintf(file, "std : %lu\n", self->delta_cap.std );

    fprintf(file, "\nTravel times\n");
    fprintf(file, "min : %lu\n", self->travel.min );
    fprintf(file, "max : %lu\n", self->travel.max );
    fprintf(file, "mean: %lu\n", self->travel.mean);
    fprintf(file, "std : %lu\n", self->travel.std );

    return;
}