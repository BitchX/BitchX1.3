/*
 * IRC-II Copyright (C) 1990 Michael Sandroff and others, 
 * This file - Copyright (C) 1993,1995 Aaron Gifford
 * This file written by Aaron Gifford and contributed to the EPIC
 *   project by Jeremy Nelson to whom it was contributed in Nov, 1993.
 *
 * Used with permission.  See the COPYRIGHT file for copyright info.
 */
/*
	DATE OF THIS VERSION:
	---------------------
	Sat Nov 27 23:00:20 MST 1993

	NEW SINCE 20 NOV 1993:
	----------------------
  	Two new functions were added: GETMATCHES() and GETRMATCHES()

	BUGS:
	-----
  	I had a script that used these functions that caused ircII to crash 
	once or twice, but I have been unable to trace the cause or reproduce 
	the crash.  I would appreciate any help or info if anyone else 
	experiences similar problems.  This IS my first time using writing code 
	to work with ircII code.

	ARGUMENTS:
	----------
  	array_name  : A string of some sort (no spaces, case insensitive) 
		identifying an array, either an existing array, or a new array.

  	item_number : A number or index into the array.  All array items are 
		numbered beginning at zero and continuing sequentially.  The 
		item number can be an existing item of an existing array, a 
		new item in an existing array (if the previous greatest item 
		number was 5, then the new item number must be 6, maintaining 
		the sequence), or a new item (must be item zero) in a new array.
  	data_...    : Basically any data you want.  It is stored internally as a
                character string, and in indexed (more to come) internally
                using the standard C library function strcmp().

	FUNCTIONS:
	----------
 	SETITEM(array_name item_number data_to_be_stored)
    	Use SETITEM to set an existing item of an existing array to a new value,
    	to create a new value in an existing array (see note about item_number),
    	or to create a new array (see note about item_number above).
    	RETURNS: 0 (zero) if it successfully sets and existing item in the array
             	 1 if it successfully creates a new array
             	 2 if it successfully adds a new item to an existing array 
            	-1 if it was unable to find the array specified (item_number > 0)
            	-2 if it was unable to find the item in an existing array
               		(item_number was too large)

  	GETITEM(array_name item_number)
    	Use this to retrieve previously stored data.
    	RETURNS: the data requested
             OR an empty string if the array did not exist or the item did not.

  	NUMITEMS(array_name)
    	RETURNS: the number of items in the array
             OR zero if the array name is invalid.  Useful for auto-adding to
             an array:
                 alias ADDDATA {
                     if (SETITEM(my-array $NUMITEMS(my-array) $*) >= 0) {
                         echo ADDED DATA
                     } {
                         echo FAILED TO ADD DATA
                     }
                 }
 
  	DELITEM(array_name item_number)
    	This deletes the item requested from the array.  If it is the last item
    	(item zero), it deletes the array.  If there are items numbered higher
    	than this item, they will each be moved down.  So if we had a 25 item
    	array called "MY-ARRAY" and deleted item 7, items 8 through 24 (remember
    	that a 25 item array is numbered 0 to 24) would be moved down and become
    	items 7 through 23.
    	RETURNS:  Zero on success,
              -1 if unable to find the array,
              -2 if unable find the item.

  	MATCHITEM(array_name pattern)
    	Searches through the items in the array for the item that best matches 
	the pattern, much like the MATCH() function does.
    	RETURNS: zero or a positive number which is the item_number of the match
             OR -1 if unable to find the array,
             OR -2 if no match was found in the array

  	RMATCHITEM(array_name data_to_look_for)
    	This treats the items in the array as patterns, searching for the 
	pattern in the array that best matches the data_to_look_for, working 
	similarly to the RMATCH() function.
    	RETURNS: zero or a positive number which is the item_number of the match
             OR -1 if unable to find the array,
             OR -2 if no match was found in the array

  	GETMATCHES(array_name pattern)
    	Seeks all array items that match the pattern.
   	RETURNS: A string containing a space-separated list of item numbers of
             array elements that match the pattern, or a null string if no
             matches were found, or if the array was not found.

  	GETRMATCHES(array_name data_to_look_for)
    	Treats all array items as patterns, and seeks to find all patterns that
    	match the data_to_look_for.
    	RETURNS: A string containing a space-separated list of item numbers of
             array elements that match the data, or a null string if no
             matches were found, or if the array was not found.

  	FINDITEM(array_name data_to_search_for)
    	This does a binary search on the items stored in the array and returns
    	the item number of the data.  It is an EXACT MATCH search.  It is highly
    	case sensitive, using C's strcmp() and not IRCII's caseless comparison
   	functions.  I did it this way, because I needed it, so there!  ;)
    	RETURNS: zero or a positive number on success -- this number IS the
             item_number of the first match it found
             OR -1 if unable to find the array,
             OR -2 if the item was not found in the array.

  	IGETITEM(array_name index_number)
    	This is exactly like GETITEM() except it uses a index number in the same
   	range as the item_number's.  It returns the item that corresponds to the
    	internal alphabetized index that these functions maintain.  Thus if you
    	access items 0 to 24 of "MY-ARRAY" with this function, you would observe
    	that the items returned came in an almost alphabetized manner.  They 
	would not be truly alphabetized because the ordering is done using 
	strcmp() which is case sensitive.
    	RETURNS: the data to which the index refers
             OR an empty string on failure to find the array or index.

  	INDEXTOITEM(array_name index_number)
    	This converts an index_number to an item_number.
    	RETURNS: the item_number that corresponds to the index_number for 
		the array
             OR -1 if unable to find the array,
             OR -2 if the index_number was invalid

  	ITEMTOINDEX(array_name item_number)
    	This converts an item_number to an index_number.
    	RETURNS: the index_number that corresponds to the item_number for 
		the array
             OR -1 if unable to find the array,
             OR -2 if the item_number was invalid

  	DELARRAY(array_name)
    	This deletes all items in an array.
    	RETURNS:  zero on success, -1 if it couldn't find the array.

  	NUMARRAYS()
    	RETURNS: the number of arrays that currently exist.

  	GETARRAYS()
    	RETURNS: a string consisting of all the names of all the arrays 
		 separated by spaces.

Thanks, 
Aaron Gifford
Karll on IRC
<agifford@sci.dixie.edu>

*/
/*
 * FILE:       alias.c
 * WRITTEN BY: Aaron Gifford (Karll on IRC)
 * DATE:       Sat Nov 27 23:00:20 MST 1993
 */

