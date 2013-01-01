/*
 * numbers.c:handles all those strange numeric response dished out by that
 * wacky, nutty program we call ircd 
 *
 *
 * written by michael sandrof
 *
 * copyright(c) 1990 
 *
 * see the copyright file, or do a help ircii copyright 
 */


#include "irc.h"
static char cvsrevision[] = "$Id: numbers.c 144 2011-10-13 12:45:27Z keaston $";
CVS_REVISION(numbers_c)
#include "struct.h"

#include "dcc.h"
#include "input.h"
#include "commands.h"
#include "ircaux.h"
#include "vars.h"
#include "lastlog.h"
#include "list.h"
#include "hook.h"
#include "server.h"
#include "who.h"
#include "numbers.h"
#include "window.h"
#include "screen.h"
#include "output.h"
#include "names.h"
#include "notify.h"
#include "whowas.h"
#include "funny.h"
#include "parse.h"
#include "misc.h"
#include "status.h"
#include "struct.h"
#include "timer.h"
#include "userlist.h"
#include "tcl_bx.h"
#include "gui.h"
#define MAIN_SOURCE
#include "modval.h"

static	void	channel_topic		(char *, char **, int);
static	void	not_valid_channel	(char *, char **);
static	void	cannot_join_channel	(char *, char **);

static	int	number_of_bans = 0;
static	int	number_of_exempts = 0;
static	char	*last_away_msg = NULL;
static	char	*last_away_nick = NULL;


#define BITCHXINFO "For more information about \002BitchX\002 type \002/about\002"

void	parse_364		(char *, char *, char *);
void	parse_365		(char *, char *, char *);
void	add_to_irc_map		(char *, char *);
void	show_server_map		(void);
int	stats_k_grep		(char **);
void	who_handlekill		(char *, char *, char *);
void	handle_tracekill	(int, char *, char *, char *);
int	no_hook_notify;
extern  AJoinList *ajoin_list;
void	remove_from_server_list (int);

char	*thing_ansi = NULL;

/*
 * numeric_banner: This returns in a static string of either "xxx" where
 * xxx is the current numeric, or "***" if SHOW_NUMBERS is OFF 
 */
char	*numeric_banner(void)
{
	static	char	thing[4];
	if (!get_int_var(SHOW_NUMERICS_VAR))
		return (thing_ansi?thing_ansi:empty_string);
	sprintf(thing, "%3.3u", -current_numeric);
	return (thing);
}

int check_sync(int comm, char *channel, char *nick, char *whom, char *bantime, ChannelList *chan)
{
ChannelList *tmp = NULL;
BanList *new;
int in_join = 0;

	if (!chan)
		if (!(tmp = lookup_channel(channel, from_server, CHAN_NOUNLINK)))
			return -1; /* channel lookup problem */

	if (tmp == NULL)
		tmp = chan;	

	in_join = in_join_list(channel, from_server);
	if (!in_join)
		return -1;

	switch (comm)
	{
		case 367: /* bans on channel */
		{
			if (tmp)
			{
				new = (BanList *)new_malloc(sizeof(BanList));
				malloc_strcpy(&new->ban, nick);
				if (bantime)
					new->time = my_atol(bantime);
				else
					new->time = now;
				if (whom)
					new->setby = m_strdup(whom);
				add_to_list((List **)&tmp->bans,(List *)new); 
			}
			break;
		}
		case 348: /* exemptbans */
		{
		        if (tmp)
			{
			        new = (BanList *)new_malloc(sizeof(BanList));
				malloc_strcpy(&new->ban, nick);
				if (bantime)
				        new->time = my_atol(bantime);
				else
				        new->time = now;
				if (whom)
				        new->setby = m_strdup(whom);
				add_to_list((List **)&tmp->exemptbans,(List *)new);
			}
			break;
		}
		default:
			break;
	}	
	return 0;
}

static int handle_stats(int comm, char *from, char **ArgList)
{
char *format_str = NULL;
char *format2_str = NULL;
char *args;
	args = PasteArgs(ArgList, 0);
	switch (comm)
	{
/*
 ^on ^raw_irc "% 211 *" {suecho \(L:line\) $[20]sklnick($3) : $4|$5|$6|$7|$8|$strip(: 9)|$strip(: $10);suecho $[8]11 $[30]sklhost($3)}
 ^on ^raw_irc "% 212 *" {suecho $[9]3: $4 $5}
 ^on ^raw_irc "% 213 *" {suecho \($3:line\) \($[20]4\)\($[20]6\) \($[5]7\)\($strip(: $[4]8)\)}
 ^on ^raw_irc "% 214 *" {suecho \($3:line\) \($[20]4\)\($[20]6\) \($[5]7\)\($strip(: $[4]8)\)}
 ^on ^raw_irc "% 215 *" {suecho \($3:line\) \($[20]4\)\($[20]6\) \($[5]7\)\($strip(: $[4]8)\)}
 ^on ^raw_irc "% 216 *" {suecho \($3:line\) \($[20]4\) \($[15]6\)}
 ^on ^raw_irc "% 218 *" {suecho \($3:line\) \($[5]4\) \($[5]5\) \($[5]6\) \($[5]7\) \($strip(: $[8]8)\)}
 ^on ^raw_irc "% 242 *" {suecho $0 has been up for $5- }
 ^on ^raw_irc "% 243 *" {suecho \($3:line\) \($[20]4\) \($[9]6\)}
 ^on ^raw_irc "% 244 *" {suecho \($3:line\) \($[25]4\) \($[25]6\)}
*/
		case 211:
			format_str = "L:line $[20]left($rindex([ $1) $1)  :$[-5]2 $[-5]3 $[-6]4 $[-5]5 $[-6]6 $[-8]strip(: $7) $strip(: $8)";
			format2_str = "$[8]9 $[30]mid(${rindex([ $1)+1} ${rindex(] $1) - $rindex([ $1) + 1} $1)";
			break;
		case 212:
			format_str = "$[10]1 $[-10]2 $[-10]3";
			break;
		case 213:
		case 214:
		case 215:
 			format_str = "$1:line $[25]2 $[25]4 $[-5]5 $[-5]6";
			break;
		case 216:
			format_str = "$1:line $[25]2 $[10]4  $5-";
			break;
		case 218:
			format_str = "$1:line $[-5]2 $[-6]3 $[-6]4 $[-6]5 $[-10]6";
			break;
		case 241:
			format_str = "$1:line $2-";
			break;
		case 242:
			format_str = "%G$0%n has been up for %W$3-";
			break;
		case 243:
			format_str = "$1:line $[25]2 $[9]4";
			break;
		case 244:
			format_str = "$1:line $[25]2 $[25]4";
		default:
			break;
	}
	if (format_str && args)
		put_it("%s", convert_output_format(format_str, "%s %s", from, args)); 
	if (format2_str && args)
		put_it("%s", convert_output_format(format2_str, "%s %s", from, args)); 
	return 0;
}

static int trace_oper(char *from, char **ArgList)
{
char *rest = PasteArgs(ArgList, 0);
	put_it("%s", convert_output_format(fget_string_var(FORMAT_TRACE_OPER_FSET), "%s %s", from, rest));
	return 0;
}

static int trace_server(char *from, char **ArgList)
{
char *rest = PasteArgs(ArgList, 0);
	put_it("%s", convert_output_format(fget_string_var(FORMAT_TRACE_SERVER_FSET), "%s %s", from, rest));
	return 0;
}

