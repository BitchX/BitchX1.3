
/*
 * Copyright Colten Edwards (July 1/1998).
 * This code is for the purpose of re-attaching to a detached BitchX session.
 * The idea for this program was by kasper@efnet after I mentioned the trouble
 * I was having reconnecting to a detached terminal.
 */
 
 /* 
  * Version 1.0 released with BitchX 75
  * $Id: scr-bx.c 119 2011-04-08 12:29:55Z keaston $
  */

#include "irc.h"
#include "struct.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#ifdef USING_CURSES
#include <curses.h>
#endif
#include <stdarg.h>
#include <string.h>
#include "ircterm.h"
#include "screen.h"
#include "ircaux.h"

#if defined(_ALL_SOURCE) || defined(__EMX__) || defined(__QNX__) || defined(_FreeBSD__)
#include <termios.h>
#else
#include <sys/termios.h>
#endif

#include <sys/ioctl.h>



#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#ifdef MEM_DEBUG
#include <dmalloc.h>
#endif

#ifdef TRANSLATE
char translation = 0;
unsigned char   transToClient[256];    /* Server to client translation. */
#endif

int dumb_mode = 0;
int already_detached = 1;
int do_check_pid = 0;
char socket_path[500];
char attach_ttyname[500];


struct param 
{
	pid_t	pgrp,
		pid;
	uid_t	uid;
	int	cols;
	int	rows;
	char	tty[80];
	char	cookie[30];
	char	password[80];
	char	termid[81];
};
                                
struct param parm;                                                
char *old_pass = NULL;
Screen *output_screen = NULL, *last_input_screen = NULL, *main_screen = NULL;
char empty_string[] = "";
int foreground = 0;
int use_input = 1;
int use_flow_control = 0;
static int displays = 0;

#define SOCKMODE (S_IWRITE | S_IREAD | (displays ? S_IEXEC : 0))

#ifdef CLOAKED
extern char proctitlestr[140];
extern char **Argv;             /* pointer to argument vector */
extern char *LastArgv;          /* end of argv */
#endif


char	*n_m_strdup (const char *str, const char *module, const char *file, const int line)
{
	char *ptr;
	if (!str)
		str = empty_string;
	ptr = (char *)malloc(strlen(str) + 1);
	return strcpy(ptr, str);
}

void ircpanic(char *string, ...)
{
	return;
}

int get_int_var(int var)
{
	return 1;
}

char *ltoa (long foo)
{
	static char buffer[BIG_BUFFER_SIZE/8+1];
	char *pos = buffer + BIG_BUFFER_SIZE/8-1;
	unsigned long absv;
	int negative;

	absv = (foo < 0) ? (unsigned long)-foo : (unsigned long)foo;
	negative = (foo < 0) ? 1 : 0;

	buffer[BIG_BUFFER_SIZE/8] = 0;
	for (; absv > 9; absv /= 10)
		*pos-- = (absv % 10) + '0';
	*pos = (absv) + '0';

	if (negative)
		*--pos = '-';

	return pos;
}

char *lower(char *str)
{
register char   *ptr = NULL;

	if (str)
	{
		ptr = str;
		for (; *str; str++)
		{
			if (isupper(*str))
				*str = tolower(*str);
		}
	}
	return (ptr);
}

#ifndef HAVE_GETPASS
char *getpass(char *);
char *get_string_var(int var)
{
	return NULL;
}
#endif

#ifdef WINNT
void refresh_screen(int i, char *u)
{
	return;
}
#endif

char *find_tty_name(char *name)
{
static char tty[20];
char *q, *s;
	*tty = 0;
	if ((q = strrchr(name, '/')))
	{
		q++;
		if ((q = strchr(q, '.')))
		{
			q++;
			if ((s = strchr(q, '.')))
				strncpy(tty, q, s-q);
		}
	}
	return tty;
}

char *find_tty_path(char *name)
{
static char ttypath[200];
char *q;
	*ttypath = 0;
	if ((q = strrchr(name, '/')))
		strncpy(ttypath, name, q - name);
	return ttypath;
}

