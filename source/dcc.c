
/*
 * dcc.c: Things dealing with client to client connections. 
 *
 * Copyright(c) 1998 Colten Edwards aka panasync.
 * After alot of unhappiness with the old dcc.c code I took it upon myself
 * to rewrite this code. The only parts not greatly changed by myself was
 * code I had written by me in the first place. ie the ftp code and the 
 * dcc botlink code. Some great improvements in dcc speed were realized
 * using non-blocking connects. All the previous dcc modes/commands are still
 * in place and available. 
 */

#include "irc.h"
static char cvsrevision[] = "$Id: dcc.c 104 2010-09-30 13:26:06Z keaston $";
CVS_REVISION(dcc_c)
#include "struct.h"

#include <sys/stat.h>
#include <stdarg.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/ioctl.h>

#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif

#if defined(HAVE_SYSCONF) && defined(_SC_OPEN_MAX) && !defined(__EMX__)
# define IO_ARRAYLEN sysconf(_SC_OPEN_MAX)
#else
# ifdef FD_SETSIZE
#  define IO_ARRAYLEN FD_SETSIZE
# else
#  define IO_ARRAYLEN NFDBITS
# endif
#endif

#include "ctcp.h"
#include "encrypt.h"
#include "cdcc.h"
#include "dcc.h"
#include "hook.h"
#include "ircaux.h"
#include "lastlog.h"
#include "list.h"
#include "newio.h"
#include "output.h"
#include "parse.h"
#include "server.h"
#include "status.h"
#include "vars.h"
#include "who.h"
#include "window.h"
#include "screen.h"
#include "ircterm.h"
#include "hook.h"
#include "misc.h"
#include "tcl_bx.h"
#include "userlist.h"
#include "hash2.h"
#include "gui.h"
#define MAIN_SOURCE
#include "modval.h"

#include <float.h>

#ifdef WINNT
#include <windows.h>
#endif

extern int use_socks;

#define DCC_HASHSIZE 11
HashEntry dcc_no_flood[DCC_HASHSIZE];

struct _dcc_types_ 
{
	char	*name;
	char	*module;
	int	type;
	int 	(*init_func)();
	int	(*open_func)();
	int	(*input)();
	int	(*output)();
	int	(*close_func)();
	
} _dcc_types[] =
{
	{"<none>",	NULL, 0,		NULL, NULL, NULL, NULL, NULL},
	{"CHAT",	NULL, DCC_CHAT,		NULL, NULL, NULL, NULL, NULL},
	{"SEND",	NULL, DCC_FILEOFFER,	NULL, NULL, NULL, NULL, NULL},
	{"GET",		NULL, DCC_FILEREAD,	NULL, NULL, NULL, NULL, NULL},
	{"RAW_LISTEN",	NULL, DCC_RAW_LISTEN,	NULL, NULL, NULL, NULL, NULL},
	{"RAW",		NULL, DCC_RAW,		NULL, NULL, NULL, NULL, NULL},
	{"RESEND",	NULL, DCC_REFILEOFFER,	NULL, NULL, NULL, NULL, NULL},
	{"REGET",	NULL, DCC_REFILEREAD,	NULL, NULL, NULL, NULL, NULL},
	{"BOT",		NULL, DCC_BOTMODE,	NULL, NULL, NULL, NULL, NULL},
	{"FTP",		NULL, DCC_FTPOPEN, 	NULL, NULL, NULL, NULL, NULL},
	{"FTPGET",	NULL, DCC_FTPGET,	NULL, NULL, NULL, NULL, NULL},
	{"FTPSEND",	NULL, DCC_FTPSEND,	NULL, NULL, NULL, NULL, NULL},
	{NULL,		NULL, 0,		NULL, NULL, NULL, NULL, NULL}
};

struct _dcc_types_ **dcc_types = NULL;

static	char		DCC_current_transfer_buffer[BIG_BUFFER_SIZE/4];
	unsigned int	send_count_stat = 0;
	unsigned int	get_count_stat = 0;
	char		*last_chat_req = NULL;
static	int		dcc_quiet = 0;
static	int		dcc_paths = 0;
static	int		dcc_overwrite_var = 0;
	double		dcc_bytes_in = 0;
	double		dcc_bytes_out = 0;
	double  	dcc_max_rate_in = 0.0;
static 	double  	dcc_min_rate_in = DBL_MAX;
	double  	dcc_max_rate_out = 0.0;
static 	double  	dcc_min_rate_out = DBL_MAX;

	time_t		dcc_timeout = 600; /* global /set dcc_timeout */

static	int		dcc_count = 0;




/* new dcc handler */
#include "irc.h"
#include "ircaux.h"
#include "struct.h"
#include "misc.h"

extern Server *server_list;

typedef struct _DCC_List
{
	struct _DCC_List *next;
	char *nick;		/* do NOT free this. it's a shared pointer. */
	SocketList sock;
} DCC_List;

static DCC_List *pending_dcc = NULL;


static void process_dcc_chat(int);
#ifndef BITCHX_LITE
static void process_dcc_bot(int);
#endif
static void
	start_dcc_chat(int);
static unsigned char byte_order_test (void);
static void dcc_update_stats(int);

static void update_transfer();

static char *strip_path(char *);
static void dcc_getfile_resume_demanded(char *, char *, char *, char *);
static void dcc_getfile_resume_start (char *, char *, char *, char *);
static	void 	output_reject_ctcp (UserhostItem *, char *, char *);

extern int use_nat_address;
extern struct in_addr nat_address;

#if 0
			/* sock num, type, address, port */
int (*dcc_open_func) (int, int, struct sockaddr_foobar, int) = NULL;
			/* type, sock num, buffer, buff_len */
int (*dcc_output_func) (int, int, char *, int) = NULL;
			/* sock num, type, buffer, new line, len buffer */
int (*dcc_input_func)  (int, int, char *, int, int) = NULL;
			/* socket num, address, port */
int (*dcc_close_func) (int, sockaddr_foobar, int) = NULL;
#endif


#define DCC_OPEN 1
#define DCC_INPUT 2
#define DCC_OUTPUT 3
#define DCC_CLOSE 4

int check_dcc_init(char *type, char *nick, char *uhost, char *description, char *size, char *extra, unsigned long TempLong, unsigned int TempInt);
		/* bind type, sock num, type, address, port, buffer, buflen , newline */

#ifndef O_BINARY
#define O_BINARY 0
#endif

#define BAR_LENGTH 60

typedef struct
{
	char	*	name;
	dcc_function 	function;
	char	*	help;
} DCC_commands;

DCC_dllcommands	*dcc_dllcommands = NULL;


void dcc_chat(char *command, char *args);
void BX_dcc_filesend(char *command, char *args);
void BX_dcc_resend(char *command, char *args);
void dcc_getfile(char *command, char *args);
void dcc_regetfile(char *command, char *args);
void dcc_glist(char *command, char *args);
void dcc_resume(char *command, char *args);
void dcc_rename(char *command, char *args);

void dcc_show_active(char *command, char *args);
void dcc_set_quiet(char *command, char *args);
void dcc_set_paths(char *command, char *args);
void dcc_tog_rename(char *command, char *args);
void dcc_tog_resume(char *command, char *args);
void dcc_overwrite_toggle(char *command, char *args);
void dcc_tog_auto(char *command, char *args);
void dcc_stats(char *command, char *args);
void dcc_close(char *command, char *args);
void dcc_closeall(char *command, char *args);
void dcc_help1(char *command, char *args);
void dcc_exempt(char *command, char *args);
void dcc_ftpopen(char *command, char *args);

DCC_commands	dcc_commands[] =
{
#ifndef BITCHX_LITE
	{ "BOT",	dcc_chat,		NULL },
#endif
	{ "CHAT",	dcc_chat,		"[nick]\n- Starts a dcc chat connection" },
	{ "CLOSE",	dcc_close,		NULL },
	{ "CLOSEALL",	dcc_closeall,		NULL },
	{ "SEND",	BX_dcc_filesend,		"[-b block] [-e passwd] [-p port] [nick|nick1,nick2] filename(s)\n- Send list of files to nick(s)" },
	{ "RESEND",	BX_dcc_resend,		"[-b block] [-e passwd] [-p port] [nick|nick1,nick2] filename(s)\n- Send list of files to nick(s)" },
	{ "TSEND",	BX_dcc_filesend,		"[-b block] [-e passwd] [-p port] [nick|nick1,nick2] filename(s)\n- Send list of files to nick(s)" },
	{ "TRESEND",	BX_dcc_resend,		"[-b block] [-e passwd] [-p port] [nick|nick1,nick2] filename(s)\n- Send list of files to nick(s)" },
#ifdef MIRC_BROKEN_DCC_RESUME
	{ "RESUME",	dcc_resume,		NULL },
#endif
	{ "GET",	dcc_getfile,		NULL },
	{ "REGET",	dcc_regetfile,		NULL },
	{ "TGET",	dcc_getfile,		NULL },
	{ "TREGET",	dcc_regetfile,		NULL },
	{ "LIST",	dcc_glist,		NULL },
	{ "FTP",	dcc_ftpopen,		NULL },
	{ "RENAME",	dcc_rename,		"nick filename" },
	{ "ACTIVECOUNT",dcc_show_active,	NULL },
	{ "QUIETMODE",	dcc_set_quiet,		NULL },
	{ "SHOWPATHS",	dcc_set_paths,		NULL },
	{ "AUTORENAME",	dcc_tog_rename,		NULL },
	{ "AUTORESUME",	dcc_tog_resume,		NULL },
	{ "AUTOOVERWRITE",dcc_overwrite_toggle,	NULL },
	{ "AUTOGET",	dcc_tog_auto,		NULL },
	{ "STATS",	dcc_stats,		NULL },
	{ "EXEMPT",	dcc_exempt,		NULL },
	{ "HELP",	dcc_help1,		NULL },
	{ NULL,		(dcc_function) NULL,	NULL }
};



static int doing_multi = 0;	/* 
				 * set this if doing multiple requests 
				 * helps prevent console spam by reducing
				 * the amount of output.
				 */

#if 0
int add_dcc_command(char *name, char *help, void (*func)(char *, char *))
{
int count = 0;
	if (!dcc_dllcommands) /* ooh first time. need to init. make it double*/
		dcc_dllcommands = new_malloc(sizeof(DCC_commands) * 2);
	else
	{
		for ( ; dcc_dllcommands[count].name; count++) ;
		RESIZE(dcc_dllcommands, DCC_commands, count + 2);
	}
	malloc_strcpy(&dcc_dllcommands[count].name, name);
	malloc_strcpy(&dcc_dllcommands[count].help, help);
	dcc_dllcommands[count].function = func;
	return count + 1;
}

int remove_dcc_hook(char *name)
{
int i, count = 0;
int done = 0;
	for ( ; dcc_dllcommands[count].name; count++) ;
	for (i = 0; i < count; i++)
	{
		if (dcc_dllcommands[i].name && !my_stricmp(dcc_dllcommands[i].name, name))
		{
			new_free(&dcc_dllcommands[i].name);
			new_free(&dcc_dllcommands[i].help);
			dcc_dllcommands[i].function = NULL;
			done++;
			break;
		}
	}
	return done;
}
#endif

void send_ctcp_booster(char *nick, char *type, char *file, unsigned long address, unsigned short portnum, unsigned long filesize)
{
	if (filesize)
		send_ctcp(CTCP_PRIVMSG, nick, CTCP_DCC,
			 "%s %s %lu %u %lu", type, file,address, portnum, filesize);
	else
		send_ctcp(CTCP_PRIVMSG, nick, CTCP_DCC,
			 "%s %s %lu %u", type, file, address, portnum);
}

void send_reject_ctcp(char *nick, char *type, char *name)
{
	/*
	 * And output it to the user
	 */
	userhostbase(nick, output_reject_ctcp, 1, "%s %s %s", nick, type, name ? name : "<any>");
}

void BX_erase_dcc_info(int s, int reject, char *format, ...)
{
DCC_int *n = NULL;
SocketList *s1;
static time_t last_reject = 0;

	if (format)
	{
		va_list args;
		char buffer[BIG_BUFFER_SIZE+1];
		va_start(args, format);
		vsnprintf(buffer, BIG_BUFFER_SIZE, format, args);
		va_end(args);
		put_it("%s", buffer);
	}
	dcc_update_stats(s);

	s1 = get_socket(s);
	if (check_dcc_socket(s) && (n = (DCC_int *)get_socketinfo(s)))
	{
		if (dcc_types[s1->flags & DCC_TYPES]->close_func)
			(*dcc_types[s1->flags & DCC_TYPES]->close_func)(s, n->remote.s_addr, n->remport);
		if (reject && ((now - last_reject) < 2))
			send_reject_ctcp(s1->server, 
				(((s1->flags & DCC_TYPES) == DCC_FILEOFFER) ? "GET" :
				 ((s1->flags & DCC_TYPES) == DCC_FILEREAD)  ? "SEND" : 
				   dcc_types[s1->flags & DCC_TYPES]->name),
				n->filename);
		last_reject = now;
		new_free(&n->user);
		new_free(&n->userhost);
		new_free(&n->encrypt);
		new_free(&n->filename);
		new_free(&n->othername);
		if (n->file > 0) close(n->file);
		new_free(&n);
	}
	set_socketinfo(s, NULL);
	update_transfer();
	dcc_count--;
/*	dcc_renumber_active();*/
}

/*
 * finds an active dcc connection. one that either is already started
 * or one that has been initiated on our side.
 */
 
SocketList *BX_find_dcc(char *nick, char *desc, char *other, int type, int create, int active, int num)
{
int i;
SocketList *s;
DCC_int *n;
char *othername = NULL;

	for (i = 0; i < get_max_fd()+1; i++)
	{
		if (!check_dcc_socket(i))
			continue;
		s = get_socket(i);
		n = (DCC_int *) s->info;
		if ( (type != -1) && !((s->flags & DCC_TYPES) == type) )
			continue;
		if ((num != -1) && !(n->dccnum == num))
			continue;
		if (nick && my_stricmp(nick, s->server))
			continue;
		if (desc && n->filename && my_stricmp(desc, n->filename))
		{
			char *last;
			if (!(last = strrchr(n->filename, '/')))
				continue;
			last++;
			if (last && my_stricmp(desc, last))
			{
				if (!othername || !n->othername)
					continue;
				if (my_stricmp(othername, n->othername))
					continue;
			}
		}
		if (other && n->othername && my_stricmp(other, n->othername))
			continue;
		if (active == 0 && (s->flags & DCC_ACTIVE))
			continue;
		if (active == 1 && !(s->flags & DCC_ACTIVE))
			continue;
		return s;
	}
	return NULL;
}

/* 
 * finds a pending dcc which the other end has initiated.
 */
DCC_List *find_dcc_pending(char *nick, char *desc, char *othername, int type, int remove, int num)
{
unsigned long dcc_type;
unsigned long flags;
SocketList *s;
DCC_int *n;
DCC_List *new_i;
DCC_List *last_i = NULL;
	for (new_i = pending_dcc; new_i; last_i = new_i, new_i = new_i->next)
	{
		flags = new_i->sock.flags;
		s = &new_i->sock;
		n = (DCC_int *)s->info;
		dcc_type = s->flags & DCC_TYPES;
		if ((type != -1) && !(dcc_type == type))
			continue;
		if ((num != -1) && !(n->dccnum == num))
			continue;
		if (nick && my_stricmp(nick, new_i->sock.server))
			continue;
		if ((desc && my_stricmp(desc, n->filename)))
		{
			char *last;
			if (!(last = strrchr(n->filename, '/')))
				continue;
			last++;
			if (last && my_stricmp(desc, last))
				continue;
		}
		if (othername && n->othername && my_stricmp(othername, n->othername))
			continue;
		if (remove)
		{
			if (last_i)
				last_i->next = new_i->next;
			else
				pending_dcc = new_i->next;
		}
		return new_i;
	}
	return NULL;
}

void add_userhost_to_chat(UserhostItem *stuff, char *nick, char *args)
{
SocketList *Client = NULL;
DCC_int *n = NULL;
	if (!stuff || !nick || !stuff->nick || !stuff->user || !stuff->host || !strcmp(stuff->user, "<UNKNOWN>"))
		return;
	if ((Client = find_dcc(nick, "chat", NULL, DCC_CHAT, 0, -1, -1)))
		n = (DCC_int *)get_socketinfo(Client->is_read);
	if (n)	
	{
		n->userhost = m_sprintf("%s@%s", stuff->user, stuff->host);
		n->server = from_server;
	}
	return;
}

static void refileread_send_start(int s, DCC_int *n)
{
struct stat buf;
	lseek(n->file, 0, SEEK_END);
	/* get the size of our file to be resumed */
	fstat(n->file, &buf);
	n->transfer_orders.packet_id = DCC_PACKETID;
	n->transfer_orders.byteoffset = buf.st_size;
	n->transfer_orders.byteorder = byte_order_test();
	put_it("%s", convert_output_format("$G %RDCC %YTelling uplink we want to start at%n: $0", "%s", ulongcomma(n->transfer_orders.byteoffset)));
	send(s, (const char *)&n->transfer_orders, sizeof(struct transfer_struct), 0);
} 

#ifdef HAVE_SSL
int SSL_dcc_create(SocketList *s, int sock, int doconnect)
{
	set_blocking(sock);
	if(doconnect)
		s->ctx = SSL_CTX_new (SSLv23_client_method());
	else
		s->ctx = SSL_CTX_new (SSLv23_server_method());
	SSL_CTX_set_cipher_list(s->ctx, "ADH:@STRENGTH");
	s->ssl_fd = SSL_new (s->ctx);
	SSL_set_fd (s->ssl_fd, sock);
	if(doconnect)
		return SSL_connect (s->ssl_fd);
	return SSL_accept(s->ssl_fd);
}
#endif

