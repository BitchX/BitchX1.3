
#define NAP_VERSION "0.07"

/*
 *
 * This is the  Napster handler.
 * based on code donated by Drago.
 * Written by Colten Edwards. (C) Nov 99
 */
#include "irc.h"
#include "struct.h"
#include "dcc.h"
#include "ircaux.h"
#include "misc.h"
#include "output.h"
#include "lastlog.h"
#include "screen.h"
#include "status.h"
#include "window.h"
#include "vars.h"
#include "input.h"
#include "module.h"
#include "hook.h"
#define INIT_MODULE
#include "modval.h"
#include "./napster.h"

#include <sys/ioctl.h>
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif

SocketList *naphub = NULL;
int nap_socket = -1;
int nap_data = -1;

#undef DEBUG

char napbuf[NAP_BUFFER_SIZE+1];
int nap_port = 8875;
int connection_speed = 2;
int nap_numeric = 0;
static int channel_count =  0;

static int nap_error = 0;

#define MAX_SPEED 10
                
char *_n_speed[] = 
	{"?", "14.4", "28.8", "33.6", "56.7", "64k ISDN", "128k ISDN", "Cable", "DSL", "T1", "T3 >", 0};
char *_speed_color[] = 
	{ "%K", "%g", "%y", "%p", "%r", "%n", "%B", "%R", "%P", "%Y", "%G", "" };
 	/* 0     1     2     3     4     5     6     7     8     9     10     11 */

_N_AUTH auth = { 0 };

char nap_version[] = NAP_VERSION;

ChannelStruct *nchannels = NULL;
char *nap_current_channel = NULL;

FileStruct *file_search = NULL;

FileStruct *file_browse = NULL;

NickStruct *nap_hotlist = NULL;


_NAP_COMMANDS nap_commands[] =
{
	{CMDS_UNKNOWN		, cmd_unknown },/* 1 */
	{CMDS_LOGIN		, cmd_login }, 	/* 2 */
	{CMDR_EMAILADDR		, cmd_email }, /* 3 */
	{CMDR_BASTARD		, cmd_bastard}, /* 4 */
	{CMDS_REGISTERINFO	, NULL }, /* 6 */
	{CMDS_CREATEUSER	, NULL }, /* 7 */
	{CMDR_CREATED		, cmd_registerinfo }, /* 8 */
	{CMDR_CREATEERROR	, cmd_alreadyregistered }, /* 9 */
	{CMDR_MSTAT		, NULL }, /* 15 */
	{CMDR_REQUESTUSERSPEED	, NULL }, /* 89 */
	{CMDR_SENDFILE		, NULL }, /* 95 */
	{CMDS_ADDFILE		, NULL }, /* 100 is for adding to shared. */
	{CMDR_GETQUEUE		, NULL }, /* 108 */
	{CMDR_MOTD		, NULL }, /* 109 */
	{CMDR_ANOTHERUSER	, NULL }, /* 148 */
	{CMDS_SEARCH		, NULL }, /* 200 */
	{CMDR_SEARCHRESULTS	, cmd_search }, /* 201 */
	{CMDR_SEARCHRESULTSEND	, cmd_endsearch }, /* 202 */
	{CMDS_REQUESTFILE	, NULL }, /* 203 */
	{CMDR_FILEREADY		, cmd_getfile }, /* 204 */
	{CMDS_SENDMSG		, cmd_msg }, /* 205 */
	{CMDR_GETERROR		, NULL }, /* 206 */

	{CMDS_ADDHOTLIST	, NULL }, /* 207 */
	{CMDS_ADDHOTLISTSEQ	, NULL }, /* 208 */
	{CMDR_HOTLISTONLINE	, cmd_hotlist }, /* 209 */
	{CMDR_USEROFFLINE	, cmd_offline }, /* 210 */

	{CMDS_BROWSE		, NULL },
	{CMDR_BROWSERESULT	, cmd_browse },
	{CMDR_BROWSEENDRESULT	, cmd_endbrowse },
	{CMDR_STATS		, cmd_stats }, /* 214 */

	{CMDR_RESUMESUCCESS	, cmd_resumerequest },
	{CMDR_RESUMEEND		, cmd_resumerequestend },
		
	{CMDR_HOTLISTSUCCESS	, cmd_hotlistsuccess }, /* 301 */
	{CMDR_HOTLISTERROR	, cmd_hotlisterror }, /* 302 */
	
	{CMDS_JOIN		, NULL }, /* 400 */
	{CMDS_PART		, NULL }, /* 401 */
	{CMDS_SEND		, NULL }, /* 402 */
	{CMDR_PUBLIC		, cmd_public }, /* 403 */
	{CMDR_ERRORMSG		, NULL },  /* 404 */
	{CMDR_JOIN		, cmd_joined }, /* 405 */
	{CMDR_JOINNEW		, cmd_names }, /* 406 */
	{CMDR_PARTED		, cmd_parted }, /* 407 */
	{CMDR_NAMES		, cmd_names }, /* 408 */
	{CMDR_ENDNAMES		, cmd_endnames }, /* 409 */
	{CMDS_TOPIC		, cmd_topic }, /* 410 */
	{CMDR_FILEINFOFIRE	, cmd_firewall_request }, /* 501 */

	{CMDS_REQUESTINFO	, NULL }, /* 600 */	
	{CMDS_FILESIZE		, cmd_getfileinfo }, /* 601 */

	{CMDS_WHOIS		, cmd_whois }, /* 603 */
	{CMDR_WHOIS		, cmd_whois }, /* 604 */
	{CMDR_WHOWAS		, cmd_whowas }, /* 605 */
	{CMDR_FILEREQUEST	, cmd_filerequest }, /* 607 */
	{CMDR_ACCEPTERROR	, cmd_accepterror }, /* 609 */
	{CMDR_BANLIST_IP	, cmd_banlist }, /* 616 */
	{CMDS_LISTCHANNELS	, NULL }, /* 617 */
	{CMDR_LISTCHANNELS	, cmd_channellist }, /* 618 */
	{CMDR_SENDLIMIT		, cmd_send_limit_msg }, /* 620 */
	{CMDR_MOTDS		, NULL }, /* 621 */
	{CMDR_DATAPORTERROR	, cmd_dataport }, /* 626 */
	{CMDR_BANLIST_NICK	, cmd_banlist }, /* 629 */
	{CMDR_NICK		, cmd_recname }, /* 825 */
	{CMDS_PING		, cmd_ping }, /* 751 */
	{CMDS_PONG		, cmd_ping }, /* 752 */
	{CMDS_NAME		, cmd_endname }, /* 830 */
	{CMDS_SENDME		, cmd_sendme }, /* 824 */
	{ -1			, NULL }
};

#define NUMBER_OF_COMMANDS (sizeof(nap_commands) / sizeof(_NAP_COMMANDS)) - 1


static char *nap_ansi = NULL;

char *n_speed(int speed)
{
	if (speed > MAX_SPEED)
		speed = MAX_SPEED;
	return _n_speed[speed];
}

char *speed_color(int speed)
{
	if (speed > MAX_SPEED)
		speed = MAX_SPEED;
	return _speed_color[speed];
}

#if 0
void flush_napster (int snum)
{
	fd_set rd;
	struct timeval timeout;
	int	flushing = 1;
	char	buffer[NAP_BUFFER_SIZE + 1];

	if (snum == -1)
		return;
	timeout.tv_usec = 0;
	timeout.tv_sec = 1;
	nap_say("Flushing output/input");
	while (flushing)
	{
		FD_ZERO(&rd);
		FD_SET(snum, &rd);
		switch (select(snum + 1, &rd, NULL, NULL, &timeout))
		{
			case -1:
			case 0:
				flushing = 0;
				break;
			default:
				if (FD_ISSET(snum, &rd))
				{
					int count;
					if (ioctl(snum, FIONREAD, &count) == -1)
						break;					
					if (read(snum, buffer, count) == -1)
						flushing = 0;
				}
				break;
		}
	}
}
#endif

void set_napster_socket(int number)
{
int on = 32000;
	setsockopt(number, SOL_SOCKET, SO_RCVBUF, (char *)&on, sizeof(on));
	on = 60000;
	setsockopt(number, SOL_SOCKET, SO_SNDBUF, (char *)&on, sizeof(on));
}

int connectbynumber(char *hostn, unsigned short *portnum, int service, int protocol, int nonblocking)
{
	int	fd = -1;
	int	sock_type, 
		proto_type;

	sock_type = AF_INET;
	proto_type = (protocol == PROTOCOL_TCP) ? SOCK_STREAM : SOCK_DGRAM;

	if ((fd = socket(sock_type, proto_type, 0)) < 0)
		return -1;

	set_napster_socket(fd);

	if (service == SERVICE_SERVER)
	{
		int length;
#ifdef IP_PORTRANGE
		int ports;
#endif
		int on = 1;
		struct sockaddr_in name;

		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
		on = 1;
		setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof(on));

		memset(&name, 0, sizeof(struct sockaddr_in));
		name.sin_family = AF_INET;
		name.sin_addr.s_addr = htonl(INADDR_ANY);
		name.sin_port = htons(*portnum);
#ifdef PARANOID
		name.sin_port += (unsigned short)(rand() & 255);
#endif
		
#ifdef IP_PORTRANGE
		if (getenv("EPIC_USE_HIGHPORTS"))
		{
			ports = IP_PORTRANGE_HIGH;
			setsockopt(fd, IPPROTO_IP, IP_PORTRANGE, 
					(char *)&ports, sizeof(ports));
		}
#endif

		if (bind(fd, (struct sockaddr *)&name, sizeof(name)))
			return close(fd), -2;

		length = sizeof (name);
		if (getsockname(fd, (struct sockaddr *)&name, &length))
			return close(fd), -5;

		*portnum = ntohs(name.sin_port);

		if (protocol == PROTOCOL_TCP)
			if (listen(fd, 4) < 0)
				return close(fd), -3;
#ifdef NON_BLOCKING_CONNECTS
		if (nonblocking && set_non_blocking(fd) < 0)
			return close(fd), -4;
#endif
	}

	/* Inet domain client */
	else if (service == SERVICE_CLIENT)
	{
		struct sockaddr_foobar server;
		struct hostent *hp;

		memset(&server, 0, sizeof(struct sockaddr_in));

		if (isdigit(hostn[strlen(hostn)-1]))
			inet_aton(hostn, (struct in_addr *)&server.sf_addr);
		else
		{
			if (!(hp = gethostbyname(hostn)))
	  			return close(fd), -6;
			memcpy(&server.sf_addr, hp->h_addr, hp->h_length);
		}
		server.sf_family = AF_INET;
		server.sf_port = htons(*portnum);

#ifdef NON_BLOCKING_CONNECTS
		if (nonblocking && set_non_blocking(fd) < 0)
			return close(fd), -4;
#endif

		alarm(get_int_var(CONNECT_TIMEOUT_VAR));
		if (connect (fd, (struct sockaddr *)&server, sizeof(server)) < 0)
		{
			alarm(0);
#ifdef NON_BLOCKING_CONNECTS
			if (!nonblocking)
#endif
				return close(fd), -4;
		}
		alarm(0);
	}

	/* error */
	else
		return close(fd), -7;

	return fd;
}



