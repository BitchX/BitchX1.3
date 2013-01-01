/*
 * notify.h: header for notify.c
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: notify.h 3 2008-02-25 09:49:14Z keaston $
 */

#ifndef __notify_h_
#define __notify_h_

	void	show_notify_list (int);
	BUILT_IN_COMMAND(notify);
	void	do_notify (void);
	void	notify_mark (char *, char *, int, int);
	void	save_notify (FILE *);
	void	set_notify_handler (Window *, char *, int);
	void	make_notify_list (int);
	int	hard_uh_notify (int, char *, char *, char *);
extern	char	*get_notify_nicks (int, int, char *, int);
	void	add_delay_notify (int);
	void	notify_count (int, int *, int *);
	void	rebuild_notify_ison (int);
	void	rebuild_all_ison (void);
	void	save_watch(FILE *);
	BUILT_IN_COMMAND(watchcmd);
	void	show_watch_list (int);
	void	make_watch_list (int);
	void	show_watch_notify (char *, int, char **);
	void	send_watch (int);
	char	*get_watch_nicks (int, int, char *, int);
				
#endif /* __notify_h_ */