DCC_int *BX_dcc_create(char *nick, char *filename, char *passwd, unsigned long filesize, int port, int type, unsigned long flags, void (*func)(int))
{

struct  sockaddr_foobar myip, blah;
int	ofs	= from_server;
int	s;
DCC_int *new 	= NULL;
DCC_List		*new_i;


	if (from_server == -1)
		from_server = get_window_server(0);
	if (from_server == -1)
		return NULL;
	if (get_int_var(DCC_USE_GATEWAY_ADDR_VAR))
		blah = get_server_uh_addr(from_server);
	else
		blah = get_server_local_addr(from_server);
	memcpy(&myip, &blah, sizeof(struct sockaddr_foobar));

	if (use_nat_address)
	{
		myip.sf_family = AF_INET;
		myip.sf_addr.s_addr = nat_address.s_addr;
	}

	set_display_target(NULL, LOG_DCC);
	if ( ((new_i = find_dcc_pending(nick, filename, NULL, type, 1, -1)) && new_i->sock.flags & DCC_OFFER) || (flags & DCC_OFFER))
	{
		SocketList 		*s = NULL;
		int			new_s;
		struct	sockaddr_in	remaddr;
		int			rl = sizeof(remaddr);


		if (!new_i && !(new_i = find_dcc_pending(nick, filename, NULL, type, 1, -1)))
			return NULL;
		s = &new_i->sock;
		new = (DCC_int *)s->info;

#ifdef DCC_CNCT_PEND
		flags |= DCC_CNCT_PEND;
#endif
		new->remport = ntohs(new->remport);
		if ((new_s = connect_by_number(inet_ntoa(new->remote), &new->remport, SERVICE_CLIENT, PROTOCOL_TCP, 0)) < 0
#ifdef HAVE_SSL
			||  (flags & DCC_SSL ? SSL_dcc_create(s, new_s, 1) : 0) < 0
#endif
		   )
		{
#ifdef HAVE_SSL
			SSL_show_errors();
#endif
			erase_dcc_info(s->is_read, 1, "%s", convert_output_format("$G %RDCC%n Unable to create connection: $0-", "%s", errno ? strerror(errno) : "Unknown Host"));
			close_socketread(s->is_read);
			from_server = ofs;
			new_free(&new_i->nick);
			new_free(&new_i);
			reset_display_target();
			return NULL;
		}
		new->remport = ntohs(new->remport);
		flags |= DCC_ACTIVE;
		add_socketread(new_s, port, flags|type, nick, new_i->sock.func_read, NULL);
		set_socketinfo(new_s, new);
		if ((getpeername(new_s, (struct sockaddr *) &remaddr, &rl)) != -1)
		{

			flags &= ~DCC_OFFER;
			if (type != DCC_RAW)
			{
				int blah;
				
				if (type == DCC_FILEOFFER)
					blah = do_hook(DCC_CONNECT_LIST,"%s %s %s %d %ld %s",
						nick, dcc_types[type]->name, 
						inet_ntoa(remaddr.sin_addr),
						ntohs(remaddr.sin_port),(unsigned long)new->filesize, new->filename);
				else
					blah = do_hook(DCC_CONNECT_LIST,"%s %s %s %d",
						nick, dcc_types[type]->name, 
						inet_ntoa(remaddr.sin_addr),
						ntohs(remaddr.sin_port));
				if (blah && !dcc_quiet)
					put_it("%s", convert_output_format(fget_string_var(FORMAT_DCC_CONNECT_FSET), 
						"%s %s %s %s %s %d", update_clock(GET_TIME), dcc_types[type]->name, nick, new->userhost?new->userhost:"u@h", 
						inet_ntoa(remaddr.sin_addr), (int)ntohs(remaddr.sin_port)));
			}
		}
		if (dcc_types[type]->open_func)
			(*dcc_types[type]->open_func)(new_s, type, new->remote.s_addr, ntohs(remaddr.sin_port));
		if (type == DCC_REFILEREAD)
			refileread_send_start(new_s, new);
		if (get_int_var(DCC_FAST_VAR)
#ifdef HAVE_SSL
			&& !(flags & DCC_SSL)
#endif
		   )
		{
			set_non_blocking(new_s);
			new->dcc_fast = 1;
		}
		set_socketflags(new_s, flags|type);
		get_time(&new->starttime);
		get_time(&new->lasttime);
		new_free(&new_i->nick);
		new_free(&new_i);
	}
	else
	{
		char *nopath = NULL;
		char *file = NULL;
		char *Type;
		unsigned short 	portnum = port;

		/* If a port isn't specified and random local ports is on
		 * generate a random port to use.
		 */
		if (!port && get_int_var(RANDOM_LOCAL_PORTS_VAR))
			portnum = random_number(65535 - 1024) + 1024;

		if (get_int_var(DCC_FORCE_PORT_VAR))
			portnum = get_int_var(DCC_FORCE_PORT_VAR);

#ifdef DCC_CNCT_PEND
		flags |= DCC_CNCT_PEND;
#endif
		flags |= DCC_WAIT;
		if ((s = connect_by_number(NULL, &portnum, SERVICE_SERVER, PROTOCOL_TCP, 1)) < 0)
		{
			/* If a port was specified and we failed, and
			 * random local ports is on, try a random port.
			 */
			if(port && get_int_var(RANDOM_LOCAL_PORTS_VAR))
				portnum = random_number(65535 - 1024) + 1024;
			else
				portnum = 0;
			if ((s = connect_by_number(NULL, &portnum, SERVICE_SERVER, PROTOCOL_TCP, 1)) < 0)
			{
				/* Finally try to get a system port number by using 0 */
				portnum = 0;
				s = connect_by_number(NULL, &portnum, SERVICE_SERVER, PROTOCOL_TCP, 1);
			}
		}
		if (s < 0)
		{
			put_it("%s", convert_output_format("$G %RDCC%n Unable to create connection: $0-", "%s", errno ? strerror(errno) : "Unknown Host"));
			reset_display_target();
			from_server = ofs;
			return NULL;
		}
		Type = dcc_types[type]->name;
		add_socketread(s, portnum, flags|type, nick, func, NULL);
		add_sockettimeout(s, 120, NULL);

		new = new_malloc(sizeof(DCC_int));
		new->struct_type = DCC_STRUCT_TYPE;
		new->othername = m_strdup(ltoa(portnum));
		new->localport = port;
		new->dccnum = ++dcc_count;
		get_time(&new->lasttime);
		set_socketinfo(s, new);
		
		if (!(flags & DCC_TWOCLIENTS))
		{
			reset_display_target();
			from_server = ofs;
			return new;
		}			
		if (passwd)
			new->encrypt = m_strdup(passwd);
		if (filename)
		{
			char *p;
			if ((nopath = strrchr(filename, '/')))
				nopath++;
			else
				nopath = filename;
			file = LOCAL_COPY(nopath);
			p = file;
			while ((p = strchr(p, ' ')))
				*p = '_';
			if (strcmp(filename, "bxglobal"))
				new->filename = m_strdup(filename);
			else
				new->filename = m_strdup("_bxglobal");
		}
		new->filesize = filesize;
#if 0
		if (use_socks && get_string_var(SOCKS_HOST_VAR))
		{
			struct hostent *hp;
			struct sockaddr_in socks;
			unsigned long addr;
			hp = gethostbyname(get_string_var(SOCKS_HOST_VAR));
			memcpy(&socks, hp->h_addr, sizeof(hp->h_length));
			addr = inet_addr(inet_ntoa(socks.sin_addr));
			send_ctcp_booster(nick, Type, file, addr, portnum, filesize);
		}
		else
#endif
			send_ctcp_booster(nick, Type, file, ntohl(myip.sf_addr.s_addr), portnum, filesize);
		if (!doing_multi && !dcc_quiet)
		{
			if (filesize)
			{
				if  (do_hook(DCC_OFFER_LIST, "%s %s %s %lu",
					nick, Type, file, filesize))
					put_it("%s", convert_output_format(fget_string_var(FORMAT_SEND_DCC_CHAT_FSET), 
						"%s %s %s", update_clock(GET_TIME), Type, nick));
			} 
			else
			{
				if  (do_hook(DCC_OFFER_LIST, "%s %s %s", nick, Type, file))
					put_it("%s", convert_output_format(fget_string_var(FORMAT_SEND_DCC_CHAT_FSET), 
						"%s %s %s", update_clock(GET_TIME), Type, nick));
			}
		}
	}	
	reset_display_target();
	from_server = ofs;
	return new;
}

static void start_dcc_chat(int s)
{
struct	sockaddr_in	remaddr;
int	sra;
int	type;
int	new_s = -1;
char	*nick = NULL;	
unsigned long flags;
DCC_int *n = NULL;
SocketList *sa, *new_sa;
void	(*func)(int) = process_dcc_chat;

	sa = get_socket(s);
	flags = sa->flags;
	nick = sa->server;
	sra = sizeof(struct sockaddr_in);
	new_s = my_accept(s, (struct sockaddr *) &remaddr, &sra);
	type = flags & DCC_TYPES;
	n = get_socketinfo(s);
	n->user = m_strdup(nick);
	set_display_target(NULL, LOG_DCC);
#ifndef BITCHX_LITE
	if (type == DCC_BOTMODE) func = process_dcc_bot;
#endif			
	if ((add_socketread(new_s, ntohs(remaddr.sin_port), flags, nick, func, NULL)) < 0)
	{
		erase_dcc_info(s, 0, "%s", convert_output_format("$G %RDCC error: accept() failed. punt!!", NULL, NULL));
		close_socketread(s);
		return;
	}
	flags &= ~DCC_WAIT;
	flags |= DCC_ACTIVE;
	set_socketflags(new_s, flags);
	set_socketinfo(new_s, n);

	new_sa = get_socket(new_s);
#ifdef HAVE_SSL
	if((flags & DCC_SSL) && SSL_dcc_create(new_sa, new_s, 0) < 0)
	{
		say("SSL_accept failed.");
		SSL_show_errors();
		return;
	}
#endif

	if (do_hook(DCC_CONNECT_LIST, "%s %s %s %d", nick, dcc_types[type]->name,
		inet_ntoa(remaddr.sin_addr), ntohs(remaddr.sin_port)))
		put_it("%s", convert_output_format(fget_string_var(FORMAT_DCC_CONNECT_FSET), 
			"%s %s %s %s %s %d", update_clock(GET_TIME),
			dcc_types[type]->name,
			nick, n->userhost?n->userhost:"u@h",
			inet_ntoa(remaddr.sin_addr),ntohs(remaddr.sin_port)));
	if ((type == DCC_BOTMODE) && n->encrypt)
		new_free(&n->encrypt);
	get_time(&n->starttime);
	close_socketread(s);

	if (dcc_types[type]->open_func)
		(*dcc_types[type]->open_func)(new_s, type, n->remote.s_addr, ntohs(remaddr.sin_port));
	reset_display_target();
}

static void process_dcc_chat(int s)
{
unsigned long	flags;
char		tmp[BIG_BUFFER_SIZE+1];
char		*bufptr;
long		bytesread;
char		*nick;
int		type;
SocketList 	*sl;
        
	flags = get_socketflags(s);
	nick =  get_socketserver(s);
	type = flags & DCC_TYPES;
	sl = get_socket(s);	
	bufptr = tmp;
	if (dcc_types[type]->input)
		bytesread = (*dcc_types[type]->input)(s, type, bufptr, 1, BIG_BUFFER_SIZE);
	else
#ifdef HAVE_SSL
		bytesread = dgets(bufptr, s, 1, BIG_BUFFER_SIZE, sl->ssl_fd);
#else
		bytesread = dgets(bufptr, s, 1, BIG_BUFFER_SIZE, NULL);
#endif

	set_display_target(nick, LOG_DCC);
	switch(bytesread)
	{
		case -1:
		{
			const char *real_tmp = dgets_strerror(dgets_errno);
	                if (do_hook(DCC_LOST_LIST, "%s %s %s", nick, dcc_types[type]->name, real_tmp))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_DCC_ERROR_FSET), 
					"%s %s %s %s", update_clock(GET_TIME), 
					dcc_types[type]->name, nick, real_tmp));
			erase_dcc_info(s, 1, NULL);
			close_socketread(s);
			break;
		}
		case 0:
			break;
		default:
		{
			char *equal_nickname = NULL;
			char *uhost, *p;
			DCC_int *n = NULL;
			
			n = get_socketinfo(s);									
			if ((p = strrchr(tmp, '\r')))
				*p = 0;
			if ((p = strrchr(tmp, '\n')))
				*p = 0;
			my_decrypt(tmp, strlen(tmp), n->encrypt);
			n->bytes_read += bytesread;
			set_display_target(nick, LOG_DCC);
			uhost = alloca(200);
			*uhost = 0;
			if (n->userhost)
				uhost = n->userhost;
			else
				strmopencat(uhost, 99, "Unknown@", "unknown"/*inet_ntoa(remaddr.sin_addr)*/, NULL);
			FromUserHost = uhost;

#ifndef BITCHX_LITE
			if (*tmp == DEFAULT_BOTCHAR)
			{ 
				char *pc;
				pc = tmp+1;
				if ((flags & DCC_BOTCHAT))
				{
					if (my_strnicmp(pc, "chat", 4))
					{
						if (check_tcl_dcc(pc, nick, n->userhost?n->userhost:empty_string, s))
							break;
					}
				} else if (!my_strnicmp(pc, "chat", 4) && check_tcl_dcc(pc, nick, n->userhost?n->userhost:empty_string, s))
					break;
			}
			else if (get_int_var(BOT_MODE_VAR) && !(flags & DCC_BOTCHAT))
			{
				extern int cmd_password(int, char *);
				if (cmd_password(s, tmp)) 
				{
					dcc_printf(s, "Wrong Password\n");
					erase_dcc_info(sl->is_read, 1, NULL);
					close_socketread(sl->is_read);
				}
				break;
			}
#endif			
#undef CTCP_REPLY
#define CTCP_MESSAGE "CTCP_MESSAGE "
#define CTCP_MESSAGE_LEN strlen(CTCP_MESSAGE)
#define CTCP_REPLY "CTCP_REPLY "
#define CTCP_REPLY_LEN strlen(CTCP_REPLY)

			equal_nickname = alloca(strlen(nick) + 10);
			*equal_nickname = 0;
			strmopencat(equal_nickname, strlen(nick)+4, "=", nick, NULL);


			if (!strncmp(tmp, CTCP_MESSAGE, CTCP_MESSAGE_LEN) ||
				(*tmp == CTCP_DELIM_CHAR && !strncmp(tmp+1, "ACTION", 6)))
			{
				char *tmp2;
				tmp2 = LOCAL_COPY(tmp);
				strcpy(tmp, do_ctcp(equal_nickname, get_server_nickname(from_server), stripansicodes((tmp2 + ((*tmp2 == CTCP_DELIM_CHAR) ? 0 : CTCP_MESSAGE_LEN)) ) ));
				if (!*tmp)
					break;
			}
			else if (!strncmp(tmp, CTCP_REPLY, CTCP_REPLY_LEN) || *tmp == CTCP_DELIM_CHAR)
			{
				char *tmp2;
				tmp2 = LOCAL_COPY(tmp);
				strcpy(tmp, do_notice_ctcp(equal_nickname, get_server_nickname(from_server), stripansicodes(tmp2  + (((*tmp2 == CTCP_DELIM_CHAR) ? 0 : CTCP_REPLY_LEN)))));
				if (!*tmp)
					break;
			}
			if (do_hook(DCC_CHAT_LIST, "%s %s", nick, tmp))
			{
				addtabkey(equal_nickname, "msg", 0);
				put_it("%s", convert_output_format(fget_string_var(FORMAT_DCC_CHAT_FSET), 
					"%s %s %s %s", update_clock(GET_TIME), nick, FromUserHost, tmp));
				add_last_type(&last_dcc[0], MAX_LAST_MSG, nick, NULL, FromUserHost, tmp);
				logmsg(LOG_DCC, nick, 0, "%s", tmp);
			}
			FromUserHost = empty_string;
			break;
		}
	}	
	reset_display_target();
}

#ifndef BITCHX_LITE
static void process_dcc_bot(int s)
{
unsigned long	flags;
char		tmp[BIG_BUFFER_SIZE+1];
char		*bufptr;
long		bytesread;
char		*nick;
int		type;
SocketList *sl;

	flags = get_socketflags(s);
	nick =  get_socketserver(s);
	type = flags & DCC_TYPES;
	sl = get_socket(s);

	bufptr = tmp;
	if (dcc_types[type]->input)
		bytesread = (*dcc_types[type]->input) (type, s, bufptr, 1, BIG_BUFFER_SIZE);
	else
#ifdef HAVE_SSL
		bytesread = dgets(bufptr, s, 1, BIG_BUFFER_SIZE, sl->ssl_fd);
#else
		bytesread = dgets(bufptr, s, 1, BIG_BUFFER_SIZE, NULL);
#endif

	set_display_target(nick, LOG_DCC);
	switch(bytesread)
	{
		case -1:
		{
			const char *real_tmp = dgets_strerror(dgets_errno);
			if (do_hook(DCC_LOST_LIST, "%s %s %s", nick, dcc_types[type]->name, real_tmp))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_DCC_ERROR_FSET), 
					"%s %s %s %s", update_clock(GET_TIME), 
					dcc_types[type]->name, nick, real_tmp));
			erase_dcc_info(s, 1, NULL);
			close_socketread(s);
			break;
		}
		case 0:
			break;
		default:
		{
			DCC_int *n = NULL;
			char *p;			
			n = get_socketinfo(s);									
			if ((p = strrchr(tmp, '\r')))
				*p = 0;
			if ((p = strrchr(tmp, '\n')))
				*p = 0;

			my_decrypt(tmp, strlen(tmp), n->encrypt);
			n->bytes_read += bytesread;
			set_display_target(nick, LOG_DCC);
			handle_dcc_bot(s, tmp);
		}
	}	
	reset_display_target();
}
#endif

/* flag == 1 means show it.  flag == 0 used by redirect (and /ctcp) */

static void	new_dcc_message_transmit (char *user, char *text, char *text_display, int type, int flag, char *cmd, int check_host)
{
SocketList *s = NULL;
DCC_int		*n = NULL;
char		tmp[MAX_DCC_BLOCK_SIZE+1];
int		list = 0;
int 		len = 0;
char		*host = NULL;
char		thing = 0;

	*tmp = 0;

	switch(type)
	{
		case DCC_CHAT:
			thing = '=';
			host = "chat";
			list = SEND_DCC_CHAT_LIST;
			break;
		case DCC_RAW:
			if (check_host)
			{
				if (!(host = next_arg(text, &text)))
				{
					put_it("%s", convert_output_format("$G %RDCC%n No host specified for DCC RAW", NULL, NULL));
					return;
				}
			}
			break;
	}
	s = find_dcc(user, host, NULL, type, 0, 1, -1);
	if (!s || !(s->flags & DCC_ACTIVE))
	{
		put_it("%s", convert_output_format("$G %RDCC No active $0:$1 connection for $2", "%s %s %s", dcc_types[type]->name, host?host:"(null)", user));
		return;
	}
	n = (DCC_int *)s->info;

	/*
	 * Check for CTCPs... whee.
	 */
	if (cmd && *text == CTCP_DELIM_CHAR && strncmp(text+1, "ACTION", 6))
	{
		if (!strcmp(cmd, "PRIVMSG"))
			strmcpy(tmp, "CTCP_MESSAGE ", n->blocksize);
		else
			strmcpy(tmp, "CTCP_REPLY ", n->blocksize);
	}

	strmcat(tmp, text, n->blocksize-3);
	strmcat(tmp, "\n", n->blocksize-2); 

	len = strlen(tmp);
	my_encrypt(tmp, len, n->encrypt);

	if (dcc_types[type]->output)
		(*dcc_types[type]->output) (type, s->is_read, tmp, len);
	else
#ifdef HAVE_SSL
		if(s->ssl_fd)
			SSL_write(s->ssl_fd, tmp, len);
		else
#endif
		write(s->is_read, tmp, len);
	n->bytes_sent += len;

	if (flag && type != DCC_RAW)
	{
		if (do_hook(list, "%s %s", user, text_display ? text_display : text))
			put_it("%s", convert_output_format(fget_string_var(FORMAT_SEND_DCC_CHAT_FSET), "%c %s %s", thing, user, text_display?text_display:text));
	}
}

extern void	dcc_chat_transmit (char *user, char *text, char *orig, char *type, int noisy)
{
	int	fd;

	/*
	 * This is just a quick hack.  Its not fast, its not elegant,
	 * and its not the best way to do this.  But i whipped it up in
	 * 15 minutes.  The "best" soLution is to rewrite dcc_message_transmit
	 * to allow us to do this sanely.
	 */
	set_display_target(NULL, LOG_DCC);
	if ((fd = atol(user)))
	{
		SocketList *	s;
		char *		bogus;

		if (!check_dcc_socket(fd))
		{
			put_it("%s", convert_output_format("$G %RDCC%n Descriptor [$0] is not an open DCC RAW", "%d", fd));
			return;
		}
		s = get_socket(fd);
		bogus = alloca(strlen(text) + strlen(((DCC_int *)s->info)->filename) + 3);

		strcpy(bogus, ((DCC_int *)s->info)->filename);
		strcat(bogus, space);
		strcat(bogus, text);

		new_dcc_message_transmit(user, bogus, orig, DCC_RAW, noisy, type, 0);
	}
	else
		new_dcc_message_transmit(user, text, orig, DCC_CHAT, noisy, type, 0);
	reset_display_target();
}

extern void	dcc_bot_transmit (char *user, char *text, char *type)
{
	set_display_target(user, LOG_DCC);
	new_dcc_message_transmit(user, text, NULL, DCC_BOTMODE, 0, type, 1);
	reset_display_target();
}

extern void dcc_chat_transmit_quiet (char *user, char *text, char *type)
{
	new_dcc_message_transmit(user, text, NULL, DCC_CHAT, 0, type, 0);
}