char    *numeric_banner(int curr)
{
	static  char    thing[4];
	if (!get_dllint_var("napster_show_numeric"))
		return (nap_ansi?nap_ansi:empty_string);
	sprintf(thing, "%3.3u", curr);
	return (thing);
}

static void set_numeric_string(Window *win, char *value, int unused)
{
	if (value)
		malloc_strcpy(&nap_ansi, value);
	else
		new_free(&nap_ansi);
}
                                                        

char *convert_time (time_t ltime)
{
unsigned long days = 0,hours = 0,minutes = 0,seconds = 0;
static char buffer[40];
	*buffer = '\0';
	seconds = ltime % 60;
	ltime = (ltime - seconds) / 60;
	minutes = ltime%60;
	ltime = (ltime - minutes) / 60;
	hours = ltime % 24;
	days = (ltime - hours) / 24;
	sprintf(buffer, "%2lud %2luh %2lum %2lus", days, hours, minutes, seconds);
	return(*buffer ? buffer : empty_string);
}


int send_ncommand(unsigned int ncmd, char *fmt, ...)
{
char buffer[2*NAP_BUFFER_SIZE+1];
_N_DATA n_data = {0};
va_list ap;
int rc;
	if (nap_socket == -1)
		return -1;
	if (fmt)
	{
		va_start(ap, fmt);
		n_data.len = vsnprintf(buffer, 2*NAP_BUFFER_SIZE, fmt, ap);
		va_end(ap);
	}
	n_data.command = ncmd;
#ifdef DEBUG
	put_it("send: %d %d to %d", n_data.len, n_data.command, nap_socket);
	put_it("data: %s", buffer); 
#endif
	rc = NAP_send(&n_data, sizeof(n_data));
	if (fmt)
		return NAP_send(buffer, n_data.len);	
	return (rc != -1) ? 0 : -1;
}

void send_hotlist(void)
{
NickStruct *new;
ChannelStruct *ch;
	for (new = nap_hotlist; new; new = new->next)
		send_ncommand(CMDS_ADDHOTLISTSEQ, new->nick);
	for (ch = nchannels; ch; ch = ch->next)
	{
		send_ncommand(CMDS_JOIN, ch->channel);		
		if (!ch->next)
			malloc_strcpy(&nap_current_channel, ch->channel);
	}
}


int nap_say(char *format, ...)
{
int     lastlog_level;

	lastlog_level = set_lastlog_msg_level(LOG_CRAP);
	if (get_dllint_var("napster_window") > 0)
		if (!(target_window = get_window_by_name("NAPSTER")))
			target_window = current_window;
	if (window_display && format)
	{
		va_list args;
		va_start (args, format);
		vsnprintf(&(napbuf[strlen(get_dllstring_var("napster_prompt"))+1]), 2*NAP_BUFFER_SIZE, format, args);
		va_end(args);
		strcpy(napbuf, get_dllstring_var("napster_prompt"));
		napbuf[strlen(get_dllstring_var("napster_prompt"))] = ' ';
		if (get_dllint_var("napster_show_numeric"))
			strmopencat(napbuf, sizeof(napbuf)-1, " ", "[", ltoa(nap_numeric), "]", NULL);
                if (*napbuf)
		{
			add_to_log(irclog_fp, 0, napbuf, 0);
			add_to_screen(napbuf);
		}
	}
	target_window = NULL;
	set_lastlog_msg_level(lastlog_level);
	return 0;
}

int nap_put(char *format, ...)
{
int     lastlog_level;

	lastlog_level = set_lastlog_msg_level(LOG_CRAP);
	if (get_dllint_var("napster_window") > 0)
		if (!(target_window = get_window_by_name("NAPSTER")))
			target_window = current_window;
	if (window_display && format)
	{
		va_list args;
		va_start (args, format);
		vsnprintf(napbuf, sizeof(napbuf), format, args);
		va_end(args);
		if (get_dllint_var("napster_show_numeric"))
			strmopencat(napbuf, sizeof(napbuf)-1, " ", "[", ltoa(nap_numeric), "]", NULL);
                if (*napbuf)
		{
			add_to_log(irclog_fp, 0, napbuf, 0);
			add_to_screen(napbuf);
		}
	}
	target_window = NULL;
	set_lastlog_msg_level(lastlog_level);
	return 0;
}

int check_naplink(SocketList *Client, char *str, int active)
{
	if ((active && !Client) || (!active && Client))
	{
		nap_say(str?str:"Connect to Napster first");
		return 0;
	}
	return 1;
}

void clear_filelist(FileStruct **f)
{
FileStruct *last_file, *f1 = *f;
	while (f1)
	{
		last_file = f1->next;
		new_free(&f1->name);
		new_free(&f1->nick);
		new_free(&f1->checksum);
		new_free(&f1);
		f1 = last_file;
	}
	*f = NULL;
}

void free_nicks(ChannelStruct *ch)
{
NickStruct *n, *last;
	n = ch->nicks;
	while(n)
	{
		last = n;
		n = n->next;
		new_free(&last->nick);
		new_free(&last);
	}
}

void clear_nchannels()
{
ChannelStruct *lastch;
	while (nchannels)
	{
		lastch = nchannels->next;
		free_nicks(nchannels);
		new_free(&nchannels->topic);
		new_free(&nchannels);
		nchannels = lastch;
	}
	nchannels = NULL;
}

void clear_nicks()
{
ChannelStruct *lastch;
	for (lastch = nchannels; lastch; lastch = lastch->next)
	{
		free_nicks(lastch);
		lastch->nicks = NULL;
		lastch->injoin = 1;
	}
}

BUILT_IN_DLL(nclose)
{
NickStruct *new;
	if (nap_data != -1)
		close_socketread(nap_data);
	nap_data = -1;
	if (nap_socket != -1)
		close_socketread(nap_socket);
	naphub = NULL;
	nap_socket = -1;	
	if (do_hook(MODULE_LIST, "NAP close"))
		nap_say("%s", cparse("Closed Napster connection", NULL));
	clear_nicks();
	clear_filelist(&file_search);
	clear_filelist(&file_browse);
	new_free(&nap_current_channel);

	statistics.shared_files = 0;
	statistics.shared_filesize = 0;
	statistics.libraries = 0;
	statistics.gigs = 0;
	statistics.songs = 0;
	build_napster_status(NULL);
	for (new = nap_hotlist; new; new = new->next)
		new->speed = -1;

}

BUILT_IN_DLL(nap_link)
{
char *host = NULL, *passwd = NULL, *port = NULL, *username = NULL, *tmp;
int sucks = 0;
int create = 0;
int got_host = 0;

	if (!check_naplink(naphub, "Already connected to Napster", 0))
		return;
	if (args && *args)
	{
		if (!my_strnicmp(args, "-create", 3))
		{
			next_arg(args, &args);
			create = 1;
		}
	}
/*
 *	user pass host port
 *	user pass host
 *	user pass
 *	user
 */	
	while ((tmp = next_arg(args, &args)))
	{
		if (!got_host && !strchr(tmp, '.'))
		{
			if (!username)
				username = tmp;
			else
				passwd = tmp;
		}
		else
		{
			got_host = 1;
			if (!host)
				host = tmp;
			else
				port = tmp;
		}	
	}
	if (!username)
		username = get_dllstring_var("napster_user");
	else
		set_dllstring_var("napster_user", username);
	if (!passwd)
		passwd = get_dllstring_var("napster_pass");
	else
		set_dllstring_var("napster_pass", passwd);

	if (!host)
		host = get_dllstring_var("napster_host");
	if (!port)
		sucks = get_dllint_var("napster_port");
	else
		sucks = my_atol(port);		
#if 0
	if (create)
	{
		if (!(username = next_arg(args, &args)))
			username = get_dllstring_var("napster_user");
		else
			set_dllstring_var("napster_user", username);
		if (!(passwd = next_arg(args, &args)))
			passwd = get_dllstring_var("napster_pass");
		else
			set_dllstring_var("napster_pass", passwd);
	}
	else 
	{
		if (!(username = next_arg(args, &args)))
			username = get_dllstring_var("napster_user");
		else
			set_dllstring_var("napster_user", username);
		if (!(passwd = next_arg(args, &args)))
			passwd = get_dllstring_var("napster_pass");
		else
			set_dllstring_var("napster_pass", passwd);
	}
	if (!(host = next_arg(args, &args)))
		host = get_dllstring_var("napster_host");
	if (!(port = next_arg(args, &args)))
		sucks = get_dllint_var("napster_port");
	else 
		sucks = my_atol(port);
#endif
	if (!sucks)
	{
		nap_say("Invalid port specified %d", sucks);
		return;
	}
	if (host && sucks && username && passwd)
	{
		malloc_strcpy(&auth.username, username);
		malloc_strcpy(&auth.password, passwd);
		auth.connection_speed = get_dllint_var("napster_speed");
		naplink_getserver(host, (unsigned short)sucks, create);
	}
	else if (do_hook(MODULE_LIST, "NAP error connect"))
		nap_say("No %s specified", !host?"host":!username?"username":!passwd?"passwd":"arrggh");
}

NAP_COMM(cmd_sendme)
{
char *chan, *nick;
	chan = next_arg(args, &args);
	nick = next_arg(args, &args);
	put_it("* %s/%s %s", chan, nick, args);
	return 0;
}


NAP_COMM(cmd_bastard)
{
	nap_say("Got bastard command %s", args);
	return 0;
}

NAP_COMM(cmd_ping)
{
char *nick;
	if ((nick = next_arg(args, &args)))
	{
		nap_say("%s", cparse("$0 has requested a ping", "%s", nick));
		send_ncommand(CMDS_PONG, "%s%s%s", nick, args ? " ":empty_string, args ? args : empty_string);
	}
	return 0;
}

NAP_COMM(cmd_hotlist)
{
char *nick;
NickStruct *new;
	nick = next_arg(args, &args);
	if ((new = (NickStruct *)find_in_list((List **)&nap_hotlist, nick, 0)))
	{
		new->speed = my_atol(next_arg(args, &args));
		if (do_hook(MODULE_LIST, "NAP HOTLIST %s %d", new->nick, new->speed)) 
			nap_say("%s", cparse(" %R*%n HotList User $0 $1 has signed on", "%s %s", new->nick, n_speed(new->speed)));
	}
	return 0;
}

NAP_COMM(cmd_dataport)
{
	if (do_hook(MODULE_LIST, "NAP DATAPORT"))
		nap_say("%s", cparse("* Data port misconfigured. Reconfiguring", NULL));
	make_listen(-1);
	return 0;
}

NAP_COMM(cmd_banlist)
{
	if (do_hook(MODULE_LIST, "NAP BANLIST %s", args))
		nap_say("%s", cparse("* $0-", "%s", args));
	return 0;
}