#include "irc.h"
static char cvsrevision[] = "$Id: array.c 81 2009-11-24 12:06:54Z keaston $";
CVS_REVISION(array_c)

#include "array.h"
#include "ircaux.h"
#include "output.h"
#undef index

#define MAIN_SOURCE
#include "modval.h"

#define ARRAY_THRESHOLD	100

typedef struct an_array_struct {
	char **item;
	long *index;
	long size;
} an_array;

static an_array array_info = {
        NULL,
        NULL,
        0L
};

static an_array *array_array = NULL;

/*
 * find_item() does a binary search of array.item[] using array.index[]
 * to find an exact match of the string *find.  If found, it returns the item
 * number (array.item[item_number]) of the match.  Otherwise, it returns a
 * negative number.  The negative number, if made positive again, and then
 * having 1 subtracted from it, will be the item_number where the string *find
 * should be inserted into the array.item[].  The function_setitem() makes use
 * of this attribute.
 */
extern 	long		find_item (an_array array, char *find)
{
        long top, bottom, key, cmp;

        top = array.size - 1;
        bottom = 0;
	
        while (top >= bottom)
	{
                key = (top - bottom) / 2 + bottom;
                cmp = strcmp(find, array.item[array.index[key]]);
                if (cmp == 0)
                        return key;
                if (cmp < 0)
                        top = key - 1;
                else
                        bottom = key + 1;
        } 
        return -bottom - 1;
}

/*
 * insert_index() takes a valid index (newIndex) and inserts it into the array
 * **index, then increments the *size of the index array.
 */
extern 	void		insert_index (long **index, long *size, long newIndex)
{
	long cnt;

	if (*size)
		*index = (long *)RESIZE(*index, long, *size + 1);
	else
	{
		*index = (long *)new_malloc(sizeof(long));
		newIndex = 0;
	}
	
	for (cnt = *size; cnt > newIndex; cnt--)
		(*index)[cnt] = (*index)[cnt - 1];
	(*index)[newIndex] = *size;
	(*size)++;
}

