/*
 * screen.h: header for screen.c
 *
 * written by matthew green.
 *
 * copyright (c) 1993, 1994.
 *
 * see the copyright file, or type help ircii copyright
 *
 * @(#)$Id: screen.h 3 2008-02-25 09:49:14Z keaston $
 */

#ifndef __screen_h_
#define __screen_h_

#include "window.h"

#define WAIT_PROMPT_LINE        0x01
#define WAIT_PROMPT_KEY         0x02
#define WAIT_PROMPT_DUMMY	0x04

/* Stuff for the screen/xterm junk */

#define ST_NOTHING      -1
#define ST_SCREEN       0
#define ST_XTERM        1

/* This is here because it happens in so many places */
#define curr_scr_win	current_screen->current_window

	void	clear_window (Window *);
	int	BX_output_line (const unsigned char *);
	Window	*BX_create_additional_screen (void);
	void	BX_scroll_window (Window *);
	void	update_all_windows (void);
	void	BX_add_wait_prompt (char *, void (*) (char *, char *), char *, int, int);
	void	BX_cursor_in_display (Window *);
	int	BX_is_cursor_in_display (Screen *);
	void	BX_cursor_not_in_display (Screen *);
	void	set_current_screen (Screen *);
	void	window_redirect (char *, int);
	void	redraw_resized (Window *, ShrinkInfo, int);
	void	close_all_screen (void);
RETSIGTYPE	sig_refresh_screen (int);
	int	check_screen_redirect (char *);
	void	BX_kill_screen (Screen *);
	int	rite (Window *, const unsigned char *);
	ShrinkInfo	resize_display (Window *);
	void	redraw_window (Window *, int);
	void	redraw_all_windows (void);
	void	BX_add_to_screen (unsigned char *);
	void	do_screens (fd_set *);
	unsigned	char	**BX_split_up_line(const unsigned char *, int);
	void	BX_xterm_settitle(void);
	void	BX_add_to_window(Window *, const unsigned char *);
	
Screen  * BX_create_new_screen(void);

#ifdef GUI
	void	refresh_window_screen(Window *);
#endif
			
	u_char *BX_strip_ansi		(const u_char *);
	char   *normalize_color		(int, int, int, int);
const	u_char *BX_skip_ctl_c_seq		(const u_char *, int *, int *, int);
	u_char **BX_prepare_display	(const u_char *, int, int *, int);
	int	BX_output_with_count	(const unsigned char *, int, int);
unsigned char	*BX_skip_incoming_mirc	(unsigned char *);
void delchar(unsigned char **text, int cnum);

/* Dont do any word-wrapping, just truncate each line at its place. */
#define PREPARE_NOWRAP	0x01

extern	Screen	*main_screen;
extern	Screen	*last_input_screen;
extern	Screen	*screen_list;
extern	Screen	*output_screen;
extern  Window	*debugging_window;

extern	int	strip_ansi_never_xlate;

#endif /* __screen_h_ */