void update_napster_window(Window *tmp)
{
	char statbuff[NAP_BUFFER_SIZE];
	char *st;
	st = napster_status();
	sprintf(statbuff, "[1;45m %d/%d/%dgb %%>%s ", statistics.libraries, statistics.songs, statistics.gigs, tmp->double_status ? "" : st);
	set_wset_string_var(tmp->wset, STATUS_FORMAT1_WSET, statbuff);
	sprintf(statbuff, "[1;45m %%>%s ", st);
	set_wset_string_var(tmp->wset, STATUS_FORMAT2_WSET, statbuff);
	update_window_status(tmp, 1);
	new_free(&st);
}

int build_napster_status(Window *tmp)
{
Window *tmp1;
	if (!(tmp1 = tmp))
		tmp1 = get_window_by_name("NAPSTER");
	if (tmp1)
	{
		update_napster_window(tmp1);
		build_status(tmp1, NULL, 0);
#if 0
		if (nap_socket != -1)
			set_input_prompt(tmp, "%K[%YNap%K]%n ", 0);
		else
			set_input_prompt(tmp, "[0] ", 0);
#endif
		update_all_windows();
		return 1;
	}
	return 0;
}


NAP_COMM(cmd_stats)
{
	sscanf(args, "%d %d %d", &statistics.libraries, &statistics.songs, &statistics.gigs);
	if (!build_napster_status(NULL))
		if (do_hook(MODULE_LIST, "NAP STATS %d %d %d", statistics.libraries, statistics.songs, statistics.gigs))
			nap_say("%s", cparse("Libs[$0] Songs[$1] GB[$2]", "%d %d %d", statistics.libraries, statistics.songs, statistics.gigs));
	return 0;
}

NAP_COMM(cmd_whowas)
{
	if (do_hook(MODULE_LIST, "NAP WHOWAS %s", args))
	{
		char *nick, *class, *l_ip, *l_port, *d_port, *email;
		int t_up, t_down;
		time_t online;
		nick = new_next_arg(args, &args);
		class = new_next_arg(args, &args);
		online = my_atol(new_next_arg(args, &args));
		t_down = my_atol(next_arg(args, &args));
		t_up = my_atol(next_arg(args, &args));
		l_ip = next_arg(args, &args);
		l_port = next_arg(args, &args);
		d_port = next_arg(args, &args);
		email = next_arg(args, &args);
		
		nap_put("%s", cparse("ÚÄÄÄÄÄ---Ä--ÄÄ-ÄÄÄÄÄÄ---Ä--ÄÄ-ÄÄÄÄÄÄÄÄÄ--- --  -", NULL));
		if (l_ip)
			nap_put("%s", cparse("| User    : $0($1) $2 l:$3 d:$4", "%s %s %s %s %s", nick, email, l_ip, l_port, d_port));
		else
			nap_put("%s", cparse("| User       : $0", "%s", nick));
		nap_put("%s", cparse("³ Class      : $0", "%s", class));
		nap_put("%s", cparse(": Last online: $0-", "%s", my_ctime(online)));
		if (t_down || t_up)
			nap_put("%s", cparse(": Total Uploads : $0 Downloading : $1", "%d %d", t_up, t_down));
	}
	return 0;
}

NAP_COMM(cmd_whois)
{
	if (do_hook(MODULE_LIST, "NAP WHOIS %s", args))
	{
		char *nick, *class, *status, *channels, *ver, *l_ip, *l_port, *d_port, *email;
		int shared, download, upload, speed, t_down, t_up;
		time_t online;

		nick = new_next_arg(args, &args);
		class = new_next_arg(args, &args);
		online = my_atol(new_next_arg(args, &args));
		channels = new_next_arg(args, &args);
		status = new_next_arg(args, &args);			
		shared = my_atol(new_next_arg(args, &args));
		download = my_atol(new_next_arg(args, &args));
		upload = my_atol(new_next_arg(args, &args));
		speed = my_atol(new_next_arg(args, &args));
		ver = new_next_arg(args, &args);

		t_down = my_atol(next_arg(args, &args));
		t_up = my_atol(next_arg(args, &args));
		l_ip = next_arg(args, &args);
		l_port = next_arg(args, &args);
		d_port = next_arg(args, &args);		
		email = next_arg(args, &args);
		
		nap_put("%s", cparse("ÚÄÄÄÄÄ---Ä--ÄÄ-ÄÄÄÄÄÄ---Ä--ÄÄ-ÄÄÄÄÄÄÄÄÄ--- --  -", NULL));
		if (l_ip)
			nap_put("%s", cparse("| User    : $0($1) $2 l:$3 d:$4", "%s %s %s %s %s", nick, email, l_ip, l_port, d_port));
		else
			nap_put("%s", cparse("| User    : $0", "%s", nick));
		nap_put("%s", cparse("| Class   : $0", "%s", class));
		nap_put("%s", cparse("³ Line    : $0-", "%s", n_speed(speed)));
		nap_put("%s", cparse("³ Time    : $0-", "%s", convert_time(online)));
		nap_put("%s", cparse("³ Channels: $0-", "%s", channels ? channels : empty_string));
		nap_put("%s", cparse("³ Status  : $0-", "%s", status));
		nap_put("%s", cparse("³ Shared  : $0", "%d", shared));
		nap_put("%s", cparse(": Client  : $0-", "%s", ver));
		nap_put("%s", cparse(": Uploading : $0 Downloading : $1", "%d %d", upload, download));
		if (t_down || t_up)
			nap_put("%s", cparse(": Total Uploads : $0 Downloading : $1", "%d %d", t_up, t_down));
	}
	return 0;
}

NAP_COMM(cmd_error)
{
	if (do_hook(MODULE_LIST, "NAP ERROR %s", args))
	{
		if (args && !strcmp(args, "Invalid Password!"))
		{
			nap_say("%s", cparse("$0-", "%s", args));
			nap_error = 11;
		}
		else
			nap_say("%s", cparse("Recieved error for [$0] $1-.", "%d %s", cmd, args ? args : empty_string));
	}
	if (nap_error > 10)
	{
		nclose(NULL, NULL, NULL, NULL, NULL);
		nap_error = 0;
	}
	return 0;	
}

NAP_COMM(cmd_unknown)
{
	if (do_hook(MODULE_LIST, "NAP UNKNOWN %s", args))
		nap_say("%s", cparse("Recieved unknown [$0] $1-.", "%d %s", cmd, args));
	return 0;	
}

NAP_COMM(cmd_login)
{
	send_ncommand(CMDS_LOGIN, "%s %s %d \"BX-nap v%s\" %d",
		get_dllstring_var("napster_user"), 
		get_dllstring_var("napster_pass"), 
		get_dllint_var("napster_dataport"), 
		nap_version,
		get_dllint_var("napster_speed"));
	return 0;
}

NAP_COMM(cmd_email)
{
	nap_say("%s", cparse("EMAIL address is $0-", "%s", args));
	return 0;	
}

NAP_COMM(cmd_joined)
{
char *chan;
	if ((chan = next_arg(args, &args)) && !find_in_list((List **)&nchannels, chan, 0))
	{
		ChannelStruct *new;
		new = (ChannelStruct *)new_malloc(sizeof(ChannelStruct));
		new->channel = m_strdup(chan);
		add_to_list((List **)&nchannels, (List *)new);
		new->injoin = 1;
		if (do_hook(MODULE_LIST, "NAP JOINED %s", chan))
			nap_say("%s", cparse("Joined channel $0", "%s", chan));
		malloc_strcpy(&nap_current_channel, chan);
		build_napster_status(NULL);
	}
	return 0;
}

NAP_COMM(cmd_parted)
{
char *chan;
ChannelStruct *new;
NickStruct *n;
	if ((chan = next_arg(args, &args)) && (new = (ChannelStruct *)find_in_list((List **)&nchannels, chan, 0)))
	{
		char *nick;
		if (!(nick = next_arg(args, &args)))
			return 0;
		if (!my_stricmp(nick, get_dllstring_var("napster_user")))
		{
			if ((new = (ChannelStruct *)remove_from_list((List **)&nchannels, chan)))
			{
				free_nicks(new);
				new_free(&new->topic);
				new_free(&new);
			}
			if (do_hook(MODULE_LIST, "NAP PARTED %s", chan))
				nap_say("%s", cparse("You have parted $0", "%s", chan));
		}
		else if ((n = (NickStruct *)remove_from_list((List **)&nchannels->nicks, nick)))
		{
			int shared = 0;
			int speed = 0;
			shared = my_atol(next_arg(args, &args));
			speed = my_atol(args);
			new_free(&n->nick);
			new_free(&n);
			if (do_hook(MODULE_LIST, "NAP PARTED %s %s %d %d", nick, chan, shared, speed))
			{
				char part_str[200], *p;
				strcpy(part_str, "$0 has parted $1 %K[  $2/$3%n%K]");
				if ((p = strstr(part_str, "  ")))
					memcpy(p, speed_color(speed), 2);
				nap_say("%s", cparse(part_str, "%s %s %d %s", nick, chan, shared, n_speed(speed)));
			}
		}
	}	
	return 0;
}

NAP_COMM(cmd_topic)
{
char *chan;
ChannelStruct *new;
	if ((chan = next_arg(args, &args)) && (new = (ChannelStruct *)find_in_list((List **)&nchannels, chan, 0)))
	{
		new->topic = m_strdup(args);
		if (do_hook(MODULE_LIST, "NAP TOPIC %s", args))
			nap_say("%s", cparse("Topic for $0: $1-", "%s %s", chan, args));
	}
	return 0;
}

NAP_COMM(cmd_names)
{
ChannelStruct *ch;
char *chan, *nick;
	chan = next_arg(args, &args);
	nick = next_arg(args, &args);
	if (!nick || !chan)
		return 0;
	ch = (ChannelStruct *)find_in_list((List **)&nchannels, chan, 0);
	if (ch)
	{
		NickStruct *n;
		if (!(n = (NickStruct *)find_in_list((List **)&ch->nicks, nick, 0)))
		{
			n = (NickStruct *)new_malloc(sizeof(NickStruct));
			n->nick = m_strdup(nick);
			add_to_list((List **)&ch->nicks, (List *)n);
		}
		n->shared = my_atol(next_arg(args, &args));
		n->speed = my_atol(args);
		if (!ch->injoin)
			if (do_hook(MODULE_LIST, "NAP NAMES %s %d %d", nick, n->shared, n->speed))
			{
				char join_str[200], *p;
				strcpy(join_str, "$0 has joined $1 %K[  $2/$3-%n%K]");
				p = strstr(join_str, "  ");
				memcpy(p, speed_color(n->speed), 2);
				nap_say("%s", cparse(join_str, "%s %s %d %s", nick, chan, n->shared, n_speed(n->speed)));
			}
	}
	return 0;
}