/*
 * move_index() moves the array.index[] up or down to make room for new entries
 * or to clean up so an entry can be deleted.
 */
extern	void		move_index (an_array *array, long oldindex, long newindex)
{
	long temp;

	if (newindex > oldindex)
		newindex--;
	if (newindex == oldindex)
		return;
	
	temp = array->index[oldindex];

	if (oldindex < newindex)
		for (; oldindex < newindex; oldindex++)
			array->index[oldindex] = array->index[oldindex + 1];
	else
		for(; oldindex > newindex; oldindex--)
			array->index[oldindex] = array->index[oldindex - 1];
	
	array->index[newindex] = temp;
}

/*
 * find_index() attempts to take an item and discover the index number
 * that refers to it.  find_index() assumes that find_item() WILL always return
 * a positive or zero result (be successful) because find_index() assumes that
 * the item is valid, and thus a find will be successful.  I don't know what
 * value ARRAY_THRESHOLD ought to be.  I figured someone smarter than I am
 * could figure it out and tell me or tell me to abandon the entire idea.
 */
extern	long		find_index (an_array *array, long item)
{
	long i = 0;

	if (array->size >= ARRAY_THRESHOLD)
	{
		i = find_item(*array, array->item[item]);
		while (i >= 0 && !strcmp(array->item[array->index[i]], array->item[item]))
			i--;
		i++;
	}
	while(array->index[i] != item && i < array->size)
		i++;

	if (i == array->size)
		say("ERROR in find_index()");	
	return i;
}

/*
 * get_array() searches and finds the array referenced by *name.  It returns
 * a pointer to the array, or a null pointer on failure to find it.
 */
extern 	an_array *	get_array (char *name)
{
	long index;

	if (array_info.size && *name)
        {
                upper(name);
                if ((index = find_item(array_info, name)) >= 0)
                        return &array_array[array_info.index[index]];
	}
	return NULL;
}


/*
 * delete_array() deletes the contents of an entire array.  It assumes that
 * find_item(array_info, name) will succeed and return a valid zero or positive
 * value.
 */
extern	void		delete_array (char *name)
{
        char **ptr;
        long cnt;
        long index;
        long item;
        an_array *array;

        index = find_item(array_info, name);
        item = array_info.index[index];
        array = &array_array[item];
        for (ptr=array->item, cnt=0; cnt < array->size; cnt++, ptr++)
                new_free((char **)ptr);
        new_free((char **)&array->item);
        new_free((char **)&array->index);

        if (array_info.size > 1)
        {
                for(cnt = 0; cnt < array_info.size; cnt++)
                        if (array_info.index[cnt] > item)
                                (array_info.index[cnt])--;
                move_index(&array_info, index, array_info.size);
                array_info.size--;
                for(ptr=&array_info.item[item], cnt=item; cnt < array_info.size; cnt++, ptr++, array++)
                {
                        *ptr = *(ptr + 1);
                        *array = *(array + 1);
                }
                array_info.item = (char**)RESIZE(array_info.item, char *, array_info.size);
                array_info.index = (long *)RESIZE(array_info.index, long, array_info.size);
                RESIZE(array_array, an_array, array_info.size);
        }
        else
        {
                new_free((char **)&array_info.item);
                new_free((char **)&array_info.index);
                new_free((char **)&array_array);
                array_info.size = 0;
        }
}

void delete_all_arrays(void)
{
        char **ptr;
        long index;
        an_array *array;
#if 0
m_s3cat(&result, space, array_info.item[array_info.index[index]]);
#endif	
        for (index = 0; index < array_info.size; index++)
        {
        	array = &array_array[index];
		ptr = array->item;
		while (array->size > 0)
		{
        	        new_free((char **)ptr);
			ptr++;
			array->size--;
		}
	        new_free((char **)&array->item);
        	new_free((char **)&array->index);
	}
	for (index = 0; index < array_info.size; index++)
	{
		ptr = (char **)&array_info.item[index];
		new_free(ptr);
		ptr++;		
	}
	new_free((char **)&array_info.item);
	new_free((char **)&array_info.index);
	new_free((char **)&array_array);
	array_info.size = 0;
}

