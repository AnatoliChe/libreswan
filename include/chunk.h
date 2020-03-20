/*
 * memory chunks, for libreswan
 *
 * Copyright (C) 1997 Angelos D. Keromytis.
 * Copyright (C) 1998-2001, 2013 D. Hugh Redelmeier <hugh@mimosa.com>
 * Copyright (C) 2012 Paul Wouters <paul@libreswan.org>
 * Copyright (C) 2013 Tuomo Soini <tis@foobar.fi>
 * Copyright (C) 2018 Andrew Cagney
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <https://www.gnu.org/licenses/gpl2.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#ifndef CHUNK_H
#define CHUNK_H

#include <stdbool.h>	/* bool */
#include <stddef.h>	/* size_t */
#include <stdint.h>	/* uint8_t */

#include "hunk.h"
#include "lswalloc.h"	/* for freeanychunk() refering to pfree() which can go away */

/*
 * chunk is a simple pointer-and-size abstraction
 *
 * It's for dealing with raw bytes, for strings see shunk_t.
 *
 * Where possible, implement using non-inline functions.  This way all
 * code is found in chunk.c.  And debugging doesn't run into grief
 * with either macros or badly inlined functions.
 */

typedef struct /*chunk*/ {
	uint8_t *ptr;
	size_t len;
} chunk_t;

chunk_t chunk1(char *ptr);
chunk_t chunk2(void *ptr, size_t len);

/*
 * Convert writeable THING to a writeable CHUNK.  When compiled with
 * GCC (at least) and THING is read-only, a warning will be generated.
 *
 * This works because GCC doesn't like implictly converting a 'const'
 * &THING actual parameter to the non-const 'void*' formal parameter.
 * Using an explicit cast (such as in a static initializer) suppresses
 * this warning.
 *
 * For a read-only CHUNK like object, see THING_AS_SHUNK().
 */
#define THING_AS_CHUNK(THING) chunk2(&(THING), sizeof(THING))

chunk_t alloc_chunk(size_t count, const char *name);

/* result is always a WRITEABLE chunk; NULL->NULL */
#define clone_hunk(HUNK, NAME) ({					\
			typeof(HUNK) hunk_ = HUNK; /* evaluate once */	\
			clone_bytes_as_chunk(hunk_.ptr, hunk_.len, NAME); \
		})

/* clone(first+second) */
chunk_t clone_chunk_chunk(chunk_t first, chunk_t second, const char *name);

/* always NUL terminated; NULL is NULL */
char *clone_chunk_as_string(chunk_t chunk, const char *name);

/* BYTES==NULL => NULL_CHUNK */
chunk_t clone_bytes_as_chunk(const void *bytes, size_t sizeof_bytes, const char *name);

/*
 * Free contents of chunk (if any) and blat chunk.
 */

void free_chunk_content(chunk_t *chunk); /* blats *CHUNK */

/*
 * misc ops.
 */

extern const chunk_t empty_chunk;
#define EMPTY_CHUNK ((const chunk_t) { .ptr = NULL, .len = 0 })

#define PRI_CHUNK "%p@%zu"
#define pri_chunk(CHUNK) (CHUNK).ptr, (CHUNK).len

chunk_t chunk_from_hex(const char *hex, const char *name);

#endif