void name_print(NickStruct *n, int hotlist)
{
char buffer[NAP_BUFFER_SIZE+1];
int cols = get_dllint_var("napster_names_columns") ? get_dllint_var("napster_names_columns") : get_int_var(NAMES_COLUMNS_VAR);
int count = 0;
NickStruct *n1;
	if (!cols)
		cols = 1;
	*buffer = 0;
	for (n1 = n; n1; n1 = n1->next)
	{
		if (hotlist)
			strcat(buffer, convert_output_format((n1->speed != -1)?get_dllstring_var("napster_hotlist_online") : get_dllstring_var("napster_hotlist_offline"), "%s %d", n1->nick, n1->speed));
		else
		{
			char tmp[200], *p;
			strcpy(tmp, get_dllstring_var("napster_names_nickcolor"));
			if ((p = strstr(tmp, "  ")))
				memcpy(p, speed_color(n1->speed), 2);
			strcat(buffer, convert_output_format(tmp, "%s %d %d", n1->nick, n1->speed, n1->shared));
		}
		strcat(buffer, space);
		if (count++ >= (cols - 1))
		{
			nap_put("%s", buffer);
			*buffer = 0;
			count = 0;
		}
	}
	if (*buffer)
		nap_put("%s", buffer);
}

NAP_COMM(cmd_endname) /* 830 */
{
	nap_say("%s", cparse("End of names", NULL));
	return 0;
}

NAP_COMM(cmd_recname)
{
	nap_say("%s", cparse("$[20]0 $[8]1 $2", "%s", args));
	return 0;
}

NAP_COMM(cmd_endnames)
{
ChannelStruct *ch;
char *chan;
	if (!(chan = next_arg(args, &args)))
		return 0;
	ch = (ChannelStruct *)find_in_list((List **)&nchannels, chan, 0);
	ch->injoin = 0;
	if (do_hook(MODULE_LIST, "NAP ENDNAMES %s", chan))
	{
		if (ch)
			name_print(ch->nicks, 0);
	}
	malloc_strcpy(&nap_current_channel, chan);
	return 0;
}

NAP_COMM(cmd_public)
{
	char *from, *chan;
	chan = next_arg(args, &args);
	from = next_arg(args, &args);
	if (!chan || !from || check_nignore(from))
		return 0;
	if (nap_current_channel && !my_stricmp(nap_current_channel, chan))
	{
		if (do_hook(MODULE_LIST, "NAP PUBLIC %s %s %s", from, chan, args))
			nap_put("%s",cparse(fget_string_var(FORMAT_PUBLIC_FSET), "%s %s %s %s", update_clock(GET_TIME), from, chan, args));
	}
	else
	{
		if (do_hook(MODULE_LIST, "NAP PUBLIC_OTHER %s %s %s", from, chan, args))
			nap_put("%s",cparse(fget_string_var(FORMAT_PUBLIC_OTHER_FSET), "%s %s %s %s", update_clock(GET_TIME), from, chan, args));
	}
	return 0;
}

NAP_COMM(cmd_msg)
{
	char *from;
	from = next_arg(args, &args);
	if (!from || check_nignore(from))
		return 0;
	if (do_hook(MODULE_LIST, "NAP MSG %s %s", from, args))
		nap_put("%s", convert_output_format(fget_string_var(FORMAT_MSG_FSET), "%s %s %s %s", update_clock(GET_TIME), from, "*@*", args));
	addtabkey(from, "nmsg", 0);
	return 0;
}

static void naplink_handler (int s)
{
char tmpstr[2*NAP_BUFFER_SIZE+1];
char *tmp = tmpstr;
_N_DATA n_data;
unsigned char blah[5];
int i;
	set_display_target("NAPSTER", LOG_CRAP);
	memset(tmpstr, 0, sizeof(tmpstr));

	/* rain thinks this might work better on big-endian */
	i = read(s, blah, 4);
	n_data.len = blah[0] + ((blah[1] << 8) & 0xff00);
	n_data.command = blah[2] + ((blah[3] << 8) & 0xff00);

	if (i <= 0)
	{
		nap_say("Read error [%s]", strerror(errno));
		nclose(NULL, NULL, NULL, NULL, NULL);
		return;
	}
	if (!n_data.command)
		nap_error++;
	else
		nap_error = 0;
	if ((i = read(s, tmp, n_data.len)) != n_data.len)
	{
		int len;
		len = n_data.len - i;
		if ((i == -1) || (i = read(s, tmp+i, len)) != len)
		{
			nap_say("Read error [%s]", strerror(errno));
			nclose(NULL, NULL, NULL, NULL, NULL);
			close_socketread(s);
			reset_display_target();
			return;
		}
	}
	nap_numeric = n_data.command;
	
	for (i = 0; i < NUMBER_OF_COMMANDS; i++)
	{
		if (nap_commands[i].cmd == n_data.command)
		{
			if (nap_commands[i].func)
				(nap_commands[i].func)(n_data.command, tmpstr);
			else
				nap_say("%s %s", numeric_banner(n_data.command), tmpstr);
			nap_error = 0;
			reset_display_target();
			return;
		}
	}
	cmd_error(n_data.command, tmpstr);
	reset_display_target();
}

SocketList *naplink_connect(char *host, u_short port)
{
	struct	in_addr	address;
	struct	hostent	*hp;
	int	lastlog_level;
	
	lastlog_level = set_lastlog_msg_level(LOG_DCC);
	if ((address.s_addr = inet_addr(host)) == -1)
	{
		if (!my_stricmp(host, "255.255.255.0") || !(hp = gethostbyname(host)))
		{
			nap_say("%s", cparse("%RDCC%n Unknown host: $0-", "%s", host));
			set_lastlog_msg_level(lastlog_level);
			return NULL;
		}
		bcopy(hp->h_addr, (char *)&address, sizeof(address));
	}
	nap_socket = connectbynumber(host, &port, SERVICE_CLIENT, PROTOCOL_TCP, 0);
	if (nap_socket < 0)
	{
		nap_socket = -1;
		naphub = NULL;
		return NULL;
	}
	add_socketread(nap_socket, port, 0, host, naplink_handler, NULL);
	(void) set_lastlog_msg_level(lastlog_level);
	naphub = get_socket(nap_socket);
	return naphub;
}

NAP_COMM(cmd_registerinfo)
{
	if (do_hook(MODULE_LIST, "NAP REGISTER %s", get_dllstring_var("napster_user")))
		nap_say("%s", cparse("Registered Username $0", "%s", get_dllstring_var("napster_user")));
	send_ncommand(CMDS_REGISTERINFO," %s %s %d \"BX-nap v%s\" %d %s", 
			get_dllstring_var("napster_user"), 
			get_dllstring_var("napster_pass"),
			get_dllint_var("napster_dataport"), nap_version,
			get_dllint_var("napster_speed"), 
			get_dllstring_var("napster_email"));
	return 0;
}

NAP_COMM(cmd_alreadyregistered)
{
	if (do_hook(MODULE_LIST, "NAP REGISTER_ERROR"))
		nap_say("%s", cparse("Already Registered", NULL));
	nclose(NULL, NULL, NULL, NULL, NULL);
	return 0;
}

NAP_COMM(cmd_offline)
{
	if (do_hook(MODULE_LIST, "NAP OFFLINE %s", args))
		nap_say("%s", cparse("User $0 offline", "%s", args));
	return 0;
}

void _naplink_connectserver(char *tmp, int create)
{
char *s_port;
unsigned short port;
	if (do_hook(MODULE_LIST, "NAP CONNECT %s", tmp))
		nap_say("%s", cparse("Got server. Attempting connect to $0.", "%s", tmp));
	naphub = NULL;
	nap_socket = -1;	
	if (!(s_port = strchr(tmp, ':')))
	{
		next_arg(tmp, &s_port);
		if (!s_port)
		{
			nap_say("%s", cparse("error in naplink_connectserver()", NULL));
			return;
		}
	} else
		*s_port++ = 0;
	port = atoi(s_port);
	if ((naplink_connect(tmp, port)))
	{
		set_napster_socket(nap_socket);
		nap_say("%s", cparse("Connected. Attempting Login to $0:$1.", "%s %s", tmp, s_port));
		if (create)
			send_ncommand(CMDS_CREATEUSER, "%s", get_dllstring_var("napster_user"));
		else
			cmd_login(CMDS_LOGIN, empty_string);
		make_listen(get_dllint_var("napster_dataport"));
		send_hotlist();
	}
}

static void naplink_connectserver (int s)
{
char tmpstr[NAP_BUFFER_SIZE+1];
char *tmp = tmpstr;
SocketList *s1;
	s1 = get_socket(s);
	memset(tmpstr, 0, sizeof(tmpstr));
	read(s, tmp, sizeof(tmpstr)-1);
	close_socketread(s);
	if (*tmp)
		_naplink_connectserver(tmp, s1->flags);
	else
		nap_say("Error connecting to server");
}

void naplink_getserver(char *host, u_short port, int create)
{
	struct	in_addr	address;
	struct	hostent	*hp;
	int	lastlog_level;
	lastlog_level = set_lastlog_msg_level(LOG_DCC);
	if ((address.s_addr = inet_addr(host)) == -1)
	{
		if (!my_stricmp(host, "255.255.255.0") || !(hp = gethostbyname(host)))
		{
			nap_say("%s", cparse("%RDCC%n Unknown host: $0-", "%s", host));
			set_lastlog_msg_level(lastlog_level);
			return;
		}
		bcopy(hp->h_addr, (char *)&address, sizeof(address));
	}
	nap_socket = connectbynumber(host, &port, SERVICE_CLIENT, PROTOCOL_TCP, 1);
	if (nap_socket < 0)
	{
		nap_socket = -1;
		naphub = NULL;
		return;
	}
	add_socketread(nap_socket, port, create, host, naplink_connectserver, NULL);
	nap_say("%s", cparse("Attempting to get host from $0:$1.","%s %d", host, port));
	(void) set_lastlog_msg_level(lastlog_level);
}

static void toggle_napwin_hide (Window *win, char *unused, int onoff)
{
Window *tmp;
	if ((tmp = get_window_by_name("NAPSTER")))
	{
		if (onoff)
		{
			if (tmp->screen)
				hide_window(tmp);
			build_napster_status(tmp);
			update_all_windows();
			cursor_to_input();
		}
		else
		{
			show_window(tmp);
			resize_window(2, tmp, 6);
			build_napster_status(tmp);
			update_all_windows();
			cursor_to_input();
		}
	}
}

static void toggle_napwin (Window *win, char *unused, int onoff)
{
Window *tmp;
	if (onoff)
	{
		if ((tmp = get_window_by_name("NAPSTER")))
			return;
		if ((tmp = new_window(win->screen)))
		{
			resize_window(2, tmp, 6);
			tmp->name = m_strdup("NAPSTER");
#undef query_cmd
			tmp->query_cmd = m_strdup("nsay");
			tmp->double_status = 0;
			tmp->absolute_size = 1;
			tmp->update_status = update_napster_window;
			tmp->server = -2;
                	set_wset_string_var(tmp->wset, STATUS_FORMAT1_WSET, NULL);
                	set_wset_string_var(tmp->wset, STATUS_FORMAT2_WSET, NULL);
                	set_wset_string_var(tmp->wset, STATUS_FORMAT3_WSET, NULL);
                	set_wset_string_var(tmp->wset, STATUS_FORMAT_WSET, NULL);

			if (get_dllint_var("napster_window_hidden"))
				hide_window(tmp);
			else
				set_screens_current_window(tmp->screen, tmp);
			build_napster_status(tmp);
			update_all_windows();
			cursor_to_input();
		}
	}
	else
	{
		if ((tmp = get_window_by_name("NAPSTER")))
		{
			if (tmp == target_window)
				target_window = NULL;
			delete_window(tmp);
			update_all_windows();
			cursor_to_input();
			                        
		}
	}
}