void display_socket_list(char *path, int unl, char *arg)
{
DIR	*dptr;
struct	dirent	*dir;
struct	stat	st;
char buffer[2000];
char *new_path, *p;
int count = 0;
int doit = 0;

	new_path = alloca(strlen(path)+1);
	strcpy(new_path, path);
	if ((p = strrchr(new_path, '/')))
		*p = 0;
	if (!(dptr = opendir(new_path)))
	{
		fprintf(stderr, "No such directory %s\r\n ", new_path);
		exit(1);
	}
	while ((dir = readdir(dptr)))
	{
		doit = 0;
		if (!dir->d_ino)
			continue;
		if (dir->d_name[0] == '.')
			continue;
		sprintf(buffer, "%s/%s", new_path, dir->d_name);
		if ((stat(buffer, &st) == -1))
			continue;
		if (arg && strstr(dir->d_name, arg))
			doit++;
		if (!count && !unl)
			fprintf(stderr, "There is more than one sockets available - \r\n");
		else if (!count)
			fprintf(stderr, "unlinking the following\r\n");
		count++;
		if (unl)
		{
			if (!((st.st_mode & 0700) == 0600) || doit)
			{
				fprintf(stderr, "%30s\r\n", dir->d_name);
				unlink(buffer);
			}
		} 
		else if ((!doit && !arg) || (doit && arg))
			fprintf(stderr, "%30s %s\r\n", dir->d_name, ((st.st_mode & 0700) == 0600) ? "detached":"Attached or dead");
	}
	if (!count)
		fprintf(stderr, "No sockets to attach to.\r\n");
	closedir(dptr);
	exit(1);
}

char *find_detach_socket(char *path, char *name)
{
char	*new_path;
DIR	*dptr;
struct	dirent	*dir;
struct	stat	st;
char *ret = NULL, *p;
int count = 0;
	new_path = alloca(strlen(path)+1);
	strcpy(new_path, path);
	if ((p = strrchr(new_path, '/')))
		*p = 0;
	else
		return NULL;
	if (!(dptr = opendir(new_path)))
		return NULL;
	ret = malloc(2000);
	*ret = 0;
	while ((dir = readdir(dptr)))
	{
		*ret = 0;
		if (!dir->d_ino)
			continue;
		if (dir->d_name[0] == '.')
			continue;
		sprintf(ret, "%s/%s", new_path, dir->d_name);
		p = strrchr(ret, '/'); p++;
		if ((stat(ret, &st) == -1) || (st.st_uid != getuid()) || S_ISDIR(st.st_mode))
		{
			*ret = 0;
			continue;
		}
		if (name)
		{
			char *pid, *n_tty, *h_name;
			pid = alloca(strlen(p)+1);
			strcpy(pid, p);
			n_tty = strchr(pid, '.'); *n_tty++ = 0;
			h_name = strchr(n_tty, '.'); *h_name++ = 0;
			if (strcmp(name, pid))
			{
				if (strcmp(n_tty, name))
				{
					if (strcmp(h_name, name))
					{
						if (strcmp(p, name))
						{
							if (!strstr(p, name))
							{
								*ret = 0;
								continue;
							}
						}
					}
				}
			}
		}
		if ((st.st_mode & 0700) == 0600)
			break;
		count++;
		*ret = 0;
	}
	closedir(dptr);
	if (ret && !*ret)
	{
		free(ret);
		ret = NULL;
	}
	switch (count)
	{
		case 0:
			break;
		case 1:
			break;
		default:
			display_socket_list(path, 0, name);
			if (ret) free(ret);
			ret = NULL;
	}
	return ret;
}

void charset_ibmpc (void)
{
	fwrite("\033(U", 3, 1, stdout);	/* switch to IBM code page 437 */
}

SIGNAL_HANDLER(handle_pipe)
{
	
	term_cr();
	term_clear_to_eol();
	term_reset2();
	fprintf(stdout, "\rdetached from %s. To re-attach type scr-bx %s\n\r", attach_ttyname, old_pass? "password":"");
	fflush(stdout);
	exit(0);
}

SIGNAL_HANDLER(handle_hup)
{
	term_cr();
	term_clear_to_eol();
	term_reset2();
	fprintf(stdout, "\r");
	fflush(stdout);
	exit(0);
}