int dcc_activechat(char *user)
{
	return find_dcc(user, NULL, NULL, DCC_CHAT, 0, 1, -1) ? 1 : 0;
}

int dcc_activebot(char *user)
{
	return find_dcc(user, NULL, NULL, DCC_BOTMODE, 0, 1, -1) ? 1 : 0;
}

int dcc_activeraw(char *user)
{
	return find_dcc(user, NULL, NULL, DCC_RAW, 0, 1, -1) ? 1 : 0;
}

int get_dcc_type(char *type)
{
int tdcc = 0;
int i;
	if (!type || !*type)
		return -1;
	upper(type);
	if (*type == 'T')
		type++, tdcc = DCC_TDCC;

	for (i = 0; dcc_types[i]->name; i++)
		if (!strcmp(type, dcc_types[i]->name))
			return i | tdcc;
	return -1;
}

void process_dcc_send1(int s);
void start_dcc_get(int s);

void register_dcc_type(char *nick, char *type, char *description, char *address, char *port, char *size, char *extra, char *uhost, void (*func1)(int))
{
int Ctype;
unsigned long filesize = 0;
SocketList *s;
DCC_int *n;
DCC_List *new_d = NULL;
unsigned long TempLong;
unsigned int  TempInt;
void (*func)(int) = NULL;
int autoget = 0;
int autoresume = 0;
unsigned long tdcc = 0;
char *fullname = NULL;
UserList *ul = NULL;

	set_display_target(NULL, LOG_DCC);
	if (description)
	{
		char *c;
#if defined(__EMX__) || defined(WINNT)
		if ((c = strrchr(description, '/')) || (c = strrchr(description, '\\')))
#else
		if ((c = strrchr(description, '/')))
#endif
			description = c + 1;
		if (description && *description == '.')
			*description = '_';
	}
	if (size && *size)
		filesize = atol(size);

	TempLong = strtoul(address, NULL, 10);
	TempInt = (unsigned)strtoul(port, NULL, 10);
	if (TempInt < 1024)
	{
		put_it("%s", convert_output_format("$G %RDCC%n Privileged port attempt [$0]", "%d", TempInt));
		reset_display_target();
		return;
	}
	else if (TempLong == 0 || TempInt == 0)
	{
		struct in_addr in;
		in.s_addr = htonl(TempLong);
		put_it("%s", convert_output_format("$G %RDCC%n Handshake ignored because of 0 port or address [$0:$1]", "%d %s", TempInt, inet_ntoa(in)));
		reset_display_target();
		return;
	}

	if (check_dcc_init(type, nick, uhost, description, size, extra, TempLong, TempInt))
		return;
	if (*type == 'T' && *(type+1)) 
	{
		tdcc = DCC_TDCC;
		type++;
	}	


	if (!my_stricmp(type, "CHAT"))
		Ctype = DCC_CHAT;
	else if (!my_stricmp(type, "BOT"))
		Ctype = DCC_BOTMODE;
	else if (!my_stricmp(type, "SEND"))
		Ctype = DCC_FILEREAD;
	else if (!my_stricmp(type, "RESEND"))
		Ctype = DCC_REFILEREAD;
#ifdef MIRC_BROKEN_DCC_RESUME
	else if (!my_stricmp(type, "RESUME"))
		
	{
		/* 
		 * Dont be deceieved by the arguments we're passing it.
		 * The arguments are "out of order" because MIRC doesnt
		 * send them in the traditional order.  Ugh. Comments
		 * borrowed from epic.
		 */
		dcc_getfile_resume_demanded(nick, description, address, port);
		return;
	}
	else if (!my_stricmp(type, "ACCEPT"))
	{
		/*
		 * See the comment above.
		 */
		dcc_getfile_resume_start (nick, description, address, port);
		return;
	}
#endif
       	else
       	{
		put_it("%s", convert_output_format("$G %RDCC%n Unknown DCC $0 ($1) recieved from $2", "%s %s %s", type, description, nick));
		reset_display_target();
       	        return;
       	}
	
	if (tdcc) type--;
	
	if ((s = find_dcc(nick, description, NULL, Ctype, 0, -1, -1)))
	{
		if ((s->flags & DCC_ACTIVE))
		{
			/* collision. */
			put_it("%s", convert_output_format("$G %RDCC%n Recieved DCC $0 request from $1 while previous session active", "%s %s", type, nick));
			reset_display_target();
			return;
		}
		if ((s->flags && DCC_WAIT))
		{
			if (Ctype == DCC_CHAT)
			{
				autoget = 1;
		                if (do_hook(DCC_CONNECT_LIST,"%s CHAT %s",nick, "already requested connecting"))
					put_it("%s", convert_output_format(fget_string_var(FORMAT_DCC_CONNECT_FSET), 
						"%s %s %s %s %s %d", update_clock(GET_TIME), "CHAT", nick, 
						space, "already requested connecting...", 0));
		  		dcc_chat(NULL, nick);
				return;
		  	}
			send_ctcp(CTCP_NOTICE, nick, CTCP_DCC, "DCC %s collision occured while connecting to %s (%s)", type, nickname, description);
			erase_dcc_info(s->is_read, 1, "%s", convert_output_format("$G %RDCC%n $0 collision for $1:$2", "%s %s %s", type, nick, description));
			close_socketread(s->is_read);
			reset_display_target();
			return;
		}
  	}
	if (do_hook(DCC_REQUEST_LIST,"%s %s %s %lu",nick, dcc_types[Ctype]->name, description, filesize))
	{
#ifdef WANT_USERLIST
		ul = lookup_userlevelc(nick, FromUserHost, "*", NULL);
#endif
		if (!dcc_quiet)
		{
			struct in_addr in;	
			char buf[40];
			in.s_addr = htonl(TempLong);
			sprintf(buf, "%2.4g",_GMKv(filesize));
			if (filesize)
				put_it("%s", convert_output_format(fget_string_var(FORMAT_DCC_REQUEST_FSET), 
					"%s %s \"%s\" %s %s %s %d %s %s", 
					update_clock(GET_TIME),dcc_types[Ctype]->name, description,
					nick, FromUserHost,
					inet_ntoa(in), TempInt,
					_GMKs(filesize), buf));
			else
				put_it("%s", convert_output_format(fget_string_var(FORMAT_DCC_REQUEST_FSET), 
					"%s %s %s %s %s %s %d", 
					update_clock(GET_TIME),dcc_types[Ctype]->name,
					description, nick, FromUserHost,
					inet_ntoa(in), TempInt));
		}
		if (Ctype == DCC_CHAT)
		{
#ifdef WANT_USERLIST
			if (get_int_var(BOT_MODE_VAR) && ul)
				autoget = 1;
			else
#endif
			{
				extern char *last_chat_req;
				bitchsay("Type /chat to answer or /nochat to close");
				malloc_strcpy(&last_chat_req, nick);
			}
			if (beep_on_level & LOG_DCC)
				beep_em(1);
		}
#if 0
		if ((Ctype == DCC_BOTMODE) && (p = get_string_var(BOT_PASSWORD_VAR)))
		{
			if (encrypt && !strcmp(p, encrypt))
				;/* do it */
			message_from(NULL, LOG_CRAP);
			return;
		}
#endif
	}
	n = new_malloc(sizeof(DCC_int));
	if (uhost)
		n->userhost = m_strdup(uhost);
	n->ul = ul;
	n->remport = htons(TempInt);
	n->remote.s_addr = htonl(TempLong);
	n->filesize = filesize;
	n->filename = m_strdup(description);
	n->user = m_strdup(nick);
	n->blocksize = get_int_var(DCC_BLOCK_SIZE_VAR);
	n->server = from_server;
	n->file = -1; /* just in case */
	n->struct_type = DCC_STRUCT_TYPE;
		
	if ((Ctype == DCC_FILEREAD) || (Ctype == DCC_REFILEREAD))
	{
		struct stat statit;
		char *tmp = NULL, *p;
		malloc_sprintf(&tmp, "%s/%s", get_string_var(DCC_DLDIR_VAR), n->filename);
		p = expand_twiddle(tmp);

		if ((get_int_var(DCC_AUTOGET_VAR) || find_name_in_genericlist(nick, dcc_no_flood, DCC_HASHSIZE, 0)) &&
			(n->filesize/1024 < get_int_var(DCC_MAX_AUTOGET_SIZE_VAR)) )
			autoget = 1;
		if (Ctype == DCC_FILEREAD)
		{
			int exist = 0;
			if ( !dcc_overwrite_var && autoget && ((exist = stat(p, &statit)) == 0))
			{
				if (!get_int_var(DCC_AUTORENAME_VAR))
				{
					if (!get_int_var(DCC_AUTORESUME_VAR))
					{
						/* the file exists. warning is generated */
						put_it("%s", convert_output_format("$G %RDCC%n Warning: File $0 exists: use /DCC rename if you dont want to overwrite", "%s", p));
						autoget = 0;
					}
					else
					{
						put_it("%s", convert_output_format("$G %RDCC%n Warning: File $0 exists: trying to autoresume", "%s", p));
						autoresume = 1;
					}
				}
				else
					rename_file(p, &n->filename);
			}
#ifdef MIRC_BROKEN_DCC_RESUME
			if (!autoget && exist)
				put_it("%s", convert_output_format("$G %RDCC%n Warning: File $0 exists: use /DCC RESUME nick if you want to resume this file", "%s", p));
#endif
		}
		malloc_sprintf(&tmp, "%s/%s", get_string_var(DCC_DLDIR_VAR), n->filename);
		fullname = expand_twiddle(tmp);
		new_free(&tmp); new_free(&p);
	}

	if (!func)
	{
		switch (Ctype)
		{
#ifndef BITCHX_LITE
			case DCC_BOTMODE:
				func = process_dcc_bot;
				break;
#endif
			case DCC_CHAT:
				func = process_dcc_chat;
				break;
			case DCC_REFILEREAD:
			case DCC_FILEREAD:
				func = start_dcc_get;
				break;
			case DCC_FILEOFFER:
			case DCC_REFILEOFFER:
				func = process_dcc_send1;
				break;
			default:
				yell("Unsupported dcc type [%s]", type);
				return;
		}
	}
	else
		func = func1;
	n->dccnum = ++dcc_count;
	new_d = new_malloc(sizeof(DCC_List));
	new_d->sock.info = n;
	new_d->sock.server = new_d->nick = m_strdup(nick);
	new_d->sock.port = TempInt;
	new_d->sock.flags = Ctype|DCC_OFFER|tdcc;
	new_d->sock.time = now + dcc_timeout;
	new_d->sock.func_read = func; /* point this to the startup function */
	new_d->next = pending_dcc;
	pending_dcc = new_d;
	if (autoget && fullname)
	{
		if (!n->filesize)
		{
			put_it("%s", convert_output_format("$G %RDCC Caution Filesize is 0!! No Autoget", NULL, NULL));
			reset_display_target();
			return;
		}
		if (autoget)
		{
			struct stat sb;
			if (autoresume && stat(fullname, &sb) != -1) {
				n->transfer_orders.byteoffset = sb.st_size;
				n->bytes_read = 0L;
				send_ctcp(CTCP_PRIVMSG, nick, CTCP_DCC, "RESUME %s %d %ld", n->filename, ntohs(n->remport), sb.st_size);
			} else {
				DCC_int *new = NULL;
				int mode = O_WRONLY | O_CREAT | O_BINARY;
				if (!dcc_quiet)
				{
					char *prompt;
					char local_type[30];
					*local_type = 0;
					if (tdcc)
						*local_type = 'T';
					strcat(local_type, dcc_types[Ctype]->name);
					lower(local_type);
					prompt = m_strdup(convert_output_format(get_string_var(CDCC_PROMPT_VAR), NULL, NULL));
					put_it("%s", convert_output_format("$0 Auto-$1ing file %C$3-%n from %K[%C$2%K]", "%s %s %s %s",
													   prompt, local_type, nick, n->filename));
					new_free(&prompt);
				}
				if (Ctype == DCC_REFILEREAD)
					mode |= O_APPEND;
				if ((n->file = open(fullname, mode, 0644)) > 0)
				{
					if ((new = dcc_create(nick, n->filename, NULL, n->filesize, 0, Ctype, DCC_OFFER|tdcc, func)))
						new->blocksize = get_int_var(DCC_BLOCK_SIZE_VAR);
				}
				else
					put_it("%s", convert_output_format("$G %RDCC%n Unable to open $0-", "%s", fullname));
			}
		}
	}
	if (Ctype == DCC_CHAT && autoget)
		dcc_create(nick, n->filename, NULL, n->filesize, 0, Ctype, DCC_OFFER, func);
	reset_display_target();
	new_free(&fullname);
}

void	process_dcc(char *args)
{
	char	*command;
	int	i;

	if (!(command = next_arg(args, &args)))
		return;
	reset_display_target();
	if (dcc_dllcommands)
	{
		DCC_dllcommands *dcc_comm;
		if ((dcc_comm = (DCC_dllcommands *)find_in_list((List **)&dcc_dllcommands, command, 0)))
		{
			dcc_comm->function(dcc_comm->name, args);
			reset_display_target();
			return;
		}
	}
	for (i = 0; dcc_commands[i].name != NULL; i++)
	{
		if (!my_strnicmp(dcc_commands[i].name, command, strlen(command)))
		{
			dcc_commands[i].function(dcc_commands[i].name, args);
			reset_display_target();
			return;
		}
	}
	put_it("%s", convert_output_format("$G Unknown %RDCC%n command: $0", "%s", command));
	reset_display_target();
}


char *get_dcc_args(char **args, char **passwd, char **port, int *blocksize)
{
char *user = NULL;
	while (args && *args && **args)
	{
		char *argument = new_next_arg(*args, args);
		if (argument && *argument)
		{
		
			if (*argument == '-')
			{
				argument++;
				if (*argument == 'e')
					*passwd = next_arg(*args, args);
				else if (*argument == 'p')
					*port = next_arg(*args, args);
				else if (*argument == 'b')
					*blocksize = my_atol(next_arg(*args, args));
			}
			else
				user = argument;
		}
		if (user) break;
	}
	return user;
}

void dcc_chat(char *command, char *args)
{
	char	*user;
	char 	*nick;
	
	char	*passwd = NULL;
	char	*port 	= NULL;
	char	*equal_user = NULL;
	DCC_int *new 	= NULL;
	SocketList *s	= NULL;
	int blocksize = get_int_var(DCC_BLOCK_SIZE_VAR);
	int bot = 0;
	int flags = 0;

#ifndef BITCHX_LITE
	if (command && !my_stricmp(command, "BOT"))
		bot++;
#endif

#ifdef HAVE_SSL
	if(my_strnicmp(args, "-SSL", 4) == 0)
	{
		new_next_arg(args, &args);
		flags = DCC_SSL;
	}
#endif

	user = get_dcc_args(&args, &passwd, &port, &blocksize);
	if (!user)
	{
		put_it("%s", convert_output_format("$G %RDCC%n You must supply a nickname for DCC chat", NULL, NULL));
		return;
	}
	if (!blocksize || blocksize > MAX_DCC_BLOCK_SIZE)
		blocksize = get_int_var(DCC_BLOCK_SIZE_VAR);

	while ((nick = next_in_comma_list(user, &user)))
	{
		if (!nick || !*nick)
			break;
		if (isme(nick)) continue;	
		if ((s = find_dcc(nick, "chat", NULL, bot?DCC_BOTMODE:DCC_CHAT, 1, -1, -1)))
		{
			if ((s->flags & DCC_ACTIVE) || (s->flags & DCC_WAIT))
				put_it("%s", convert_output_format("$G %RDCC%n A previous DCC chat to $0 exists", "%s", nick));
			new = (DCC_int *)s->info;
			if (s->flags & DCC_WAIT)
			{
				send_ctcp_booster(nick, 
					dcc_types[DCC_CHAT]->name, "chat", 
					htonl(get_int_var(DCC_USE_GATEWAY_ADDR_VAR) ? get_server_uh_addr(from_server).sf_addr.s_addr : get_server_local_addr(from_server).sf_addr.s_addr), 
					htons(s->port), new->filesize);
				add_sockettimeout(s->is_read, 120, NULL);
			}
			continue;
		}
		if ((new = dcc_create(nick, "chat", passwd, 0, port? atol(port) : 0, bot?DCC_BOTMODE:DCC_CHAT, DCC_TWOCLIENTS|flags, start_dcc_chat)))
		{
			new->blocksize = blocksize;
			if (!bot)
			{
				equal_user = alloca(strlen(nick)+4);
				strcpy(equal_user, "="); 
				strcat(equal_user, nick);
				addtabkey(equal_user, "msg", 0);
			}
			userhostbase(nick, add_userhost_to_chat, 1, NULL);
		}
		doing_multi++;
	}
	doing_multi = 0;
}

void close_dcc_file(int snum)
{
SocketList	*s;
DCC_int		*n;
char		lame_ultrix[30];	/* should be plenty */
char		lame_ultrix2[30];
char		lame_ultrix3[30];
char		buffer[50];
char		*tofrom = NULL;
char		*filename, *p;
#ifdef GUI
char		*who;
#endif

double		xtime;
double		xfer;
double		temp;
unsigned long	type;
char		lame_type[30];

	s = get_socket(snum);
	if (!s || !(n = (DCC_int *)s->info))
		return;
	type = s->flags & DCC_TYPES;
	*lame_type = 0;
	if (s->flags & DCC_TDCC)
		strcpy(lame_type, "T");	
	strcat(lame_type, dcc_types[type]->name);

	xtime = BX_time_diff(n->starttime, get_time(NULL));
	xfer = (double)(n->bytes_sent ? n->bytes_sent : n->bytes_read);

	if (xfer == 0.0)
		xfer = 1.0;
	if (xtime == 0.0)
		xtime = 1.0;
	temp = xfer / xtime;
	sprintf(lame_ultrix, "%2.4g %s", _GMKv(temp), _GMKs(temp));
	/* Cant pass %g to put_it (lame ultrix/dgux), fix suggested by sheik. */
	sprintf(lame_ultrix2, "%2.4g%s", _GMKv(xfer), _GMKs(xfer));
	sprintf(lame_ultrix3, "%2.4g", xtime);
	sprintf(buffer, "%%s %s %%s %%s TRANSFER COMPLETE", lame_type);

	filename = LOCAL_COPY(n->filename);
	p = filename;
	while ((p = strchr(p, ' ')))
		*p = '_';

	switch(type)
	{
		case DCC_FILEREAD:
		case DCC_REFILEREAD:
			tofrom = "from";
			break;
		case DCC_FILEOFFER:
		case DCC_REFILEOFFER:
			tofrom = "to";
			break;
		default:
			tofrom = "to";
	}
	set_display_target(NULL, LOG_DCC);
	if(do_hook(DCC_LOST_LIST,buffer,s->server, strip_path(n->filename), lame_ultrix))
		put_it("%s", convert_output_format(fget_string_var(FORMAT_DCC_LOST_FSET),
			"%s %s %s %s %s %s %s %s", 
			update_clock(GET_TIME), lame_type, strip_path(filename), 
			lame_ultrix2, tofrom, s->server, lame_ultrix3, lame_ultrix));

#ifdef GUI
	who = LOCAL_COPY(s->server);
#endif
	erase_dcc_info(snum, 0, NULL);
	close_socketread(snum);
	update_transfer();
	update_all_status(current_window, NULL, 0);
#ifdef GUI
	gui_setfileinfo(filename, who, from_server);
#endif
	reset_display_target();
}

/*
 * following 3 functions process dcc filesends.
 */

