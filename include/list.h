/*
 * list.h: header for list.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: list.h 3 2008-02-25 09:49:14Z keaston $
 */

#ifndef __list_h_
#define __list_h_

	void	BX_add_to_list (List **, List *);
	List	*BX_find_in_list (List **, char *, int);
	List	*BX_remove_from_list (List **, char *);
	List	*BX_removewild_from_list (List **, char *);
	List	*BX_list_lookup_ext (List **, char *, int, int, int (*)(List *, char *));
	List	*BX_list_lookup (List **, char *, int, int);
	List	*BX_remove_from_list_ext (List **, char *, int (*)(List *, char *));
	void	BX_add_to_list_ext (List **, List *, int (*)(List *, List *));
	List	*BX_find_in_list_ext (List **, char *, int, int (*)(List *, char *));
	int	list_strnicmp (List *item1, char *str);

#define REMOVE_FROM_LIST 1
#define USE_WILDCARDS 1

#endif /* __list_h_ */
