/* buffer (pointer+length) like structs, for libreswan
 *
 * Copyright (C) 2018-2019 Andrew Cagney <cagney@gnu.org>
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

#ifndef HUNK_H
#define HUNK_H

#include <stdbool.h>
#include <stddef.h>		/* size_t */
#include <stdint.h>		/* uint8_t */

/*
 * Macros and functions for manipulating hunk like structures.  Any
 * struct containing .ptr and .len fields is considered a hunk.
 *
 * The two most common hunks are:
 *
 * chunk_t: for a writeable buffer; also the original structure and
 * why the DATA field is called .ptr (.data, as used by NSS would have
 * been better).
 *
 * shunk_t: for a readonly buffer; the S is for STRING and originally
 * for static constant string manipulation.
 *
 * However, it is also possible to use these macros to manipulate
 * pre-sized buffers such as ckaid_t and struct crypt_hash where .ptr
 * is an array (hence comment above about .data being a better
 * choice).
 *
 * To avoid repeated evaluation of fuctions, the macros below first
 * make a copy of the hunk being manipulated.  For structures such as
 * ckaid_t where that will copy the buffer contents, it is assumed
 * that the compiler will see that things are constant and eliminate
 * them.
 */

#define THING_AS_HUNK(THING) { .ptr = &(THING), .len = sizeof(THING), }
#define NULL_HUNK { .ptr = NULL, .len = 0, }
/* #define EMPTY_HUNK { .ptr = &buffer, .len = 0, } */

/*
 * hunk version of compare functions (or at least libreswan's
 * versions).
 *
 * (Confusingly and just like POSIX, *case* ignores case).
 *
 * Just like a NULL and EMPTY ("") string, a NULL (uninitialized) and
 * EMPTY (pointing somewhere but no bytes) are considered different.
 */

bool bytes_eq(const void *l_ptr, size_t l_len,
	      const void *r_ptr, size_t r_len);
bool case_eq(const void *l_ptr, size_t l_len,
	     const void *r_ptr, size_t r_len);

#define hunk_isempty(HUNK)			\
	({					\
		(HUNK).len == 0;		\
	})

#define hunk_eq(L,R)							\
	({								\
		typeof(L) l_ = L; /* evaluate once */			\
		typeof(R) r_ = R; /* evaluate once */			\
		bytes_eq(l_.ptr, l_.len, r_.ptr, r_.len);		\
	})

#define hunk_caseeq(L, R) /* case independent */			\
	({								\
		const typeof(L) l_ = L; /* evaluate once */		\
		const typeof(R) r_ = R; /* evaluate once */		\
		case_eq(l_.ptr, l_.len, r_.ptr, r_.len);		\
	})

#define hunk_streq(HUNK, STRING)					\
	({								\
		const typeof(HUNK) hunk_ = HUNK; /* evaluate once */	\
		const char *string_ = STRING; /* evaluate once */	\
		bytes_eq(hunk_.ptr, hunk_.len, string_,			\
			 string_ != NULL ? strlen(string_) : 0);	\
	})

#define hunk_strcaseeq(HUNK, STRING) /* case independent */		\
	({								\
		const typeof(HUNK) hunk_ = HUNK; /* evaluate once */	\
		const char *string_ = STRING; /* evaluate once */	\
		case_eq(hunk_.ptr, hunk_.len, string_,			\
			string_ != NULL ? strlen(string_) : 0);		\
	})

#define hunk_memeq(HUNK, MEM, SIZE)					\
	({								\
		const typeof(HUNK) hunk_ = HUNK; /* evaluate once */	\
		const void *mem_ = MEM; /* evaluate once */		\
		size_t size_ = SIZE; /* evaluate once */		\
		bytes_eq(hunk_.ptr, hunk_.len, mem_, size_);		\
	})

#define hunk_thingeq(SHUNK, THING) hunk_memeq(SHUNK, &(THING), sizeof(THING))

/* bool hunk_strneq(HUNK, STRING, SIZE) -- what would N mean? */
#define hunk_startswith(HUNK, STRING)					\
	({								\
		const typeof(HUNK) hunk_ = HUNK; /* evaluate once */	\
		const char *string_ = STRING; /* evaluate once */	\
		size_t slen_ = string_ != NULL ? strlen(string_) : 0;	\
		hunk_.len >= slen_ ? bytes_eq(hunk_.ptr, slen_, string_, slen_) : false; \
	})

/*
 * Manipulate the hunk as an array of characters.
 */

/* returns '\0' when out of range */
#define hunk_char(HUNK, INDEX)						\
	({								\
		const typeof(HUNK) hunk_ = HUNK; /* evaluate once */	\
		size_t index_ = INDEX;/* evaluate once */		\
		const char *string_ = hunk_.ptr;			\
		index_ < hunk_.len ? string_[INDEX] : '\0';		\
	})

#define hunk_char_isdigit(HUNK, OFFSET)				\
	({							\
		unsigned char c_ = hunk_char(HUNK, OFFSET);	\
		isdigit(c_);					\
	})

#define hunk_char_ischar(HUNK, OFFSET, CHARS)			\
	({							\
		unsigned char c_ = hunk_char(HUNK, OFFSET);	\
		strchr(CHARS, c_);				\
	})

#define memcpy_hunk(DST, HUNK, SIZE)					\
	({								\
		const typeof(HUNK) hunk_ = HUNK; /* evaluate once */	\
		passert(hunk_.len == SIZE);				\
		memcpy(DST, hunk_.ptr, SIZE);				\
	})

#endif
