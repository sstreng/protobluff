/*
 * Copyright (c) 2013-2015 Martin Donath <martin.donath@squidfunk.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef PB_INCLUDE_CURSOR_H
#define PB_INCLUDE_CURSOR_H

#include <assert.h>
#include <stddef.h>

#include <protobluff/common.h>
#include <protobluff/message.h>

/* ----------------------------------------------------------------------------
 * Type definitions
 * ------------------------------------------------------------------------- */

typedef struct pb_cursor_t {
  pb_message_t message;                /*!< Message */
  pb_tag_t tag;                        /*!< Tag to look for */
  struct {
    pb_tag_t tag;                      /*!< Current tag */
    pb_offset_t offset;                /*!< Current offsets */
  } current;
  size_t pos;                          /*!< Current position */
  pb_error_t error;                    /*!< Error code */
} pb_cursor_t;

/* ----------------------------------------------------------------------------
 * Interface
 * ------------------------------------------------------------------------- */

PB_WARN_UNUSED_RESULT
PB_EXPORT pb_cursor_t
pb_cursor_create(
  pb_message_t *message,               /* Message */
  pb_tag_t tag);                       /* Tag */

PB_WARN_UNUSED_RESULT
PB_EXPORT pb_cursor_t
pb_cursor_create_nested(
  pb_message_t *message,               /* Message */
  const pb_tag_t tags[],               /* Tags */
  size_t size);                        /* Tag count */

PB_EXPORT void
pb_cursor_destroy(
  pb_cursor_t *cursor);                /* Cursor */

PB_EXPORT int
pb_cursor_next(
  pb_cursor_t *cursor);                /* Cursor */

PB_EXPORT int
pb_cursor_rewind(
  pb_cursor_t *cursor);                /* Cursor */

PB_EXPORT int
pb_cursor_seek(
  pb_cursor_t *cursor,                 /* Cursor */
  const void *value);                  /* Pointer holding value */

PB_EXPORT int
pb_cursor_match(
  pb_cursor_t *cursor,                 /* Cursor */
  const void *value);                  /* Pointer holding value */

PB_WARN_UNUSED_RESULT
PB_EXPORT pb_error_t
pb_cursor_get(
  pb_cursor_t *cursor,                 /* Cursor */
  void *value);                        /* Pointer receiving value */

PB_WARN_UNUSED_RESULT
PB_EXPORT pb_error_t
pb_cursor_put(
  pb_cursor_t *cursor,                 /* Cursor */
  const void *value);                  /* Pointer holding value */

PB_WARN_UNUSED_RESULT
PB_EXPORT pb_error_t
pb_cursor_erase(
  pb_cursor_t *cursor);                /* Cursor */

PB_EXPORT void *
pb_cursor_raw(
  pb_cursor_t *cursor);                /* Cursor */

/* ----------------------------------------------------------------------------
 * Macros
 * ------------------------------------------------------------------------- */

/*!
 * Retrieve the underlying message of a cursor.
 *
 * \param[in] cursor Cursor
 * \return           Message
 */
#define pb_cursor_message(cursor) \
  (assert(cursor), (const pb_message_t *)&((cursor)->message))

/*!
 * Retrieve the tag at the current position of a cursor.
 *
 * \param[in] cursor Cursor
 * \return           Current tag
 */
#define pb_cursor_tag(cursor) \
  (assert(cursor), (const pb_tag_t)(cursor)->current.tag)

/*!
 * Retrieve the current position of a cursor.
 *
 * \param[in] cursor Cursor
 * \return           Current position
 */
#define pb_cursor_pos(cursor) \
  (assert(cursor), (const size_t)(cursor)->pos)

/*!
 * Retrieve the internal error state of a cursor.
 *
 * \param[in] cursor Cursor
 * \return           Error code
 */
#define pb_cursor_error(cursor) \
  (assert(cursor), (const pb_error_t)(cursor)->error)

/*!
 * Test whether a cursor is valid.
 *
 * \param[in] cursor Cursor
 * \return           Test result
 */
#define pb_cursor_valid(cursor) \
  (assert(cursor), !pb_cursor_error(cursor))

#endif /* PB_INCLUDE_CURSOR_H */