volatile int ctrl_c = 0;

SIGNAL_HANDLER(handle_ctrlc)
{
	ctrl_c++;
}

/* set's socket options */
void set_socket_options (int s)
{
	int	opt = 1;
	int	optlen = sizeof(opt);
#ifndef NO_STRUCT_LINGER
	struct linger	lin;

	lin.l_onoff = lin.l_linger = 0;
	setsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&lin, optlen);
#endif

	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, optlen);
	opt = 1;
	setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char *)&opt, optlen);
}


char *get_cookie(char *name)
{
	static char cookie[80];
	FILE *fp = NULL;
	*cookie = 0;
	if ((fp = fopen(name, "r")))
	{
		fread(cookie, 40, 1, fp);
		fclose(fp);
		if (*cookie)
			cookie[strlen(cookie)-1] = 0;
	}
	return cookie;
}

void reattach_tty(char *tty, char *password)
{
int s = -1;
char *name;
struct sockaddr_in addr;
struct hostent *hp;
int len = 0;
fd_set rd_fd;
struct timeval tm = {0};
char chr_c[] = "\003";

/* 
 * this buffer has to be big enough to handle a full screen of 
 * information from the detached process.
 */
unsigned char buffer[6 * BIG_BUFFER_SIZE+1];
char *p;
int port = 0;
#if defined (TIOCGWINSZ)
struct winsize window;
#endif
	memset(&parm, 0, sizeof(struct param));

	if (!(name = find_detach_socket(socket_path, tty)))
	{
		fprintf(stderr, "No detached process to attach to.\r\n");
		_exit(1);
	}

	strcpy(parm.cookie, get_cookie(name));
	if (!*parm.cookie)
		_exit(1);
	if ((p = strrchr(name, '/')))
		p++;
	sscanf(p, "%d.%*s.%*s", &port);
	displays = 1;
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		displays = 0;
		_exit(1);
	}

	chmod(name, SOCKMODE);
	set_socket_options(s);
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;
	if((hp = gethostbyname("localhost")))
		memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);
	else
		inet_aton("127.0.0.1", (struct in_addr *)&addr.sin_addr);
	if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		fprintf(stderr, "connection refused for %s\r\n", name);
		_exit(1);
	}

	parm.pid = getpid();
	parm.pgrp = getpgrp();
	parm.uid = getuid();
	strcpy(parm.tty, ttyname(0));
	strncpy(parm.termid, getenv("TERM"), 80);
	if (password) 
		strncpy(parm.password, password, 60);
	fprintf(stderr, "attempting to wakeup %s\r\n", find_tty_name(name));
#if defined (TIOCGWINSZ)
	if (ioctl(0, TIOCGWINSZ, &window) > -1)
	{
		parm.cols = window.ws_col;
		parm.rows = window.ws_row;
	}
	else
#endif
	{
		parm.cols = 79;
		parm.rows = 25;
	}
	write(s, &parm, sizeof(struct param));	
	sleep(2);
	alarm(15);
	len = read(s, &parm, sizeof(struct param));
	alarm(0);
	if (len <= 0)
	{
		fprintf(stderr, "[1;37merror[0;37m reconnecting to %s\r\n", find_tty_name(name));
		displays = 0;
		chmod(name, SOCKMODE);
		exit(1);
	}
	unlink(name);

	term_init(parm.termid);
	set_term_eight_bit(1);
	charset_ibmpc();
	term_clear_screen();
	term_resize();
	term_move_cursor(0,0);

	my_signal(SIGPIPE, handle_pipe, 0);
	my_signal(SIGINT,  handle_ctrlc, 0);
	my_signal(SIGHUP,  handle_hup, 0);

	/*
	 * according to MHacker we need to set errno to 0 under BSD.
	 * for some reason we get a address in use from a socket 
	 *
	 */
	errno = 0;
	while (1)
	{
		FD_ZERO(&rd_fd);
		FD_SET(0, &rd_fd);
		FD_SET(s, &rd_fd);
		tm.tv_sec = 2;
		
		switch(select(s+1, &rd_fd, NULL, NULL, &tm))
		{
			case -1:
				if (ctrl_c)
				{
					write(s, chr_c, 1);
					ctrl_c = 0;
				}
				else if (errno != EINTR)
				{
					close(s);
					_exit(1);
				}
				break;
			case 0:
				break;
			default:
			{
				if (FD_ISSET(0, &rd_fd))
				{
					len = read(0, buffer, sizeof(buffer)-1);
					write(s, buffer, len);
				}
				if (FD_ISSET(s, &rd_fd))
				{
					len = read(s, buffer, sizeof(buffer)-1);
					write(1, buffer, len);
				}
			}
		}
	}
	close(s);
	fprintf(stderr, "Never should have got here");
	_exit(1);			 

	return; /* error return */
}

