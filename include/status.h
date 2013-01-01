/*
 * status.h: header for status.c
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: status.h 3 2008-02-25 09:49:14Z keaston $
 */

#ifndef __status_h_
#define __status_h_

	void	make_status (Window *);
	void	set_alarm (Window *, char *, int);
	char	*BX_update_clock (int);
	void	reset_clock (Window *, char *, int);
	void	BX_build_status (Window *, char *, int);
	int	BX_status_update (int);

#define GET_TIME 1
#define RESET_TIME 2

#endif /* __status_h_ */