/*
 * Now for the actual alias functions
 * ==================================
 */

/*
 * These are the same ones found in alias.c
 */
#define EMPTY empty_string
#define RETURN_EMPTY return m_strdup(EMPTY)
#define RETURN_IF_EMPTY(x) if (empty( x )) RETURN_EMPTY
#define GET_INT_ARG(x, y) {RETURN_IF_EMPTY(y); x = my_atol(safe_new_next_arg(y, &y));}
#define GET_FLOAT_ARG(x, y) {RETURN_IF_EMPTY(y); x = atof(safe_new_next_arg(y, &y));}
#define GET_STR_ARG(x, y) {RETURN_IF_EMPTY(y); x = new_next_arg(y, &y);RETURN_IF_EMPTY(x);}
#define RETURN_STR(x) return m_strdup(x ? x : EMPTY);
#define RETURN_INT(x) return m_strdup(ltoa(x));

/*
 * function_matchitem() attempts to match a pattern to the contents of an array
 * RETURNS -1 if it cannot find the array, or -2 if no matches occur
 */
BUILT_IN_FUNCTION(function_matchitem)
{
	char	*name;
	long	index;
	an_array *array;
        long     current_match;
        long     best_match = 0;
        long     match = -1;

	if ((name = next_arg(input, &input)) && (array = get_array(name)))
	{
		match = -2;
        	if (*input)
        	{
			for (index = 0; index < array->size; index++)
                	{
                        	if ((current_match = wild_match(input, array->item[index])) > best_match)
                        	{
                               		match = index;
                               		best_match = current_match;
                        	}
                	}
		}
	}

	RETURN_INT(match);
}

BUILT_IN_FUNCTION(function_igetmatches)
{
	char    *result = NULL;
	char    *name = NULL;
	long    item;
	an_array *array;

	if ((name = next_arg(input, &input)) &&
		(array = get_array(name)) && *input)
	{
		if (*input)
		{
			for (item = 0; item < array->size; item++)
			{
				if (wild_match(input, array->item[item]) > 0)
					m_s3cat(&result, space, ltoa(find_index(array, item)));
			}
		}
	}

	if (!result)
		RETURN_EMPTY;

	return result;
}

/*
 * function_listarray() attempts to list the contents of an array
 * RETURNS "" if it cannot find the array
 */
BUILT_IN_FUNCTION(function_listarray)
{
	char	*name;
	an_array *array;
	long index;
	char *result = NULL;
	
	if ((name = next_arg(input, &input)) && (array = get_array(name)))
	{
		for (index = 0; index < array->size; index++)
			m_s3cat(&result, space, array->item[index]);
	}
	return result ? result : m_strdup(empty_string);
}

/*
 * function_getmatches() attempts to match a pattern to the contents of an
 * array and returns a list of item_numbers of all items that match the pattern
 * or it returns an empty string if not items matches or if the array was not
 * found.
 */
BUILT_IN_FUNCTION(function_getmatches)
{
        char    *result = NULL;
        char    *name = NULL;
        long    index;
        an_array *array;

        if ((name = next_arg(input, &input)) && 
	    (array = get_array(name)) && *input)
        {
                if (*input)
                {
                        for (index = 0; index < array->size; index++)
                        {
                                if (wild_match(input, array->item[index]) > 0)
					m_s3cat(&result, space, ltoa(index));
                        }
                }
        }
	if (!result)
		RETURN_EMPTY;
        return result;
}

/*
 * function_rmatchitem() attempts to match the input text with an array of
 * patterns, much like RMATCH()
 * RETURNS -1 if it cannot find the array, or -2 if no matches occur
 */
BUILT_IN_FUNCTION(function_rmatchitem)
{
        char    *name = NULL;
        long    index;
        an_array *array;
        long     current_match;
        long     best_match = 0;
        long     match = -1;

        if ((name = next_arg(input, &input)) && (array = get_array(name)))
        {
                match = -2;
                if (*input)
                {
                        for (index = 0; index < array->size; index++)
                        {
                                if ((current_match = wild_match(array->item[index], input)) > best_match)
                                {
                                        match = index;
                                        best_match = current_match;
                                }
                        }
                }
        }
	RETURN_INT(match)
}