void process_dcc_send1(int snum)
{
SocketList *s;
DCC_int *n;
u_32int_t bytes = 0;
int bytesread = 0;
char *buffer = alloca(MAX_DCC_BLOCK_SIZE+1);
	s = get_socket(snum);
	n = (DCC_int *)s->info;
	if (n->readwaiting || n->eof)
	{
		int numbytes = 0;
		if (!(s->flags & DCC_TDCC) && n->readwaiting)
		{
			if ((ioctl(snum, FIONREAD, &numbytes)) == -1)
			{
				erase_dcc_info(snum, 1, convert_output_format("$G %RDCC%n Remote $0 closed dcc send", "%s", s->server));
				close_socketread(snum);
				return;
			}
			if (numbytes)
			{	
				if (read(snum, &bytes, sizeof(u_32int_t)) < sizeof(u_32int_t))
				{
					erase_dcc_info(snum, 1, convert_output_format("$G %RDCC%n Remote closed dcc send", NULL));
					close_socketread(snum);
				}
				bytes = (unsigned long)ntohl(bytes);
				get_time(&n->lasttime);
				if (bytes == (n->filesize - n->transfer_orders.byteoffset))
				{
					close_dcc_file(snum);
					return;
				}
				else if (!n->dcc_fast && (bytes != n->bytes_sent))
					return;
				n->readwaiting = 0;
			}
		}
		else if (n->eof)
		{
			u_32int_t *buf;
			n->readwaiting = 1;
			if ((ioctl(snum, FIONREAD, &numbytes) == -1))
			{
				close_dcc_file(snum);
				return;
			}
			buf = alloca(numbytes+1);
			numbytes = read(snum, buf, numbytes);
			switch(numbytes)
			{
				case -1:
					erase_dcc_info(snum, 1, convert_output_format("$G %RDCC%n Remote $0 closed dcc send", "%s", s->server));
					close_socketread(snum);
					break;
				case 0:
					close_dcc_file(snum);
					break;
			}
			return;
		}
	}

	if ((bytesread = read(n->file, buffer, n->blocksize)) > 0)
	{
		int num;
		my_encrypt(buffer, bytesread, n->encrypt);
		num = send(snum, buffer, bytesread, 0);
		if (num != bytesread)
		{
			if (num == -1)
			{
				if (errno == EWOULDBLOCK || errno == ENOBUFS)
					lseek(n->file, -bytesread, SEEK_CUR);
				else
				{
					erase_dcc_info(snum, 1, convert_output_format("$G %RDCC%n Remote $0 closed dcc send", "%s", s->server));
					close_socketread(snum);
					return;
				}
			}
			else 
			{
				lseek(n->file, -(bytesread - num), SEEK_CUR);
				n->bytes_sent += num;
			}
			n->readwaiting = 1;
			return;
		}
		n->bytes_sent += bytesread;
		n->packets = n->bytes_sent / n->blocksize;
		n->readwaiting = 1;
		get_time(&n->lasttime);
		if (!(n->packets % 10))
		{
			update_transfer();
			if (!(n->packets % 20))
				update_all_status(current_window, NULL, 0);
		}
	}
	else if (!n->eof)
	{
		n->eof = 1;
		n->readwaiting = 1;
	}
	else if (!n->dcc_fast)
		close_dcc_file(snum);
	else
		n->readwaiting = 1;
}

void start_dcc_send(int s)
{
struct	sockaddr_in	remaddr;
int	sra;
int	type;
int	new_s = -1;
int	tdcc = 0;
char	*nick = NULL;	
unsigned long flags;
DCC_int *n = NULL;
struct	transfer_struct received = {0};
char local_type[30];
                
	if (!(flags = get_socketflags(s))) 
		return; /* wrong place damn it */
	nick = get_socketserver(s);
	sra = sizeof(struct sockaddr_in);
	new_s = my_accept(s, (struct sockaddr *) &remaddr, &sra);
	type = flags & DCC_TYPES;
	n = get_socketinfo(s);		

	set_display_target(NULL, LOG_DCC);
	if ((add_socketread(new_s, ntohs(remaddr.sin_port), flags, nick, process_dcc_send1, NULL)) < 0)
	{
		erase_dcc_info(s, 1, "%s", convert_output_format("$G %RDCC error: accept() failed. punt!!", NULL, NULL));
		close_socketread(s);
		return;
	}
	flags &= ~DCC_WAIT;
	flags |= DCC_ACTIVE;
	set_socketflags(new_s, flags);
	set_socketinfo(new_s, n);
	if ((n->file = open(n->filename, O_RDONLY | O_BINARY)) == -1)
	{
		erase_dcc_info(new_s, 1, "%s", convert_output_format("$G %RDCC%n Unable to open $0: $1-", "%s %s", n->filename, errno ? strerror(errno) : "Unknown Host"));
		close_socketread(new_s);
		close_socketread(s);
		return;
	}
	if (type == DCC_REFILEOFFER)
	{
		alarm(1);
		recv(new_s, (char *)&received, sizeof(struct transfer_struct), 0);
		alarm(0);
		if (byte_order_test() != received.byteorder)
		{
			/* the packet sender orders bytes differently than us,
			 * reverse what they sent to get the right value 
			 */
			n->transfer_orders.packet_id = 
				((received.packet_id & 0x00ff) << 8) | 
				((received.packet_id & 0xff00) >> 8);

			 n->transfer_orders.byteoffset = 
				((received.byteoffset & 0xff000000) >> 24) |
				((received.byteoffset & 0x00ff0000) >> 8)  |
				((received.byteoffset & 0x0000ff00) << 8)  |
				((received.byteoffset & 0x000000ff) << 24);
		}
		else
			memcpy(&n->transfer_orders,&received,sizeof(struct transfer_struct));
		if (n->transfer_orders.packet_id != DCC_PACKETID)
		{
			put_it("%s", convert_output_format("$G %RDCC%n reget packet is invalid!!", NULL, NULL));
			memset(&n->transfer_orders, 0, sizeof(struct transfer_struct));
		}
		else
			put_it("%s", convert_output_format("$G %RDCC%n reget starting at $0", "%s", ulongcomma(n->transfer_orders.byteoffset)));
	}
	if (get_int_var(DCC_FAST_VAR) && !(flags & DCC_TDCC))
	{
		set_non_blocking(new_s);
		n->dcc_fast = 1;
	}
	lseek(n->file, n->transfer_orders.byteoffset, SEEK_SET);
	errno = 0;
	*local_type = 0;
	if (tdcc)
		strcpy(local_type, "T");
	strcat(local_type, dcc_types[type]->name);
	if (do_hook(DCC_CONNECT_LIST, "%s %s %s %d %ld %s", nick, local_type,
		inet_ntoa(remaddr.sin_addr), ntohs(remaddr.sin_port), (unsigned long)n->filesize, n->filename) && !dcc_quiet)
		put_it("%s", convert_output_format(fget_string_var(FORMAT_DCC_CONNECT_FSET),
			"%s %s %s %s %s %d %d", update_clock(GET_TIME),
			local_type, nick, n->userhost?n->userhost:"u@h",
			inet_ntoa(remaddr.sin_addr),ntohs(remaddr.sin_port),n->transfer_orders.byteoffset ));
	get_time(&n->starttime);
	get_time(&n->lasttime);
	close_socketread(s);
	process_dcc_send1(new_s);
	reset_display_target();
}

void real_file_send(char *nick, char *filename, char *passwd, char *port, int tdcc, unsigned long type, int blocksize) 
{
	DCC_int *new 	= NULL;
	SocketList *s	= NULL;
	unsigned long	filesize = 0;
	char	FileBuf[BIG_BUFFER_SIZE+1];
	struct	stat st;
	
#if defined(__EMX__) || defined(WINNT)
	if (*filename == '/' || *filename == '\\')
#else
	if (*filename == '/')
#endif
		strncpy(FileBuf, filename, BIG_BUFFER_SIZE);
	else if (*filename == '~')
	{
		char *fullname;
		if (!(fullname = expand_twiddle(filename)))
		{
			put_it("%s", convert_output_format("$G %RDCC%n Unable to access $0", "%s", filename));
			return;
		}
		strncpy(FileBuf, fullname, BIG_BUFFER_SIZE);	
		new_free(&fullname);
	}
#if defined(__EMX__) || defined(WINNT)
	else if (strlen(filename) > 3 && ( (*(filename+1) == ':') && (*(filename+2) == '/' || *(filename+2) == '\\')) )
		strlcpy(FileBuf, filename, BIG_BUFFER_SIZE);
#endif
	else
	{
		getcwd(FileBuf, BIG_BUFFER_SIZE);
		strlcat(FileBuf, "/", BIG_BUFFER_SIZE);
		strlcat(FileBuf, filename, BIG_BUFFER_SIZE);
	}
#if defined(__EMX__) || defined(WINNT)
	convert_unix(FileBuf);
#endif

	if (strchr(FileBuf , '*'))
	{
		char *path, *c, *filebuf = NULL;
		DIR *dp;
		struct dirent *dir;
		int count = 0;

		path = LOCAL_COPY(FileBuf);

		if ((c = strrchr(path, '/')))
			*c++ = 0;

		if (!(dp = opendir(path)))
			return;
		while ((dir = readdir(dp)))
		{
			if (!dir->d_ino)
				continue;
			if (!wild_match(c, dir->d_name))
				continue;
			malloc_sprintf(&filebuf, "%s/%s", path, dir->d_name);
			stat(filebuf, &st);
			if (S_ISDIR(st.st_mode))
			{
				new_free(&filebuf);
				continue;
			}
			/* finally. do a send */
			if ((s = find_dcc(nick, filebuf, NULL, type, 1, -1, -1)))
			{
				if ((s->flags & DCC_ACTIVE) || (s->flags & DCC_WAIT))
					put_it("%s", convert_output_format("$G %RDCC%n A previous DCC send to $0 exists", "%s", nick));
				new = (DCC_int *)s->info;
				if (s->flags & DCC_WAIT)
				{
					send_ctcp_booster(nick, dcc_types[type]->name, filebuf, htonl(get_server_local_addr(from_server).sf_addr.s_addr), htons(s->port), new->filesize);
					add_sockettimeout(s->is_read, 120, NULL);
				}
				continue;
			}
			if ((new = dcc_create(nick, filebuf, passwd, st.st_size, port? atol(port) : filesize, type, (tdcc? DCC_TDCC: 0) |DCC_TWOCLIENTS, start_dcc_send)))
				new->blocksize = blocksize;
			
			count++;
			new_free(&filebuf);
		}
		closedir(dp);
		if (!dcc_quiet)
		{
			char buff[30];
			strcpy(buff, "file");
			if (count > 1)
				strcat(buff, "s");
			if (count)
				put_it("%s", convert_output_format("$G %RDCC%n Sent DCC SEND request to $0 for $1 $2-", "%s %s %s", nick, buff, filename));
			else
				put_it("%s", convert_output_format("$G %RDCC%n No Files found matching $*", "%s", filename));
		}
		return;
	}
	if (access(FileBuf, R_OK))
	{
		put_it("%s", convert_output_format("$G %RDCC%n No such file $0 exists", "%s", FileBuf));
		return;
	}
	stat(FileBuf, &st);
	if (S_ISDIR(st.st_mode))
	{
		put_it("%s", convert_output_format("$G %RDCC%n $0 is a directory", "%s", FileBuf));
		return;
	}
	if ((s = find_dcc(nick, FileBuf, NULL, type, 1, -1, -1)))
	{
		if ((s->flags & DCC_ACTIVE) || (s->flags & DCC_WAIT))
			put_it("%s", convert_output_format("$G %RDCC%n A previous DCC send to $0 exists", "%s", nick));
		new = (DCC_int *)s->info;
		if (s->flags & DCC_WAIT)
		{
			send_ctcp_booster(nick, dcc_types[type]->name, FileBuf, htonl(get_server_local_addr(from_server).sf_addr.s_addr), htons(s->port), new->filesize);
			add_sockettimeout(s->is_read, 120, NULL);
		}
		return;
	}
	if ((new = dcc_create(nick, FileBuf, passwd, st.st_size, port? atol(port) : filesize, type, (tdcc? DCC_TDCC: 0) |DCC_TWOCLIENTS, start_dcc_send)))
		new->blocksize = blocksize;
}

void BX_dcc_filesend(char *command, char *args)
{
	char	*user;
	char 	*nick;
	
	char	*passwd = NULL;
	char	*filename = NULL;
	char	*port 	= NULL;
	int tdcc = 0;
	int blocksize = get_int_var(DCC_BLOCK_SIZE_VAR);
		
	user = get_dcc_args(&args, &passwd, &port, &blocksize);
	if (!user)
	{
		put_it("%s", convert_output_format("$G %RDCC%n You must supply a nickname for DCC send", NULL, NULL));
		return;
	}
	if (!blocksize || blocksize > MAX_DCC_BLOCK_SIZE)
		blocksize = get_int_var(DCC_BLOCK_SIZE_VAR);

	if (command && *command == 'T')
		tdcc = DCC_TDCC;
	while ((nick = next_in_comma_list(user, &user)))
	{
		char *new_args;
		if (!nick || !*nick)
			break;
		new_args = LOCAL_COPY(args);
		while ((filename = new_next_arg(new_args, &new_args)))
		{
			if (!filename || !*filename)
				break;
			real_file_send(nick, filename, passwd, port, tdcc, DCC_FILEOFFER, blocksize);
		}
		doing_multi++;
	}
	doing_multi = 0;
}

void BX_dcc_resend(char *command, char *args)
{
	char	*user;
	char 	*nick;
	
	char	*passwd = NULL;
	char	*filename = NULL;
	char	*port 	= NULL;
	int tdcc = 0;
	int blocksize = get_int_var(DCC_BLOCK_SIZE_VAR);
		
	user = get_dcc_args(&args, &passwd, &port, &blocksize);
	if (!user)
	{
		put_it("%s", convert_output_format("$G %RDCC%n You must supply a nickname for DCC resend", NULL, NULL));
		return;
	}
	if (!blocksize || blocksize > MAX_DCC_BLOCK_SIZE)
		blocksize = get_int_var(DCC_BLOCK_SIZE_VAR);

	if (command && *command == 'T')
		tdcc = DCC_TDCC;
	while ((nick = next_in_comma_list(user, &user)))
	{
		char *new_args;
		if (!nick || !*nick)
			break;
		new_args = LOCAL_COPY(args);
		while ((filename = new_next_arg(new_args, &new_args)))
		{
			if (!filename || !*filename)
				break;
			real_file_send(nick, filename, passwd, port, tdcc, DCC_REFILEOFFER, blocksize);
		}
		doing_multi++;
	}
	doing_multi = 0;
}



void start_dcc_get(int snum)
{
DCC_int *n;
SocketList *s;
int bytes_read;
unsigned long type;
int tdcc = 0;
char buffer[MAX_DCC_BLOCK_SIZE+1];
int err;
	s = get_socket(snum);
	n = (DCC_int *)s->info;
	type = s->flags & DCC_TYPES;
	tdcc = s->flags & DCC_TDCC;



	set_display_target(NULL, LOG_DCC);
	bytes_read = read(snum, buffer, MAX_DCC_BLOCK_SIZE);
	switch(bytes_read)
	{
		case -1:
			erase_dcc_info(snum, 1, "%s", convert_output_format("$G %RDCC get to $0 lost: Remote peer closed connection", "%s", s->server));
			close_socketread(snum);
			return;
		case 0:
			close_dcc_file(snum);
			return;
	}	
	my_decrypt(buffer, bytes_read, n->encrypt);
	err = write(n->file, buffer, bytes_read);
	if (err == -1)
	{
		erase_dcc_info(snum, 1, "write to local file failed");
		reset_display_target();
		close_socketread(snum);
		return;
	}
	n->bytes_read += bytes_read;
	n->packets = n->bytes_read / n->blocksize;
	{
		u_32int_t bytes;
		bytes = htonl(n->bytes_read);
		send(snum, (char *)&bytes, sizeof(u_32int_t), 0);
	}	
	if (n->filesize)
	{
		if (n->bytes_read + n->transfer_orders.byteoffset > n->filesize)
		{
			put_it("%s", convert_output_format("$G %RDCC%n Warning: incoming file is larger than the handshake said", NULL, NULL));
			put_it("%s", convert_output_format("$G %RDCC%n Warning: GET: closing connection", NULL, NULL));
			erase_dcc_info(snum, 1, NULL);
			close_socketread(snum);
		}
		else if ((n->bytes_read + n->transfer_orders.byteoffset) == n->filesize)
		{
			close_dcc_file(snum);
			reset_display_target();
			return;
		}
		if (!(n->packets % 10))
		{
			update_transfer();
			if (!(n->packets % 20))
				update_all_status(current_window, NULL, 0);
		}
	}
	reset_display_target();
}

void real_get_file(char *user, char *filename, char *port, char *passwd, int tdcc, int blocksize)
{
DCC_int *new 	= NULL;
SocketList *s	= NULL;
DCC_List *s1	= NULL;
char *nick;

	while ((nick = next_in_comma_list(user, &user)))
	{
		if (!nick || !*nick)
			break;
		
		if ((s1 = find_dcc_pending(nick, filename, NULL, DCC_FILEREAD, 0, -1)))
		{
			new = (DCC_int *)s1->sock.info;
			if ((s = find_dcc(nick, new->filename, NULL, DCC_FILEREAD, 1, -1, -1)))
			{
				put_it("%s", convert_output_format("$G %RDCC%n A previous DCC GET from $0 exists", "%s", nick));
				continue;
			}
		}
		else
			continue;
		if ((new = dcc_create(nick, filename, passwd, 0, port? atol(port) : 0, DCC_FILEREAD, (tdcc?DCC_TDCC:0) | DCC_TWOCLIENTS|DCC_OFFER, start_dcc_get)))
		{
			char *tmp = NULL;	
			char *fullname = NULL;
			new->blocksize = blocksize;
			if (get_string_var(DCC_DLDIR_VAR))
				malloc_sprintf(&tmp, "%s/%s", get_string_var(DCC_DLDIR_VAR), new->filename);
			else
				tmp = m_strdup(new->filename);
			if (!(fullname = expand_twiddle(tmp)))
				malloc_strcpy(&fullname, tmp);
#if defined(WINNT) || defined(__EMX__)
			convert_unix(fullname);
#endif
			s = find_dcc(nick, filename, NULL, DCC_FILEREAD, 1, -1, -1);
			if ((new->file = open(fullname, O_WRONLY | O_TRUNC | O_CREAT | O_BINARY, 0644)) == -1)
			{
				erase_dcc_info(s->is_read, 1, "%s", convert_output_format("$G %RDCC%n Unable to open $0: $1-", "%s %s", fullname, errno?strerror(errno):"Unknown"));
				close_socketread(s->is_read);
			}
			new_free(&fullname);
			new_free(&tmp);
						
		}
		doing_multi++;
	}
}

void dcc_getfile(char *command, char *args)
{
char	*user;
	
char	*passwd = NULL;
char	*filename = NULL;
char	*port 	= NULL;
int	tdcc = 0;
int	blocksize = get_int_var(DCC_BLOCK_SIZE_VAR);
		
	user = get_dcc_args(&args, &passwd, &port, &blocksize);
	if (!user)
	{
		put_it("%s", convert_output_format("$G %RDCC%n You must supply a nickname for DCC get", NULL, NULL));
		return;
	}
	if (!blocksize || blocksize > MAX_DCC_BLOCK_SIZE)
		blocksize = get_int_var(DCC_BLOCK_SIZE_VAR);
	if (command && *command == 'T')
		tdcc = 1;

	if (!args || !*args)
		real_get_file(user, NULL, port, passwd, tdcc, blocksize);
	else
	{
		char *u = alloca(strlen(user)+1);
		while ((filename = next_in_comma_list(args, &args)))
		{
			if (!filename || !*filename)
				break;
			strcpy(u, user);
			real_get_file(u, filename, port, passwd, tdcc, blocksize);
		}
	}
	doing_multi = 0;
}