static int trace_user(char *from, char **ArgList)
{
char *rest = PasteArgs(ArgList, 0);
	put_it("%s", convert_output_format(fget_string_var(FORMAT_TRACE_USER_FSET), "%s %s", from, rest));
	return 0;
}

static int check_server_sync(char *from, char **ArgList)
{
	static char *desync = NULL;

	/* If we get a "permission denied" type numeric from a _remote_ server,
	 * then it means the network is desyched.
	 */
	if (my_stricmp(from, get_server_itsname(from_server)) && 
		(!desync || my_stricmp(desync, from)))
	{
		malloc_strcpy(&desync, from);
		if (do_hook(DESYNC_MESSAGE_LIST, "%s %s", from, ArgList[0]))
			put_it("%s", convert_output_format(fget_string_var(FORMAT_DESYNC_FSET), "%s %s %s", update_clock(GET_TIME), ArgList[0], from));
		return 1;
	} 
	return 0;
}

/*
 * display_msg: handles the displaying of messages from the variety of
 * possible formats that the irc server spits out.  you'd think someone would
 * simplify this 
 */
void display_msg(char *from, char **ArgList)
{
	char	*ptr,
		*s,
		*rest;

	rest = PasteArgs(ArgList, 0);
	if ((s = find_numeric_fset(-current_numeric)))
	{
		put_it("%s", convert_output_format(s, "%s %d %s %s", update_clock(GET_TIME), -current_numeric, from, rest));
		return;
	}
	if (from && (my_strnicmp(get_server_itsname(from_server), from,
			strlen(get_server_itsname(from_server))) == 0))
		from = NULL;
	ptr = strchr(rest, ':');
	if (ptr && ptr > rest && ptr[-1] == ' ')
		*ptr++ = 0;
	else
		ptr = NULL;
		
        put_it("%s %s%s%s%s%s%s",
                numeric_banner(),
                strlen(rest)        ? rest     : empty_string,
                strlen(rest)        ? space    : empty_string,
                ptr                 ? ptr      : empty_string,
                from                ? "(from " : empty_string,
                from                ? from     : empty_string,
                from                ? ")"      : empty_string
              );
}

static	void channel_topic(char *from, char **ArgList, int what)
{
	char	*topic, *channel;
	ChannelList *chan;
	
	
	if (ArgList[1] && is_channel(ArgList[0]))
	{
		channel = ArgList[0];
		topic = ArgList[1];
		set_display_target(channel, LOG_CRAP);
		if (what == 333 && ArgList[2])
			put_it("%s", convert_output_format(fget_string_var(FORMAT_TOPIC_SETBY_FSET), "%s %s %s %l", update_clock(GET_TIME), channel, topic, my_atol(ArgList[2])));
		else if (what != 333)
		{
			if ((chan = lookup_channel(channel, from_server, 0)))
				malloc_strcpy(&chan->topic, topic);
			put_it("%s", convert_output_format(fget_string_var(FORMAT_TOPIC_FSET), "%s %s %s", update_clock(GET_TIME), channel, topic));

		}		
	}
	else
	{
		PasteArgs(ArgList, 0);
		set_display_target(NULL, LOG_CURRENT);
		put_it("%s", convert_output_format(fget_string_var(FORMAT_TOPIC_FSET), "%s %s", update_clock(GET_TIME), ArgList[0]));
	}
}

static	void not_valid_channel(char *from, char **ArgList)
{
	char	*channel;
	
	if (!(channel = ArgList[0]) || !ArgList[1])
		return;
	PasteArgs(ArgList, 1);
	if (!my_stricmp(from, get_server_itsname(from_server)))
	{
		if (strcmp(channel, "*"))
			remove_channel(channel, from_server);
		put_it("%s", convert_output_format(fget_string_var(FORMAT_SERVER_MSG2_FSET), "%s %s %s", update_clock(GET_TIME), channel, ArgList[1]));
	}
}

/* from ircd .../include/numeric.h */
/*
#define ERR_CHANNELISFULL    471
#define ERR_INVITEONLYCHAN   473
#define ERR_BANNEDFROMCHAN   474
#define ERR_BADCHANNELKEY    475
#define ERR_BADCHANMASK      476
*/
static	void cannot_join_channel(char *from, char **ArgList)
{
	char	buffer[BIG_BUFFER_SIZE + 1];
	char	*f = NULL;
	char	*chan;		
	

	if (ArgList[0])
		chan = ArgList[0];
	else
		chan = get_chan_from_join_list(from_server);

	remove_from_join_list(chan, from_server);
	if (!is_on_channel(chan, from_server, get_server_nickname(from_server)))
		remove_channel(chan, from_server);
	else
		return;

	set_display_target(chan, LOG_CURRENT);
	PasteArgs(ArgList, 0);
	strlcpy(buffer, ArgList[0], sizeof buffer);
	switch(-current_numeric)
	{
		case 437: 
			strlcat(buffer, 
				" (Channel is temporarily unavailable)",
				sizeof buffer);
			break;
		case 471:
			strlcat(buffer, " (Channel is full)", sizeof buffer);
			break;
		case 473:
			strlcat(buffer, " (You must be invited)", 
				sizeof buffer);
			break;
		case 474:
			strlcat(buffer, " (You are banned)", sizeof buffer);
			break;
		case 475:
			strlcat(buffer, " (Bad channel key)", sizeof buffer);
			break;
		case 476:
			strlcat(buffer, " (Bad channel mask)", sizeof buffer);
			break;
		default:
			return;
	}
	if ((f = find_numeric_fset(-current_numeric)))
		put_it("%s", convert_output_format(f, "%s %s", update_clock(GET_TIME), ArgList[0]));
	else
		put_it("%s %s", numeric_banner(), buffer);
	reset_display_target();
}

/* divide_rounded()
 *
 * Implements an integer division of a / b, where 1.5 rounds up to 2 and 
 * 1.49 rounds down to 1. Returns zero if divisor is zero.
 */
static int divide_rounded(int a, int b)
{
	if (a < 0)
	{
		a = -a;
		b = -b;
	}

	return b ? (a + abs(b / 2)) / b : 0;
}