/*
 * function_getrmatches() attempts to match the input text with an array of
 * patterns, and returns a list of item_numbers of all patterns that match the
 * given text, or it returns a null string if no matches occur or if the array
 * was not found.
 */
BUILT_IN_FUNCTION(function_getrmatches)
{
        char    *result = NULL;
        char    *name = NULL;
        long    index;
        an_array *array;

        if ((name = next_arg(input, &input)) && (array = get_array(name)))
        {
                if (*input)
                {
                        for (index = 0; index < array->size; index++)
                        {
                                if (wild_match(array->item[index], input) > 0)
					m_s3cat(&result, space, ltoa(index));
                        }
                }
        }

	if (!result)
		RETURN_EMPTY;
        return result;
}

/*
 * function_numitems() returns the number of items in an array, or -1 if unable
 * to find the array
 */
BUILT_IN_FUNCTION(function_numitems)
{
        char *name = NULL;
	an_array *array;
	long items = 0;

        if ((name = next_arg(input, &input)) && (array = get_array(name)))
                items = array->size;

	RETURN_INT(items);
}

/*
 * function_getitem() returns the value of the specified item of an array, or
 * returns an empty string on failure to find the item or array
 */
BUILT_IN_FUNCTION(function_getitem)
{
	char *name = NULL;
	char *itemstr = NULL;
	long item;
	an_array *array;
	char *found = NULL;

	if ((name = next_arg(input, &input)) && (array = get_array(name)))
	{
		if ((itemstr = next_arg(input, &input)))
		{
			item = my_atol(itemstr);
			if (item >= 0 && item < array->size)
				found = array->item[item];
		}
	}
	RETURN_STR(found);
}

/*
 * function_setitem() sets an item of an array to a value, or creates a new
 * array if the array doesn not already exist and the item number is zero, or
 * it adds a new item to an existing array if the item number is one more than
 * the prevously largest item number in the array.
 * RETURNS: 0 on success
 *          1 on success if a new item was added to an existing array
 *          2 on success if a new array was created and item zero was set
 *         -1 if it is unable to find the array (and item number was not zero)
 *         -2 if it was unable to find the item (item < 0 or item was greater
 *            than 1 + the prevous maximum item number
 */
BUILT_IN_FUNCTION(function_setitem)
{
        char *name = NULL;
	char *itemstr = NULL;
	long item;
	long index = 0;
	long oldindex;
	an_array *array;
	int result = -1;

        if ((name = next_arg(input, &input)))
        {
		if (strlen(name) && (itemstr = next_arg(input, &input)))
		{
			item = my_atol(itemstr);
			if (item >= 0)
			{
				upper(name);
				if (array_info.size && ((index = find_item(array_info, name)) >= 0))
       	         		{
					array =  &array_array[array_info.index[index]];
					result = -2;
					if (item < array->size)
					{
						oldindex = find_index(array, item);
						index = find_item(*array, input);
						index = (index >= 0) ? index : (-index) - 1;
						move_index(array, oldindex, index);
						new_free(&array->item[item]);
						malloc_strcpy(&array->item[item], input);
						result = 0;
					}
					else if (item == array->size)
					{
						array->item = (char **)RESIZE(array->item, char *, (array->size + 1));
						array->item[item] = NULL;
						malloc_strcpy(&array->item[item], input);
						index = find_item(*array, input);
						index = (index >= 0) ? index : (-index) - 1;
						insert_index(&array->index, &array->size, index);
						result = 2;
					}
				}
				else
				{
					if (item == 0)
					{
						if (array_info.size)
							RESIZE(array_array, an_array, (array_info.size + 1));
						else
							array_array = (an_array*)new_malloc(sizeof(an_array));
						array = &array_array[array_info.size];
						array->size = 1;
						array->item = (char **)new_malloc(sizeof(char *));
						array->index = (long *)new_malloc(sizeof(long));
						array->item[0] = (char*) 0;
						array->index[0] = 0;
						malloc_strcpy(&array->item[0], input);
						if (array_info.size)
							array_info.item = (char **)RESIZE(array_info.item, char *, (array_info.size + 1));
						else
							array_info.item = (char **)new_malloc(sizeof(char *));
						array_info.item[array_info.size] = NULL;
						malloc_strcpy(&array_info.item[array_info.size], name);
						insert_index(&array_info.index, &array_info.size, (-index) - 1);
						result = 1;
					}
				}
			}
		}
	}
	RETURN_INT(result);
}

