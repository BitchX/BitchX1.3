/*
 * window.h: header file for window.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: window.h 26 2008-04-30 13:57:56Z keaston $
 */

#ifndef __window_h_
#define __window_h_

#include "irc_std.h"
#include "lastlog.h"

/* used by the update flag to determine what needs updating */
#define REDRAW_DISPLAY_FULL	1
#define REDRAW_DISPLAY_FAST	2
#define UPDATE_STATUS		4
#define REDRAW_STATUS		8

#define	LT_UNLOGGED		0
#define	LT_LOGHEAD		1
#define	LT_LOGTAIL		2

/* var_settings indexes */
#define OFF			0
#define ON			1
#define TOGGLE			2

#define WINDOW_NO_SERVER	-1
#define WINDOW_DLL		-2
#define WINDOW_SERVER_CLOSED	-3

	Window	*BX_new_window			(struct ScreenStru *);
	void	BX_delete_window			(Window *);
	void	BX_add_to_invisible_list		(Window *);
	Window	*BX_add_to_window_list		(struct ScreenStru *, Window *);
	void	BX_remove_from_window_from_screen	(Window *);
	void	BX_recalculate_window_positions	(struct ScreenStru *);
	void	BX_redraw_all_windows		(void);
	void	BX_recalculate_windows		(struct ScreenStru *);
	void	BX_rebalance_windows		(struct ScreenStru *);
	void	BX_update_all_windows		(void);
	void	BX_set_current_window		(Window *);
	void	BX_hide_window			(Window *);
	void	BX_swap_last_window		(char, char *);
	void	BX_next_window			(char, char *);
	void	BX_swap_next_window		(char, char *);
	void	BX_previous_window			(char, char *);
	void	BX_swap_previous_window		(char, char *);
	void	BX_back_window			(char, char *);
	Window	*BX_get_window_by_refnum		(unsigned);
	Window	*BX_get_window_by_name		(const char *);
	char	*BX_get_refnum_by_window		(const Window *);
	int	is_window_visible		(char *);
	void	BX_update_window_status		(Window *, int);
	void	BX_update_all_status		(Window *, char *, int);
	void	BX_set_prompt_by_refnum		(unsigned, char *);
	char	*BX_get_prompt_by_refnum		(unsigned);
	char	*BX_get_target_by_refnum		(unsigned);
	void	BX_set_query_nick			(char *, char *, char *);
	int	BX_is_current_channel		(char *, int, int);
const	char	*BX_set_current_channel_by_refnum	(unsigned, char *);
	char	*BX_get_current_channel_by_refnum	(unsigned);
	int	BX_is_bound_to_window		(const Window *, const char *);
	Window	*BX_get_window_bound_channel	(const char *);
	int	BX_is_bound_anywhere		(const char *);
	int	BX_is_bound			(const char *, int);
	void	BX_unbind_channel 			(const char *, int);
	char	*BX_get_bound_channel		(Window *);
	int	BX_get_window_server		(unsigned);
	void	BX_set_window_server		(int, int, int);
	int	windows_connected_to_server	(int);
	void	BX_set_level_by_refnum		(unsigned, unsigned long);
	void	BX_message_to			(unsigned long);

#if 0
	void	save_message_from		(char **, unsigned long *);
	void	restore_message_from		(char *, unsigned long);
	void	message_from			(char *, unsigned long);
	int	message_from_level		(unsigned long);
#endif
	void	BX_set_display_target		(const char *, unsigned long);
	void	set_display_target_by_winref	(unsigned int);
	void	BX_set_display_target_by_desc	(char *);
	void	BX_reset_display_target		(void);
	void	BX_save_display_target		(const char **, unsigned long *);
	void	BX_restore_display_target		(const char *, unsigned long);

	void	BX_clear_all_windows		(int, int);
	void	BX_clear_window_by_refnum		(unsigned);
	void	set_scroll			(Window *, char *, int);
	void	BX_set_scroll_lines		(Window *, char *, int);
	void	BX_set_continued_lines		(Window *, char *, int);
	unsigned BX_current_refnum			(void);
	int	BX_number_of_windows_on_screen	(Window *);
	void	delete_display_line		(Display *);
	Display	*new_display_line		(Display *);
	void	BX_scrollback_backwards_lines	(int);
	void	BX_scrollback_forwards_lines	(int);
	void	BX_scrollback_backwards		(char, char *);
	void	BX_scrollback_forwards		(char, char *);
	void	BX_scrollback_end			(char, char *);
	void	BX_scrollback_start		(char, char *);
	void	BX_set_hold_mode		(Window *, int, int);
	void	BX_unstop_all_windows		(char, char *);
	void	BX_reset_line_cnt			(Window *, char *, int);
	void	BX_toggle_stop_screen		(char, char *);
	void	BX_flush_everything_being_held	(Window *);
	int	BX_unhold_a_window			(Window *);
	char	*BX_get_target_cmd_by_refnum	(u_int);
	void	BX_recalculate_window_cursor	(Window *);
	int	BX_is_window_name_unique		(char *);
	int	BX_get_visible_by_refnum		(char *);
	void	BX_resize_window			(int, Window *, int);
	Window	*window_list			(Window *, char **, char *);
	void	BX_move_window			(Window *, int);
	void	BX_show_window			(Window *);
	int	BX_traverse_all_windows		(Window **);
	Window	*BX_get_window_by_desc		(const char *);
	char	*BX_get_nicklist_by_window		(Window *); /* XXX */
	void	BX_set_scrollback_size		(Window *, char *, int);
	Window	*window_query			(Window *, char **, char *);
	int	BX_unhold_windows			(void);
	void	free_window			(Window *);
	Window	*BX_get_window_target_by_desc	(char *);
	BUILT_IN_COMMAND(windowcmd);

	char	*BX_get_status_by_refnum		(unsigned , unsigned);
	void	BX_unclear_window_by_refnum	(unsigned);
	void	BX_clear_scrollback		(Window *);
	void 	BX_clear_window			(Window *window);
	void	BX_repaint_window			(Window *, int, int);
	void	BX_remove_window_from_screen	(Window *);
	void	BX_set_screens_current_window	(Screen *, Window *);
	void	BX_make_window_current		(Window *);
	void	BX_make_window_current_by_refnum	(int);
	void	BX_free_formats			(Window *);
	void	BX_goto_window			(Screen *, int);
	void	BX_update_window_status_all	(void);
	void	BX_window_check_servers		(int);
	int	windows_connected_to_server	(int);
	void	window_change_server		(int, int);
	void	make_window_current_by_winref	(int);
	void	make_window_current_by_desc	(char *);
	int	get_winref_by_desc		(const char *);
	int	get_current_winref		(void);
	void	make_to_window_by_desc		(char *);
	Window	*is_querying			(char *);
	void	open_window			(char *);
	void	close_window			(char *);
	void	win_create(int, int);
	void	BX_change_window_server		(int, int);	
unsigned long	message_from_level		(unsigned long);
						
extern	Window	*invisible_list;
extern	unsigned long	who_level;
extern	int	in_window_command;
extern	unsigned int	window_display;
extern	Window	*target_window;
extern	Window	*current_window;
extern	void	*default_output_function;
extern	int	status_update_flag;

#define BUILT_IN_WINDOW(x) Window *x (Window *window, char **args, char *usage) 

BUILT_IN_WINDOW(window_server);

#define WINDOW_NOTIFY	((unsigned) 0x0001)
#define WINDOW_NOTIFIED	((unsigned) 0x0002)

#endif /* __window_h_ */