char *stripdev(char *ttynam)
{
	if (ttynam == NULL)
		return NULL;
#ifdef SVR4
  /* unixware has /dev/pts012 as synonym for /dev/pts/12 */
	if (!strncmp(ttynam, "/dev/pts", 8) && ttynam[8] >= '0' && ttynam[8] <= '9')
	{
		static char b[13];
		sprintf(b, "pts/%d", atoi(ttynam + 8));
		return b;
	}
#endif /* SVR4 */
	if (!strncmp(ttynam, "/dev/", 5))
		return ttynam + 5;
	return ttynam;
}


void init_socketpath(void)
{
#if !defined(__EMX__) && !defined(WINNT)
struct stat st;
extern char socket_path[], attach_ttyname[];

	sprintf(socket_path, "%s/.BitchX/screens", getenv("HOME"));
	if (access(socket_path, F_OK))
		return;
	if (stat(socket_path, &st) != -1)
	{
		char host[BIG_BUFFER_SIZE+1];
		char *ap;
		if (!S_ISDIR(st.st_mode))
			return;
		gethostname(host, BIG_BUFFER_SIZE);
		if ((ap = strchr(host, '.')))
			*ap = 0;
		ap = &socket_path[strlen(socket_path)];
		sprintf(ap, "/%%d.%s.%s", stripdev(attach_ttyname), host);
		ap++;
		for ( ; *ap; ap++)
			if (*ap == '/')
				*ap = '-';
	}	        
#endif
}


char *old_tty = NULL;
void parse_args(int argc, char **argv)
{
int ac = 1;
int disp_sock = 0;

	for (; ac < argc; ac++)
	{
		
		if (!strncasecmp(argv[ac], "tty", 3))
		{
			old_tty = malloc(strlen(argv[ac])+1);
			strcpy(old_tty, argv[ac]);
		}
		else if (argv[ac][0] == '-' && argv[ac][1] == 'p')
		{
			char *pass;
			pass = getpass("Enter password : ");
			old_pass = malloc(strlen(pass)+1);
			strcpy(old_pass, pass);
		}
		else if (argv[ac][0] == '-' && argv[ac][1] == 'h')
		{
			char *p;
			if ((p = strrchr(argv[0], '/')))
				p++;
			else
				p = argv[0];
			fprintf(stderr, "Usage %s: [tty] [-p] [-h] [-l]\r\n\t\ttty is the name of a tty\r\n\t\t-p to specify a password\r\n\t\t-l to list available sockets\r\n\t\t-w to wipe out dead sockets\r\n", p);
			exit(0);
		}
		else if (argv[ac][0] == '-' && argv[ac][1] == 'l')
			disp_sock = 1;
		else if (argv[ac][0] == '-' && argv[ac][1] == 'w')
			disp_sock = 2;
		else if (!old_tty)
		{
			old_tty = malloc(strlen(argv[ac])+1);
			strcpy(old_tty, argv[ac]);
		}
	}
	if (disp_sock)
		display_socket_list(socket_path, disp_sock - 1, old_tty);
}


int main(int argc, char **argv)
{
#ifdef MEM_DEBUG
	dmalloc_debug(0x1df47dfb);
#endif
	*socket_path = 0;
	strcpy(attach_ttyname, ttyname(0));
	init_socketpath();
	parse_args(argc, argv);
        chdir(getenv("HOME"));
	reattach_tty(old_tty, old_pass);
	return 0;
}
