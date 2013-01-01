/*
 * alist.h -- resizeable arrays
 * Copyright 1997 EPIC Software Labs
 */

#ifndef __alist_h__
#define __alist_h__

#include "irc.h"
#include "ircaux.h"

/*
 * Anyone have any ideas how to optimize this further?
 */

#ifdef _cs_alist_hash_
static __inline u_32int_t 	cs_alist_hash (const char *s, u_32int_t *mask)
{
	u_32int_t x;

	if (s[0] == 0)
	{
		x = 0;
		*mask = 0;
	}
	else if (s[1] == 0)
	{
		x = (s[0] << 24);
		*mask = 0xff000000;
	}
	else if (s[2] == 0)
	{
		x = (s[0] << 24) | (s[1] << 16);
		*mask = 0xffff0000;
	}
	else
	{
		x = (s[0] << 24) | (s[1] << 16) | (s[2] << 8) | s[3];
		*mask = 0xffffff00 | (s[3] ? 0xff : 0x00);
	}

	return x;
}
#endif

#ifdef _ci_alist_hash_
static __inline u_32int_t 	ci_alist_hash (const char *s, u_32int_t *mask)
{
	u_32int_t x;

	if (s[0] == 0)
	{
		x = 0;
		*mask = 0;
	}
	else if (s[1] == 0)
	{
		x = (stricmp_table[(int)s[0]] << 24);
		*mask = 0xff000000;
	}
	else if (s[2] == 0)
	{
		x = (stricmp_table[(int)s[0]] << 24) | (stricmp_table[(int)s[1]] << 16);
		*mask = 0xffff0000;
	}
	else
	{
		x = (stricmp_table[(int)s[0]] << 24) | (stricmp_table[(int)s[1]] << 16) | (stricmp_table[(int)s[2]] << 8) | stricmp_table[(int)s[3]];
		*mask = 0xffffff00 | (s[3] ? 0xff : 0x00);
	}

	return x;
}
#endif

/*
 * Everything that is to be filed with this system should have an
 * identifying name as the first item in the struct.
 */
typedef struct
{
	char *name;
	u_32int_t hash;	
} Array_item;

typedef int (*alist_func) (const char *, const char *, size_t);
typedef enum {
	HASH_INSENSITIVE,
	HASH_SENSITIVE
} hash_type;

/*
 * This is the actual list, that contains structs that are of the
 * form described above.  It contains the current size and the maximum
 * size of the array.
 */
typedef struct
{
	Array_item **list;
	int max;
	int total_max;
	alist_func func;
	hash_type hash;
} Array;

Array_item *BX_add_to_array	(Array *, Array_item *);
Array_item *BX_remove_from_array	(Array *, char *);
Array_item *BX_array_pop		(Array *, int);

Array_item *BX_remove_all_from_array (Array *, char *);
Array_item *BX_array_lookup	(Array *, char *, int wild, int delete);
Array_item *BX_find_array_item	(Array *, char *, int *cnt, int *loc);

void *BX_find_fixed_array_item	(void *Array, size_t size, int siz, char *, int *cnt, int *loc);

#endif
