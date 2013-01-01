/*
 * files.h -- header file for files.c
 *
 * Direct file manipulation for irc?  Unheard of!
 * (C) 1995 Jeremy Nelson
 * See the COPYRIGHT file for copyright information
 *
 */

#ifndef FILES_H
#define FILES_H

#include "irc_std.h"

extern	int	open_file_for_read (char *);
extern	int	open_file_for_write (char *);
extern	int	file_write (int, char *);
extern	int	file_writeb (int, char *);
extern	char *	file_read (int);
extern	char *	file_readb (int, int);
extern	int	file_eof (int);
extern	int	file_close (int);
extern 	int	open_file_for_bwrite (char *);
extern	int	file_copy (int, int);
extern        int     file_valid (int);

#endif