int handle_server_stats(char *from, char **ArgList, int comm)
{
static	int 	norm = 0, 
		invisible = 0, 
		services = 0,
		servers = 0, 
		ircops = 0, 
		unknown = 0, 
		chans = 0, 
		local_users = 0,
		total_users = 0,
		services_flag = 0;
	int	ret = 1;
	char	*line;

	line = LOCAL_COPY(ArgList[0]);
	switch(comm)
	{
		case 251: /* number of users */
			BreakArgs(line, NULL, ArgList, 1);
			if (ArgList[2] && ArgList[5] && ArgList[8])
			{
				servers = ircops = unknown = chans = 0;
				norm = my_atol(ArgList[2]);
				if ((services_flag = my_stricmp(ArgList[6], "services"))) 
				    invisible = my_atol(ArgList[5]); 
				else 
				    services = my_atol(ArgList[5]);
				servers = my_atol(ArgList[8]);
			}
			break;
		case 252: /* number of ircops */
			if (ArgList[0])
				ircops = my_atol(ArgList[0]);
			break;
		case 253: /* number of unknown */
			if (ArgList[0])
				unknown = my_atol(ArgList[0]);
			break;
			
		case 254: /* number of channels */
			if (ArgList[0])
				chans = my_atol(ArgList[0]);
			break;
		case 255: /* number of local print it out */
			BreakArgs(line, NULL, ArgList, 1);
			if (ArgList[2])
				local_users = my_atol(ArgList[2]);
			total_users = norm + invisible;
			if (total_users)
			{
				put_it("%s", convert_output_format("$G %K[%nlocal users on irc%K(%n\002$0\002%K)]%n $1%%", 
					"%d %d", local_users, divide_rounded(local_users * 100, total_users)));
				put_it("%s", convert_output_format("$G %K[%nglobal users on irc%K(%n\002$0\002%K)]%n $1%%",
					"%d %d", norm, divide_rounded(norm * 100, total_users)));
				if (services_flag)
				{
					put_it("%s", convert_output_format("$G %K[%ninvisible users on irc%K(%n\002$0\002%K)]%n $1%%",
						 "%d %d", invisible, divide_rounded(invisible * 100, total_users)));
				}
				else
					put_it("%s", convert_output_format("$G %K[%nservices on irc%K(%n\002$0\002%K)]%n", 
						"%d", services));
				put_it("%s", convert_output_format("$G %K[%nircops on irc%K(%n\002$0\002%K)]%n $1%%", 
					"%d %d", ircops, divide_rounded(ircops * 100, total_users)));
			}
			put_it("%s", convert_output_format("$G %K[%ntotal users on irc%K(%n\002$0\002%K)]%n", 
				"%d", total_users));
			put_it("%s", convert_output_format("$G %K[%nunknown connections%K(%n\002$0\002%K)]%n", 
				"%d", unknown));

			put_it("%s", convert_output_format(
				"$G %K[%ntotal servers on irc%K(%n\002$0\002%K)]%n %K(%navg. $1 users per server%K)", 
				"%d %d", servers, divide_rounded(total_users, servers)));

			put_it("%s", convert_output_format(
				"$G %K[%ntotal channels created%K(%n\002$0\002%K)]%n %K(%navg. $1 users per channel%K)",
				"%d %d", chans, divide_rounded(total_users, chans)));
			break;
		case 250:
		{
			char *p;
			BreakArgs(line, NULL, ArgList, 1);
			if (ArgList[3] && ArgList[4])
			{
				p = ArgList[4]; p++;
				put_it("%s", convert_output_format("$G %K[%nHighest client connection count%K(%n\002$0\002%K) %K(%n\002$1\002%K)]%n", "%s %s", ArgList[3], p));
			}
			break;
		}
		default:
			ret = 0;
			break;
	}
	return ret;
}

void whohas_nick (UserhostItem *stuff, char *nick, char *args)
{
	if (!stuff || !stuff->nick || !nick || !strcmp(stuff->user, "<UNKNOWN>"))
		return;
	put_it("%s", convert_output_format("$G Your nick %R[%n$0%R]%n is owned by %W$1@$2", "%s %s %s", nick, stuff->user, stuff->host));
	return;
}

void change_alt_nick(int server, char *nick)
{
char *t = NULL;
	nick_command_is_pending(server, 0);
	if (nick && (t = get_string_var(ALTNICK_VAR)))
	{ 
		if (!my_stricmp(t, nick))		
			fudge_nickname(server, 0);
		else
			change_server_nickname(server, t);
	} else
#if 0
  		if (get_int_var(AUTO_NEW_NICK_VAR))
  			fudge_nickname(from_server);
  		else
			reset_nickname(from_server);
#endif
		fudge_nickname(server, 0);
}

/*
 * numbered_command: does (hopefully) the right thing with the numbered
 * responses from the server.  I wasn't real careful to be sure I got them
 * all, but the default case should handle any I missed (sorry) 
 */
void numbered_command(char *from, int comm, char **ArgList)
{
	char	*user;
	char	none_of_these = 0;
	int	flag;
	int	old_current_numeric = current_numeric;
	AJoinList *tmp = NULL;

	
	if (!ArgList[1] || !from || !*from)
		return;
	
	user = (*ArgList[0]) ? ArgList[0] : NULL;

	reset_display_target();

	ArgList++;
	current_numeric = -comm;	/* must be negative of numeric! */

	switch (comm)
	{
	case 1:	/* #define RPL_WELCOME          001 */
	{		
		char *s = NULL;
		bitchsay(BITCHXINFO);
		set_server2_8(from_server, 1);
		accept_server_nickname(from_server, user);
		set_server_motd(from_server, 1);
 		server_is_connected(from_server, 1);
		PasteArgs(ArgList, 0);
		userhostbase(NULL, got_my_userhost, 1, NULL);
		if (do_hook(current_numeric, "%s %s", from, *ArgList)) 
			display_msg(from, ArgList);
		if (send_umode && *send_umode == '+')
			send_to_server("MODE %s %s", user, send_umode);
		if ((s = get_server_cookie(from_server)))
			send_to_server("COOKIE %s", s);
		break;
	}		
	case 4:	/* #define RPL_MYINFO           004 */
	{
		got_initial_version_28(ArgList);
		load_scripts();
		PasteArgs(ArgList, 0);
		if (do_hook(current_numeric, "%s %s", from, *ArgList))
			display_msg(from, ArgList);
		/* if the server kicks us, then we maybe should reconnect to 
		 * all our previous channels.
		 */
		if (1/*get_int_var(AUTO_REJOIN_VAR)*/)
		{
			char *channels = NULL, *keys = NULL;
			for (tmp = ajoin_list; tmp; tmp = tmp->next)
			{
				if(!tmp->group || is_server_valid(tmp->group, from_server))
				{
					if (!in_join_list(tmp->name, from_server))
					{
						if ((get_server_version(from_server) > Server2_8hybrid))
							send_to_server("JOIN %s%s%s", tmp->name, tmp->key ? space : empty_string, tmp->key ? tmp->key : empty_string);
						else
						{
							m_s3cat(&channels, ",", tmp->name);
							m_s3cat(&keys, ",", tmp->key?tmp->key:empty_string);
						}
						if (get_int_var(JOIN_NEW_WINDOW_VAR))
						{
							WhowasChanList *bleah = check_whowas_chan_buffer(tmp->name, -1, 0);
							if (bleah)
							{
								if (bleah->channellist)
									add_to_join_list(tmp->name, from_server, bleah->channellist->refnum);
							}
							else
							{
								if (tmp != ajoin_list)
									win_create(JOIN_NEW_WINDOW_VAR, 1);
								add_to_join_list(tmp->name, from_server, current_window->refnum);
							}
						}
					}
				}
			}
			if (channels)
				send_to_server("JOIN %s %s", channels, keys?keys:empty_string);
			new_free(&channels); new_free(&keys);
		}
		break;
	}
	case 5:
	{
		char *p;
		PasteArgs(ArgList, 0);
		if ((p = strstr(*ArgList, "WATCH=")))
		{
			p = strchr(p, '=');
			if (p && *p && *(p+1))
			{
				p++;
				set_server_watch(from_server, my_atol(p));
				send_watch(from_server);
			}
		}
		if (do_hook(current_numeric, "%s %s", from, *ArgList))
			display_msg(from, ArgList);
		break;
	}
	case 8:
	{
		set_server_cookie(from_server, *ArgList);
		if (do_hook(current_numeric, "%s %s", from, *ArgList))
			display_msg(from, ArgList);
		break;
	}
	case 250:
	case 251:
	case 252:
	case 253:
	case 254:
	case 255:
	{
		if (do_hook(current_numeric, "%s %s", from, *ArgList))
		{
			
			if (!handle_server_stats(from, ArgList, comm))
				display_msg(from, ArgList);
		}
		break;
	}
	case 301:		/* #define RPL_AWAY             301 */
	{
		PasteArgs(ArgList, 1);
		if (get_int_var(SHOW_AWAY_ONCE_VAR))
		{
			if (!last_away_msg || strcmp(last_away_nick, from) || strcmp(last_away_msg, ArgList[1]))
			{
				malloc_strcpy(&last_away_nick, from);
				malloc_strcpy(&last_away_msg, ArgList[1]);
			}
			else break;
		}
		if (do_hook(current_numeric, "%s %s", ArgList[0], ArgList[1]))
			put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_AWAY_FSET),"%s %s", ArgList[0], ArgList[1]));
		break;
	}
	case 302:		/* #define RPL_USERHOST         302 */
	{
		userhost_returned(from, ArgList);
		break;
	} 
	case 303:		/* #define RPL_ISON             303 */
	{
		ison_returned(from, ArgList);
		break;
	}
	case 307: /* WHOIS_REGGED */
	{
		if (do_hook(current_numeric, "%s %s %s", from, ArgList[0], ArgList[1])) 
			put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_REGISTER_FSET),"%s %s", ArgList[0], " is a registered nick"));
		break;
	}
	case 295:
	case 308: /* WHOIS_ADMIN */
	{
		if (do_hook(current_numeric, "%s %s %s", from, ArgList[0], ArgList[1])) 
			put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_ADMIN_FSET),"%s %s", ArgList[0], " is an IRC Server Administrator"));

		break;
	}
	case 309: /* WHOIS_SERVICES */
	{
		if (do_hook(current_numeric, "%s %s %s", from, ArgList[0], ArgList[1])) 
			put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_SERVICE_FSET),"%s %s", ArgList[0], " is a Services Administrator"));
		break;
	}
