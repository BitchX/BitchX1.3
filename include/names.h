/*
 * names.h: Header for names.c
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: names.h 134 2011-08-30 14:50:42Z keaston $
 */

#ifndef __names_h_
#define __names_h_

#include "window.h"
#include "irc.h"

/* for lookup_channel() */
#define	CHAN_NOUNLINK	1
#define CHAN_UNLINK	2

#define	GOTNAMES	0x01
#define	GOTMODE		0x02
#define GOTBANS		0x04
#define GOTWHO		0x08
#define	GOTEXEMPT	0x10

/* Channel mode flags */
#define MODE_ANONYMOUS	(1U << 0)	/* av2.9 */
#define MODE_C			(1U << 1)	/* erf/TS4 */
#define MODE_INVITE 	(1U << 2)	/* RFC */
#define MODE_KEY    	(1U << 3)	/* RFC */
#define MODE_LIMIT		(1U << 4)	/* RFC */
#define MODE_MODERATED	(1U << 5)	/* RFC */
#define MODE_MSGS		(1U << 6)	/* RFC */
#define MODE_PRIVATE	(1U << 7)	/* RFC */
#define MODE_REGISTERED (1U << 8)	/* Dalnet */
#define MODE_SECRET		(1U << 9)	/* RFC */
#define MODE_TOPIC		(1U << 10)	/* RFC */
#define MODE_Z          (1U << 11)  /* erf/TS4 */
#define MODE_RESTRICTED (1U << 12)  /* Dalnet */

void		add_to_join_list (char *, int, int);
void		remove_from_join_list (char *, int);
char		*get_chan_from_join_list (int);
int		get_win_from_join_list (char *, int);
int		in_join_list (char *, int);
int		got_info (char *, int, int);
	
int		is_channel_mode (char *, int, int);
int		BX_is_chanop (char *, char *);
int		BX_is_halfop (char *, char *);
char		*is_chanoper (char *, char *);
ChannelList	*BX_lookup_channel (char *, int, int);
char		*BX_get_channel_mode (char *, int);
#ifdef	INCLUDE_UNUSED_FUNCTIONS
void		set_channel_mode (char *, int, char *);
#endif /* INCLUDE_UNUSED_FUNCTIONS */
ChannelList *	BX_add_channel (char *, int, int);
ChannelList *	BX_add_to_channel (char *, char *, int, int, int, char *, char *, char *, int, int);
void		BX_remove_channel (char *, int);
void		BX_remove_from_channel (char *, char *, int, int, char *);
int		BX_is_on_channel (char *, int, char *);
void		list_channels (void);
void		reconnect_all_channels (int);
void		switch_channels (char, char *);
char		*what_channel (char *, int);
ChannelList	*walk_channels (char *, int, int);
char		*real_channel (void);
void		BX_rename_nick (char *, char *, int);
void		update_channel_mode (char *, char *, int, char *, ChannelList *);
void		set_channel_window (Window *, char *, int);
char		*BX_create_channel_list (Window *);
int		BX_get_channel_oper (char *, int);
int		BX_get_channel_halfop (char *, int);
void		channel_server_delete (int);
void		change_server_channels (int, int);
void		clear_channel_list (int);
void		set_waiting_channel (int);
void		remove_from_mode_list (char *, int);
int		chan_is_connected (char *, int);
int		BX_im_on_channel (char *, int);
char		*BX_recreate_mode (ChannelList *);	
int		BX_get_channel_voice (char *, int);
char		*BX_get_channel_key(char *, int);
char		*BX_fetch_userhost (int, char *);
void		unset_window_current_channel (Window *);
void		move_window_channels (Window *);
void		reassign_window_channels (Window *);
void		check_channel_limits();
void		BX_clear_bans(ChannelList *);
char		*BX_do_compress_modes(ChannelList *, int, char *, char*);
int		BX_got_ops(int, ChannelList *);
void		BX_flush_channel_stats (void);
char		*BX_get_channel_bans(char *, int, int);

#endif /* __names_h_ */