/*
 * function_getarrays() returns a string containg the names of all currently
 * existing arrays separated by spaces
 */
BUILT_IN_FUNCTION(function_getarrays)
{
	long index;
	char *result = NULL;

	for (index = 0; index < array_info.size; index++)
                if (!input || !*input || wild_match(input, array_info.item[array_info.index[index]]))
                        m_s3cat(&result, space, array_info.item[array_info.index[index]]);

	if (!result)
		RETURN_EMPTY;

	return result;
}

/*
 * function_numarrays() returns the number of currently existing arrays
 */
BUILT_IN_FUNCTION(function_numarrays)
{
	RETURN_INT(array_info.size)
}

/*
 * function_finditem() does a binary search and returns the item number of
 * the string that exactly matches the string searched for, or it returns
 * -1 if unable to find the array, or -2 if unable to find the item.
 */
BUILT_IN_FUNCTION(function_finditem)
{
        char    *name = NULL;
        an_array *array;
	long	item = -1;

	if ((name = next_arg(input, &input)) && (array = get_array(name)))
        {
		if (*input)
		{
			item = find_item(*array, input);
			item = (item >= 0) ? array->index[item] : -2;
		}
        }
	RETURN_INT(item)
}

/*
 * function_ifinditem() does a binary search and returns the index number of
 * the string that exactly matches the string searched for, or it returns
 * -1 if unable to find the array, or -2 if unable to find the item.
 */
BUILT_IN_FUNCTION(function_ifinditem)
{
        char    *name = NULL;
        an_array *array;
        long    item = -1;

        if ((name = next_arg(input, &input)) && (array = get_array(name)))
        {
		if (*input)
		{
			if ((item = find_item(*array, input)) < 0)
				item = -2;
		}
        }
	RETURN_INT(item)
}

/*
 * function_igetitem() returns the item referred to by the passed-in index
 * or returns an empty string if unable to find the array or if the index was
 * invalid.
 */
BUILT_IN_FUNCTION(function_igetitem)
{
        char *name = NULL;
        char *itemstr = NULL;
        long item;
        an_array *array;
        char *found = NULL;

        if ((name = next_arg(input, &input)) && (array = get_array(name)))
        {
                if ((itemstr = next_arg(input, &input)))
                {
                        item = my_atol(itemstr);
                        if (item >= 0 && item < array->size)
                                found = array->item[array->index[item]];
                }
        }
	RETURN_STR(found)
}

/*
 * function_indextoitem() converts an index number to an item number for the
 * specified array.  It returns a valid item number, or -1 if unable to find
 * the array, or -2 if the index was invalid.
 */
BUILT_IN_FUNCTION(function_indextoitem)
{
        char *name = NULL;
        char *itemstr = NULL;
        long item;
        an_array *array;
	long found = -1;

        if ((name = next_arg(input, &input)) && (array = get_array(name)))
        {
		found = -2;
                if ((itemstr = next_arg(input, &input)))
                {
                        item = my_atol(itemstr);
                        if (item >= 0 && item < array->size)
                                found = array->index[item];
                }
        }
	RETURN_INT(found)
}

/*
 * function_itemtoindex() takes an item number and searches for the index that
 * refers to the item.  It returns the index number, or -1 if unable to find
 * the array, or -2 if the item was invalid.
 */
BUILT_IN_FUNCTION(function_itemtoindex)
{
        char *name;
        char *itemstr;
        long item;
        an_array *array;
        long found = -1;

        if ((name = next_arg(input, &input)) && (array = get_array(name)))
	{
                found = -2;
		if ((itemstr = next_arg(input, &input)))
                {
                        item = my_atol(itemstr);
                        if (item >= 0 && item < array->size)
                                found = find_index(array, item);
                }
        }
	RETURN_INT(found)
}

