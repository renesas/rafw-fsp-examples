/***********************************************************************************************************************
 * File Name    : r_list.c
 * Description  : Simple helper to manage single-linked list
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#include <app_lib/r_list.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

void R_List_add (void ** head, void * elem)
{
    struct list_elem * e = elem;

    e->next = *head;
    *head   = e;
}

void * R_List_pop_back (void ** head)
{
    struct list_elem * e = *head;
    struct list_elem * p = NULL;

    if (e)
    {
        while (e->next)
        {
            p = e;
            e = e->next;
        }

        if (p)
        {
            p->next = NULL;
        }
        else
        {
            *head = NULL;
        }
    }

    return e;
}

void * R_List_peek_back (void ** head)
{
    struct list_elem * e = *head;

    if (e)
    {
        while (e->next)
        {
            e = e->next;
        }
    }

    return e;
}

uint8_t R_List_size (void * head)
{
    uint8_t            n = 0;
    struct list_elem * e = head;

    while (e)
    {
        ++n;
        e = e->next;
    }

    return n;
}

void R_List_append (void ** head, void * elem)
{
    struct list_elem * e = *head;

    if (!e)
    {
        R_List_add(head, elem);

        return;
    }

    while (e->next)
    {
        e = e->next;
    }

    e->next = elem;
    e       = elem;
    e->next = NULL;
}

void * R_List_find (void * head, list_elem_match_t match, const void * ud)
{
    struct list_elem * e = head;

    while (e && !match(e, ud))
    {
        e = e->next;
    }

    return e;
}

void * R_List_unlink (void ** head, list_elem_match_t match, const void * ud)
{
    struct list_elem * e = *head;
    struct list_elem * p = NULL;

    while (e && !match(e, ud))
    {
        p = e;
        e = e->next;
    }

    if (e)
    {
        if (p)
        {
            p->next = e->next;
        }
        else
        {
            *head = e->next;
        }
    }

    return e;
}

void R_List_remove (void ** head, list_elem_match_t match, const void * ud)
{
    void * e = R_List_unlink(head, match, ud);

    if (e)
    {
        free(e);
    }
}

void R_List_filter (void ** head, list_elem_match_t match, const void * ud)
{
    struct list_elem * e = *head;
    struct list_elem * p = NULL;

    while (e)
    {
        if (match(e, ud))
        {
            if (p)
            {
                p->next = e->next;
            }
            else
            {
                *head = e->next;
            }

            struct list_elem * t = e;

            e = e->next;
            free(t);
        }
        else
        {
            p = e;
            e = e->next;
        }
    }
}

void R_List_foreach (void * head, list_elem_cb_t cb, const void * ud)
{
    struct list_elem * e = head;

    while (e)
    {
        cb(e, ud);
        e = e->next;
    }
}

void R_List_free (void ** head, list_elem_cb_t cb, const void * ud)
{
    while (*head)
    {
        struct list_elem * e = *head;
        *head = e->next;

        if (cb)
        {
            cb(e, ud);
        }

        free(e);
    }
}