void real_reget_file(char *user, char *filename, char *port, char *passwd, int tdcc, int blocksize)
{
DCC_int *new 	= NULL;
SocketList *s	= NULL;
char *nick;

	while ((nick = next_in_comma_list(user, &user)))
	{
		char *tmp = NULL;	
		char *fullname = NULL;
		DCC_List *s1 = NULL;
		if (!nick || !*nick)
			break;
	
		if ((s1 = find_dcc_pending(nick, filename, NULL, DCC_REFILEREAD, 0, -1)))
		{
			new = (DCC_int *)s1->sock.info;
			if ((s = find_dcc(nick, new->filename, NULL, DCC_REFILEREAD, 1, -1, -1)))
			{
				put_it("%s", convert_output_format("$G %RDCC%n A previous DCC REGET from $0 exists", "%s", nick));
				continue;
			}
		}
		else
			continue;
		if (get_string_var(DCC_DLDIR_VAR))
			malloc_sprintf(&tmp, "%s/%s", get_string_var(DCC_DLDIR_VAR), new->filename);
		else
			tmp = m_strdup(new->filename);
		if (!(fullname = expand_twiddle(tmp)))
			malloc_strcpy(&fullname, tmp);
		if ((new->file = open(fullname, O_WRONLY | O_CREAT | O_BINARY, 0644)) != -1)
		{
			if ((new = dcc_create(nick, new->filename, passwd, 0, port? atol(port) : 0, DCC_REFILEREAD, (tdcc?DCC_TDCC:0) | DCC_TWOCLIENTS|DCC_OFFER, start_dcc_get)))
				new->blocksize = blocksize;
		}
		new_free(&fullname);
		new_free(&tmp);
		doing_multi++;
	}
}

void dcc_regetfile(char *command, char *args)
{
char	*user;
char	*passwd = NULL;
char	*filename = NULL;
char	*port 	= NULL;
int 	tdcc = 0;
int 	blocksize = get_int_var(DCC_BLOCK_SIZE_VAR);
		
	user = get_dcc_args(&args, &passwd, &port, &blocksize);
	if (!user)
	{
		put_it("%s", convert_output_format("$G %RDCC%n You must supply a nickname for DCC reget", NULL, NULL));
		return;
	}
	if (!blocksize || blocksize > MAX_DCC_BLOCK_SIZE)
		blocksize = get_int_var(DCC_BLOCK_SIZE_VAR);
	if (command && *command == 'T')
		tdcc = 1;
	if (!args || !*args)
		real_reget_file(user, NULL, port, passwd, tdcc, blocksize);
	else
	{
		char *u = alloca(strlen(user)+1);
		while ((filename = next_in_comma_list(args, &args)))
		{
			if (!filename || !*filename)
				break;
			strcpy(u, user);
			real_reget_file(u, filename, port, passwd, tdcc, blocksize);
		}
	}
	doing_multi = 0;
}

static char *get_bar_percent(int percent)
{
#ifdef ONLY_STD_CHARS
static char *_dcc_offer[12] = {"%K-.........%n",		/*  0 */
				"%K-.........%n",		/* 10 */
				"%K-=........%n",		/* 20 */
				"%K-=*.......%n",		/* 30 */
				"%K-=*%1%K=%0%K......%n",	/* 40 */
				"%K-=*%1%K=-%0%K.....%n",	/* 50 */
				"%K-=*%1%K=-.%0%K....%n",	/* 60 */
				"%K-=*%1%K=-. %0%K...%n",	/* 70 */
				"%K-=*%1%K=-. %R.%0%K..%n",	/* 80 */
				"%K-=*%1%K=-. %R.-%0%K.%n",	/* 90 */
				"%K-=*%1%K=-. %R.-=%n",		/* 100 */
				empty_string};
#else
static char *_dcc_offer[12] = {"%K±°°°°°°°°°%n",		/*  0 */
				"%K±°°°°°°°°°%n",		/* 10 */
				"%K±²°°°°°°°°%n",		/* 20 */
				"%K±²Û°°°°°°°%n",		/* 30 */
				"%K±²Û%1%K²%0%K°°°°°°%n",	/* 40 */
				"%K±²Û%1%K²±%0%K°°°°°%n",	/* 50 */
				"%K±²Û%1%K²±°%0%K°°°°%n",	/* 60 */
				"%K±²Û%1%K²±°ÿ%0%K°°°%n",	/* 70 */
				"%K±²Û%1%K²±°ÿ%R°%0%K°°%n",	/* 80 */
				"%K±²Û%1%K²±°ÿ%R°±%0%K°%n",	/* 90 */
				"%K±²Û%1%K²±°ÿ%R°±²%n",		/* 100 */
				empty_string};
#endif
	if (percent <= 100)
		return _dcc_offer[percent];
	return empty_string;
}


void dcc_glist(char *command, char *args)
{
char	*dformat =
	"#$[3]0 $[6]1%Y$2%n $[11]3 $[25]4 $[7]5 $6-";
char	*d1format =
	"#$[3]0 $[6]1%Y$2%n $[11]3 $4 $[7]5 $[11]6 $[7]7 $8-";
char	*c1format =
	"#$[3]0 $[6]1%Y$2%n $[11]3 $4 $[-4]5 $[-4]6 $[-3]7 $[-3]8  $[7]9 $10";

int i;
DCC_int *n = NULL;
SocketList *s;
int type;
int tdcc = 0;
char *status;
int count = 0;
DCC_List *c;
char stats[80];
char kilobytes[20];
int barlen = BAR_LENGTH;
double barsize = 0.0;
char spec[BIG_BUFFER_SIZE];
char *filename, *p;

	reset_display_target();
#if !defined(WINNT) && !defined(__EMX__)
	charset_ibmpc();
#endif
	if (do_hook(DCC_HEADER_LIST, "%s %s %s %s %s %s %s", "Dnum","Type","Nick", "Status", "K/s", "File","Encrypt"))
	{
#ifdef ONLY_STD_CHARS
		put_it("%s", convert_output_format("%G#  %W|%n %GT%gype  %W|%n %GN%gick      %W|%n %GP%gercent %GC%gomplete        %W|%n %GK%g/s   %W|%n %GF%gile", NULL, NULL));
		put_it("%s", convert_output_format("%W------------------------------------------------------------------------------", NULL, NULL));
#else
		put_it("%s", convert_output_format("%G#  %W³%n %GT%gype  %W³%n %GN%gick      %W³%n %GP%gercent %GC%gomplete        %W³%n %GK%g/s   %W³%n %GF%gile", NULL, NULL));
		put_it("%s", convert_output_format("%KÄÄ%nÄ%WÄ%nÄ%KÄÄÄÄÄ%nÄ%WÄ%nÄ%KÄÄÄÄÄÄÄÄÄ%nÄ%WÄ%nÄ%KÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ%nÄ%WÄ%nÄ%KÄÄÄÄÄ%nÄ%WÄ%nÄ%KÄÄÄÄÄ%nÄ%WÄ%nÄ%KÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ", NULL, NULL));
#endif
	}
	for (c = pending_dcc; c; c = c->next, count++)	
	{
		char local_type[30];
		char *filename, *p;
		s = &c->sock;
		type = c->sock.flags & DCC_TYPES;
		tdcc = c->sock.flags & DCC_TDCC;
		n = (DCC_int *)c->sock.info;
		*local_type = 0;
		if (tdcc)
			strcpy(local_type, "T");
		strcat(local_type, dcc_types[type]->name);
		filename = LOCAL_COPY(n->filename);

		p = filename;
		while ((p = strchr(p, ' ')))
			*p = '_';

		if (do_hook(DCC_STAT_LIST, "%d %s %s %s %s %s %s", 
				n->dccnum, local_type, 
				c->sock.server,
				s->flags & DCC_OFFER ? "Offer" :
				s->flags & DCC_WAIT ? "Wait" :
				s->flags & DCC_ACTIVE ? "Active" :
				"Unknown",
				"N/A", strip_path(filename), n->encrypt?"E":empty_string))
		{				
			put_it("%s", convert_output_format(dformat, "%d %s %s %s %s %s %s", 
				n->dccnum, 
				local_type, 
				n->encrypt ? "E" : "ÿ",
				c->sock.server,
			
				s->flags & DCC_OFFER ?     "Offer " :
				s->flags & DCC_WAIT ?      "Wait  " :
				s->flags & DCC_ACTIVE ?    "Active" :
				"Unknown",
				"N/A", 
				strip_path(filename)));
		}
		count++;
	}
	for (i = 0; i < get_max_fd() + 1; i++, count++)
	{
		double perc = 0.0;
		double bytes = 0.0;
		char local_type[30];
		time_t xtime;
		int seconds = 0, minutes = 0;
		int iperc = 0;
		int size = 0;
		if (!check_dcc_socket(i))
			continue;
		s = get_socket(i);
		n = (DCC_int *)s->info;
		type = s->flags & DCC_TYPES;
		tdcc = s->flags & DCC_TDCC;
		xtime = now - n->starttime.tv_sec;
	
		if (xtime <= 0)
			xtime = 1;
		*local_type = 0;
		if (tdcc)
			strcpy(local_type, "T");
		strcat(local_type, dcc_types[type]->name);

		filename = LOCAL_COPY(n->filename);
		p = filename;
		while ((p = strchr(p, ' ')))
			*p = '_';
	
		if (((type == DCC_CHAT) || (type == DCC_RAW) || (type == DCC_BOTMODE) || (type == DCC_FTPOPEN)))
		{
			if (!(s->flags & DCC_ACTIVE))
				xtime = now - s->time;
			if (do_hook(DCC_STAT_LIST, "%d %s %s %s %s %s %s", 
					n->dccnum, local_type, s->server,
					s->flags & DCC_OFFER ? "Offer " :
					s->flags & DCC_WAIT ?  "Wait  " :
					s->flags & DCC_ACTIVE ?"Active" :
					"Unknown",
					"N/A", strip_path(filename), n->encrypt?"E":empty_string))
				put_it("%s", convert_output_format(c1format, "%d %s %s %s %s %s %s %s", 
					n->dccnum, 
					local_type, 
					n->encrypt ? "E" : "ÿ",
					s->server,
					s->flags & DCC_OFFER ? "Offer" :
					s->flags & DCC_WAIT ? "Wait" :
					s->flags & DCC_ACTIVE ? "Active" :
					"Unknown",
					convert_time(xtime),
					"N/A",
					strip_path(filename)));
		}
		else
		{
			if (!(s->flags & DCC_ACTIVE))
			{
			if (do_hook(DCC_STAT_LIST, "%d %s %s %s %s %s %s", 
					n->dccnum, local_type, s->server,
					s->flags & DCC_OFFER ? "Offer " :
					s->flags & DCC_WAIT ?  "Wait  " :
					s->flags & DCC_ACTIVE ?"Active" :
					"Unknown",
					"N/A", strip_path(filename), n->encrypt?"E":empty_string))
				put_it("%s", convert_output_format(dformat, "%d %s %s %s %s %s %s", 
					n->dccnum, 
					local_type, 
					n->encrypt ? "E" : "ÿ",
					s->server,
					s->flags & DCC_OFFER ?     "Offer" :
					s->flags & DCC_WAIT ?      "Wait" :
					s->flags & DCC_ACTIVE ?    "Active" :
					"Unknown",
					"N/A", 
					strip_path(filename)));
				continue;
			}


			bytes = n->bytes_read + n->bytes_sent;

			sprintf(kilobytes, "%2.4g", bytes / 1024.0 / xtime);

			type = s->flags & DCC_TYPES;
			tdcc = s->flags & DCC_TDCC;
			status = s->flags & DCC_OFFER ? "Offer":s->flags & DCC_ACTIVE ? "Active": s->flags&DCC_WAIT?"Wait":"Unknown";
			if ((bytes >= 0) && (s->flags & DCC_ACTIVE))
			{
				if (bytes && (n->filesize - n->transfer_orders.byteoffset) >= bytes)
				{
					perc = (100.0 * ((double)bytes + n->transfer_orders.byteoffset)   / (double)(n->filesize));
					if ( perc > 100.0) perc = 100.0;
					else if (perc < 0.0) perc = 0.0;
					seconds = (int) (( (n->filesize - n->transfer_orders.byteoffset - bytes) / (bytes / xtime)) + 0.5);
					minutes = seconds / 60;
					seconds = seconds - (minutes * 60);
					if (minutes > 999) {
						minutes = 999;
						seconds = 59;
					}	
					if (seconds < 0) seconds = 0;
				} else
					seconds = minutes = perc = 0;
				
				iperc = ((int)perc) / 10;
				barsize = ((double) (n->filesize)) / (double) barlen;
	
				size = (int) ((double) bytes / (double)barsize);
	
				if (n->filesize == 0)
					size = barlen;
				sprintf(stats, "%4.1f", perc);
				if (!get_int_var(DCC_BAR_TYPE_VAR))
					sprintf(spec, "%s %s%s %02d:%02d", get_bar_percent(iperc), stats, "%%", minutes, seconds);
				else
					sprintf(spec, "%s%s %02d:%02d", stats, "%%", minutes, seconds);

				strcpy(spec, convert_output_format(spec, NULL, NULL));
			}	
			if (do_hook(DCC_STATF_LIST, "%d %s %s %s %s %s %s", 
				n->dccnum, local_type, s->server, status,
				kilobytes, strip_path(filename), 
				n->encrypt?"E":empty_string))
			{
				char *s1;
				if (!get_int_var(DCC_BAR_TYPE_VAR))
					s1 = d1format;
				else
					s1 = dformat;

				put_it("%s", convert_output_format(s1, "%d %s %s %s %s %s %s", 
					n->dccnum, local_type, n->encrypt ? "E":"ÿ",
					s->server, spec, kilobytes, 
					strip_path(filename)));
			}

			if (do_hook(DCC_STATF1_LIST, "%s %lu %lu %d %d", stats, (unsigned long)bytes, (unsigned long)n->filesize, minutes, seconds))
			{
				char *bar_end, *c;
				if (!get_int_var(DCC_BAR_TYPE_VAR))
					continue;
				memset(spec, 0, 500);
				sprintf(stats, "%4.1f%% (%lu of %lu bytes)", perc, (unsigned long)bytes, (unsigned long)n->filesize);
				strcpy( spec, "\002[\026");
				sprintf(spec+3, "%*s", size+1, space);
				bar_end = spec + strlen(spec);
				sprintf(bar_end, "%*s", barlen-size+1, space);
				if (size <= barlen)
				{
					memmove(bar_end+1, bar_end, strlen(bar_end));
					*bar_end = '\026';
				}
				strcat(spec, "]\002 ETA \002%02d:%02d\002");
				bar_end = (spec+(((BAR_LENGTH+2) / 2) - (strlen(stats) / 2)));
				for (c = stats;*c; bar_end++)
				{
					if (*bar_end == '\026' || *bar_end == '\002')
						continue;
					*bar_end = *c++;
				}
				put_it(spec, minutes, seconds);	
			}
		}
	}
#if !defined(WINNT) && !defined(__EMX__)
#if defined(LATIN1)
	charset_lat1();
#elif defined(CHARSET_CUSTOM)
	charset_cst();
#endif
#endif
	if (!count)
		bitchsay("No active/pending dcc's");
	else if (do_hook(DCC_POST_LIST, "%s %s %s %s %s %s %s", "DCCnum","Type","Nick", "Status", "K/s", "File","Encrypt"))
		;
}

unsigned char byte_order_test(void)
{
	unsigned short test = DCC_PACKETID;

	if (*((unsigned char *)&test) == ((DCC_PACKETID & 0xff00) >> 8))
		return 0;

	if (*((unsigned char *)&test) == (DCC_PACKETID & 0x00ff))
		return 1;
	return 0;
}

int BX_get_active_count(void)
{
int active = 0;
register int i;
register SocketList *s;
register int type;
	for (i = 0; i < get_max_fd() + 1; i++)
	{
		if (check_dcc_socket(i))
		{
			s = get_socket(i);
			type = s->flags & DCC_TYPES;
			if (type == DCC_REFILEREAD || type == DCC_FTPGET || 
				type == DCC_FILEREAD || type == DCC_FILEOFFER || 
				type == DCC_REFILEOFFER)
				active++;
		}
	}
	return active;
}

#if 0
void dcc_renumber_active(void)
{
register int i;
SocketList *s;
DCC_int *n;
int j = 1;

	for (i = 0; i < get_max_fd() + 1; i++)
	{
		if (check_dcc_socket(i))
		{
			s = get_socket(i);
			n = (DCC_int *)s->info;
			n->dccnum = j++;
		}
	}
	dcc_count = j;
}
#endif

int check_dcc_list (char *name)
{
int do_it = 0;
register int i = 0;
	for (i = 0; i < get_max_fd() + 1; i++)
	{
		if (check_dcc_socket(i))
		{
			SocketList *s;
			s = get_socket(i);
			if (s->server && !my_stricmp(name, s->server) && !(s->flags & DCC_ACTIVE))
			{
				switch (s->flags & DCC_TYPES)
				{
					case DCC_FILEOFFER:
					case DCC_FILEREAD:
					case DCC_REFILEOFFER:
					case DCC_REFILEREAD:
						do_it++;
						erase_dcc_info(i, 0, NULL);
						close_socketread(i);
					default:
						break;
				}
			}
		}
	}
	return do_it;
}


void update_transfer(void)
{
register unsigned	count= 0;
	double		perc = 0.0;
register int		i = 0;
/*	char		temp_str[60];*/
	double		bytes;
	SocketList	*s;
	DCC_int		*n;
	unsigned long	flags;
	char		transfer_buffer[BIG_BUFFER_SIZE];
	char		*c;
			
	*transfer_buffer = 0;
	for (i = 0; i < get_max_fd() + 1 ; i++)
	{
		if (!check_dcc_socket(i))
			continue;
		s = get_socket(i);
		flags = s->flags & DCC_TYPES;
		if ((s->flags & DCC_OFFER) || (s->flags & DCC_WAIT) || (flags == DCC_RAW) || (flags == DCC_RAW_LISTEN) || (flags == DCC_CHAT) || (flags == DCC_BOTMODE) || (flags == DCC_FTPOPEN))
			continue;
		n = (DCC_int *)s->info;
		bytes = n->bytes_read + n->bytes_sent + n->transfer_orders.byteoffset;
		if (bytes >= 0) 
		{
			if (n->filesize >= bytes && (n->filesize > 0))
			{
				perc = (100.0 * ((double)bytes)   / (double)(n->filesize));
				if ( perc > 100.0) perc = 100.0;
				else if (perc < 0.0) perc = 0.0;

			}		
			strmopencat(transfer_buffer, BIG_BUFFER_SIZE, ltoa((int)perc), "% ", NULL);
#if 0
			sprintf(temp_str,"%d%% ",(int) perc);
			strcat(transfer_buffer,temp_str);
#endif
		}
		if (count++ > 9)
			break;
	}
	if (*(c = transfer_buffer))
		chop(transfer_buffer, 1);
	do_hook(DCC_UPDATE_LIST, "%s", *c ? c : empty_string);
	if (*c)
	{
		while ((c = strchr(c, ' '))) *c = ',';
	}
	if (count)
	{
/*		chop(transfer_buffer, 1);*/
		if (fget_string_var(FORMAT_DCC_FSET))
		{
			sprintf(DCC_current_transfer_buffer, convert_output_format(fget_string_var(FORMAT_DCC_FSET), "%s", transfer_buffer));
			chop(DCC_current_transfer_buffer, 4);
		}
		else
			sprintf(DCC_current_transfer_buffer, "[%s]", transfer_buffer);
	}
	else
		*DCC_current_transfer_buffer = 0;
}


/* 
 * returns the string without the path (if present) Also 
 *	converts filename from " " to _ 
 */
static char * strip_path(char *str)
{
	char	*ptr;
#if 0
	char	*q;

	q = str;
	while ((q = strchr(q, ' ')))
		*q = '_';
#endif
	if (dcc_paths)
		return str;
	ptr = strrchr(str,'/');
	if (ptr == NULL)
		return str;
	else
		return ptr+1;
}

