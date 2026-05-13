/***********************************************************************************************************************
 * File Name    : r_queue.c
 * Description  : Simple helper to manage queue
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#include "app_lib/r_queue.h"
#include <stdbool.h>
#include <stddef.h>

struct queue_elem
{
    struct queue_elem * next;
};

void R_Queue_Init (queue_t * q)
{
    q->size = 0;
    q->head = NULL;
    q->tail = NULL;
}

void R_Queue_PushFront (queue_t * q, void * data)
{
    struct queue_elem * el = data;

    el->next = q->head;
    q->head  = el;

    if (!q->tail)
    {
        q->tail = el;
    }

    q->size++;
}

void R_Queue_PushBack (queue_t * q, void * data)
{
    struct queue_elem * el = data;

    el->next = NULL;

    if (q->tail)
    {
        q->tail->next = el;
    }

    q->tail = el;

    if (!q->head)
    {
        q->head = el;
    }

    q->size++;
}

void * R_Queue_PopFront (queue_t * q)
{
    struct queue_elem * el = q->head;

    if (!el)
    {
        return NULL;
    }

    q->head = el->next;

    if (el == q->tail)
    {
        q->tail = NULL;
    }

    q->size--;

    return el;
}

void * R_Queue_PeekFront (const queue_t * q)
{
    return q->head;
}

void * R_Queue_PeekBack (const queue_t * q)
{
    return q->tail;
}

size_t R_Queue_Length (const queue_t * q)
{
    return q->size;
}

void R_Queue_Foreach (const queue_t * q, queue_foreach_func_t func, void * user_data)
{
    struct queue_elem * el = q->head;

    while (el)
    {
        func(el, user_data);

        el = el->next;
    }
}

void * R_Queue_Find (const queue_t * q, queue_match_func_t func, const void * match_data)
{
    struct queue_elem * el = q->head;

    while (el)
    {
        if (func(el, match_data))
        {
            return el;
        }

        el = el->next;
    }

    return NULL;
}

static void remove_element (queue_t * q, struct queue_elem * prev_el, struct queue_elem * el)
{
    if (!el)
    {
        return;
    }

    if (prev_el)
    {
        prev_el->next = el->next;
    }
    else
    {
        // no prev_el means we matched at head
        q->head = el->next;
    }

    if (el == q->tail)
    {
        // update tail if matched at tail
        q->tail = prev_el;
    }

    q->size--;
}

void * R_Queue_Remove (queue_t * q, queue_match_func_t func, const void * match_data)
{
    struct queue_elem * prev_el = NULL;
    struct queue_elem * el      = q->head;

    while (el)
    {
        if (func(el, match_data))
        {
            break;
        }

        prev_el = el;
        el      = el->next;
    }

    remove_element(q, prev_el, el);

    return el;
}

void R_Queue_RemoveAll (queue_t * q, queue_destroy_func_t func)
{
    struct queue_elem * el = q->head;

    while (el)
    {
        q->head = el->next;

        func(el);

        el = q->head;
    }

    R_Queue_Init(q);
}

void R_Queue_Filter (queue_t * q, queue_match_func_t m_func, const void * match_data, queue_destroy_func_t d_func)
{
    struct queue_elem * prev_el = NULL;
    struct queue_elem * el      = q->head;

    while (el)
    {
        struct queue_elem * next_el = el->next;

        if (m_func(el, match_data))
        {
            remove_element(q, prev_el, el);

            if (d_func)
            {
                d_func(el);
            }

            /* prev_el stays the same! */
        }
        else
        {
            prev_el = el;
        }

        el = next_el;
    }
}
