/*
 * alias.h: header for alias.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: alias.h 3 2008-02-25 09:49:14Z keaston $
 */

#ifndef _ALIAS_H_
#define _ALIAS_H_

#include "irc_std.h"

#define DCC_STRUCT 0
#define USERLIST_STRUCT 1
#define NICK_STRUCT 2
#define SERVER_STRUCT 3


extern BuiltInDllFunctions *dll_functions;

/*
 * XXXX - These need to go away
 */
#define COMMAND_ALIAS 0
#define VAR_ALIAS 1
#define VAR_ALIAS_LOCAL 2

/*
 * These are the user commands.  Dont call these directly.
 */
extern	void	aliascmd  		(char *, char *, char *, char *);
extern	void	assigncmd 		(char *, char *, char *, char *);
extern	void	localcmd  		(char *, char *, char *, char *);
extern	void	stubcmd   		(char *, char *, char *, char *);
extern	void	dumpcmd   		(char *, char *, char *, char *);

extern	void 	add_var_alias      	(char *, char *);
extern	void 	add_local_alias    	(char *, char *);
/*extern	void 	add_cmd_alias 	   	(char *, char *);*/
extern	void 	add_var_stub_alias 	(char *, char *);
extern	void 	add_cmd_stub_alias 	(char *, char *);

extern	char *	get_variable		(char *);
extern	char **	glob_cmd_alias		(char *, int *);
extern	char *	get_cmd_alias   	(char *, int *, char **, void **);
extern	char **	get_subarray_elements 	(char *, int *, int);


/* These are in expr.c */
/*
 * This function is a general purpose interface to alias expansion.
 * The second argument is the text to be expanded.
 * The third argument are the command line expandoes $0, $1, etc.
 * The fourth argument is a flag whether $0, $1, etc are used
 * The fifth argument, if set, controls whether only the first "command"
 *   will be expanded.  If set, this argument will be set to the "rest"
 *   of the commands (after the first semicolon, or the null).  If NULL,
 *   then the entire text will be expanded.
 */
extern	char *	BX_expand_alias 		(const char *, const char *, int *, char **);

/*
 * This is the interface to the "expression parser"
 * The first argument is the expression to be parsed
 * The second argument is the command line expandoes ($0, $1, etc)
 * The third argument will be set if the command line expandoes are used.
 */
extern	char *	BX_parse_inline 		(char *, const char *, int *);

/*
 * This function is used to call a user-defined function.
 * Noone should be calling this directly except for call_function.
 */
extern	char *	call_user_function 	(char *, char *);
extern	void	call_user_alias		(char *, char *, char *, void *);



/*
 * This function is sued to save all the current aliases to a global
 * file.  This is used by /SAVE and /ABORT.
 */
extern	void	save_aliases 		(FILE *, int);
extern	void	save_assigns 		(FILE *, int);

/*
 * This function is in functions.c
 * This function allows you to execute a primitive "BUILT IN" expando.
 * These are the $A, $B, $C, etc expandoes.
 * The argument is the character of the expando (eg, 'A', 'B', etc)
 *
 * This is in functions.c
 */
extern 	char *	built_in_alias		(char, int *);



/* BOGUS */

/*
 * This function is used by parse_command to directly execute an alias.
 * Noone should be calling this function directly. (call parse_line.)
 */
/*extern	void	execute_alias 		(char *, char *, char *);*/
		void	prepare_alias_call      (void *, char **);
		void	destroy_alias_call      (void *);


/*
 * This is in functions.c
 * This is only used by next_unit and expand_alias to call built in functions.
 * Noone should call this function directly.
 */
extern char *	call_function		(char *, const char *, int *);



/*
 * These are the two primitives for runtime stacks.
 */
extern	void	BX_make_local_stack 	(char *);
extern	void	BX_destroy_local_stack 	(void);
extern	void	set_current_command 	(char *);
extern	void	bless_local_stack 	(void);
extern	void	unset_current_command 	(void);
extern	void	dump_call_stack		(void);
extern	void	panic_dump_call_stack 	(void);
extern  void    BX_lock_stack_frame        (void);
extern  void    BX_unlock_stack_frame      (void);
extern	void	destroy_aliases		(int);
	Alias	*find_var_alias		(char *);
	void	delete_var_alias	(char *, int);
	char	*parse_line_with_return	(char *, char *, char *, int, int);
/*
 * This is the alias interface to the /STACK command.
 */
extern	void	do_stack_alias 		(int, char *, int);
	char	*BX_next_unit		(char *, const char *, int *, int);
	char	*BX_alias_special_char(char **buffer, char *ptr, const char *args, char *quote_em, int *args_flag);

#endif /* _ALIAS_H_ */