void dcc_show_active(char *command, char * args) 
{
	put_it("%s", convert_output_format("$G %RDCC%n  DCC Active = \002$0\002, Limit = \002$1\002", 
		"%d %d", get_active_count(), get_int_var(DCC_SEND_LIMIT_VAR)));
}

void dcc_set_quiet(char *command, char *args ) 
{
	dcc_quiet ^=1;
	put_it("%s", convert_output_format("$G %RDCC%n  DCC Quiet = \002$0\002", "%s", on_off(dcc_quiet)));
}

void dcc_set_paths(char *command, char *args)
{
	dcc_paths ^= 1;
	put_it("%s", convert_output_format("$G %RDCC%n  DCC paths is now \002$0\002", "%s", on_off(dcc_paths)));
}	

void dcc_tog_rename(char *command, char *args)
{
int	arename = get_int_var(DCC_AUTORENAME_VAR);
	arename ^= 1;
	set_int_var(DCC_AUTORENAME_VAR, arename);
	put_it("%s", convert_output_format("$G %RDCC%n  DCC auto rename is now \002$0\002", "%s", on_off(get_int_var(DCC_AUTORENAME_VAR))));
}

void dcc_tog_resume(char *command, char *args)
{
	int   arename = get_int_var(DCC_AUTORESUME_VAR);
	arename ^= 1;
	set_int_var(DCC_AUTORESUME_VAR, arename);
	put_it("%s", convert_output_format("$G %RDCC%n  DCC auto resume is now \002$0\002", "%s", on_off(get_int_var(DCC_AUTORESUME_VAR))));
}

void dcc_overwrite_toggle(char *command, char *args)
{
	dcc_overwrite_var ^= 1;
	put_it("%s", convert_output_format("  DCC overwrite is now \002$0\002", "%s", on_off(dcc_overwrite_var)));
}	

void dcc_tog_auto(char *command, char *args)
{
	int dcc_auto = get_int_var(DCC_AUTOGET_VAR);
	dcc_auto ^= 1;
	set_int_var(DCC_AUTOGET_VAR, dcc_auto);	
	put_it("%s", convert_output_format("  DCC autoget is now \002$0\002", "%s", on_off(dcc_auto)));
}	

void dcc_stats (char *command, char *unused)
{
char max_rate_in[20];
char min_rate_in[20];
char max_rate_out[20];
char min_rate_out[20];

	sprintf(max_rate_in, "%6.2f", dcc_max_rate_in/1024.0);
	sprintf(min_rate_in, "%6.2f", ((dcc_min_rate_in != DBL_MAX )?dcc_min_rate_in/1024.0: 0.0));
	sprintf(max_rate_out, "%6.2f", dcc_max_rate_out/1024.0);
	sprintf(min_rate_out, "%6.2f", ((dcc_min_rate_out != DBL_MAX) ? dcc_min_rate_out/1024.0: 0.0));
	if (do_hook(DCC_TRANSFER_STAT_LIST, "%lu %s %s %lu %s %s %lu %u %u %s %s %s %s", 
		(unsigned long)dcc_bytes_in, max_rate_in, min_rate_in,
		(unsigned long)dcc_bytes_out, max_rate_out, min_rate_out,
		(unsigned long)(send_count_stat+get_count_stat), 
		get_active_count(), get_int_var(DCC_SEND_LIMIT_VAR),
		on_off(get_int_var(DCC_AUTOGET_VAR)), on_off(dcc_paths), 
		on_off(dcc_quiet), on_off(dcc_overwrite_var)))
	{
		char in[50], out[50];
		sprintf(in,  "%3.2f%s", _GMKv(dcc_bytes_in),  _GMKs(dcc_bytes_in));
		sprintf(out, "%3.2f%s", _GMKv(dcc_bytes_out), _GMKs(dcc_bytes_out));

#ifdef ONLY_CTD_CHARS
		put_it("%s",convert_output_format("       %G========================%K[%Cdcc transfer stats%K]%G=======================", NULL));
		put_it("%s",convert_output_format("       %G|                                                                 |", NULL));
		put_it("%s",convert_output_format("       %G|%g|-%K[%Cx%cferd %Ci%cn%K]%g-|-%K[%Cx%cferd %Co%cut%K]%g-|-%K[%Ct%cotal %Cf%ciles%K]%g-|-%K[%Ca%cctive%K]%g-|-[%Cl%cimit%K]%g-|%G|", NULL));
		put_it("%s",convert_output_format("       %G|%g| %W$[-10]0 %g|  %W$[-10]1 %g|    %W$[-10]2 %g| %W$[-8]3 %g| %W$[-7]4 %g|%G|", "%s %s %d %d %d", in, out,send_count_stat+get_count_stat,get_active_count(),get_int_var(DCC_SEND_LIMIT_VAR)));
		put_it("%s",convert_output_format("       %G|%g|------------|-------------|---------------|----------|---------|%G|", NULL));
		put_it("%s",convert_output_format("       %G|                                                                 |", NULL));
		put_it("%s",convert_output_format("       %g|----%K[%Ci%cn %Cs%ctats%K]%g---|---%K[%Co%cut %Cs%ctats%K]%g---|----------%K[%Ct%coggles%K]%g----------|", NULL));
		put_it("%s",convert_output_format("       %g| %Cm%nax: %W$[-6]0%n%Rkb/s %g| %Cm%nax: %W$[-6]1%n%Rkb/s %g|   %Ca%nutoget: %W$[-3]2%n   %Cp%naths: %W$[-3]3 %g|", "%s %s %s %s", max_rate_in, max_rate_out, on_off(get_int_var(DCC_AUTOGET_VAR)),on_off(dcc_paths)));
		put_it("%s",convert_output_format("       %g| %Cm%nin: %W$[-6]0%n%Rkb/s %g| %Cm%nin: %W$[-6]1%n%Rkb/s %g| %Co%nverwrite: %W$[-3]2%n   %Cq%nuiet: %W$[-3]3 %g|", "%s %s %s %s", min_rate_in, min_rate_out, on_off(dcc_overwrite_var), on_off(dcc_quiet)));
		put_it("%s",convert_output_format("       %g|-----------------|-----------------|-----------------------------|", NULL));

#else

		put_it("%s",convert_output_format("       %GÕÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ%K[%Cdcc transfer stats%K]%GÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¸", NULL));
		put_it("%s",convert_output_format("       %G³                                                                 ³", NULL));
		put_it("%s",convert_output_format("       %G³%gÖÄ%K[%Cx%cferd %Ci%cn%K]%gÄÖ-%K[%Cx%cferd %Co%cut%K]%gÄ·Ä%K[%Ct%cotal %Cf%ciles%K]%gÄÖÄ%K[%Ca%cctive%K]%gÄ·Ä[%Cl%cimit%K]%gÄ·%G³", NULL));
		put_it("%s",convert_output_format("       %G³%gº %W$[-10]0 %gº  %W$[-10]1 %gº    %W$[-10]2 %gº %W$[-8]3 %gº %W$[-7]4 %gº%G³", "%s %s %d %d %d", in, out,send_count_stat+get_count_stat,get_active_count(),get_int_var(DCC_SEND_LIMIT_VAR)));
		put_it("%s",convert_output_format("       %G³%gÓÄÄÄÄÄÄÄÄÄÄÄÄ½ÄÄÄÄÄÄÄÄÄÄÄÄÄÓÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ½ÄÄÄÄÄÄÄÄÄÄÓÄÄÄÄÄÄÄÄÄ½%G³", NULL));
		put_it("%s",convert_output_format("       %G³                                                                 ³", NULL));
		put_it("%s",convert_output_format("       %gÖÄÄÄÄ%K[%Ci%cn %Cs%ctats%K]%gÄÄÄÖÄÄÄ%K[%Co%cut %Cs%ctats%K]%gÄÄÄ·ÄÄÄÄÄÄÄÄÄÄ%K[%Ct%coggles%K]%gÄÄÄÄÄÄÄÄÄÄ·", NULL));
		put_it("%s",convert_output_format("       %gº %Cm%nax: %W$[-6]0%n%Rkb/s %gº %Cm%nax: %W$[-6]1%n%Rkb/s %gº   %Ca%nutoget: %W$[-3]2%n   %Cp%naths: %W$[-3]3 %gº", "%s %s %s %s", max_rate_in, max_rate_out, on_off(get_int_var(DCC_AUTOGET_VAR)),on_off(dcc_paths)));
		put_it("%s",convert_output_format("       %gº %Cm%nin: %W$[-6]0%n%Rkb/s %gº %Cm%nin: %W$[-6]1%n%Rkb/s %gº %Co%nverwrite: %W$[-3]2%n   %Cq%nuiet: %W$[-3]3 %gº", "%s %s %s %s", min_rate_in, min_rate_out, on_off(dcc_overwrite_var), on_off(dcc_quiet)));
		put_it("%s",convert_output_format("       %gÓÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ½ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÓÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ½", NULL));

#endif

	}
}

/*
 * only call this on dcc finish
 */
void dcc_update_stats (int snum)
{
time_t	xtime;
SocketList *s;
DCC_int *n;	
	s = get_socket(snum);
	if (!s || !(n = (DCC_int *)s->info))
		return;
	dcc_bytes_in += n->bytes_read;
	dcc_bytes_out += n->bytes_sent;
	xtime = BX_time_diff(n->starttime, get_time(NULL));

	if (xtime <= 0)
		xtime = 1;
	if (n->bytes_read)
	{
		get_count_stat++;
		if ((double)n->bytes_read/(double)xtime > dcc_max_rate_in)
			dcc_max_rate_in = (double)n->bytes_read/(double)xtime;
		if ((double)n->bytes_read/ (double)xtime < dcc_min_rate_in)
			dcc_min_rate_in = (double)n->bytes_read/(double)xtime;	
	}
	if (n->bytes_sent)
	{
		send_count_stat++;
		if ((double)n->bytes_sent/(double)xtime > dcc_max_rate_out)
			dcc_max_rate_out = (double)n->bytes_sent/(double)xtime;
		if ((double)n->bytes_sent/(double)xtime < dcc_min_rate_out)
			dcc_min_rate_out = (double)n->bytes_sent/ (double)xtime;	
	}
}

/* Looks for the dcc transfer that is "current" (last recieved data)
 * and returns information for it
 */
extern char *DCC_get_current_transfer (void)
{
	return DCC_current_transfer_buffer;
}



BUILT_IN_COMMAND(chat)
{
	int no_chat = 0, flags = 0;

	if (!my_strnicmp(command, "NOC", 3))
		no_chat = 1;

#if HAVE_SSL
	if(my_strnicmp(args, "-SSL", 4) == 0)
	{
		new_next_arg(args, &args);
		flags = DCC_SSL;
	}
#endif

	if (args && *args)
	{
		char *tmp = NULL;
		if (no_chat)
			malloc_sprintf(&tmp, "CLOSE CHAT %s", args);
		else
#ifdef HAVE_SSL
			if(flags & DCC_SSL)
				malloc_sprintf(&tmp, "CHAT -ssl %s", args);
			else
#endif
			malloc_sprintf(&tmp, "CHAT %s", args);
		process_dcc(tmp);
		new_free(&tmp);
	}
	else if (last_chat_req)
	{
		DCC_int *new;
		if (no_chat)
		{
			char *tmp = NULL;
			malloc_sprintf(&tmp, "CLOSE CHAT %s", last_chat_req);
			process_dcc(tmp);
			new_free(&tmp);
		}
		if ((new = dcc_create(last_chat_req, "chat", NULL, 0, 0, DCC_CHAT, DCC_TWOCLIENTS|DCC_OFFER|flags, start_dcc_chat)))
		{
			char *equal_user;
			new->blocksize = get_int_var(DCC_BLOCK_SIZE_VAR);
			equal_user = alloca(strlen(last_chat_req)+4);
			strcpy(equal_user, "="); 
			strcat(equal_user, last_chat_req);
			addtabkey(equal_user, "msg", 0);
			userhostbase(last_chat_req, add_userhost_to_chat, 1, NULL);
			new_free(&last_chat_req);
		}
	} 
}


void dcc_exempt(char *command, char *args)
{
int remove;
List *nptr = NULL;
char *nick;
	if (!args || !*args)
	{
		int count = 0;
		for (nptr = next_namelist(dcc_no_flood, NULL, DCC_HASHSIZE); nptr; nptr = next_namelist(dcc_no_flood, nptr, DCC_HASHSIZE))
		{
			if (count == 0)
				put_it("%s", convert_output_format("$G %RDCC%n autoget list/no flood list", NULL, NULL));
			put_it("%s", nptr->name);
			count++;
		}
		if (count == 0)
			userage("/dcc exempt", "+nick to add, nick to remove"); 
		return;
	}
	nick = next_arg(args, &args);
	while (nick && *nick)
	{
		remove = 1;
		if (*nick == '+')
		{
			remove = 0;
			nick++;
		}
		nptr = find_name_in_genericlist(nick, dcc_no_flood, DCC_HASHSIZE, remove);
		if (remove && nptr)
		{
			bitchsay("removed %s from dcc exempt list", nick);
			new_free(&nptr->name);
			new_free((char **)&nptr);
		}
		else if (!remove && !nptr)
		{
			add_name_to_genericlist(nick, dcc_no_flood, DCC_HASHSIZE);
			bitchsay("added %s to dcc exempt list", nick);
		}
		else if (remove && !nptr)
			put_it("%s", convert_output_format("$G: %RDCC%n No such nick on the exempt list %K[%W$0%K]", "%s", nick));
		nick = next_arg(args, &args);
	}
}

int dcc_exempt_save(FILE *fptr)
{
int count = 0;
List *nptr = NULL;
	if (dcc_no_flood)
	{
		fprintf(fptr, "# Dcc Exempt from autoget OFF list\n");
		fprintf(fptr, "DCC EXEMPT ");
	}
	for (nptr = next_namelist(dcc_no_flood, NULL, DCC_HASHSIZE); nptr; nptr = next_namelist(dcc_no_flood, nptr, DCC_HASHSIZE))
	{
		fprintf(fptr, "+%s ", nptr->name);
		count++;
	}
	if (dcc_no_flood)
	{
		fprintf(fptr, "\n");
		if (count && do_hook(SAVEFILE_LIST, "DCCexempt %d", count))
			bitchsay("Saved %d DccExempt entries", count);
		                        
	}
	return count;
}

/*
 * This is a callback.  When we want to do a CTCP DCC REJECT, we do
 * a WHOIS to make sure theyre still on irc, no sense sending it to
 * nobody in particular.  When this gets called back, that means the
 * peer is indeed on irc, so we send them the REJECT.
 */
static	void 	output_reject_ctcp (UserhostItem *stuff, char *nick, char *args)
{
	char	*nickname_requested;
	char	*nickname_recieved;
	char	*type;
	char	*description;
	if (!stuff || !stuff->user || !strcmp(stuff->user, "<UNKNOWN>"))
	{
		return;
	}
	/*
	 * XXX This is, of course, a monsterous hack.
	 */
	nickname_requested 	= next_arg(args, &args);
	type 			= next_arg(args, &args);
	description 		= next_arg(args, &args);
	nickname_recieved 	= stuff->nick; 

	if (nickname_recieved && *nickname_recieved)
		send_ctcp(CTCP_NOTICE, nickname_recieved, CTCP_DCC,
				"REJECT %s %s", type, description);
}

extern void dcc_reject (char *from, char *type, char *args)
{
	SocketList *s;
	DCC_List *s1 = NULL;
	char	*description;
	int	CType;
	int	tdcc = 0;
	
	upper(type);
	if (*type == 'T' && *(type+1))
		tdcc = 1;
	for (CType = 0; dcc_types[CType]->name != NULL; CType++)
		if (!strcmp(type+tdcc, dcc_types[CType]->name))
			break;

	if (!dcc_types[CType]->name)
		return;
	if (tdcc)
		CType |= DCC_TDCC;
		
	description = next_arg(args, &args);
	if ((s = find_dcc(from, description, NULL, CType, 0, -1, -1)) || (s1 = find_dcc_pending(from, description, NULL, CType, 1, -1)))
	{
		DCC_int *n;
		if (s1)
			s = & s1->sock;
		n = (DCC_int *)s->info;
                if (do_hook(DCC_LOST_LIST,"%s %s %s REJECTED", from, type, description ? description : "<any>"))
			put_it("%s", convert_output_format(fget_string_var(FORMAT_DCC_ERROR_FSET), "%s %s %s %s", update_clock(GET_TIME), type, s->server, n->filename));


		if (!s1)
			erase_dcc_info(s->is_read, 0, NULL);
		close_socketread(s->is_read);
		if (s1)
		{
			new_free(&s->server);
			new_free(&n->userhost);
			new_free(&n->user);
			new_free(&n->encrypt);
			new_free(&n->filename);
			new_free(&s1);
		}
#ifdef WANT_CDCC
		dcc_sendfrom_queue();
#endif
		update_transfer();
		update_all_status(current_window, NULL, 0);
	}
}

BUILT_IN_COMMAND(dcx)
{
int i;
unsigned long flag;
SocketList *s;
int all = 0;
unsigned long type = 0;
char *nick = NULL;
	if (!my_stricmp(command, "DCA"))
		all = 1;
	else if (!my_stricmp(command, "DCG"))
		type = DCC_FILEREAD;
	else if (!my_stricmp(command, "DCS"))
		type = DCC_FILEOFFER;
	else if (!my_stricmp(command, "DCX"))
		type = DCC_CHAT;
	else
		return;
	nick = next_arg(args, &args);
	if (!all && !nick)
		return;
	for (i = 0; i < get_max_fd() + 1; i++)
	{
		if (!check_dcc_socket(i))
			continue;
		s = get_socket(i);
		flag = s->flags & DCC_TYPES;
		if (all || (type == flag))
		{
			if ((!all && nick) || (all && nick))
			{
				if (!wild_match(nick, s->server))
					continue;
			}
			erase_dcc_info(i, 1, "%s", convert_output_format("$G %RDCC%n closing dcc $0 to $1", "%s %s", dcc_types[flag]->name, s->server));
			close_socketread(i);
		}
	}
	return;
}

void close_all_dcc(void)
{
int i;
SocketList *s;
unsigned long flag;
	for (i = 0; i < get_max_fd() + 1; i++)
	{
		if (!check_dcc_socket(i))
			continue;
		s = get_socket(i);
		flag = s->flags & DCC_TYPES;
		erase_dcc_info(i, 1, "%s", convert_output_format("$G %RDCC%n closing dcc $0 to $1", "%s %s", dcc_types[flag]->name, s->server));
		close_socketread(i);
	}
	return;
}


static char *last_notify = NULL;

void cancel_dcc_auto(int i, unsigned long flags, SocketList *s)
{
	if (!last_notify || strcmp(s->server,last_notify))
	{
		send_to_server("NOTICE %s :DCC %s Auto Closed", s->server, dcc_types[flags]->name);
		malloc_strcpy(&last_notify, s->server);
	}
	erase_dcc_info(i, 1, "%s", convert_output_format("$G %RDCC%n Auto-closing idle dcc $0 to $1", "%s %s", dcc_types[flags]->name, s->server));
	close_socketread(i);
}