BUILT_IN_DLL(naphelp)
{
	if (do_hook(MODULE_LIST, "NAP HELP"))
	{
		nap_say("%s", cparse("First Set your napster_user and napster_pass variables", NULL));
		nap_say("%s", cparse("then we can use /napster to find a server and connect", NULL));
		nap_say("%s", cparse("typing /n<tab> will display a list of various napster commands", NULL));
		nap_say("%s", cparse("also /set napster will display a list of variables", NULL));
	}
	return;
}

BUILT_IN_DLL(napsave)
{
IrcVariableDll *newv = NULL;
FILE *outf = NULL;
char *expanded = NULL;
char buffer[NAP_BUFFER_SIZE+1];
NickStruct *new;
char *p = NULL;
	if (get_string_var(CTOOLZ_DIR_VAR))
		snprintf(buffer, NAP_BUFFER_SIZE, "%s/Napster.sav", get_string_var(CTOOLZ_DIR_VAR));
	else
		sprintf(buffer, "~/Napster.sav");
	expanded = expand_twiddle(buffer);
	if (!expanded || !(outf = fopen(expanded, "w")))
	{
		nap_say("error opening %s", expanded ? expanded : buffer);
		new_free(&expanded);
		return;
	}
	for (newv = dll_variable; newv; newv = newv->next)
	{
		if (!my_strnicmp(newv->name, "napster", 7))
		{
			if (newv->type == STR_TYPE_VAR)
			{
				if (newv->string)
					fprintf(outf, "SET %s %s\n", newv->name, newv->string);
			}
			else if (newv->type == BOOL_TYPE_VAR)
				fprintf(outf, "SET %s %s\n", newv->name, on_off(newv->integer));
			else
				fprintf(outf, "SET %s %d\n", newv->name, newv->integer);
		}
	}
	for (new = nap_hotlist; new; new = new->next)
		m_s3cat(&p, " ", new->nick);
	if (p)
	{
		fprintf(outf, "NHOTLIST %s\n", p);
		new_free(&p);
	}	
	if (do_hook(MODULE_LIST, "NAP SAVE %s", buffer))
		nap_say("Finished saving Napster variables to %s", buffer);
	fclose(outf);	
	new_free(&expanded);
	return;
}


NAP_COMM(cmd_channellist)
{
	if (do_hook(MODULE_LIST, "NAP CHANNEL %s", args))
	{
		if (channel_count == 0)
			nap_put("%s", cparse("Num Channel              Topic", NULL));
		nap_put("%s", cparse("$[-3]1 $[20]0 $5-", "%s", args));
	}
	channel_count++;
	return 0;
}


BUILT_IN_DLL(nap_channel)
{
	if (command)
	{
		ChannelStruct *ch = NULL;
		char *cmd = next_arg(args, &args);
		if (!my_stricmp(command, "njoin"))
		{
			if (cmd)
			{
				if (!(ch = (ChannelStruct *)find_in_list((List **)&nchannels, cmd, 0)))
				{
					send_ncommand(CMDS_JOIN, cmd);
					do_hook(MODULE_LIST, "NAP JOIN %s", cmd);
				}
				else
				{
					malloc_strcpy(&nap_current_channel, ch->channel);
					do_hook(MODULE_LIST, "NAP SWITCH_CHANNEL %s", ch->channel);
				}
			}
			else if (nap_current_channel)
			{
				ch = (ChannelStruct *)find_in_list((List **)&nchannels, nap_current_channel, 0);
				if (ch && ch->next)
					malloc_strcpy(&nap_current_channel, ch->next->channel);
				else if (nchannels)
					malloc_strcpy(&nap_current_channel, nchannels->channel);
			}
			build_napster_status(NULL);
		}
		else if (!my_stricmp(command, "npart"))
		{
			if (cmd)
			{
				if ((ch = (ChannelStruct *)remove_from_list((List **)&nchannels, cmd)))
					send_ncommand(CMDS_PART, cmd);
			}
			else if (nap_current_channel)
			{
				if ((ch = (ChannelStruct *)remove_from_list((List **)&nchannels, nap_current_channel)))
					send_ncommand(CMDS_PART, nap_current_channel);
			}
			if (ch)
			{
				if (do_hook(MODULE_LIST, "NAP PART %s", ch->channel))
					nap_say("%s", cparse("Parted $0", "%s", ch->channel));
				free_nicks(ch);
				if (!my_stricmp(ch->channel, nap_current_channel))
				{
					if (ch->next)
						malloc_strcpy(&nap_current_channel, ch->next->channel);
					else if (nchannels)
						malloc_strcpy(&nap_current_channel, nchannels->channel);
				}
				new_free(&ch->channel);
				new_free(&ch->topic);
				new_free(&ch);
			}

			if (!nap_current_channel && nchannels)
				malloc_strcpy(&nap_current_channel, nchannels->channel);
			else if (nap_current_channel && !nchannels)
				new_free(&nap_current_channel);
			build_napster_status(NULL);
		}
		else if (!my_stricmp(command, "ntopic"))
		{
			ChannelStruct *ch;
			ch = (ChannelStruct *)find_in_list((List **)&nchannels, cmd ? cmd : nap_current_channel ? nap_current_channel : empty_string, 0);
			if (ch)
			{
				if (args && *args)
				{
					send_ncommand(CMDS_TOPIC, "%s %s", ch->channel, args);
					if (do_hook(MODULE_LIST, "NAP TOPIC %s %s", ch->channel, args))
						nap_say("%s", cparse("Topic for $0: $1-", "%s %s", ch->channel, args));
				}
				else
				{
					if (do_hook(MODULE_LIST, "NAP TOPIC %s %s", ch->channel, ch->topic))
						nap_say("%s", cparse("Topic for $0: $1-", "%s %s", ch->channel, ch->topic));
				}
			}
			else if (do_hook(MODULE_LIST, "NAP TOPIC No Channel"))
				nap_say("%s", cparse("No Channel found $0", "%s", cmd ? cmd : empty_string));
		}
		else if (!my_stricmp(command, "nlist"))
		{
			send_ncommand(CMDS_LISTCHANNELS, NULL);
			channel_count = 0;
		}
		else if(!my_stricmp(command, "ninfo"))
			send_ncommand(CMDS_WHOIS, !cmd ? get_dllstring_var("napster_user") : cmd);
	}
}

NAP_COMM(cmd_search)
{
FileStruct *new;
	if (!args || !*args)
		return 0;
	new = (FileStruct *)new_malloc(sizeof(FileStruct));
	new->name = m_strdup(new_next_arg(args, &args));
	new->checksum = m_strdup(next_arg(args, &args));
	new->filesize = my_atol(next_arg(args, &args));
	new->bitrate = my_atol(next_arg(args, &args));
	new->freq = my_atol(next_arg(args, &args));
	new->seconds = my_atol(next_arg(args, &args));
	new->nick = m_strdup(next_arg(args, &args));
	new->ip = my_atol(next_arg(args, &args));
	new->speed = my_atol(next_arg(args, &args));
	if (!new->name || !new->checksum || !new->nick || !new->filesize)
	{
		new_free(&new->name);
		new_free(&new->checksum);
		new_free(&new->nick);
		new_free(&new);
		return 1;
	}
	add_to_list((List **)&file_search, (List *)new);
	return 0;
}

NAP_COMM(cmd_browse)
{
/* nick filename checksum filesize bitrate freq seconds */
FileStruct *new;
	new = (FileStruct *)new_malloc(sizeof(FileStruct));
	new->nick = m_strdup(next_arg(args, &args));
	new->name = m_strdup(new_next_arg(args, &args));
	new->checksum = m_strdup(next_arg(args, &args));
	new->filesize = my_atol(next_arg(args, &args));
	new->bitrate = my_atol(next_arg(args, &args));
	new->freq = my_atol(next_arg(args, &args));
	new->seconds = my_atol(next_arg(args, &args));
	new->speed = my_atol(args);
	if (!new->name || !new->checksum || !new->nick || !new->filesize)
	{
		new_free(&new->name);
		new_free(&new->checksum);
		new_free(&new->nick);
		new_free(&new);
		return 1;
	}
	add_to_list((List **)&file_browse, (List *)new);
	return 0;
}

char *base_name(char *str)
{
char *p;
	if ((p = strrchr(str, '\\')))
		p++;
	else if ((p = strrchr(str, '/')))
		p++;
	else
		p = str;
	return p;
}

char *mp3_time(unsigned long t)
{
static char str[40];
int seconds;
int minutes;
	minutes = t / 60;
	seconds = t % 60;
	sprintf(str, "%02d:%02d", minutes, seconds);
	return str;
}

void print_file(FileStruct *f, int count)
{
	if (!f || !f->name)
		return;
	if (count == 1)
	{
		if (do_hook(MODULE_LIST, "NAP PRINTFILE_HEADER"))
		{
			nap_put("Number ³ Song ³ Bitrate ³ Frequency ³ Length ³ Size ³ Computer ³ Speed");
			nap_put("ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ");
		}
	}
	if (do_hook(MODULE_LIST, "NAP PRINTFILE %d %s %u %u %lu %lu %s %d", 
	     count, f->name, f->bitrate, f->freq, f->seconds, f->filesize, f->nick, f->speed))
	{
		if (((f->ip & 0xff) == 0xc0) && ((f->ip & 0xff00) == 0xa800))
		nap_put("%.3d %s %u %u %s %4.2f%s %s %s XXX",
			count, base_name(f->name), f->bitrate, f->freq,
			mp3_time(f->seconds), (float)_GMKv(f->filesize), _GMKs(f->filesize),
			f->nick, n_speed(f->speed));
		else
		nap_put("%.3d %s %u %u %s %4.2f%s %s %s",
			count, base_name(f->name), f->bitrate, f->freq,
			mp3_time(f->seconds), (float)_GMKv(f->filesize), _GMKs(f->filesize),
			f->nick, n_speed(f->speed));
	}
}

NAP_COMM(cmd_fileinfo)
{
char *nick;
char *ip;
int port;
char *file;
char *checksum;
int speed;
	nick = next_arg(args, &args);
	ip = next_arg(args, &args);
	port = my_atol(next_arg(args, &args));
	file = new_next_arg(args, &args);
	checksum = next_arg(args, &args);
	speed = my_atol(next_arg(args, &args));
	nap_put("Number ³ Song ³ Speed");
	nap_put("ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ");
	nap_put("%.3d %s %d %d", 1, base_name(file), port, n_speed(speed));
	return 0;
}

NAP_COMM(cmd_endbrowse)
{
int count = 1;
FileStruct *f;

	if (do_hook(MODULE_LIST, "NAP ENDBROWSE"))
	{
		for (f = file_browse; f; f = f->next, count++)
			print_file(f, count);		 		
		if (!file_browse)
			nap_say("%s", cparse("Browse finished. No results", NULL));
	}
	return 0;
}

