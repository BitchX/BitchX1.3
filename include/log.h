/*
 * log.h: header for log.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: log.h 3 2008-02-25 09:49:14Z keaston $
 */

#ifndef __log_h_
#define __log_h_

	void	do_log (int, char *, FILE **);
	void	logger (Window *, char *, int);
	void	set_log_file (Window *, char *, int);
	void	BX_add_to_log (FILE *, time_t, const char *, int mangler);


#endif /* __log_h_ */