void dcc_check_idle(void)
{
int i;
time_t dcc_idle_time = get_int_var(_CDCC_CLOSE_IDLE_SENDS_TIME_VAR);
#ifdef WANT_CDCC
int minidlecheck = get_int_var(_CDCC_MINSPEED_TIME_VAR);
#endif
SocketList *s = NULL;
DCC_int *n = NULL;
unsigned long flags;

	if (pending_dcc)
	{
		DCC_List *s1 = pending_dcc, *last = NULL;
		while (s1)
		{
			s = &s1->sock;
			n = (DCC_int *)s->info;
			if (now >= s->time)
			{
				if (last)
					last->next = s1->next;
				else
					pending_dcc = s1->next;
				new_free(&s->server);
				new_free(&n->user);
				new_free(&n->filename);
				new_free(&n->userhost);
				new_free(&n->encrypt);
				close_socketread(s->is_read);
				new_free(&n);
				new_free(&s1);
				break;
			}
			last = s1;
			s1 = s1->next;
		}
	}
	for (i = 0; i < get_max_fd()+1; i++)
	{
		time_t	client_idle, 
			this_idle_time;

		if (!check_dcc_socket(i))
			continue;
		s = get_socket(i);
		n = (DCC_int *)s->info;

		flags = s->flags & DCC_TYPES;
		client_idle = now - n->lasttime.tv_sec;
		switch (flags)
		{
			case DCC_FILEOFFER:
			case DCC_FILEREAD:
			case DCC_REFILEOFFER:
			case DCC_REFILEREAD:
				this_idle_time = dcc_idle_time * 3;
				break;
			default:
				this_idle_time = dcc_timeout;
				break;
		}
		if (!(s->flags & DCC_ACTIVE))
		{
			if ((client_idle > this_idle_time))
				cancel_dcc_auto(i, flags, s);
			continue;
		}
#ifdef WANT_CDCC
		switch (flags)
		{
			case DCC_FILEOFFER:
			case DCC_REFILEOFFER:
				if (cdcc_minspeed && minidlecheck && (((now - n->starttime.tv_sec) % minidlecheck) == 0))
				{
					unsigned long sent = n->bytes_sent / 1024;
					double this_speed = 0.0;
					char lame_ultrix1[20];
					char lame_ultrix[20];
					this_speed = (double)((double) sent / (double)(now- n->starttime.tv_sec));
					if (this_speed < (float)cdcc_minspeed)
					{
						sprintf(lame_ultrix, "%2.4g", (double)(sent / (now - n->starttime.tv_sec)));
						sprintf(lame_ultrix1,"%2.4g", (double)cdcc_minspeed);
						if (!last_notify || strcmp(s->server,last_notify))
						{
							send_to_server("NOTICE %s :CDCC Slow dcc %s Auto Closed. Require %sKB/s got %sKB/s", s->server, dcc_types[flags]->name, lame_ultrix1, lame_ultrix);
							malloc_strcpy(&last_notify, s->server);
						}
						erase_dcc_info(i, 1, "%s", convert_output_format("$G %RDCC%n Auto-closing Slow dcc $0 to $1 require $2KB/s got $3KB/s", "%s %s %s %s", dcc_types[flags]->name, s->server, lame_ultrix1, lame_ultrix));
						close_socketread(i);
					}
					break;
				}
				if (client_idle > this_idle_time)
					cancel_dcc_auto(i, flags, s);
			default:
				break;
		}
#endif
	}
#ifdef WANT_CDCC
	cdcc_timer_offer();
#endif
	return;
}

void dcc_close(char *command, char *args)
{
char *type;
char *file;
char *nick;
int any_type = 0;
int any_user = 0;
int count = 0;
SocketList *s;
DCC_List *s1;
int i;
int num = -1;

	type = next_arg(args, &args);
	nick = next_arg(args, &args);
	file = next_arg(args, &args);
	if (type)
	{
		if (!my_stricmp(type, "-all") || !strcmp(type, "*"))
			any_type = 1;
		else if (*type == '#' && (num = my_atol(type+1)))
			any_type = 1;
	}
	if (nick && (!my_stricmp(nick, "-all") || !strcmp(nick, "*")))
		any_user = 1;	
	if (!type || (!nick && (!any_type || num == -1)))
	{
		bitchsay("Specify a nick and a type to close or a number");
		return;
	}
	if (!any_type)
	{
		for (i = 0; dcc_types[i]->name; i++)
			if (!my_stricmp(dcc_types[i]->name, type))
				break;
		if (!dcc_types[i]->name)
		{
			bitchsay("Unknown dcc type for close");
			return;
		}
	} else 
		i = -1;
	if (any_user)
		nick = NULL;
	while ((s = find_dcc(nick, file, NULL, i, 0, -1, num)))
	{
		DCC_int *n;
		n = (DCC_int *)s->info;
		count++;
                if (do_hook(DCC_LOST_LIST,"%s %s %s USER ABORTED CONNECTION",
			s->server, 
			dcc_types[s->flags & DCC_TYPES]->name,
                        n->filename ? n->filename : "<any>"))
		    say("DCC %s:%s to %s closed", 
			dcc_types[s->flags & DCC_TYPES]->name,
			file ? file : "<any>", 
			s->server);
		erase_dcc_info(s->is_read, 1, NULL);
		close_socketread(s->is_read);
	}
	while ((s1 = find_dcc_pending(nick, file, NULL, i, 1, num)))
	{
		DCC_int *n;
		s = &s1->sock;
		n = (DCC_int *)s->info;
		count++;
                if (do_hook(DCC_LOST_LIST,"%s %s %s USER ABORTED CONNECTION",
			s->server, 
			dcc_types[s->flags & DCC_TYPES]->name,
                        n->filename ? n->filename : "<any>"))
		    say("DCC %s:%s to %s closed", 
			dcc_types[s->flags & DCC_TYPES]->name,
			file ? file : "<any>", 
			s->server);

		send_reject_ctcp(s->server, 
			(((s->flags & DCC_TYPES) == DCC_FILEOFFER) ? "GET" :
			 ((s->flags & DCC_TYPES) == DCC_FILEREAD)  ? "SEND" : 
			   dcc_types[s->flags & DCC_TYPES]->name),
			n->filename);

		new_free(&s->server);
		new_free(&n->user);
		new_free(&n->filename);
		new_free(&n->userhost);
		new_free(&n->encrypt);
		close_socketread(s->is_read);
		new_free(&n);
		new_free(&s1);
	}
	if (!count)
		put_it("%s", convert_output_format("$G %RDCC%n No DCC $0:$1 to $2 found", "%s %s %s",
			(i == -1 ? "<any>" : type), 
			(file ? file : "<any>"), 
			(num != -1) ? ltoa(num): nick ? nick : "(null)"));
	update_transfer();
	return;
}

void dcc_closeall(char *command, char *args)
{
	close_all_dcc();
}

extern int doing_notice;

#ifdef MIRC_BROKEN_DCC_RESUME

void dcc_getfile_resume_demanded(char *nick, char *filename, char *port, char *offset)
{
SocketList *s;
DCC_int *n;
int old_dp, old_dn, old_dc;

	if (filename && !strcmp(filename, "file.ext"))
		filename = NULL;
	if (!(s = find_dcc(nick, filename, port, DCC_FILEOFFER, 1, -1, -1)))
	{
		put_it("%s", convert_output_format("$G %RDCC%n warning in dcc_getfile_resume_demanded", NULL));
		return;
	}
	n = (DCC_int *)s->info;
	if (!offset)
		return;
	old_dp = doing_privmsg;
	old_dn = doing_notice;
	old_dc = in_ctcp_flag;

	n->bytes_read = n->transfer_orders.byteoffset = my_atol(offset);
/*	n->bytes_read = 0L;*/

	doing_privmsg = doing_notice = in_ctcp_flag = 0;
	send_ctcp(CTCP_PRIVMSG, nick, CTCP_DCC, "ACCEPT %s %s %s", filename, port, offset);
	doing_privmsg = old_dp;
	doing_notice = old_dn;
	in_ctcp_flag = old_dc;
}

void dcc_getfile_resume_start (char *nick, char *description, char *address, char *port)
{
SocketList *s;
DCC_int *n;
char *tmp = NULL;
char *fullname = NULL;
struct stat sb;

	/* resume command has been sent and accepted. */
	if (description && !strcmp(description, "file.ext"))
		description = NULL;
	if ((s = find_dcc(nick, description, NULL, DCC_FILEREAD, 1, 1, -1)))
	{
		put_it("%s", convert_output_format("$G %RDCC%n warning in dcc_getfile_resume_start", NULL));
		return;
	}
	if (!(n = dcc_create(nick, description, NULL, 0, port?atol(port):0, DCC_FILEREAD, DCC_TWOCLIENTS|DCC_OFFER, start_dcc_get)))
		return;

	if (get_string_var(DCC_DLDIR_VAR))
		malloc_sprintf(&tmp, "%s/%s", get_string_var(DCC_DLDIR_VAR), n->filename);
	else
		tmp = m_strdup(n->filename);
	if (!(fullname = expand_twiddle(tmp)))
		malloc_strcpy(&fullname, tmp);

	if (
		!(n->file = open(fullname, O_WRONLY | O_APPEND | O_BINARY, 0644)) ||
		(fstat(n->file, &sb) != 0) || 
                (sb.st_size >= n->filesize)
		)
	{
		int snum;
		if ((s = find_dcc(nick, description, NULL, DCC_FILEREAD, 0, 1, -1))) 
			n = (DCC_int *)s->info;
		if (s)
		{
			snum = s->is_read;
			erase_dcc_info(snum, 1, "%s", convert_output_format("$G %RDCC%n Unable to open $0: $1-", "%s %s", n->filename, errno?strerror(errno):"incoming file smaller than existing"));
			close_socketread(snum);
		}
	} else
		put_it("DCC RESUME starting at %lu", sb.st_size);
	new_free(&fullname);
	new_free(&tmp);
}

void dcc_resume(char *command, char *args)
{
char		*user, *nick;
char		*filename = NULL;
char		*fullname = NULL;
char		*tmp = NULL;
char		*passwd = NULL;
char		*port = NULL;
struct stat	sb;
int		old_dp, old_dn, old_dc;
int		blocksize = 0;

	user = get_dcc_args(&args, &passwd, &port, &blocksize);
	if (!user)
	{
		put_it("%s", convert_output_format("$G %RDCC%n You must supply a nickname for DCC get", NULL, NULL));
		return;
	}
	if (!blocksize || blocksize > MAX_DCC_BLOCK_SIZE)
		blocksize = get_int_var(DCC_BLOCK_SIZE_VAR);

	if (!user)
	{
		put_it("%s", convert_output_format("$G %RDCC%n You must supply a nickname for DCC RESUME", NULL));
		return;
	}
	if (args && *args)
		filename = args;

	tmp = NULL;
	
	while ((nick = next_in_comma_list(user, &user)))
	{
		SocketList *s;
		DCC_int *n;
		DCC_List *s1 = NULL;		
		if (!nick || !*nick)
			break;
		if ((s = find_dcc(nick, filename, NULL, DCC_FILEREAD, 1, 1, -1)))
		{
			put_it("%s", convert_output_format("$G %RDCC%n DCC send already active. Unable to RESUME", NULL));
			continue;
		}
		for (s1 = pending_dcc; s1; s1 = s1->next)
		{
			if (my_stricmp(s1->nick, nick))
				continue;
			if ( ((s1->sock.flags & DCC_TYPES) != DCC_FILEREAD) ||
				(s1->sock.flags & DCC_ACTIVE))
				continue;
			break;
		}
		if (!s1)
			continue;
		s = &s1->sock;
		n = (DCC_int *)s->info;		

		if (get_string_var(DCC_DLDIR_VAR))
			malloc_sprintf(&tmp, "%s/%s", get_string_var(DCC_DLDIR_VAR), n->filename);
		else
			tmp = m_strdup(n->filename);

		if (!(fullname = expand_twiddle(tmp)))
			malloc_strcpy(&fullname, tmp);
		/*
		 * This has to be done by hand, we cant use send_ctcp,
		 * because this violates the protocol, and send_ctcp checks
		 * for that.  Ugh.
		 */

		if (stat(fullname, &sb) == -1)
		{
			/* File doesnt exist.  Sheesh. */
			put_it("%s", convert_output_format("$G %RDCC%n Cannot use DCC RESUME if the file doesn't exist [$0|$1-]", "%s %s", fullname, strerror(errno)));
			continue;
		}

		if (passwd)
			n->encrypt = m_strdup(passwd);

		n->bytes_sent = 0L;
		n->blocksize = blocksize;
		n->transfer_orders.byteoffset = sb.st_size;

		old_dp = doing_privmsg;	old_dn = doing_notice; old_dc = in_ctcp_flag;
		/* Just in case we have to fool the protocol enforcement. */
		doing_privmsg = doing_notice = in_ctcp_flag = 0;
		send_ctcp(CTCP_PRIVMSG, nick, CTCP_DCC, "RESUME %s %d %ld", n->filename, ntohs(n->remport), sb.st_size);
		doing_privmsg = old_dp; doing_notice = old_dn; in_ctcp_flag = old_dc;

	}
	new_free(&tmp);
	new_free(&fullname);
	/* Then we just sit back and wait for the reply. */
}
#endif

void dcc_help1(char *command, char *args)
{
char *comm;
int i, c;
char buffer[BIG_BUFFER_SIZE+1];
DCC_dllcommands *dcc_comm = NULL;
	if (args && *args)
	{
		comm = next_arg(args, &args);
		upper(comm);
		if ((dcc_comm = (DCC_dllcommands *)find_in_list((List **)&dcc_dllcommands, comm, 0)))
		{
			put_it("%s", convert_output_format("$G Usage: %W/%R$0%n $1 %K-%n $2-", "DCC %s %s", dcc_comm->name, dcc_comm->help?dcc_comm->help:"No help availble yet"));
			return;
		}
		for (i = 0; dcc_commands[i].name != NULL; i++)
		{
			if (!strncmp(comm, dcc_commands[i].name, strlen(comm)))
			{
				put_it("%s", convert_output_format("$G Usage: %W/%R$0%n $1 %K-%n $2-", "DCC %s %s", dcc_commands[i].name, dcc_commands[i].help?dcc_commands[i].help:"No help availble yet"));
				return;
			}
		}
	}
	put_it("%s", convert_output_format("$G %RDCC%n help -", NULL, NULL));
	*buffer = 0;
	c = 0;
	for (dcc_comm = dcc_dllcommands; dcc_comm; dcc_comm = dcc_comm->next)
	{
		strcat(buffer, dcc_comm->name);
		strcat(buffer, space);
		if (++c == 5)
		{
			put_it("%s", convert_output_format("$G $[13]0 $[13]1 $[13]2 $[13]3 $[13]4", "%s", buffer));
			*buffer = 0;
			c = 0;
		}
	}
	if (c)
		put_it("%s", convert_output_format("$G $[13]0 $[13]1 $[13]2 $[13]3 $[13]4", "%s", buffer));
	*buffer = 0;
	c = 0;
	for (i = 0; dcc_commands[i].name; i++)
	{
		strcat(buffer, dcc_commands[i].name);
		strcat(buffer, space);
		if (++c == 5)
		{
			put_it("%s", convert_output_format("$G $[13]0 $[13]1 $[13]2 $[13]3 $[13]4", "%s", buffer));
			*buffer = 0;
			c = 0;
		}
	}
	if (c)
		put_it("%s", convert_output_format("$G $[13]0 $[13]1 $[13]2 $[13]3 $[13]4", "%s", buffer));
	userage("dcc help", "[command] to get help on specific commands");
}

void read_ftp_file(int snum)
{
int len = 0;
char *buf;
SocketList *s;
DCC_int *n;
int err;	
	if ((ioctl(snum, FIONREAD, &len) == -1))
	{
		put_it("%s", convert_output_format("$G %gFTP%n file error [$0-]", "%s", strerror(errno)));
		erase_dcc_info(snum, 0, NULL);
		close_socketread(snum);
		return;
	}
	s = get_socket(snum);
	n = (DCC_int *)s->info;
	buf = alloca(len+1);
	err = read(snum, buf, len);
	switch(err)
	{
		case -1:
			erase_dcc_info(snum, 0, NULL);
			close_socketread(snum);
			break;
		case 0:
			close_dcc_file(snum);
			break;
		default:
			write(n->file, buf, len);
			n->bytes_read += len;
			n->packets++;
			get_time(&n->lasttime);
	}
}

