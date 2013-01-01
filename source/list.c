/*
 * list.c: some generic linked list managing stuff 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */


#include "irc.h"
static char cvsrevision[] = "$Id: list.c 3 2008-02-25 09:49:14Z keaston $";
CVS_REVISION(list_c)
#include "struct.h"

#include "list.h"

#include "ircaux.h"
#define MAIN_SOURCE
#include "modval.h"

static	int	add_list_stricmp (List *, List *);
static	int	list_stricmp (List *, char *);
static	int	list_match (List *, char *);

/*
 * These have now been made more general. You used to only be able to
 * order these lists by alphabetical order. You can now order them
 * arbitrarily. The functions are still called the same way if you
 * wish to use alphabetical order on the key string, and the old
 * function name now represents a stub function which calls the
 * new with the appropriate parameters.
 *
 * The new function name is the same in each case as the old function
 * name, with the addition of a new parameter, cmp_func, which is
 * used to perform comparisons.
 *
 */

/*
static	int add_list_strcmp(List *item1, List *item2)
{
	return strcmp(item1->name, item2->name);
}
*/

static	int add_list_stricmp(List *item1, List *item2)
{
	return my_stricmp(item1->name, item2->name);
}

/*
static	int list_strcmp(List *item1, char *str)
{
	return strcmp(item1->name, str);
}
*/

static	int list_stricmp(List *item1, char *str)
{
	return my_stricmp(item1->name, str);
}

int list_strnicmp(List *item1, char *str)
{
	return my_strnicmp(item1->name, str, strlen(str));
}

static int  list_wildstrcmp(List *item1, char *str)
{
	if (wild_match(item1->name, str) || wild_match(str, item1->name))
		return 0;
	else
		return 1;
}

static	int list_match(List *item1, char *str)
{
	return wild_match(item1->name, str);
}

/*
 * add_to_list: This will add an element to a list.  The requirements for the
 * list are that the first element in each list structure be a pointer to the
 * next element in the list, and the second element in the list structure be
 * a pointer to a character (char *) which represents the sort key.  For
 * example 
 *
 * struct my_list{ struct my_list *next; char *name; <whatever else you want>}; 
 *
 * The parameters are:  "list" which is a pointer to the head of the list. "add"
 * which is a pre-allocated element to be added to the list.  
 */
void BX_add_to_list_ext(List **list, List *add, int (*cmp_func)(List *, List *))
{
register List	*tmp;
	 List	*last;

	if (!cmp_func)
		cmp_func = add_list_stricmp;
	last = NULL;
	for (tmp = *list; tmp; tmp = tmp->next)
	{
		if (cmp_func(tmp, add) > 0)
			break;
		last = tmp;
	}
	if (last)
		last->next = add;
	else
		*list = add;
	add->next = tmp;
}

void BX_add_to_list(List **list, List *add)
{
	add_to_list_ext(list, add, NULL);
}


/*
 * find_in_list: This looks up the given name in the given list.  List and
 * name are as described above.  If wild is true, each name in the list is
 * used as a wild card expression to match name... otherwise, normal matching
 * is done 
 */
List	* BX_find_in_list_ext(register List **list, char *name, int wild, int (*cmp_func)(List *, char *))
{
register List	*tmp;
	int	best_match,
		current_match;

	if (!cmp_func)
		cmp_func = wild ? list_match : list_stricmp;
	best_match = 0;

	if (wild)
	{
		register List	*match = NULL;

		for (tmp = *list; tmp; tmp = tmp->next)
		{
			if ((current_match = cmp_func(tmp, name)) > best_match)
			{
				match = tmp;
				best_match = current_match;
			}
		}
		return (match);
	}
	else
	{
		for (tmp = *list; tmp; tmp = tmp->next)
			if (cmp_func(tmp, name) == 0)
				return (tmp);
	}
	return NULL;
}

List	* BX_find_in_list(List **list, char *name, int wild)
{
	return find_in_list_ext(list, name, wild, NULL);
}

/*
 * remove_from_list: this remove the given name from the given list (again as
 * described above).  If found, it is removed from the list and returned
 * (memory is not deallocated).  If not found, null is returned. 
 */
List	*BX_remove_from_list_ext(List **list, char *name, int (*cmp_func)(List *, char *))
{
register	List	*tmp;
		List 	*last;

	if (!cmp_func)
		cmp_func = list_stricmp;
	last = NULL;
	for (tmp = *list; tmp; tmp = tmp->next)
	{
		if (!cmp_func(tmp, name))
		{
			if (last)
				last->next = tmp->next;
			else
				*list = tmp->next;
			return (tmp);
		}
		last = tmp;
	}
	return NULL;
}

List	*BX_remove_from_list(List **list, char *name)
{
	return remove_from_list_ext(list, name, NULL);
}

List	*BX_removewild_from_list(List **list, char *name)
{
	return remove_from_list_ext(list, name, list_wildstrcmp);
}


/*
 * list_lookup: this routine just consolidates remove_from_list and
 * find_in_list.  I did this cause it fit better with some already existing
 * code 
 */
List	*BX_list_lookup_ext(register List **list, char *name, int wild, int delete, int (*cmp_func)(List *, char *))
{
register List	*tmp;

	if (delete)
		tmp = remove_from_list_ext(list, name, cmp_func);
	else
		tmp = find_in_list_ext(list, name, wild, cmp_func);
	return (tmp);
}

List	*BX_list_lookup(List **list, char *name, int wild, int delete)
{
	return list_lookup_ext(list, name, wild, delete, NULL);
}
