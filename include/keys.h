/*
 * keys.h: header for keys.c 
 *
 * Copyright 1990 Michael Sandrof
 * Copyright 1997 EPIC Software Labs
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifndef __keys_h__
#define __keys_h__

/* I hate typedefs... */
typedef void (*KeyBinding) (char, char *);

	BUILT_IN_COMMAND(bindcmd);
	BUILT_IN_COMMAND(rbindcmd);
	BUILT_IN_COMMAND(parsekeycmd);
	BUILT_IN_COMMAND(type);

	int		get_binding	(int, unsigned char,
					 KeyBinding *, char **);
	void		save_bindings 	(FILE *, int);
	void		init_keys 	(void);
	void		init_keys2 	(void);
	void		remove_bindings	(void);
	void		unload_bindings	(const char *);
	void		resize_metamap	(int);
	void		disable_stop	(void);
	char		*convert_to_keystr (char *);
		
#ifdef GUI
enum MOUSE_ACTIONS {
   RCLICK,
   STATUSRCLICK,
   NICKLISTRCLICK,
   LCLICK,
   STATUSLCLICK,
   NICKLISTLCLICK,
   MCLICK,
   STATUSMCLICK,
   NICKLISTMCLICK,
   RDBLCLICK,
   STATUSRDBLCLICK,
   NICKLISTRDBLCLICK,
   LDBLCLICK,
   STATUSLDBLCLICK,
   NICKLISTLDBLCLICK,
   MDBLCLICK,
   STATUSMDBLCLICK,
   NICKLISTMDBLCLICK,
   MAX_MOUSE
};
#endif

#endif /* _KEYS_H_ */