int open_listen_port(int s)
{
struct sockaddr_in data_addr = { 0 };
int len = sizeof(struct sockaddr_in), data = -1;
int on = 1;
char *a, *p;

	if (getsockname(s, (struct sockaddr *)&data_addr, &len) < 0)
		return -1;

	data_addr.sin_port = 0;
	if ((data = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return -1;
	if ((setsockopt(data, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))) < 0)
		return -1;
	if ((bind(data, (struct sockaddr *)&data_addr, sizeof(data_addr))) < 0)
		return -1;
	len = sizeof(struct sockaddr_in);
	getsockname(data, (struct sockaddr *)&data_addr, &len);

	a = (char *)&data_addr.sin_addr;
	p = (char *)&data_addr.sin_port;
#define UC(b) (((int)b)&0xff)
	dcc_printf(s, "PORT %d,%d,%d,%d,%d,%d\n", UC(a[0]), UC(a[1]), UC(a[2]), UC(a[3]),UC(p[0]), UC(p[1]));
#undef UC
	if ((listen(data, 1)) < 0)
		return -1;
	return data;
}


int handle_ftp(int comm, int snum, SocketList *s, char *buf)
{
int err = 0;
DCC_int *n;
	n = (DCC_int *)s->info;
	get_time(&n->lasttime);
	switch(comm)
	{
		case 220:
			if (n->userhost)
				err = dcc_printf(snum, "user %s\r\n", n->userhost);
			new_free(&n->userhost);
			break;
		case 226:
			s->flags &= ~DCC_WAIT;
			return 0;
		case 331:
			if (n->encrypt)
			{
				err = dcc_printf(snum, "pass %s\r\n", n->encrypt);
				if (err != -1)
					err = dcc_printf(snum, "type I\r\n");
			}
			new_free(&n->encrypt); new_free(&n->othername);
			break;
		case 230:
		{
			char *dir = NULL;
			if (n->othername)
			{
				err = dcc_printf(snum, "cwd %s\r\n", n->othername);
				if (do_hook(FTP_LIST, "%d Changing directory to %s", comm, n->othername))
					put_it("%s", convert_output_format("%gFTP%n Changing directory to $0-", "%s", n->othername));
			}
			new_free(&n->encrypt);
			new_free(&n->userhost);
			new_free(&n->othername);
			new_free(&dir);
			break;
		}
		case 421:
			erase_dcc_info(snum, 0, NULL);
			close_socketread(snum);
			break;
		default:
			break;
	}
	if (do_hook(FTP_LIST, "%d %s", comm, buf))
		put_it("%s", convert_output_format("%gFTP%n $0-", "%s", buf));
	return err;
}

void start_ftp(int snum)
{
SocketList *s;
DCC_int *n;
char *buf = NULL;
int i = 0;

	s = get_socket(snum);
	n = (DCC_int *)s->info;
	if (s->flags & DCC_WAIT)
	{
		struct	sockaddr_in	remaddr;
		int			rl = sizeof(remaddr);
                                
		/* maybe we should login here. */
		if (getpeername(snum, (struct sockaddr *) &remaddr, &rl) != -1)
		{
			s->flags = DCC_ACTIVE|DCC_FTPOPEN;
			return;
		}
		erase_dcc_info(snum, 0, "DCC ftp lost");
		close_socketread(snum);
		return;
	}
	buf = alloca(n->blocksize+1);
	*buf = 0;
	switch(dgets(buf, snum, 1, n->blocksize, NULL))
	{
		case -1:
			i = -1;
			break;
		case 0:
			break;
		default:
		{
			int comm = 0;
			char *p;
			if ((p = strrchr(buf, '\r')))
				*p = 0;
			if ((p = strrchr(buf, '\n')))
				*p = 0;
			if ((comm = my_atol(buf)))
				next_arg(buf, &buf);
			i = handle_ftp(comm, snum, s, buf);
			break;
		}		
	}
	if (i == -1)
	{
		erase_dcc_info(snum, 0, NULL);
		close_socketread(snum);
	}
}


void open_ftpget(SocketList *s, char *args)
{
SocketList *sock;
DCC_int *new;
struct sockaddr_in data_addr = { 0 };
int len = sizeof(struct sockaddr_in);
char tmp[BIG_BUFFER_SIZE+1];
int data = -1, s1 = -1;
char *p, *bufptr;
off_t filesize = 0;
char *filename = NULL;

	if ((data = open_listen_port(s->is_read)) == -1)
		return;

	dcc_printf(s->is_read, "retr %s\n", args);

	memset(tmp, 0, sizeof(tmp));
	bufptr = tmp;
	while (1)
	{
		if (dgets(bufptr, s->is_read, 1, BIG_BUFFER_SIZE, NULL) == -1 && dgets_errno > 0)
			goto error_ftp;
		if (*bufptr == '5')
			goto error_ftp;
		else if (strstr(tmp, "BINARY mode data connection"))
		{
			int i = 0;
			char *q = tmp;
			for (i = 0; i < 9; i++)
				p = next_arg(q, &q);
			if (p)
			{
				p++;
				filesize = my_atol(p);
			}
			break;
		}
	}

	len = sizeof(struct sockaddr_in);
	if ((s1 = my_accept(data, (struct sockaddr *) &data_addr, &len)) < 0)
		return;
	close(data);

#if defined(WINNT) || defined(__EMX__)
	if ((p = strrchr(args, '/')) || (p = strrchr(args, '\\')))
#else
	if ((p = strrchr(args, '/')))
#endif
		filename = ++p;
	else
		filename = args;

	add_socketread(s1, 0,  DCC_FTPGET|DCC_ACTIVE, s->server, read_ftp_file, NULL);
	sock = get_socket(s1);
	new = new_malloc(sizeof(DCC_int));
	{
		char *t, *expand;
		expand = m_sprintf("%s/%s", get_string_var(DCC_DLDIR_VAR), filename);
		t = expand_twiddle(expand);
		new->file = open(t?t:expand?expand:filename, O_WRONLY|O_CREAT|O_TRUNC | O_BINARY, 0644);
		new_free(&t); new_free(&expand);
	}
	new->struct_type = DCC_STRUCT_TYPE;
	new->filesize = filesize;
	new->filename = m_strdup(filename);
	get_time(&new->starttime);
	get_time(&new->lasttime);
	set_socketinfo(s1, new);
	s->flags |= DCC_WAIT;
	if (do_hook(FTP_LIST, "%s %s", "FTP Attempting to get", filename))
		put_it("%s", convert_output_format("$G %gFTP%n Attempting to get $0", "%s", filename));
	return;
error_ftp:
	close(data);
	chop(tmp, 2);
	put_it("%s", convert_output_format("$G %gFTP%n $0-", "%s", tmp));
	return;

}

int dcc_ftpcommand (char *host, char *args)
{
SocketList *s;
	s = find_dcc(host, "ftpopen", NULL, DCC_FTPOPEN, 0, 1, -1);
	if (s)
	{
		char *command = next_arg(args, &args);
		char *t_host;
		if (s->flags & DCC_WAIT)
		{
			if (do_hook(FTP_LIST, "%s", "FTP connection is busy"))
				put_it("%s", convert_output_format("$G %gFTP%n connection is busy.", NULL, NULL));
			return 1;
		}
		
		if (command && *command)
		{
			if (!my_strnicmp(command, "ls",2) || !my_strnicmp(command, "dir",3))
			{
				int sock, s1;
				DCC_int *new;
				struct sockaddr_in data_addr = { 0 };
				int len = sizeof(struct sockaddr_in);
				char tmp[BIG_BUFFER_SIZE+1], *bufptr;
				if ((sock = open_listen_port(s->is_read)) == -1)
					return -1;
				dcc_printf(s->is_read, "list -a%s%s\r\n", (args && *args) ?space:empty_string, (args && *args) ? args : empty_string);
				memset(tmp, 0, sizeof(tmp));
				bufptr = tmp;
				while (1)
				{
					if (dgets(bufptr, s->is_read, 1, BIG_BUFFER_SIZE, NULL) == -1 && dgets_errno > 0)
						goto error_port;
					if (*bufptr == '5')
						goto error_port;
					else if (strstr(tmp, "150 Opening BINARY mode data connection"))
						break;
				}
				
				len = sizeof(struct sockaddr_in);
				set_blocking(sock);
				alarm(10);
				s1 = accept(sock, (struct sockaddr *) &data_addr, &len);
				alarm(0);
				close(sock);
				if (s1 != -1)
				{
					add_socketread(s1, 0,  DCC_FTPCOMMAND|DCC_ACTIVE, s->server, start_ftp, NULL);
					new = new_malloc(sizeof(DCC_int));
					new->struct_type = DCC_STRUCT_TYPE;
					get_time(&new->starttime);
					get_time(&new->lasttime);
					new->blocksize = BIG_BUFFER_SIZE;
					set_socketinfo(s1, new);
				} else
					bitchsay("FTP data connection failed.");
			}
			else if (!my_strnicmp(command, "more", 3))
				dcc_printf(s->is_read, "stat %s\n", (args && *args) ? "cwd" : "pwd", (args && *args) ?args:empty_string);
			else if (!my_strnicmp(command, "cd", 2))
				dcc_printf(s->is_read, "%s%s%s\n", (args && *args) ? "cwd" : "pwd", (args && *args) ? space:empty_string, (args && *args) ?args:empty_string);
			else if (!my_strnicmp(command, "get",3) && args && *args)
				open_ftpget(s, args);
			else
				dcc_printf(s->is_read, "%s%s%s\n", command, (args && *args) ? space:empty_string, (args && *args) ? args:empty_string);

			t_host = alloca(strlen(host)+4);
			strcpy(t_host, "-"); 
			strcat(t_host, host);
			addtabkey(t_host, "msg", 0);
		}
	}
	else
	{
		if (do_hook(FTP_LIST, "%s", "FTP is not connected"))
			put_it("%s", convert_output_format("$G %gFTP%n is not connected.", NULL, NULL));
		return 0;
	}
	return 1;
error_port:
	return -1;
}


char *parse_ncftp(char *buffer, char **hostname)
{
char *dir = NULL, *p;

/*bitchx,bitchx.com,panasync,,,/home/panasync,I,,34ce4360,1,*/
	if (!(p = strchr(buffer, ',')))
		return NULL;
	*p++ = 0; *hostname = p;
	if (!(p = strchr(p, ',')))
		return NULL;
	*p++ = 0; 
	if (!(p = strchr(p, ',')))
		return NULL;
	*p++ = 0; 
	if (!(p = strchr(p, ',')))
		return NULL;
	*p++ = 0; 
	if (!(p = strchr(p, ',')))
		return NULL;
	*p++ = 0; 
	
	dir = p;
	if (!(p = strchr(p, ',')))
		return NULL;
	*p = 0;
	return (dir && *dir) ? dir : NULL;
}

char *read_ncftp_config(char **hostname)
{
char *tmp, *file = NULL;
char *buffer;
char *dir = NULL;
struct stat sb;
FILE *f;
	tmp = m_sprintf("~/.ncftp/bookmarks");
	file = expand_twiddle(tmp);
	if ((stat(file, &sb) != -1) && (f = fopen(file, "r")))
	{
		char *hst = NULL;
		buffer = alloca(sb.st_size+1);
		freadln(f, buffer);
		while(!feof(f))
		{
			if ((freadln(f, buffer)))
			{
				if (*buffer == '#')
					continue;
				dir = parse_ncftp(buffer, &hst);
				if (hst)
				{ 
					if (!my_stricmp(hst, *hostname))
						break;
					else if (!my_stricmp(*hostname, buffer))
					{
						malloc_strcpy(hostname, hst);
						break;
					}
				}
				dir = NULL;
			}
		}		
		fclose(f);
	}
	new_free(&tmp); new_free(&file);
	return dir ? m_strdup(dir) : NULL;
}

void dcc_ftpopen(char *command, char *args)
{
char	u[] = "anonymous";
char	p[] = "- bxuser@";
char	*host = NULL, 
	*user = NULL, 
	*pass = NULL,
	*dir = NULL,
	*t_host = NULL;
unsigned short	port = 21;
int	blocksize = 2048;
int	snum;
SocketList *s;

	if (!(t_host = next_arg(args, &args)))
	{
		put_it ("%s /dcc ftp hostname user passwd [-p port] [-b blocksize]", convert_output_format("$G %RDCC%n", NULL, NULL));
		return;
	}
	host = m_strdup(t_host);
	while (args && *args)
	{
		char *t;
		if (!my_strnicmp(args, "-p", 2))
		{
			next_arg(args, &args);
			port = my_atol(next_arg(args, &args));
			continue;
		}
		if (!my_strnicmp(args, "-b", 2))
		{
			next_arg(args, &args);
			blocksize = my_atol(next_arg(args, &args));
			continue;
		}
		else if ((t = next_arg(args, &args)))
		{
			if (!user)
				user = t;
			else
				malloc_strcpy(&pass, t);
		}
	}
	if (!user)
		user = u;
	if (!pass)
		pass = m_sprintf("%s%s", p, hostname);

	dir = read_ncftp_config(&host);

	if ((s = find_dcc(host, "ftpopen", NULL, DCC_FTPOPEN, 0, -1, -1)))
	{
		put_it("%s", convert_output_format("$G %GFTP%n A previous DCC FTP to $0 exists", "%s", host));
		new_free(&host);
		return;
	}
		
#if defined(WINNT) || defined(__EMX__)
	if ((snum = connect_by_number(host, &port, SERVICE_CLIENT, PROTOCOL_TCP, 0)) < 0)
#else
	if ((snum = connect_by_number(host, &port, SERVICE_CLIENT, PROTOCOL_TCP, 1)) < 0)
#endif
	{
		put_it("%s", convert_output_format("$G %gFTP%n command failed connect", NULL, NULL));
		new_free(&host);
		return;
	}
	else
	{
		DCC_int *new;
		add_socketread(snum, port, DCC_WAIT|DCC_FTPOPEN, host, start_ftp, NULL);
		new = new_malloc(sizeof(DCC_int));
		new->struct_type = DCC_STRUCT_TYPE;
		new->remport = htons(port);
		new->blocksize = blocksize;
		get_time(&new->starttime);
		get_time(&new->lasttime);
		malloc_strcpy(&new->userhost, user);
		new->encrypt = pass; /* these are not a mem leak */
		new->user = host; /* free'd in handle_ftp() */
		malloc_strcpy(&new->filename, "ftpopen");
		new->othername = dir;

		set_socketinfo(snum, new);

	}
	t_host = alloca(strlen(host)+4);
	strcpy(t_host, "-"); 
	strcat(t_host, host);
	addtabkey(t_host, "msg", 0);
	return;
}


void dcc_rename(char *command, char *args)
{
DCC_List *tmp;
DCC_int	*n;
char	*nick, 
	*filename;

	nick = next_arg(args, &args);
	filename = new_next_arg(args, &args);
	if (!nick || !filename || !*filename)
	{
		put_it("%s", convert_output_format("$G /DCC rename nick filename", NULL, NULL));
		return;
	}
	for (tmp = pending_dcc; tmp; tmp = tmp->next)
	{
		if (my_stricmp(tmp->nick, nick))
			continue;
		n = (DCC_int *)tmp->sock.info;
		malloc_strcpy(&n->filename, filename);
		break;
	}
}

int check_dcc_init(char *type, char *nick, char *uhost, char *description, char *size, char *extra, unsigned long TempLong, unsigned int TempInt)
{
int i;
	for (i = 0; dcc_types[i]->name; i++)
	{
		if (!my_stricmp(type, dcc_types[i]->name))
			break;
	}
	if (!dcc_types[i]->name)
		return 0;
	if (dcc_types[i]->init_func)
		return (*dcc_types[i]->init_func)(type, nick, uhost, description, size, extra, TempLong, TempInt);
	return 0;
}

int BX_add_dcc_bind(char *name, char *module, void *init_func, void *open_func, void *input, void *output, void *close_func)
{
int i;
	for (i = 0; dcc_types[i]->name; i++)
	{
		if (!my_stricmp(dcc_types[i]->name, name))
			break;
	}
	if (i >= 0xfe) return 0;
	if (!dcc_types[i])
	{
		RESIZE(dcc_types, struct _dcc_types_ *, i + 2);
		dcc_types[i] = new_malloc(sizeof(struct _dcc_types_));
	}
	if (!dcc_types[i]->name)
		malloc_strcpy(&dcc_types[i]->name, name);
	malloc_strcpy(&dcc_types[i]->module, module);
	dcc_types[i]->init_func = init_func;
	dcc_types[i]->open_func = open_func;
	dcc_types[i]->input = input;
	dcc_types[i]->output = output;
	dcc_types[i]->close_func = close_func;
	dcc_types[i]->type = i;	
	return i;
}

int BX_remove_dcc_bind(char *name, int type)
{
int i = type & DCC_TYPES;
	if (!dcc_types[i]->module)
		return 0;
	dcc_types[i]->init_func = NULL;
	dcc_types[i]->open_func = NULL;
	dcc_types[i]->input = NULL;
	dcc_types[i]->output = NULL;
	dcc_types[i]->close_func = NULL;
	new_free(&dcc_types[i]->module);
	if (i > DCC_FTPSEND)
	{
		new_free(&dcc_types[i]->name);
		new_free(&dcc_types[i]);
/*		RESIZE(dcc_types, struct _dcc_types *, i - 1);*/
	}
	return 1;
}

int BX_remove_all_dcc_binds(char *name)
{
int ret = 0;
int i, j;
	/* scan to end of list */
	for (i = 0; dcc_types[i]->name; i++);
	i--;
	for (j = i; j > 0; j--)
		ret += remove_dcc_bind(dcc_types[j]->name, dcc_types[j]->type);
	return ret;
}

void init_dcc_table(void)
{
int i;
	for (i = 0; _dcc_types[i].name; i++);
	RESIZE(dcc_types, struct _dcc_types_ *, i + 1);
	for (i = 0; _dcc_types[i].name; i++)
	{
		dcc_types[i] = new_malloc(sizeof(struct _dcc_types_));
		dcc_types[i]->name = m_strdup(_dcc_types[i].name);
		dcc_types[i]->type = _dcc_types[i].type;
	}
	dcc_types[i] = new_malloc(sizeof(struct _dcc_types_));
}

char *get_dcc_info(SocketList *s, DCC_int *n, int i)
{
char local_type[80];
	*local_type = 0;
	if (s->flags & DCC_TDCC)
		strcpy(local_type, "T");
	strcat(local_type, dcc_types[s->flags & DCC_TYPES]->name);
	return m_sprintf("%s %s %s %lu %lu %lu %lu %s #%d %d", 
			local_type, s->server, 
			s->flags & DCC_OFFER ? "Offer": 
				s->flags & DCC_WAIT ? "Wait": 
				s->flags & DCC_ACTIVE? "Active":"Unknown", 
			n->starttime.tv_sec, n->transfer_orders.byteoffset,
			n->bytes_sent, n->bytes_read, n->filename, i, 
			n->server);
}

void dcc_raw_transmit (char *user, char *text, char *type)
{
	return;
}

#if 0
/*
 * Usage: $listen(<port>)
 */
char	*dcc_raw_listen (unsigned short port)
{
	DCC_list	*Client;
	char		*PortName;

	set_display_target(NULL, LOG_DCC);

	if (port && port < 1025)
	{
		say("May not bind to a privileged port");
		set_display_target(NULL, LOG_CURRENT);
		return NULL;
	}
	PortName = ltoa(port);
	Client = dcc_searchlist("raw_listen", PortName, 
					DCC_RAW_LISTEN, 1, NULL, -1);

	if (Client->flags & DCC_ACTIVE)
	{
		say("A previous DCC RAW_LISTEN on %s exists", PortName);
		set_display_target(NULL, LOG_CURRENT);
		return NULL;
	}

	if ((Client->read = connect_by_number(NULL, &port, SERVICE_SERVER, PROTOCOL_TCP)) < 0)
	{
		say("Couldnt establish listening socket: [%d] %s", Client->read, strerror(errno));
		Client->flags |= DCC_DELETE; 
		set_display_target(NULL, LOG_CURRENT);
		return NULL;
	}

	new_open(Client->read);
	get_time(&Client->starttime);
	Client->local_port = port;
	Client->flags |= DCC_ACTIVE;
	Client->user = m_strdup(ltoa(Client->local_port));
	set_display_target(NULL, LOG_CURRENT);

	return m_strdup(Client->user);
}


/*
 * Usage: $connect(<hostname> <portnum>)
 */
char	*dcc_raw_connect(char *host, u_short port)
{
	DCC_list	*Client;
	char	*PortName;
	struct	in_addr	address;
	struct	hostent	*hp;

	set_display_target(NULL, LOG_DCC);
	if ((address.s_addr = inet_addr(host)) == (unsigned) -1)
	{
		if (!(hp = gethostbyname(host)))
		{
			say("Unknown host: %s", host);
			set_display_target(NULL, LOG_CURRENT);
			return m_strdup(empty_string);
		}
		memmove(&address, hp->h_addr, sizeof(address));
	}
	Client = dcc_searchlist(host, ltoa(port), DCC_RAW, 1, NULL, -1);
	if (Client->flags & DCC_ACTIVE)
	{
		say("A previous DCC RAW to %s on %s exists", host, ltoa(port));
		set_display_target(NULL, LOG_CURRENT);
		return m_strdup(empty_string);
	}

	/* Sorry. The missing 'htons' call here broke $connect() */
	Client->remport = htons(port);
	memmove((char *)&Client->remote, (char *)&address, sizeof(address));
	Client->flags = DCC_THEIR_OFFER | DCC_RAW;
	if (!dcc_open(Client))
	{
		set_display_target(NULL, LOG_CURRENT);
		return m_strdup(empty_string);
	}

	PortName = ltoa(Client->read);
	Client->user = m_strdup(PortName);
	if (do_hook(DCC_RAW_LIST, "%s %s E %d", PortName, host, port))
            if (do_hook(DCC_CONNECT_LIST,"%s RAW %s %d",PortName, host, port))
		say("DCC RAW connection to %s on %s via %d established", host, PortName, port);

	set_display_target(NULL, LOG_CURRENT);
	return m_strdup(PortName);
}
#endif

char *dcc_raw_connect(char *host, u_short port)
{
	return m_strdup("-1");
}

char *dcc_raw_listen(int port)
{
#if 0
	SocketList	*Client;
	char		*PortName;

	set_display_target(NULL, LOG_DCC);

	if (port && port < 1025)
	{
		say("May not bind to a privileged port");
		set_display_target(NULL, LOG_CURRENT);
		return NULL;
	}

	PortName = ltoa(port);
	Client = find_dcc(PortName, NULL, "raw_listen", DCC_RAW_LISTEN, 1, 0, -1);	
	Client = dcc_searchlist("raw_listen", PortName, 
					DCC_RAW_LISTEN, 1, NULL, -1);

	if (Client->flags & DCC_ACTIVE)
	{
		say("A previous DCC RAW_LISTEN on %s exists", PortName);
		set_display_target(NULL, LOG_CURRENT);
		return NULL;
	}

	if ((Client->read = connect_by_number(NULL, &port, SERVICE_SERVER, PROTOCOL_TCP)) < 0)
	{
		say("Couldnt establish listening socket: [%d] %s", Client->read, strerror(errno));
		Client->flags |= DCC_DELETE; 
		set_display_target(NULL, LOG_CURRENT);
		return NULL;
	}

	new_open(Client->read);
	get_time(&Client->starttime);
	Client->local_port = port;
	Client->flags |= DCC_ACTIVE;
	Client->user = m_strdup(ltoa(Client->local_port));
	set_display_target(NULL, LOG_CURRENT);

	return m_strdup(Client->user);
#endif
	return m_strdup("-1");
}



int close_dcc_number(int number)
{
	if (!check_dcc_socket(number))
		return 0;
	erase_dcc_info(number, 1, NULL);
	close_socketread(number);
	return 1;
}