#ifdef WANT_CHATNET
	case 310: /* RPL_WHOISUSER_OPER [or something] 310 on chatnet .. fixed. =] */
	{
		PasteArgs(ArgList, 7);
		reset_display_target();
		if(do_hook(current_numeric, "%s %s %s %s %s %s %s", ArgList[0],
				ArgList[1], ArgList[2], ArgList[3], ArgList[4], ArgList[5], ArgList[6])) {
			char *host = NULL;
			char *p;
			char *ident = NULL;
			char *userhost = NULL;
#ifdef WANT_USERLIST
			UserList *tmp = NULL;
			ShitList *tmp1 = NULL;
#endif
			/* routine for getting ident and host 
			 * by-tor really wrote this little routine, 
			 * something else he did for me, thanks =] 
			 */               
                        if ((p = (char *)strchr(ArgList[2], '@'))) {
                                        *p++ = '\0';
                                        ident = (char *)alloca(strlen(ArgList[2])+1);
                                        strcpy(ident, ArgList[2]);
                                        for(;*p == ' ';)
                                                p++;
                                        host = p;
                                        if ((p = (char *)strchr(p, '\n')))
                                                *p = '\0';
                                        p = host;
                                        host = (char *)alloca(strlen(p)+1);
                                        strcpy(host, p);
			}

			put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_HEADER_FSET), NULL));
			put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_NICK_FSET),"%s %s %s %s", ArgList[0], ident, host, country(host)));
			put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_NAME_FSET),"%s %s", ArgList[4], ArgList[5]));			
			malloc_sprintf(&userhost, "%s@%s", ident, host);
#ifdef WANT_USERLIST
			if((tmp=lookup_userlevelc("*", userhost, "*", NULL)))
 				put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_FRIEND_FSET), "%s %s", convert_flags_to_str(tmp->flags), tmp->host));
			if((tmp1=nickinshit(ArgList[0], userhost)))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_SHIT_FSET),"%d %s %s %s", tmp1->level, tmp1->channels, tmp1->filter, tmp1->reason));
			if(tmp||tmp1)
				put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_CHANNELS_FSET), "%s %s", tmp?"Allowed:":"Not-Allowed:", tmp?tmp->channels:tmp1->channels));
#endif
			new_free(&userhost);
		}
		break;													
	}
#else
	case 310: /* WHOIS_HELPFUL */
	{
		if (do_hook(current_numeric, "%s %s %s", from, ArgList[0], ArgList[1])) 
			put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_HELP_FSET),"%s %s", ArgList[0], ArgList[1]?ArgList[1]:" looks very helpful."));
		break;
		/* the 4 above are dalnet numerics */
	}
#endif
	case 311:		/* #define RPL_WHOISUSER        311 */
	{
		char *host = ArgList[2];
		char *u1 = ArgList[1];
		PasteArgs(ArgList, 4);
		
		reset_display_target();
		if (do_hook(current_numeric, "%s %s %s %s %s %s", from, ArgList[0],
				ArgList[1], host, ArgList[3], ArgList[4])) {
			char *userhost = NULL;
#ifdef WANT_USERLIST
			UserList *tmp = NULL;
			ShitList *tmp1 = NULL;
#endif
			put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_HEADER_FSET), NULL));
			put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_NICK_FSET),"%s %s %s %s", ArgList[0], ArgList[1], host, country(host)));
			put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_NAME_FSET),"%s", ArgList[4]));
			malloc_sprintf(&userhost, "%s@%s", u1, host);
#ifdef WANT_USERLIST
			if ((tmp = lookup_userlevelc("*", userhost, "*", NULL)))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_FRIEND_FSET), "%s %s", convert_flags_to_str(tmp->flags), tmp->host));
			if ((tmp1 = nickinshit(ArgList[0], userhost)))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_SHIT_FSET),"%d %s %s %s", tmp1->level, tmp1->channels, tmp1->filter, tmp1->reason));
			if (tmp || tmp1)
				put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_CHANNELS_FSET),"%s %s", tmp?"Allowed:":"Not-Allowed:", tmp?tmp->channels:tmp1->channels));
