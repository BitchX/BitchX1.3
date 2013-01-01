/*
 * stack.h - header for stack.c
 *
 * written by matthew green
 *
 * copyright (c) 1993, 1994.
 *
 * @(#)$Id: stack.h 3 2008-02-25 09:49:14Z keaston $
 */

#ifndef __stack_h_
# define __stack_h_

#include "hook.h"
#include "alias.h"

	void	stackcmd  (char *, char *, char *, char *);

#define STACK_POP 0
#define STACK_PUSH 1
#define STACK_SWAP 2
#define STACK_LIST 3

#define STACK_DO_ALIAS	0x0001
#define STACK_DO_ASSIGN	0x0002

typedef	struct	AliasStru1
{
	char	*name;			/* name of alias */
	char	*stuff;			/* what the alias is */
	char	*stub;			/* the file its stubbed to */
	int	mark;			/* used to prevent recursive aliasing */
	int	global;			/* set if loaded from `global' */
	void	*what;		/* pointer to structure */
	int	struct_type;		/* type of structure */
	struct	AliasStru1 *next;	/* pointer to next alias in list */
}	Alias1;

typedef	struct	aliasstacklist1
{
	int	which;
	char	*name;
	IrcVariable *set;
	enum	VAR_TYPES var_index;
	Alias1	*list;
	struct aliasstacklist1 *next;
}	AliasStack1;

#endif /* __stack_h_ */
