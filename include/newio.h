/*
 * newio.h -- header file for newio.c
 *
 * Copyright 1990, 1995 Michael Sandrof, Matthew Green
 * Copyright 1997 EPIC Software Labs
 */

#ifndef __newio_h__
#define __newio_h__

#include "ssl.h"

extern 	int 	dgets_errno;

	const char *dgets_strerror(int);
	int 	BX_dgets 			(char *, int, int, int, void *);
	int 	new_select 		(fd_set *, fd_set *, struct timeval *);
	int	BX_new_open		(int);
	int 	BX_new_close 		(int);
	int	new_close_write		(int);
	int	new_open_write		(int);
	void 	set_socket_options 	(int);
	size_t	get_pending_bytes	(int);

#define IO_BUFFER_SIZE 8192

#endif