NAP_COMM(cmd_endsearch)
{
int count = 1;
FileStruct *f;
	if (do_hook(MODULE_LIST, "NAP ENDSEARCH"))
	{
		for (f = file_search; f; f = f->next, count++)
			print_file(f, count);		 		
		if (!file_search)
			nap_say("%s", cparse("search finished. No results", NULL));
	}
	return 0;
}

BUILT_IN_DLL(nap_search)
{
char buff[NAP_BUFFER_SIZE+1];
char s_buff[NAP_BUFFER_SIZE+1];
int n = 0;

int bitrate = 0;
unsigned int freq = 0;
int linespeed = 0;
int bit_int = -1;
int freq_int = -1;
int line_int = -1;
int do_type = 0;
int buf_len = 0;
char *search_param[] = { "EQUAL TO", "AT BEST", "AT LEAST", ""};
int soundex = 0;
char *sound_ex[] = { "FILENAME", "SOUNDEX", "" };
char any[] = "ANY";
char *type = NULL;

	if (!args || !*args)
	{
		FileStruct *f;
		int count = 1;
		for (f = file_search; f; f = f->next, count++)
			print_file(f, count);
		return;
	}
	if (command && !my_stricmp(command, "soundex"))
		soundex++;
	while (args && *args == '-')
	{
		char *cmd, *val;
		unsigned int value = 0;
		cmd = next_arg(args, &args);
		val = next_arg(args, &args);
		value = my_atol(type);
		if (!my_strnicmp(cmd, "-type", 4))
		{
			type = val;
			do_type = 1;
			continue;
		}
		else if (!my_strnicmp(cmd, "-any", 4))
		{
			type = any;
			do_type = 1;
			continue;
		}
		if (!my_strnicmp(cmd, "-maxresults", 4))
		{
			if (!args)
			{
				nap_say("%s", cparse("Default Max Results $0", "%d", get_dllint_var("napster_max_results")));
				return;
			}
			set_dllint_var("napster_max_results", value);
			continue;
		}
		if (strstr(cmd, "bitrate"))
		{
			int br[] = {20, 24, 32, 48, 56, 64, 98, 112, 128, 160, 192, 256, 320, -1 };
			int o;
			for (o = 0; br[o] != -1; o++)
				if (br[o] == value)
					break;
			if (br[o] == -1)
			{
				nap_say("%s", cparse("Allowed Bitrates 20, 24, 32, 48, 56, 64, 98, 112, 128, 160, 192, 256, 320", NULL));
				return;
			}
			if (!my_strnicmp(cmd, "-bitrate", 4))
				bitrate = value, bit_int = 0;
			else if (!my_strnicmp(cmd, "-minbitrate", 4))
				bitrate = value, bit_int = 2;
			else if (!my_strnicmp(cmd, "-maxbitrate", 4))
				bitrate = value, bit_int = 1;
		} 
		else if (strstr(cmd, "freq"))
		{
			long fr[] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, -1};
			int o;
			for (o = 0; fr[o] != -1; o++)
				if (fr[o] == value)
					break;
			if (fr[o] == -1)
			{
				nap_say("%s", cparse("Allowed Freq 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000", NULL));
				return;
			}
			if (!my_strnicmp(cmd, "-maxfreq", 4))
				freq = value, freq_int = 1;
			else if (!my_strnicmp(cmd, "-minfreq", 4))
				freq = value, freq_int = 2;
			else if (!my_strnicmp(cmd, "-freq", 4))
				freq = value, freq_int = 0;
		}
		else if (strstr(cmd, "line"))
		{
			if (value < 0 || value > 10)
			{
				nap_say("%s", cparse("Allowed linespeed 0-10", NULL));
				return;
			}
			if (!my_strnicmp(cmd, "-maxlinespeed", 4))
				linespeed = value, line_int = 1;
			else if (!my_strnicmp(cmd, "-minlinespeed", 4))
				linespeed = value, line_int = 2;
			else if (!my_strnicmp(cmd, "-linespeed", 4))
				linespeed = value, line_int = 0;
		}
	}
	

	if (!args || !*args)
		return;

	clear_filelist(&file_search);
	if (soundex)
		compute_soundex(s_buff, sizeof(s_buff), args);

	if (do_type && type)
	{
		sprintf(buff, "TYPE %s ", type);
		buf_len = strlen(buff);
	}	

	if ((n = get_dllint_var("napster_max_results")))
		sprintf(buff + buf_len, "%s CONTAINS \"%s\" MAX_RESULTS %d", sound_ex[soundex], soundex ? s_buff : args, n);
	else
		sprintf(buff + buf_len, "%s CONTAINS \"%s\"", sound_ex[soundex], soundex ? s_buff : args);

	if (!do_type && !type)
	{
		if (bitrate && (bit_int != -1))
			strmopencat(buff, NAP_BUFFER_SIZE, " BITRATE \"", search_param[bit_int], "\" \"", ltoa(bitrate), "\"", NULL);
		if (freq && (freq_int != -1))
			strmopencat(buff, NAP_BUFFER_SIZE, " FREQ \"", search_param[freq_int], "\" \"", ltoa(freq), "\"", NULL);
		if (linespeed && (line_int != -1))
			strmopencat(buff, NAP_BUFFER_SIZE, " LINESPEED \"", search_param[line_int], "\" ", ltoa(linespeed), NULL);
	}
	
	if (do_hook(MODULE_LIST, "NAP SEARCH %s %s", args, soundex ? s_buff : ""))
		nap_say("%s", cparse("* Searching for $0-", "%s %s", args, soundex ? s_buff: ""));
	send_ncommand(CMDS_SEARCH, buff);
}

BUILT_IN_DLL(nap_scan)
{
ChannelStruct *ch;
char *chan;
	if (!args || !*args)
		chan = nap_current_channel;
	else
		chan = next_arg(args, &args);
	if (!chan || !*chan)
		return;
	if (command && !my_stricmp(command, "nnames"))
		send_ncommand(CMDS_NAME, chan);
	else if ((ch = (ChannelStruct *)find_in_list((List **)&nchannels, chan, 0)))
		name_print(ch->nicks, 0);
}

BUILT_IN_DLL(nap_command)
{
char *cmd, *data;
	cmd = next_arg(args, &args);
	if (cmd)
	{
		if (!my_stricmp(cmd, "whois"))
		{
			data = next_arg(args, &args);
			if (!data)
				data = get_dllstring_var("napster_user");
			send_ncommand(CMDS_WHOIS, data);
		}
		else if (!my_stricmp(cmd, "raw"))
		{
			if ((data = next_arg(args, &args)))
				send_ncommand(my_atol(data), (args && *args) ? args : NULL);
		}
		else if (command && !my_stricmp(command, "nbrowse"))
		{
			if (!my_stricmp(cmd, get_dllstring_var("napster_user")))
			{
				nap_say("Browsing yourself is not a very smart thing");
				return;
			}
			send_ncommand(CMDS_BROWSE, cmd);
			clear_filelist(&file_browse);
		}
		else if (command && !my_stricmp(command, "nping"))
		{
			if (cmd)
				send_ncommand(CMDS_PING, "%s %s", cmd, args ? args : empty_string);
		}
	}
}

BUILT_IN_DLL(nap_connect)
{
char *str = NULL;
char buff[BIG_BUFFER_SIZE];
	if (!my_stricmp(command, "nreconnect"))
	{
		SocketList *s;
		s = get_socket(nap_socket);
		if (s)
		{
			sprintf(buff, "%s:%d", s->server, s->port);
			str = buff;
		}
	}
	else
		str = args;
	if (nap_socket != -1)
		nclose(NULL, NULL, NULL, NULL, NULL);
	if (str && *str)
		_naplink_connectserver(str, 0);
}

int make_listen(int port)
{
int fd;
unsigned short pt;
	if (nap_data > 0)
		close_socketread(nap_data);
	if (port == -1)
		pt = get_dllint_var("napster_dataport");
	else
		pt = port;
	if (!pt)
		return 0;
	fd = connectbynumber(NULL, &pt, SERVICE_SERVER, PROTOCOL_TCP, 1);
	if (fd < 0)
	{
		nap_say("%s", cparse("Cannot setup listen port [$0] $1-", "%d %s", pt, strerror(errno)));
		return -1;
	}
	add_socketread(fd, pt, 0, NULL, naplink_handlelink, NULL);
	nap_data = fd;
	return nap_data;
}

NAP_COMM(cmd_hotlistsuccess)
{
	if (do_hook(MODULE_LIST, "NAP HOTLISTADD %s", args))
		nap_say("%s", cparse("Adding $0 to your HotList", "%s", args));
	return 0;
}

NAP_COMM(cmd_hotlisterror)
{
NickStruct *new;
	if (args && (new = (NickStruct *)remove_from_list((List **)&nap_hotlist, args)))
	{
		if (do_hook(MODULE_LIST, "NAP HOTLISTERROR %s", args))
			nap_say("%s", cparse("No such nick $0", "%s", args));
		new_free(&new->nick);
		new_free(&new);
	}
	return 0;
}

BUILT_IN_DLL(naphotlist)
{
NickStruct *new;
char *nick;
	if (!args || !*args)
	{
		nap_say("%s", cparse("Your Hotlist:", NULL));
		name_print(nap_hotlist, 1);
		return;
	}
	while ((nick = next_arg(args, &args)))
	{
		if (!(*nick == '-'))
		{
			if (nap_socket != -1)
				send_ncommand(CMDS_ADDHOTLIST, nick);
			if (!(new = (NickStruct *)find_in_list((List **)&nap_hotlist, nick, 0)))
			{
				new = new_malloc(sizeof(NickStruct));
				new->nick = m_strdup(nick);
				new->speed = -1;
				add_to_list((List **)&nap_hotlist, (List *)new);
			} else if (do_hook(MODULE_LIST, "NAP HOTLISTERROR Already on your hotlist %s", nick))
				nap_say("%s", cparse("$0 is already on your Hotlist", "%s", nick));
		}
		else
		{
			nick++;
			if (*nick && (new = (NickStruct *)remove_from_list((List **)&nap_hotlist, nick)))
			{
				send_ncommand(CMDS_HOTLISTREMOVE, nick);
				if (do_hook(MODULE_LIST, "NAP HOTLISTREMOVE %s", nick))
					nap_say("%s", cparse("Removing $0 from your HotList", "%s", nick));
				new_free(&new->nick);
				new_free(&new);
			}
		}
	}
}