#endif
			new_free(&userhost);
		}

		/* Make sure we don't hide the /away message */
		new_free(&last_away_msg);
		break;
	}
	case 312:		/* #define RPL_WHOISSERVER      312 */
	{
		if (do_hook(current_numeric, "%s %s %s %s", from, ArgList[0], ArgList[1], ArgList[2]))
			put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_SERVER_FSET),"%s %s", ArgList[1], ArgList[2]));
		break;
	}
	case 313:		/* #define RPL_WHOISOPERATOR    313 */
	{
		PasteArgs(ArgList, 1);
		if (do_hook(current_numeric, "%s %s %s", from, ArgList[0], ArgList[1])) 
			put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_OPER_FSET),"%s %s", ArgList[0], " (is \002NOT\002 an IRC warrior)"));
		break;
	}
	case 314:		/* #define RPL_WHOWASUSER       314 */
	{
		PasteArgs(ArgList, 4);
		reset_display_target();
		if (do_hook(current_numeric, "%s %s %s %s %s %s", from, ArgList[0],
				ArgList[1], ArgList[2], ArgList[3], ArgList[4]))
		{
			put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOWAS_HEADER_FSET),NULL));
			put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOWAS_NICK_FSET),"%s %s %s", ArgList[0], ArgList[1], ArgList[2]));
			put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_NAME_FSET),"%s", ArgList[4]));
		}
		break;
	}
	case 315:		/* #define RPL_ENDOFWHO         315 */
	{
		PasteArgs(ArgList, 1);
		who_end(from, ArgList);
		break;
	}

	case 316:		/* #define RPL_WHOISCHANOP      316 */
		break;

	case 317:		/* #define RPL_WHOISIDLE        317 */
	{
		char	*nick, *idle_str, *startup_str;
		time_t	startup;

		nick = ArgList[0];
		idle_str = ArgList[1];
		startup_str = ArgList[2];

		if (ArgList[3])	/* undernet */
		{
			PasteArgs(ArgList, 3);
			if (nick && idle_str && do_hook(current_numeric, "%s %s %s %s %s",
						from, nick, idle_str, startup_str, ArgList[3]))
			{
				int seconds = 0;
				int hours = 0;
				int minutes = 0;
				int secs = my_atol(idle_str);
				hours = secs / 3600;
				minutes = (secs - (hours * 3600)) / 60;
				seconds = secs % 60;
				startup = my_atol(startup_str);
				put_it("%s", 
					convert_output_format(
						fget_string_var(FORMAT_WHOIS_IDLE_FSET),
						"%d %d %d %s",hours, minutes, 
						seconds, ArgList[2]?ArgList[2]:empty_string, 
						startup!=0?my_ctime(startup):empty_string));
			}
		}
		else	/* efnet */
		{
			PasteArgs(ArgList, 2);
			if (nick && idle_str && 
			    do_hook(current_numeric, "%s %s %s %s", from, nick, idle_str, ArgList[2]?ArgList[2]:empty_string))
			{
				int seconds = 0;
				int hours = 0;
				int minutes = 0;
				int secs = my_atol(idle_str);
				hours = secs / 3600;
				minutes = (secs - (hours * 3600)) / 60;
				seconds = secs % 60;
				put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_IDLE_FSET),"%d %d %d %s",hours, minutes, seconds, ArgList[2]?ArgList[2]:empty_string));
			}
		}

		break;
	}
	case 318:		/* #define RPL_ENDOFWHOIS       318 */
	{
		PasteArgs(ArgList, 0);
		if (do_hook(current_numeric, "%s %s", from, ArgList[0]) && fget_string_var(FORMAT_WHOIS_FOOTER_FSET))
			put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_FOOTER_FSET), NULL));

		break;
	}
	case 319:		/* #define RPL_WHOISCHANNELS    319 */
	{
		PasteArgs(ArgList, 1);
		if (do_hook(current_numeric, "%s %s %s", from, ArgList[0], ArgList[1]))
			put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_CHANNELS_FSET),"%s", ArgList[1]));

		break;
	}
	case 321:		/* #define RPL_LISTSTART        321 */
	{
		ArgList[0] = "Channel\0Users\0Topic";
		ArgList[1] = ArgList[0] + 8;
		ArgList[2] = ArgList[1] + 6;
		ArgList[3] = NULL;
		funny_list(from, ArgList);
		break;
	}
	
	case 322:		/* #define RPL_LIST             322 */
	{
		funny_list(from, ArgList);
		break;
	}
	case 324:		/* #define RPL_CHANNELMODEIS    324 */
	{
		funny_mode(from, ArgList);
		break;
	}

	case 338:		/* #define RPL_WHOISACTUALLY    338 (hybrid, ratbox, bahamut) */
	case 378:		/* #define RPL_WHOISHOST        378 (unreal, freenode) */
	{
		if (ArgList[2])
		{
			/* hybrid / ratbox: <nick> <ip> :actually using host */
			if (do_hook(current_numeric, "%s %s %s %s", from, ArgList[0], ArgList[1], ArgList[2]))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_ACTUALLY_FSET),"%s %s %s", ArgList[0], ArgList[2], ArgList[1]));
		}
		else
		{
			/* Bahamut: <nick> :is actually user@host [ip] */
			if (do_hook(current_numeric, "%s %s %s", from, ArgList[0], ArgList[1]))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_ACTUALLY_FSET),"%s %s", ArgList[0], ArgList[1]));
		}
		break;
	}
	case 340:		/* #define RPL_INVITING_OTHER	340 */
	{
		if (ArgList[2])
		{
			set_display_target(ArgList[0], LOG_CRAP);
			if (do_hook(current_numeric, "%s %s %s %s", from, ArgList[0], ArgList[1], ArgList[2]))
				put_it("%s %s has invited %s to channel %s", numeric_banner(), ArgList[0], ArgList[1], ArgList[2]);
			reset_display_target();
		}
		break;
	}

	case 341:		/* #define RPL_INVITING         341 */
	{
		if (ArgList[1])
		{
			set_display_target(ArgList[1], LOG_CRAP);
			if (do_hook(current_numeric, "%s %s %s", from, ArgList[0], ArgList[1]))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_INVITE_USER_FSET), "%s %s %s", update_clock(GET_TIME), ArgList[0], ArgList[1]));
			reset_display_target();
		}
		break;
	}
	case 348:
	{
		number_of_exempts++;
		if (check_sync(comm, ArgList[0], ArgList[1], ArgList[2], ArgList[3], NULL) == 0)
			break;
		if (ArgList[2])
		{
			time_t tme = (time_t) my_atol(ArgList[3]);
			if (do_hook(current_numeric, "%s %s %s %s %s", 
				from, ArgList[0], ArgList[1], ArgList[2], ArgList[3]))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_BANS_FSET), "%d %s %s %s %l", number_of_exempts, ArgList[0], ArgList[1], ArgList[2], tme));
		}
		else
			if (do_hook(current_numeric, "%s %s %s", from, ArgList[0], ArgList[1]))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_BANS_FSET), "%d %s %s %s %l", number_of_exempts, ArgList[0], ArgList[1], "unknown", now));
		break;
	}
	case 349:
	{
		int flag = 1;
		char *tmp = NULL, *chan = NULL;
		PasteArgs(ArgList, 1);
		set_display_target(ArgList[0], LOG_CURRENT);
		flag = do_hook(current_numeric, "%s %s %s", from, ArgList[0], ArgList[1]);
		reset_display_target();
		malloc_strcpy(&tmp, ArgList[0]);
		if (!(chan = strtok(tmp,space)))
			break;
                               		
		if (!in_join_list(chan, from_server))
		{
			PasteArgs(ArgList, 0);
			if (get_int_var(SHOW_END_OF_MSGS_VAR) && flag)
				display_msg(from, ArgList);
		} else
			got_info(chan, from_server, GOTEXEMPT);
		new_free(&tmp);
		number_of_exempts = 0;
		break;
	}
	case 352:		/* #define RPL_WHOREPLY         352 */
	{
		whoreply(NULL, ArgList);
		break;
	}
	case 353:		/* #define RPL_NAMREPLY         353 */
	{
		funny_namreply(from, ArgList);
		break;
	}
	case 366:		/* #define RPL_ENDOFNAMES       366 */
	{
		int	flag = 1;
		char	*tmp = NULL,
			*chan;
		PasteArgs(ArgList, 1);
		set_display_target(ArgList[0], LOG_CURRENT);
		flag = do_hook(current_numeric, "%s %s %s", from, ArgList[0], ArgList[1]);
		reset_display_target();
		malloc_strcpy(&tmp, ArgList[0]);
		if (!(chan = strtok(tmp,space)))
			break;
                               		
		if (!in_join_list(chan, from_server))
		{
			PasteArgs(ArgList, 0);
			if (get_int_var(SHOW_END_OF_MSGS_VAR) && flag)
				display_msg(from, ArgList);
		} else
			got_info(chan,from_server, GOTNAMES);
		new_free(&tmp);
	}
	break;

	case 381: 		/* #define RPL_YOUREOPER        381 */
	{
		PasteArgs(ArgList, 0);
		if (!is_server_connected(from_server))
		{
			say("Odd Server stuff from %s: %s",from, ArgList[0]);
			break;
		}
		if (do_hook(current_numeric, "%s %s", from, *ArgList))
			display_msg(from, ArgList);
		set_server_ircop_flags(from_server, -1);
		set_server_operator(from_server, 1);
		if (get_string_var(OPER_MODES_VAR))
			send_to_server("MODE %s %s", get_server_nickname(from_server), get_string_var(OPER_MODES_VAR));
		break;
	}
	case 401:		/* #define ERR_NOSUCHNICK       401 */
	{
		int foo;
		PasteArgs(ArgList, 1);
		if (check_dcc_list(ArgList[0]))
			break;
		notify_mark(ArgList[0], from, 0, 0);
		if ((foo = get_int_var(AUTO_WHOWAS_VAR)))
		{
			if (foo > -1)
				send_to_server("WHOWAS %s %d", ArgList[0], foo);
			else
				send_to_server("WHOWAS %s", ArgList[0]);
		}
		else if (do_hook(current_numeric, "%s %s %s", from, ArgList[0], ArgList[1]))
			put_it("%s", convert_output_format(fget_string_var(FORMAT_NONICK_FSET), "%s %s %s %s",update_clock(GET_TIME), ArgList[0], from, ArgList[1]));

		break;
	}
	case 421:		/* #define ERR_UNKNOWNCOMMAND   421 */
	{
		if (check_server_redirect(ArgList[0]))
			break;
		if (check_wait_command(ArgList[0]))
			break;
		PasteArgs(ArgList, 0);
		
		if ((flag = do_hook(current_numeric, "%s %s", from, *ArgList)))
			display_msg(from, ArgList);
		break;
	}
	case 432:		/* #define ERR_ERRONEUSNICKNAME 432 */
	{
		PasteArgs(ArgList, 0);
		if (do_hook(current_numeric, "%s %s", from, *ArgList))
			display_msg(from, ArgList);
		if (!is_server_connected(from_server))
			change_alt_nick(from_server, NULL);
		break;
	}
	case 437:
	{
		/*
		 * Since 437 is being used for channel collisions (channel
		 * creation delay), we have to make sure that this numeric 
		 * is for us and not for the channel we just joined.
		 */
		if ((*ArgList[0] == '#') || (*ArgList[0] == '&'))
		{
			if (get_server_nickname(from_server))
			{
				cannot_join_channel(from, ArgList);
				break;
			}
		}		
	}
	case 433:
	case 438:		/* #define ERR_NICKNAMEINUSE    433 */ 
	case 453:
	{
		if (get_server_orignick(from_server))
			check_orig_nick(NULL);
		/* for orignick */
		clean_server_queues(from_server);

		if (is_server_connected(from_server))
			userhostbase(ArgList[0], whohas_nick, 1, "%s", ArgList[0]);
		else
		{
			change_alt_nick(from_server, ArgList[0]);
			PasteArgs(ArgList, 0);
			if (do_hook(current_numeric, "%s %s", never_connected ? "-1" : from, *ArgList))
				display_msg(from, ArgList);
		}
		break;
	}
	case 451:		/* #define ERR_NOTREGISTERED    451 */
	{
	/*
	 * Sometimes the server doesn't catch the USER line, so
	 * here we send a simplified version again  -lynx 
	 */
/*		fudge_nickname(from_server, 1);*/
		register_server(from_server, NULL);

		PasteArgs(ArgList, 0);
		if (do_hook(current_numeric, "%s %s", from, *ArgList))
			display_msg(from, ArgList);
		break;

	}
	case 462:		/* #define ERR_ALREADYREGISTRED 462 */
	{
		PasteArgs(ArgList, 0);
		if (do_hook(current_numeric, "%s %s", from, *ArgList))
			display_msg(from, ArgList);
		break;
	}
	case 463:  /* CDE added for ERR_NOPERMFORHOST */
	{
		PasteArgs(ArgList, 0);
		if (do_hook(current_numeric, "%s %s", from, *ArgList))
			display_msg(from, ArgList);
		close_server(from_server, empty_string);
		window_check_servers(from_server);
		if (from_server == primary_server)
			get_connected(from_server + 1, from_server);
		break;
	}
	case 464:		/* #define ERR_PASSWDMISMATCH   464 */
	{
		PasteArgs(ArgList, 0);

		if (!is_server_open(from_server))
			break; 

		flag = do_hook(current_numeric, "%s %s", from, ArgList[0]);

		/* If we get this numeric before registering, it means that 
		 * our server password was wrong - so reprompt and reconnect.
		 *
		 * However if we're already registered, the server is trying to
		 * tell us something else (usually, OPER password wrong) so just
		 * pass it on to the user.
		 */
		if (is_server_connected(from_server))
		{
			if (flag)
				display_msg(from, ArgList);
			oper_command = 0;
		}
		else
		{
			static char	server_num[8];

			say("Password required for connection to server %s",
				get_server_name(from_server));
			close_server(from_server, empty_string);
			if (!dumb_mode)
			{
				strcpy(server_num, ltoa(from_server));
				add_wait_prompt("Server Password:", 
					password_sendline, server_num, 
					WAIT_PROMPT_LINE, 0);
			}
		}
		break;
	}
	case 465:		/* #define ERR_YOUREBANNEDCREEP 465 */
	{
		int klined = from_server;

		PasteArgs(ArgList, 0);
		if (!is_server_open(from_server))
			break;
		if (do_hook(current_numeric, "%s %s", from, ArgList[0]))
			display_msg(from, ArgList);
				
		close_server(from_server, empty_string);
		window_check_servers(from_server);
		if (server_list_size() > 1)
			remove_from_server_list(klined);
		if (klined == primary_server && (server_list_size() > 0))
			get_connected(klined, from_server);
		break;
	}

	case 484:
	{
		if (do_hook(current_numeric, "%s %s", from, ArgList[0]))
			display_msg(from, ArgList);
		set_server_flag(from_server, USER_MODE_R, 1);
		break;
	}
	case 367:
	{
		number_of_bans++;
		if (check_sync(comm, ArgList[0], ArgList[1], ArgList[2], ArgList[3], NULL) == 0)
			break;
		if (ArgList[2])
		{
			time_t tme = (time_t) my_atol(ArgList[3]);
			if (do_hook(current_numeric, "%s %s %s %s %s", 
				from, ArgList[0], ArgList[1], ArgList[2], ArgList[3]))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_BANS_FSET), "%d %s %s %s %l", number_of_bans, ArgList[0], ArgList[1], ArgList[2], tme));
		}
		else
			if (do_hook(current_numeric, "%s %s %s", from, ArgList[0], ArgList[1]))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_BANS_FSET), "%d %s %s %s %l", number_of_bans, ArgList[0], ArgList[1], "unknown", now));
		break;
	}
	case 368:		/* #define RPL_ENDOFBANLIST     368 */
	{
		if (!got_info(ArgList[0], from_server, GOTBANS))
			break;

		if (get_int_var(SHOW_END_OF_MSGS_VAR))
		{
			if (do_hook(current_numeric, "%s %d %s", from, number_of_bans, *ArgList))
			{
				put_it("%s Total Number of Bans on %s [%d]",
					numeric_banner(), ArgList[0],
					number_of_bans);
			}
		}
		number_of_bans = 0;
		break;
	}
	case 364:
	{
		if (get_int_var(LLOOK_VAR) && (get_server_linklook(from_server) == 1)) 
			parse_364(ArgList[0], ArgList[1], ArgList[2] ? ArgList[2] : zero);
		else if (get_server_linklook(from_server) == 2)	
			add_to_irc_map(ArgList[0], ArgList[2]);
		else if ((do_hook(current_numeric, "%s %s %s", ArgList[0], ArgList[1], ArgList[2] ? ArgList[2] : zero)))
		{
			if (fget_string_var(FORMAT_LINKS_FSET))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_LINKS_FSET), "%s %s %s", ArgList[0], ArgList[1], ArgList[2] ? ArgList[2] : zero));
		}
		break;
	}
	case 471:		/* #define ERR_CHANNELISFULL    471 */
	case 473:		/* #define ERR_INVITEONLYCHAN   473 */
	case 474:		/* #define ERR_BANNEDFROMCHAN   474 */
	case 475: 		/* #define ERR_BADCHANNELKEY    475 */
	case 476:		/* #define ERR_BADCHANMASK      476 */
		cannot_join_channel(from, ArgList);
		break;
	case 600: /* watch logon */
	case 601: /* watch logoff */
		show_watch_notify(from, comm == 600 ? 1 : 0, ArgList);
		break;	
	case 602: /* watchoff */
	case 603: /* watchstat */
		PasteArgs(ArgList, 0);
		if (do_hook(current_numeric, "%s %s", from, *ArgList))
			display_msg(from, ArgList);
		break;
	case 604: /* user online */
	case 605: /* user offline */
		show_watch_notify(from, comm == 604 ? 1 : 0, ArgList);
		break;
	case 606: /* watchlist */
		PasteArgs(ArgList, 0);
		if (do_hook(current_numeric, "%s %s", from, *ArgList))
			display_msg(from, ArgList);
		break;
	case 607: /* end of watchlist */
		PasteArgs(ArgList, 0);
		if (get_int_var(SHOW_END_OF_MSGS_VAR))
		{
			if (do_hook(current_numeric, "%s %s", from, *ArgList))
				display_msg(from, ArgList);
		}
		break;
	case 716:		/* #define RPL_TARGUMODEG       716 */
	{
		/* hybrid / ratbox: <nick> :is in +g mode (server side ignore) */
		if (do_hook(current_numeric, "%s %s %s", from, ArgList[0], ArgList[1]))
			put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_CALLERID_FSET),"%s %s", ArgList[0], ArgList[1]));
		break;
	}
	case 365:		/* #define RPL_ENDOFLINKS       365 */
	{
		if (get_int_var(LLOOK_VAR) && (get_server_linklook(from_server) == 1)) 
		{
			parse_365(ArgList[0], ArgList[1],NULL);
			set_server_linklook(from_server, 0);
			break;
		} 
		else if (get_server_linklook(from_server) == 2)	
		{
			show_server_map();
			set_server_linklook(from_server, 0);
			break;
		}
	}
	case 404:
	case 442:
	case 482:
	{
		if (comm == 404 || comm == 482 || comm == 442)
		{
			if (check_server_sync(from, ArgList))
			{
				send_to_server("WHO -server %s %s", from, ArgList[0]);
				break;
			}
		}
	}
	case 204:
	{
		if (comm == 204)
		{
			if (get_server_trace_flag(from_server) & TRACE_OPER)
			{
				trace_oper(from, ArgList);
				break;
			} else if (get_server_trace_flag(from_server))
				break;
		}
	}
	case 203:
	case 205:
	{
		if (comm == 203 || comm == 205)
		{
			if (get_server_trace_flag(from_server) & TRACE_USER)
			{
				trace_user(from, ArgList);
				break;
			} else if (get_server_trace_flag(from_server))
				break;
		}
	}
	case 206:
	{
		if (comm == 206)
		{
			if (get_server_trace_flag(from_server) & TRACE_SERVER)
			{
				trace_server(from, ArgList);
				break;
			} else if (get_server_trace_flag(from_server))
				break;
		}
	}
	case 200:
	case 207:
	case 209:
		if ((comm == 200) || (comm == 207) || (comm == 209))
		{
			if (get_server_trace_flag(from_server))
				break;
		}
	case 262:
	case 402:
	{
		if (comm == 402)
			clear_server_sping(from_server, ArgList[0]);
		if ((comm == 262 || comm == 402) && get_server_trace_flag(from_server))
		{
			set_server_trace_flag(from_server, 0);
			break;
		}
		if (get_server_trace_kill(from_server))
		{
			if (comm == 200 || comm == 204 || comm == 206 || comm == 207)
				break;
			if (comm == 205 || comm == 203)
			{
				handle_tracekill(comm, ArgList[2], NULL, NULL);
				break;
			}
			if (comm == 209 || comm == 262 || comm == 402)
			{
				handle_tracekill(comm, NULL, NULL, NULL);
				break;
			}
		}
	}
	case 903:		/* SASL authentication successful */
	case 904:		/* SASL authentication failed */
	case 905:		/* SASL message too long */
	case 906:		/* SASL authentication aborted */
	case 907:		/* You have already completed SASL authentication */
	{
		my_send_to_server(from_server, "CAP END");
		if (do_hook(current_numeric, "%s %s", from, *ArgList))
			display_msg(from, ArgList);
		break;
	}
	case 305:
	{
		if (comm == 305 && get_server_away(from_server))
		{
			set_server_away(from_server, NULL, 0);
			set_server_awaytime(from_server, 0);
			update_all_status(current_window, NULL, 0);
		}
	}
	case 461:
	{
		if ((comm == 461) && oper_command)
			oper_command--;
	}
	/*
	 * The following accumulates the remaining arguments
	 * in ArgSpace for hook detection. We can't use
	 * PasteArgs here because we still need the arguments
	 * separated for use elsewhere.
	 */
	default:
	{
			char	*ArgSpace = NULL;
			int	i,
				len;

			
			for (i = len = 0; ArgList[i]; len += strlen(ArgList[i++]))
				;
			len += (i - 1);
			ArgSpace = alloca(len + 1);
			ArgSpace[0] = '\0';
			/* this is cheating */

			if (ArgList[0] && is_channel(ArgList[0]))
                        	set_display_target(ArgList[0], LOG_CRAP);

			for (i = 0; ArgList[i]; i++)
			{
				if (i)
					strcat(ArgSpace, space);
				strcat(ArgSpace, ArgList[i]);
			}

			if (!do_hook(current_numeric, "%s %s", from, ArgSpace))
			{
				reset_display_target();
				break;
			}

			none_of_these = 1;
	}  /* default */
	} /* switch */
	/* the following do not hurt the ircII if intercepted by a hook */
	if (none_of_these)
	{
		switch (comm)
		{
		case 211:
		case 212:
		case 213:
		case 214:
		case 215:
		case 216: 
		case 218:
		case 241:
		case 242:
		case 243:
		case 244:
			if (get_server_stat_flag(from_server) && ((comm > 210 && comm < 219) || (comm > 240 && comm < 245)))
			{
				handle_stats(comm, from, ArgList);
				break;
			}
			else if (!get_server_stat_flag(from_server) && comm == 242)
			{
/*		case 242:		 #define RPL_STATSUPTIME      242 */
				PasteArgs(ArgList, 0);
				if (from && !my_strnicmp(get_server_itsname(from_server),
				    from, strlen(get_server_itsname(from_server))))
					from = NULL;
				if (from)
					put_it("%s %s from (%s)", numeric_banner(),
						*ArgList, from);
				else
					put_it("%s %s", numeric_banner(), *ArgList);
				break;
			}
			else
			{
				PasteArgs(ArgList, 0);
				put_it("%s %s %s", numeric_banner(), from, *ArgList);
			}
			break;
		case 219:		/* #define RPL_ENDOFSTATS       219 */
			if (get_server_stat_flag(from_server) && comm == 219)
				set_server_stat_flag(from_server, 0);
			break;
		case 221: 		/* #define RPL_UMODEIS          221 */
			put_it("%s Your user mode is [%s]", numeric_banner(), ArgList?ArgList[0]:space);
			break;

		case 328:		/* #define RPL_CHANNELURL       328 (atheme, bahamut, freenode) */
		{
			const char *channel = ArgList[0];
			const char *url = ArgList[1] ? ArgList[1] : empty_string;

			set_display_target(channel, LOG_CRAP);
			put_it("%s", convert_output_format(fget_string_var(FORMAT_CHANNEL_URL_FSET), "%s %s %s", update_clock(GET_TIME), channel, url));
			reset_display_target();
			break;
		}

		case 329:		/* #define CREATION_TIME	329 */
		{
			unsigned long ts;
			time_t tme;
			char this_sucks[80];
			ChannelList *chan = NULL;
			
			if (!ArgList[1] || !*ArgList[1])
				break;
			sscanf(ArgList[1], "%lu", &ts);
			tme = ts;
			strcpy(this_sucks, ctime(&tme));
			this_sucks[strlen(this_sucks)-1] = '\0';		

			set_display_target(ArgList[0], LOG_CRAP);
			if (!ArgList[2])
				put_it("%s Channel %s was created at %s",numeric_banner(),
					ArgList[0], this_sucks);
			else
			{
				char cts[80], pts[80], ots[80];
				sscanf(ArgList[2], "%lu", &ts);
				tme = ts;
				strcpy(cts, ctime(&tme));
				cts[strlen(cts)-1] = '\0';		
				sscanf(ArgList[2], "%lu", &ts);
				tme = ts;
				strcpy(pts, ctime(&tme));
				pts[strlen(pts)-1] = '\0';		
				ots[0] = 0;
				if (ArgList[3])
				{
					sscanf(ArgList[3], "%lu", &ts);
					tme = ts;
					strcpy(ots, ctime(&tme));
				}
				ots[strlen(ots)-1] = '\0';		
				put_it("%s Channel %s was created at %s",numeric_banner(),
					ArgList[0], this_sucks);
				put_it("%s Channel %s TS %s Password TS %s opless TS %s",numeric_banner(),
					ArgList[0], cts, pts, ots);
			}
			if ((chan = lookup_channel(ArgList[0], from_server, 0)))
				chan->channel_create.tv_sec = tme;
			reset_display_target();
			break;
		}
		case 332:		/* #define RPL_TOPIC            332 */
			set_display_target(ArgList[0], LOG_CRAP);
			channel_topic(from, ArgList, 332);
			reset_display_target();
			break;
		case 333:
			set_display_target(ArgList[0], LOG_CRAP);
			channel_topic(from, ArgList, 333);
			reset_display_target();
			break;
			
		case 351:		/* #define RPL_VERSION          351 */
			PasteArgs(ArgList, 2);
			put_it("%s", convert_output_format(fget_string_var(FORMAT_SERVER_FSET), "Server %s %s %s", ArgList[0], ArgList[1], ArgList[2]));
			break;

		case 364:		/* #define RPL_LINKS            364 */
		{

			if (ArgList[2])
			{
				PasteArgs(ArgList, 2);
				put_it("%s %-20s %-20s %s", numeric_banner(),
					ArgList[0], ArgList[1], ArgList[2]);
			}
			else
			{
				PasteArgs(ArgList, 1);
				put_it("%s %-20s %s", numeric_banner(),
					ArgList[0], ArgList[1]);
			}
			break;
		}
		case 377:
		case 372:		/* #define RPL_MOTD             372 */
			if (!get_int_var(SUPPRESS_SERVER_MOTD_VAR) ||
			    !get_server_motd(from_server))
			{
				PasteArgs(ArgList, 0);
				put_it("%s %s", numeric_banner(), ArgList[0]);
			}
			break;

		case 375:		/* #define RPL_MOTDSTART        375 */
			if (!get_int_var(SUPPRESS_SERVER_MOTD_VAR) ||
			    !get_server_motd(from_server))
			{
				PasteArgs(ArgList, 0);
				put_it("%s %s", numeric_banner(), ArgList[0]);
			}
			break;

		case 376:		/* #define RPL_ENDOFMOTD        376 */
			if (get_int_var(SHOW_END_OF_MSGS_VAR) &&
			    (!get_int_var(SUPPRESS_SERVER_MOTD_VAR) ||
			    !get_server_motd(from_server)))
			{
				PasteArgs(ArgList, 0);
				put_it("%s %s", numeric_banner(), ArgList[0]);
			}
			set_server_motd(from_server, 0);
			break;

		case 384:		/* #define RPL_MYPORTIS         384 */
			PasteArgs(ArgList, 0);
			put_it("%s %s %s", numeric_banner(), ArgList[0], user);
			break;

		case 385:		/* #define RPL_NOTOPERANYMORE   385 */
			set_server_operator(from_server, 0);
			display_msg(from, ArgList);
			update_all_status(current_window, NULL, 0);
			break;

		case 403:		/* #define ERR_NOSUCHCHANNEL    403 */
			not_valid_channel(from, ArgList);
			break;

		case 432:		/* #define ERR_ERRONEUSNICKNAME 432 */
			display_msg(from, ArgList);
			reset_nickname(from_server);
			break;


#define RPL_CLOSEEND         363
#define RPL_SERVLISTEND      235

		case 323:               /* #define RPL_LISTEND          323 */
			funny_print_widelist();

		case 232:		/* #define RPL_ENDOFSERVICES    232 */
		case 365:		/* #define RPL_ENDOFLINKS       365 */
			if (comm == 365 && get_int_var(LLOOK_VAR) && get_server_linklook(from_server)) 
			{
				parse_365(ArgList[0], ArgList[1],NULL);
				set_server_linklook(from_server, 0);
				break;
			}
		case 369:		/* #define RPL_ENDOFWHOWAS      369 */
		case 374:		/* #define RPL_ENDOFINFO        374 */
		case 394:		/* #define RPL_ENDOFUSERS       394 */
		{
			int hook;
			PasteArgs(ArgList, 0);
			hook = do_hook(current_numeric, "%s %s", from, *ArgList);
			if (hook && get_int_var(SHOW_END_OF_MSGS_VAR))
				display_msg(from, ArgList);
			break;
		}
		case 671:		/* #define RPL_WHOISSECURE      671 */
		{
			/* ratbox / unreal / freenode: <nick> :is using a secure connection */
			put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOIS_SECURE_FSET),"%s %s", ArgList[0], ArgList[1]));
			break;
		}
		default:
			display_msg(from, ArgList);
		}
	}
	current_numeric = old_current_numeric;
	reset_display_target();
}