/*
 * function_delitem() deletes an item of an array and moves the contents of the
 * array that were stored "above" the item down by one.  It returns 0 (zero)
 * on success, -1 if unable to find the array, -2 if unable to find the item.
 * Also, if the item is the last item in the array, it deletes the array.
 */
BUILT_IN_FUNCTION(function_delitem)
{
        char *name;
	char *itemstr;
        char **strptr;
        long item;
	long cnt;
	long oldindex;
        an_array *array;
        long found = -1;

        if ((name = next_arg(input, &input)) && (array = get_array(name)))
        {
		found = -2;
                if ((itemstr = next_arg(input, &input)))
                {
                        item = my_atol(itemstr);
                        if (item >= 0 && item < array->size)
			{
                                if (array->size == 1)
					delete_array(name);
				else
				{
					oldindex = find_index(array, item);
					for(cnt = 0; cnt < array->size; cnt++)
						if (array->index[cnt] > item)
							(array->index[cnt])--;
					move_index(array, oldindex, array->size);
                                	new_free(&array->item[item]);
					array->size--;
					for(strptr=&(array->item[item]), cnt=item; cnt < array->size; cnt++, strptr++)
						*strptr = *(strptr + 1);
					array->item = (char**)RESIZE(array->item, char *, array->size);
					array->index = (long*)RESIZE(array->index, long, array->size);
				}
				found = 0;
                        }
                }
        }
	RETURN_INT(found)
}

/*
 * function_delarray() deletes the entire contents of the array using the
 * delete_array() function above.  It returns 0 on success, -1 on failure.
 */
BUILT_IN_FUNCTION(function_delarray)
{
        char *name;
        long found = -1;

	if ((name = next_arg(input, &input)) && (get_array(name)))
        {
                delete_array(name);
		found = 0;
	}
	RETURN_INT(found)
}
/*
 * function_ifindfirst() returns the first index of an exact match with the
 * search string, or returns -2 if unable to find the array, or -1 if unable
 * to find any matches.
 */
BUILT_IN_FUNCTION(function_ifindfirst)
{
        char    *name;
        an_array *array;
        long    item = -1;

        if ((name = next_arg(input, &input)) && (array = get_array(name)))
        {
		if (*input)
		{
			if ((item = find_item(*array, input)) < 0)
				item = -2;
			else
			{
				while (item >= 0 && !strcmp(array->item[array->index[item]], input))
					item--;
				item++;
			}
		}
        }
	RETURN_INT(item)
}

/*
 * Given an array name with various strings in it, we wild_match() against
 * the elements within the array. This allows parsing using % and * 
 * for wildcards. We return only the best match from the array, unlike 
 * getmatch() which returns ALL the matching items.
 */

BUILT_IN_FUNCTION(function_gettmatch)
{
char *name;
an_array *array;
char *ret = NULL;
#if 0
<shade> gettmatch(users % user@host *) would match the userhost mask in the
             second word of the array
#endif
        if ((name = next_arg(input, &input)) && (array = get_array(name)))
        {
		if (*input)
          	{
			int index, current_match;
			int best_match = 0;
			int match = -1;
                        for (index = 0; index < array->size; index++)
                        {
                                if ((current_match = wild_match(input, array->item[index])) > best_match)
                                {
                                        match = index;
                                        best_match = current_match;
                                }
                        }
			if (match != -1)
				ret = array->item[match];

		}
	}
	RETURN_STR(ret);
}

BUILT_IN_FUNCTION(function_igetrmatches)
{
	char    *result = (char *) 0;
	char    *name = (char *) 0;
	long    item;
	an_array *array;

	if ((name = next_arg(input, &input)) &&
		(array = get_array(name)) && *input)
	{
		if (*input)
		{
			for (item = 0; item < array->size; item++)
			{
				if (wild_match(array->item[item], input) > 0)
					m_s3cat(&result, space, ltoa(find_index(array, item)));
			}
		}
	}

	if (!result)
		RETURN_EMPTY;

	return result;
}