BUILT_IN_DLL(nap_admin)
{
int	i;
char	*comm,
	*user;

typedef struct _Nadmin {
	char	*command;
	int	cmd;
	int	arg_count;
	int	len;
} Nadmin;

Nadmin admin_comm[] = {
	{ "killserver",	CMDS_SERVERKILL,	-1,	5 },
	{ "banuser",	CMDS_BANUSER,		1,	4 },
	{ "setdataport",CMDS_SETDATAPORT,	2,	4 }, 
	{ "setlinespeed",CMDS_SETLINESPEED,	2,	4 },
	{ "setuserlevel",CMDS_SETUSERLEVEL,	2,	4 },
	{ "connect",	CMDS_SERVERLINK,	-1,	4 },
	{ "disconnect",	CMDS_SERVERUNLINK,	-1,	4 },
	{ "config",	CMDS_SETCONFIG,		-1,	4 },
	{ "unnukeuser",	CMDS_UNNUKEUSER,	1,	3 },
	{ "unbanuser",	CMDS_UNBANUSER,		1,	3 },
	{ "unmuzzle",	CMDS_UNMUZZLE,		2,	3 },
	{ "removeserver",CMDS_SERVERREMOVE,	-1,	3 },
	{ "opsay",	CMDS_OPSAY,		-1,	1 },
	{ "announce",	CMDS_ANNOUNCE,		-1,	1 },
	{ "version",	CMDS_SERVERVERSION,	0,	1 },
	{ "reload",	CMDS_RELOADCONFIG,	-1,	1 },
	{ "kill",	CMDS_KILLUSER,		2,	1 },
	{ "nukeuser",	CMDS_NUKEUSER,		1,	1 },
	{ "banlist",	CMDS_BANLIST,		0,	1 },
	{ "muzzle",	CMDS_MUZZLE,		2,	1 },
	{ NULL,		0,			-1}
};
				
	if (!(comm = next_arg(args, &args)))
	{
		nap_say("Please specify a command for /nadmin <command> [args]");
		nap_say("    kill nukeuser unnukeuser banuser unbanuser banlist muzzle unmuzzle");
		nap_say("    setdataport setlinespeed opsay announce setuserlevel version");
		nap_say("Following are open-nap specific");
		nap_say("    connect disconnect killserver removeserver config reload");
		return;
	}
	for (i = 0; admin_comm[i].command; i++)
	{
		if (!my_strnicmp(admin_comm[i].command, comm, admin_comm[i].len))
		{
			switch(admin_comm[i].arg_count)
			{
				case 0:
				{
					send_ncommand(admin_comm[i].cmd, NULL);
					break;
				}
				case 1:
				{
					if ((user = next_arg(args, &args)))
						send_ncommand(admin_comm[i].cmd, user);
					else
						nap_say("Nothing to send for %s", admin_comm[i].command);
					break;
				}
				case 2:
				{
					user = next_arg(args, &args);
					if (args && *args)
						send_ncommand(admin_comm[i].cmd, "%s %s", user, args);
					else
						send_ncommand(admin_comm[i].cmd, "%s", user);
					break;
				}
				case -1:
				{
					if (args && *args)
						send_ncommand(admin_comm[i].cmd, "%s", args);
					else
						nap_say("Nothing to send for %s", admin_comm[i].command);
				}
			}
			return;
		}
	}
	userage(command, helparg);

#if 0	
	if (!my_stricmp(comm, "config"))
	{
		cmd = CMDS_SETCONFIG;
		count = -1;
	}
	else if (!my_stricmp(comm, "reload"))
	{
		cmd = CMDS_RELOADCONFIG;
		count = -1;
	}
	else if (!my_stricmp(comm, "version"))
	{
		cmd = CMDS_SERVERVERSION;
		count = 0;
	}
	else if (!my_stricmp(comm, "connect"))
	{
		cmd = CMDS_SERVERLINK;
		count = -1;
	}
	else if (!my_stricmp(comm, "disconnect"))
	{
		cmd = CMDS_SERVERUNLINK;
		count = -1;
	}
	else if (!my_stricmp(comm, "killserver"))
	{
		cmd = CMDS_SERVERKILL;
		count = -1;
	}
	else if (!my_stricmp(comm, "removeserver"))
	{
		cmd = CMDS_SERVERREMOVE;
		count = -1;
	}
	else if (!my_stricmp(comm, "setuserlevel"))
	{
		cmd = CMDS_SETUSERLEVEL;
		count = 2;
	}
	else if (!my_stricmp(comm, "kill"))
	{
		cmd = CMDS_KILLUSER;
		count = 2;
	}
	else if (!my_stricmp(comm, "nukeuser"))
		cmd = CMDS_NUKEUSER;
	else if (!my_stricmp(comm, "banuser"))
		cmd = CMDS_BANUSER;
	else if (!my_stricmp(comm, "setdataport"))
	{
		cmd = CMDS_SETDATAPORT;
		count = 2;
	}
	else if (!my_stricmp(comm, "unbanuser"))
		cmd = CMDS_UNBANUSER;
	else if (!my_stricmp(comm, "banlist"))
	{
		cmd = CMDS_BANLIST;
		count = 0;
	}
	else if (!my_stricmp(comm, "muzzle"))
	{
		cmd = CMDS_MUZZLE;
		count = 2;
	}
	else if (!my_stricmp(comm, "unmuzzle"))
	{
		cmd = CMDS_UNMUZZLE;
		count = 2;
	}
	else if (!my_stricmp(comm, "unnukeuser"))
		cmd = CMDS_UNNUKEUSER;
	else if (!my_stricmp(comm, "setlinespeed"))
	{
		cmd = CMDS_SETLINESPEED;
		count = 2;
	}
	else if (!my_stricmp(comm, "opsay"))
	{
		cmd = CMDS_OPSAY;
		count = -1;
	}
	else if (!my_stricmp(comm, "announce"))
	{
		cmd = CMDS_ANNOUNCE;
		count = -1;
	}
	else 
	{
		userage(command, helparg);
		return;
	}
	switch (count)
	{
		case 0:
			send_ncommand(cmd, NULL);
			break;
		case 1:
		{
			char *user;
			if ((user = next_arg(args, &args)))
				send_ncommand(cmd, user);
			break;
		}
		case 2:
		{
			char *user;
			user = next_arg(args, &args);
			if (args && *args)
				send_ncommand(cmd, "%s %s", user, args);
			else
				send_ncommand(cmd, "%s", user);
			break;
		}
		case -1:
		{
			if (args && *args)
				send_ncommand(cmd, "%s", args);
		}
	}
#endif
}

BUILT_IN_DLL(nap_msg)
{
char *loc, *nick;
	if (!args || !*args)
		return;
	loc = LOCAL_COPY(args);
	if (!my_stricmp(command, "nmsg"))
	{
		nick = next_arg(loc, &loc);
		send_ncommand(CMDS_SENDMSG, "%s", args);
		if (do_hook(MODULE_LIST, "NAP SENDMSG %s %s", nick, loc))
			nap_put("%s", cparse(fget_string_var(FORMAT_SEND_MSG_FSET), 
				"%s %s %s %s",update_clock(GET_TIME), 
				nick, get_dllstring_var("napster_user"), loc));
	}
	else if (!my_stricmp(command, "nsay") && nap_current_channel)
		send_ncommand(CMDS_SEND, "%s %s", nap_current_channel, args);
}

BUILT_IN_DLL(stats_napster)
{
	nap_say("There are %d libraries with %d songs in %dgb", statistics.libraries, statistics.songs, statistics.gigs);
	nap_say("We are sharing %d for %4.2f%s", statistics.shared_files, _GMKv(statistics.shared_filesize), _GMKs(statistics.shared_filesize));
	nap_say("There are %d files loaded with %4.2f%s", statistics.total_files, _GMKv(statistics.total_filesize), _GMKs(statistics.total_filesize));
	nap_say("We have served %lu files and %4.2f%s", statistics.files_served, _GMKv(statistics.filesize_served), _GMKs(statistics.filesize_served));
	nap_say("We have downloaded %lu files for %4.2f%s", statistics.files_received, _GMKv(statistics.filesize_received), _GMKs(statistics.filesize_received));
	nap_say("The Highest download speed has been %4.2fK/s", _GMKv(statistics.max_downloadspeed));
	nap_say("The Highest upload speed has been %4.2fK/s", _GMKv(statistics.max_uploadspeed));
}


static void set_passwd (Window *win, char *unused, int onoff)
{
	if (unused && nap_socket != -1)
		send_ncommand(CMDS_CHANGEPASS, "%s", unused);
}

static void set_email (Window *win, char *unused, int onoff)
{
	if (unused && nap_socket != -1)
	{
		if (strchr(unused, '@') && strchr(unused, '.'))
			send_ncommand(CMDS_CHANGEEMAIL, "%s", unused);
		else
		{
			nap_say("Malformed email address");
			new_free(&unused);
		}
	}
}

static void set_linespeed (Window *win, char *unused, int onoff)
{
	if (nap_socket != -1)
	{
		if (onoff >= 0 && onoff < 11)
			send_ncommand(CMDS_CHANGESPEED, "%d", onoff);
		else
			nap_say("Bad Speed Value. 0 to 10");
	}
}
static void set_dataport (Window *win, char *unused, int onoff)
{
int	yes, 
	old_data = nap_data;
	nap_data = -1;
	if (nap_socket != -1)
	{
		if ((yes = make_listen(onoff)) != -1)
		{
			close_socketread(old_data);
			send_ncommand(CMDS_CHANGEDATA, "%d", onoff);
		}
		else
			nap_data = old_data;
	}
}

char *Nap_Version(IrcCommandDll **intp)
{
	return nap_version;
}

int Nap_Lock(IrcCommandDll **intp)
{
	return 1;
}


