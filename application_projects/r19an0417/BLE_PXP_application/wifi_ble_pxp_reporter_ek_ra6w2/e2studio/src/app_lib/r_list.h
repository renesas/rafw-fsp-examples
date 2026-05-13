/***********************************************************************************************************************
 * File Name    : r_list.h
 * Description  : Simple helper to manage single-linked list
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#ifndef R_LIST_H_
#define R_LIST_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * List element
 *
 * Each list element can be of any user defined type as long as its first member is pointer to
 * structure of the same type.
 *
 */
struct list_elem
{
    struct list_elem * next;
};

/**
 * Callback for elements
 *
 * \param [in] elem  evaluated element
 * \param [in] ud    user data
 *
 */
typedef void (* list_elem_cb_t)(const void * elem, const void * ud);

/**
 * Callback for matching elements
 *
 * \param [in] elem  evaluated element
 * \param [in] ud    user data
 *
 * \return true if element is matches, false otherwise
 *
 */
typedef bool (* list_elem_match_t)(const void * elem, const void * ud);

/**
 * Add element to list
 *
 * Element is added to the beginning of list.
 *
 * \param [in,out] head   list head
 * \param [in]     elem   new element
 *
 */
void R_List_add(void ** head, void * elem);

/**
 * Delete element from the end of the list
 *
 * \param [in,out] head   list head
 *
 * \return last element in list
 *
 */
void * R_List_pop_back(void ** head);

/**
 * Peek element from the end of the list
 *
 * \param [in,out] head   list head
 *
 * \return last element in list
 *
 */
void * R_List_peek_back(void ** head);

/**
 * Get number of elements in list
 *
 * \param [in] head   list head
 *
 * \return number of elements in list
 *
 */
uint8_t R_List_size(void * head);

/**
 * Append element to list
 *
 * It's recommended to use list_add() whenever possible since it works in constant time.
 *
 * \param [in,out] head   list head
 * \param [in]     elem   new element
 *
 */
void R_List_append(void ** head, void * elem);

/**
 * Find element in list
 *
 * \param [in]     head   list head
 * \param [in]     match  callback to match element
 * \param [in]     ud     user data
 *
 * \return found element or NULL if not found
 *
 */
void * R_List_find(void * head, list_elem_match_t match, const void * ud);

/**
 * Unlink element from list
 *
 * \param [in,out] head   list head
 * \param [in]     match  callback to match element
 * \param [in]     ud     user data
 *
 * \return unlinked element or NULL if not found
 *
 */
void * R_List_unlink(void ** head, list_elem_match_t match, const void * ud);

/**
 * Remove element from list
 *
 * \param [in,out] head   list head
 * \param [in]     match  callback to match element
 * \param [in]     ud     user data
 *
 */
void R_List_remove(void ** head, list_elem_match_t match, const void * ud);

/**
 * Removes all matched elements from list
 *
 * \param [in,out] head   list head
 * \param [in]     match  callback to match element
 * \param [in]     ud     user data
 *
 */
void R_List_filter(void ** head, list_elem_match_t match, const void * ud);

/**
 * Iterates over entire list
 *
 * \param [in]     head   list head
 * \param [in]     cb     callback to be called for each element
 * \param [in]     ud     user data
 *
 */
void R_List_foreach(void * head, list_elem_cb_t cb, const void * ud);

/**
 * Remove all elements from list
 *
 * \param [in,out] head   list head
 * \param [in]     cb     callback to be called for each element before removing it
 * \param [in]     ud     user data
 *
 */
void R_List_free(void ** head, list_elem_cb_t cb, const void * ud);

#endif                                 /* R_LIST_H_ */
