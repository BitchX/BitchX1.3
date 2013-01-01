/* 
 * IRC-II (C) 1990, 1995 Michael Sandroff, Matthew Green
 * This file (C) 1993, 1995 Aaron Gifford and Jeremy Nelson
 *
 * array.h -- header file for array.c
 * See the COPYRIGHT file for copyright information
 *
 */

#ifndef ARRAY_H
#define ARRAY_H

#include "irc_std.h"

	BUILT_IN_FUNCTION(function_indextoitem);
	BUILT_IN_FUNCTION(function_itemtoindex);
	BUILT_IN_FUNCTION(function_igetitem);
	BUILT_IN_FUNCTION(function_getitem);
	BUILT_IN_FUNCTION(function_setitem);
	BUILT_IN_FUNCTION(function_finditem);
	BUILT_IN_FUNCTION(function_matchitem);
	BUILT_IN_FUNCTION(function_rmatchitem);
	BUILT_IN_FUNCTION(function_getmatches);
	BUILT_IN_FUNCTION(function_getrmatches);
	BUILT_IN_FUNCTION(function_delitem);
	BUILT_IN_FUNCTION(function_numitems);
	BUILT_IN_FUNCTION(function_getarrays);
	BUILT_IN_FUNCTION(function_numarrays);
	BUILT_IN_FUNCTION(function_delarray);
	BUILT_IN_FUNCTION(function_ifinditem);
	BUILT_IN_FUNCTION(function_ifindfirst);
	BUILT_IN_FUNCTION(function_listarray);
	BUILT_IN_FUNCTION(function_gettmatch);
	BUILT_IN_FUNCTION(function_igetmatches);
	BUILT_IN_FUNCTION(function_igetrmatches);
	void delete_all_arrays(void);

#endif