int Nap_Init(IrcCommandDll **intp, Function_ptr *global_table)
{
char buffer[BIG_BUFFER_SIZE+1];
char *p;
char nap_name[] = "napster";

	initialize_module(nap_name);	
	add_module_proc(COMMAND_PROC, nap_name, "napster", NULL, 0, 0, nap_link, "[-create] command to login to napster");
	add_module_proc(COMMAND_PROC, nap_name, "n", NULL, 0, 0, nap_command, "[whois] [raw] various raw commands");
	add_module_proc(COMMAND_PROC, nap_name, "nping", "nping", 0, 0, nap_command, "<nick> attempt to ping nick");
	add_module_proc(COMMAND_PROC, nap_name, "ninfo", "ninfo", 0, 0, nap_channel, "<nick> attempts to whois nick");
	add_module_proc(COMMAND_PROC, nap_name, "njoin", "njoin", 0, 0, nap_channel, "<channel> join a channel");
	add_module_proc(COMMAND_PROC, nap_name, "npart", "npart", 0, 0, nap_channel, "[channel] part a channel or current if none given");
#if 0
	add_module_proc(COMMAND_PROC, nap_name, "npass", "npass", 0, 0, nap_user, "<nick passwd> to change your passwd");
	add_module_proc(COMMAND_PROC, nap_name, "nemail", "nemail", 0, 0, nap_user, "<email> change your email address");
	add_module_proc(COMMAND_PROC, nap_name, "ndataport", "ndataport", 0, 0, nap_user, "<dataport> to change your dataport");
	add_module_proc(COMMAND_PROC, nap_name, "nlinespeed", "nlinespeed", 0, 0, nap_user, "<# from 0 to 11> to change your linespeed");
#endif
	add_module_proc(COMMAND_PROC, nap_name, "nlist", "nlist", 0, 0, nap_channel, "list all channels");

	add_module_proc(COMMAND_PROC, nap_name, "nsearch", NULL, 0, 0, nap_search, "<search string> search napster database");
	add_module_proc(COMMAND_PROC, nap_name, "nsound", "soundex", 0, 0, nap_search, "<search string> search napster database");
	add_module_proc(COMMAND_PROC, nap_name, "nmsg", "nmsg", 0, 0, nap_msg, "<nick msg> send a privmsg to nick");
	add_module_proc(COMMAND_PROC, nap_name, "nsay", "nsay", 0, 0, nap_msg, "<msg> say something in the current channel");
	add_module_proc(COMMAND_PROC, nap_name, "nscan", "nscan", 0, 0, nap_scan, "show list of current nicks in channel");
	add_module_proc(COMMAND_PROC, nap_name, "nnames", "nnames", 0, 0, nap_scan, "show list of current nicks in channel");
	add_module_proc(COMMAND_PROC, nap_name, "nconnect", "nconnect", 0, 0, nap_connect, "[server:port] connect to a specific server/port");
	add_module_proc(COMMAND_PROC, nap_name, "nreconnect", "nreconnect", 0, 0, nap_connect, "reconnect to the current server");
	add_module_proc(COMMAND_PROC, nap_name, "nbrowse", "nbrowse", 0, 0, nap_command, "<nick> browse nick's list of files");
	add_module_proc(COMMAND_PROC, nap_name, "ntopic", "ntopic", 0, 0, nap_channel, "[channel] display topic of channel or current channel");
	add_module_proc(COMMAND_PROC, nap_name, "nrequest", "nrequest", 0, 0, nap_request, "<nick file> request a specific file from nick");
	add_module_proc(COMMAND_PROC, nap_name, "nget", "nget", 0, 0, nap_request, "<# -search -browse> request the file # from the search list of the browse list default is the search list");
	add_module_proc(COMMAND_PROC, nap_name, "nglist", "nglist", 0, 0, nap_glist, "list current downloads");
	add_module_proc(COMMAND_PROC, nap_name, "ndel", "ndel", 0, 0, nap_del, "<#> delete numbered file requests");
	add_module_proc(COMMAND_PROC, nap_name, "nhotlist", "nhotlist", 0, 0, naphotlist, "<nick> Adds <nick> to your hotlist");
	add_module_proc(COMMAND_PROC, nap_name, "nignore", "nignore", 0, 0, ignore_user, "<nick(s)> ignore these nicks in public/msgs/files");
#if 0
	add_module_proc(COMMAND_PROC, nap_name, "nkill", "nkill", 0, 0, nap_admin, "<user> kills the user");
	add_module_proc(COMMAND_PROC, nap_name, "nnuke", "nnuke", 0, 0, nap_admin, "<user> nukes the user record");
	add_module_proc(COMMAND_PROC, nap_name, "nunnuke", "nunnuke", 0, 0, nap_admin, "<user> un-nukes the user record");
	add_module_proc(COMMAND_PROC, nap_name, "nban", "nban", 0, 0, nap_admin, "<user> ban the user");
	add_module_proc(COMMAND_PROC, nap_name, "nunban", "nunban", 0, 0, nap_admin, "<user> unban the user");
	add_module_proc(COMMAND_PROC, nap_name, "nsetdataport", "nsetdataport", 0, 0, nap_admin, "<user port> set the users dataport");
	add_module_proc(COMMAND_PROC, nap_name, "nsetuserlevel", "nsetuserlevel", 0, 0, nap_admin, "<user level> set the users level");
	add_module_proc(COMMAND_PROC, nap_name, "nopsay", "nopsay", 0, 0, nap_admin, "<msg> to all moderators/administrators/elite");
	add_module_proc(COMMAND_PROC, nap_name, "nannouce", "nannouce", 0, 0, nap_admin, "<msg> to all");
	add_module_proc(COMMAND_PROC, nap_name, "nbanlist", "nbanlist", 0, 0, nap_admin, "list current bans on server");
	add_module_proc(COMMAND_PROC, nap_name, "nsetlinespeed", "nsetlinespeed", 0, 0, nap_admin, "<user speed> set users line speed");
	add_module_proc(COMMAND_PROC, nap_name, "nmuzzle", "nunmuzzle", 0, 0, nap_admin, "<user> un-muzzle the user");
	add_module_proc(COMMAND_PROC, nap_name, "nmuzzle", "nunmuzzle", 0, 0, nap_admin, "<user> muzzle the user");
#endif
	add_module_proc(COMMAND_PROC, nap_name, "nadmin", "nadmin", 0, 0, nap_admin, "Various ADMIN commands");
	add_module_proc(COMMAND_PROC, nap_name, "necho", "necho", 0, 0, nap_echo, "[-x] Echo output");
	

	add_module_proc(COMMAND_PROC, nap_name, "nsave", NULL, 0, 0, napsave, "saves a Napster.sav");
	add_module_proc(COMMAND_PROC, nap_name, "nclose", NULL, 0, 0, nclose, "close the current napster connect");

	/* napsend.c */
	add_module_proc(COMMAND_PROC, nap_name, "nload", NULL, 0, 0, load_napserv, "[<dir dir | -recurse dir> scan dirs recursively -recurse to toggle");
	add_module_proc(COMMAND_PROC, nap_name, "nreload", NULL, 0, 0, load_napserv, "<dir dir | -recurse dir> scan dirs recursively -recurse to toggle");
	add_module_proc(COMMAND_PROC, nap_name, "nprint", NULL, 0, 0, print_napster, "display list of shared files");
	add_module_proc(COMMAND_PROC, nap_name, "nshare", NULL, 0, 0, share_napster, "Send list of shared files to napster server");
	add_module_proc(COMMAND_PROC, nap_name, "nstats", NULL, 0, 0, stats_napster, "Send list of shared files to napster server");


	add_module_proc(VAR_PROC, nap_name, "napster_prompt", (char *)convert_output_format("%K[%YNap%K]%n ", NULL, NULL), STR_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(VAR_PROC, nap_name, "napster_window", NULL, BOOL_TYPE_VAR, 0, toggle_napwin, NULL);

	add_module_proc(VAR_PROC, nap_name, "napster_host", "server.napster.com", STR_TYPE_VAR, 0, NULL, NULL);
#ifdef ME
	add_module_proc(VAR_PROC, nap_name, "napster_user", "qr", STR_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(VAR_PROC, nap_name, "napster_pass", "qr", STR_TYPE_VAR, 0, NULL, NULL);
#else
	add_module_proc(VAR_PROC, nap_name, "napster_user", NULL, STR_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(VAR_PROC, nap_name, "napster_pass", NULL, STR_TYPE_VAR, 0, set_passwd, NULL);
#endif
	add_module_proc(VAR_PROC, nap_name, "napster_email", "anon@napster.com", STR_TYPE_VAR, 0, set_email, NULL);
	add_module_proc(VAR_PROC, nap_name, "napster_port", NULL, INT_TYPE_VAR, 8875, NULL, NULL);
	add_module_proc(VAR_PROC, nap_name, "napster_dataport", NULL, INT_TYPE_VAR, 6699, set_dataport, NULL);
	add_module_proc(VAR_PROC, nap_name, "napster_speed", NULL, INT_TYPE_VAR, 3, set_linespeed, NULL);
	add_module_proc(VAR_PROC, nap_name, "napster_max_results", NULL, INT_TYPE_VAR, 100, NULL, NULL);
	add_module_proc(VAR_PROC, nap_name, "napster_numeric", NULL, STR_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(VAR_PROC, nap_name, "napster_download_dir", get_string_var(DCC_DLDIR_VAR), STR_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(VAR_PROC, nap_name, "napster_names_nickcolor", "%K[%w  $[12]0%K]", STR_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(VAR_PROC, nap_name, "napster_hotlist_online", "%K[%w$[12]0%K]", STR_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(VAR_PROC, nap_name, "napster_hotlist_offline", "%K[%R$[12]0%K]", STR_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(VAR_PROC, nap_name, "napster_show_numeric", NULL, BOOL_TYPE_VAR, 0, set_numeric_string, NULL);
	add_module_proc(VAR_PROC, nap_name, "napster_window_hidden", NULL, BOOL_TYPE_VAR, 0, toggle_napwin_hide, NULL);
	add_module_proc(VAR_PROC, nap_name, "napster_resume_download", NULL, BOOL_TYPE_VAR, 1, NULL, NULL);
	add_module_proc(VAR_PROC, nap_name, "napster_send_limit", NULL, INT_TYPE_VAR, 5, NULL, NULL);
	add_module_proc(VAR_PROC, nap_name, "napster_names_columns", NULL, INT_TYPE_VAR, get_int_var(NAMES_COLUMNS_VAR), NULL, NULL);
	add_module_proc(VAR_PROC, nap_name, "napster_share", NULL, INT_TYPE_VAR, 1, NULL, NULL);
	add_module_proc(VAR_PROC, nap_name, "napster_max_send_nick", NULL, INT_TYPE_VAR, 4, NULL, NULL);
	
	add_module_proc(ALIAS_PROC, nap_name, "mp3time", NULL, 0, 0, func_mp3_time, NULL);
	add_module_proc(ALIAS_PROC, nap_name, "ntopic", NULL, 0, 0, func_topic, NULL);
	add_module_proc(ALIAS_PROC, nap_name, "nonchan", NULL, 0, 0, func_onchan, NULL);
	add_module_proc(ALIAS_PROC, nap_name, "nonchannel", NULL, 0, 0, func_onchannel, NULL);
	add_module_proc(ALIAS_PROC, nap_name, "napconnected", NULL, 0, 0, func_connected, NULL);
	add_module_proc(ALIAS_PROC, nap_name, "nhotlist", NULL, 0, 0, func_hotlist, NULL);
	add_module_proc(ALIAS_PROC, nap_name, "ncurrent", NULL, 0, 0, func_napchannel, NULL);
	add_module_proc(ALIAS_PROC, nap_name, "nraw", NULL, 0, 0, func_raw, NULL);
	add_module_proc(ALIAS_PROC, nap_name, "md5", NULL, 0, 0, func_md5, NULL);

	add_module_proc(VAR_PROC, nap_name, "napster_format", NULL, STR_TYPE_VAR, 0, set_numeric_string, NULL);
	add_module_proc(VAR_PROC, nap_name, "napster_dir", NULL, STR_TYPE_VAR, 0, NULL, NULL);

	add_completion_type("nload", 4, FILE_COMPLETION);
	add_completion_type("nreload", 4, FILE_COMPLETION);

	naphelp(NULL, NULL, NULL, NULL, NULL);
	sprintf(buffer, "$0+Napster %s by panasync - $2 $3", nap_version);
	fset_string_var(FORMAT_VERSION_FSET, buffer);
	loading_global = 1;
	snprintf(buffer, BIG_BUFFER_SIZE, "%s/Napster.sav", get_string_var(CTOOLZ_DIR_VAR));
	p  = expand_twiddle(buffer);
	load("LOAD", p, empty_string, NULL);
	new_free(&p);
	loading_global = 0;

	return 0;
}
