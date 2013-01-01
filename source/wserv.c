/*
 * wserv.c - little program to be a pipe between a screen or
 * xterm window to the calling ircII process.
 *
 * Written by Troy Rollo, Copyright 1992 Troy Rollo
 * Finished by Matthew Green, Copyright 1993 Matthew Green
 * Support for 4.4BSD by Jeremy Nelson, Copyright 1997 EPIC Software Labs
 *
 * Works by opening up the unix domain socket that ircII bind's
 * before calling wserv, and which ircII also deleted after the
 * connection has been made.
 */

#if 0
static	char	rcsid[] = "@(#)$Id: wserv.c 3 2008-02-25 09:49:14Z keaston $";
#endif

#include "defs.h"
#include "config.h"
#include "irc.h"
#include "struct.h"

#include "ircterm.h"
#include "ircaux.h"
#define WTERM_C
#include "modval.h"
#include <errno.h>
#include <sys/uio.h>

static 	int 	s;
static	char	buffer[256];

void 	my_exit(int);
void 	ignore (int value);

#ifdef CLOAKED
extern char proctitlestr[140];
extern char **Argv;
extern char *LastArgv;
#endif

int main (int argc, char **argv)
{
	fd_set		reads;
	int		nread;
	unsigned short 	port;
	char 		*host;
	char		*tmp;
	int		t;
	char		stuff[100];
	        
#ifndef WINNT
	my_signal(SIGWINCH, SIG_IGN, 0);
#endif
	my_signal(SIGHUP, SIG_IGN, 0);
	my_signal(SIGQUIT, SIG_IGN, 0);
	my_signal(SIGINT, ignore, 0);

	if (argc != 3)    /* no socket is passed */
		my_exit(1);

	host = argv[1];
	port = (unsigned short)atoi(argv[2]);
	if (!port)
		my_exit(2);		/* what the hey */

	s = connect_by_number(host, &port, SERVICE_CLIENT, PROTOCOL_TCP, 0);
	if (s < 0)
		my_exit(23);

	/*
	 * first line from a wserv program is the tty.  this is so ircii
	 * can grab the size of the tty, and have it changed.
	 */
	tmp = ttyname(0);
	sprintf(stuff, "%s\n", tmp);
	t = write(s, stuff, strlen(stuff));
	term_init(NULL);
	printf("t is %d", t);

	/*
	 * The select call..  reads from the socket, and from the window..
	 * and pipes the output from out to the other..  nice and simple
	 */
	for (;;)
	{
		FD_ZERO(&reads);
		FD_SET(0, &reads);
		FD_SET(s, &reads);
		if (select(s + 1, &reads, NULL, NULL, NULL) <= 0)
			if (errno == EINTR)
				continue;

		if (FD_ISSET(0, &reads))
		{
			if ((nread = read(0, buffer, sizeof(buffer))))
				write(s, buffer, nread);
			else
				my_exit(3);
		}
		if (FD_ISSET(s, &reads))
		{
			if ((nread = read(s, buffer, sizeof(buffer))))
				write(1, buffer, nread);
			else
				my_exit(4);
		}
	}

	my_exit(8);
}

void ignore (int value)
{
	/* send a ^C */
	char foo = 3;
	write(s, &foo, 1);
}

void my_exit(int value)
{
	printf("exiting with %d!\n", value);
	printf("errno is %d (%s)\n", errno, strerror(errno));
	exit(value);
}

/* These are here so we can link with network.o */
char *LocalHostName = NULL;
struct sockaddr_foobar LocalHostAddr;
char empty_string[] = "";
int get_int_var (enum VAR_TYPES unused) { return 5; }
void set_socket_options (int s) { }

/* End of file */
