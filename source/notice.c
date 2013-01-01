/*
 * notice.c: special stuff for parsing NOTICEs
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1991
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#include "irc.h"
static char cvsrevision[] = "$Id: notice.c 58 2008-07-07 11:55:00Z keaston $";
CVS_REVISION(notice_c)
#include "struct.h"

#include "commands.h"
#include "who.h"
#include "ctcp.h"
#include "window.h"
#include "lastlog.h"
#include "log.h"
#include "flood.h"
#include "vars.h"
#include "ircaux.h"
#include "hook.h"
#include "ignore.h"
#include "server.h"
#include "funny.h"
#include "output.h"
#include "names.h"
#include "parse.h"
#include "notify.h"
#include "misc.h"
#include "screen.h"
#include "status.h"
#include "notice.h"
#include "hash2.h"
#include "cset.h"
#include "input.h"
#define MAIN_SOURCE
#include "modval.h"

extern	char	*FromUserHost;
static	void	parse_server_notice (char *, char *);
int	doing_notice = 0;
unsigned long default_swatch = -1;



long	oper_kills = 0,
	nick_collisions = 0,
	serv_fakes = 0,
	serv_unauth = 0,
	serv_split = 0,
	serv_rejoin = 0,
	serv_squits = 0,
	serv_connects = 0,
	client_connects = 0,
	serv_rehash = 0,
	client_exits = 0,
	serv_klines = 0,
	client_floods = 0,
	client_invalid = 0,
	stats_req = 0,
	client_bot = 0,
	client_bot_alarm = 0,
	oper_requests = 0;

#ifdef WANT_OPERVIEW

extern void check_orig_nick(char *);
static	int  handle_oper_vision(const char *from, char *cline, int *up_status)
{
	char *fr, *for_, *temp, *temp2;
	char *p;
	int done_one = 0;
	char *line;
	unsigned long flags;
	int dcount;
			
	p = fr = for_ = temp = temp2 = NULL;

	if (from_server == -1 || !(flags = get_server_ircop_flags(from_server)))
		return 0;

	line = LOCAL_COPY(cline);
	if (!strncmp(line, "*** Notice -- ", 13)) line += 14, dcount = 14;
	else if (!strncmp(line, "*** \002Notice\002 --", 15)) line += 16, dcount = 16;

	done_one++;
/*
[ss]!irc.cs.cmu.edu D-line active for think[think@skateboarders.edu]
*/
	set_display_target(NULL, LOG_SNOTE);	
	
	if (!strncmp(line, "Received KILL message for ", 26))
	{
		char *q = line + 26;
		int loc_check = 0;

		for_ = next_arg(q, &q);
		if (!end_strcmp(for_, ".", 1))
			chop(for_, 1);
		q += 5;
		fr = next_arg(q, &q);
		q += 6; 
		check_orig_nick(for_);

		
		if (strchr(fr, '.'))
		{
			nick_collisions++;
			if (!(flags & NICK_COLLIDE))
				goto done;  	
			serversay(1, from_server, "%s", convert_output_format(fget_string_var(FORMAT_SERVER_NOTICE_NICK_COLLISION_FSET), "%s %s %s %s", update_clock(GET_TIME), fr, for_, q));
		}
		else 
		{
			oper_kills++;
			if (!(flags & NICK_KILL))
				goto done;  	

			if ((temp2 = next_arg(q, &q)))
				loc_check = charcount(temp2, '!');
			if (q && *q)
				q++; chop(q, 1);
			if (loc_check <= 2)		
				serversay(1, from_server, "%s", convert_output_format(fget_string_var(FORMAT_SERVER_NOTICE_KILL_LOCAL_FSET), "%s %s %s %s", update_clock(GET_TIME), fr, for_, q));
			else
				serversay(1, from_server, "%s", convert_output_format(fget_string_var(FORMAT_SERVER_NOTICE_KILL_FSET), "%s %s %s %s", update_clock(GET_TIME), fr, for_, q));
		}
		(*up_status)++;
	}
	else if (!strncmp(line, "Nick collision on", 17) || !strncmp(line, "Nick change collision on", 24))
	{
#if 0
irc.BitchX.com *** Notice -- Nick collision on nickserv(irc.distracted.net <-
               irc.distracted.net[unknown@209.51.160.249])(both killed)
[BitchX]  Nick collision llision killed on
#endif               
		nick_collisions++;
		if (!(flags & NICK_COLLIDE))
			goto done;  	
		if (!strncmp(line+5, "change", 6))
			p = line + 24;
		else
			p = line + 18;
		serversay(1, from_server, "%s", convert_output_format(fget_string_var(FORMAT_SERVER_NOTICE_NICK_COLLISION_FSET), "%s %s", update_clock(GET_TIME), p));
		(*up_status)++;
	}
	else if (!strncmp(line, "IP# Mismatch:", 13))
	{
		if (!(flags & IP_MISMATCH))
			goto done;  	
		for_ = line + 14;
		serversay(1, from_server, "%s", convert_output_format(" IP Mismatch %C$1-", "%s %s", update_clock(GET_TIME), for_));
	}
	else if (!strncmp(line, "Hacked ops on opless channel:", 29))
	{
		if (!(flags & HACK_OPS))
			goto done;  	
		for_ = line + 29;
		serversay(1, from_server, "%s", convert_output_format(" Hacked ops on $0", "%s", for_));
	}
	else if (!strncmp(line, "connect failure:", 16))
	{
		client_connects++;
		client_exits++;
		if (!(flags & SERVER_CRAP))
			goto done;  	
		for_ = line + 16;
		serversay(1, from_server, "%s", convert_output_format(" Connect failure %K[%n$0-%K]", "%s", for_));
	} 
	else if (!strncmp(line, "Identd response differs", 22))
	{
		if (!(flags & IDENTD))
			goto done;  	
		for_ = line + 24;
		serversay(1, from_server, "%s", convert_output_format(" Identd response differs %K[%C$1-%K]", "%s %s", update_clock(GET_TIME), for_));
	}
  	else if (!strncmp(line, "Fake: ", 6)) /* MODE */
  	{
		serv_fakes++;
		if (!(flags & FAKE_MODE))
			goto done;  	
		p = line + 6;
		if ((fr = next_arg(p, &temp)))
		{
			if (lookup_channel(fr, from_server, CHAN_NOUNLINK))
				serversay(1, from_server, "%s", convert_output_format(fget_string_var(FORMAT_SERVER_NOTICE_FAKE_FSET), "%s %s %s", update_clock(GET_TIME), fr, temp));
			else 
				serversay(1, from_server, "%s", convert_output_format(fget_string_var(FORMAT_SERVER_NOTICE_FAKE_FSET), "%s %s %s", update_clock(GET_TIME), fr, temp));
		}
  	}
  	else if (!strncmp(line, "Unauthorized connection from",28))
  	{
		serv_unauth++;
		if (!(flags & UNAUTHS))
			goto done;  	
		for_ = line + 28;
		serversay(1, from_server, "%s", convert_output_format(fget_string_var(FORMAT_SERVER_NOTICE_UNAUTH_FSET), "%s %s", update_clock(GET_TIME), for_));
	}
  	else if (!strncmp(line, "Too many connections from",25))
  	{
		serv_unauth++;
		if (!(flags & TOO_MANY))
			goto done;  	
		for_ = line + 25;
		serversay(1, from_server, "%s", convert_output_format(fget_string_var(FORMAT_SERVER_NOTICE_UNAUTH_FSET), "%s %s", update_clock(GET_TIME), for_));
	}
	else if (strstr(line, "Entering high-traffic mode -") || !strncmp(line, "still high-traffic mode -", 25))
	{
		char *q;
 		serv_split++;
		if (!(flags & TRAFFIC))
			goto done;  	
		if (!strncmp(line, "Entering", 8))
		{
			p = line + 28;
			for_ = next_arg(p, &p);
			q = temp2 = p;
			if (temp2)
			{
				chop(temp2, 1);
				q = temp2+2;
			}
		}
		else if (!strncmp(line+10, "Entering", 8))
		{
			p = line + 38;
			for_ = next_arg(p, &p);
			q = temp2 = p;
			if (temp2)
			{
				chop(temp2, 1);
				q = temp2+2;
			}
		}
		else
		{
			p = line + 25;
			for_ = next_arg(p, &p);
			q = temp2 = p;
			if (temp2)
			{
				chop(temp2, 1);
				q = temp2+2;
			}
		}
		serversay(1, from_server, "%s", convert_output_format(fget_string_var(FORMAT_SERVER_NOTICE_TRAFFIC_HIGH_FSET), "%s %s %s", update_clock(GET_TIME), for_, q));
	}
	else if (!strncmp(line, "Resuming standard operation", 27))
	{
		serv_rejoin++;
		if (!(flags & TRAFFIC))
			goto done;  	
		p = line + 27;
		for_ = next_arg(p, &p);
		if (for_ && *for_ == '-')
			for_ = next_arg(p, &p); 
		temp = next_arg(p, &temp2);
		serversay(1, from_server, "%s", convert_output_format(fget_string_var(FORMAT_SERVER_NOTICE_TRAFFIC_NORM_FSET), "%s %s %s %s", update_clock(GET_TIME), for_, temp, temp2));
	}
	else if (wild_match("% is rehashing Server config*", line))
	{
		serv_rehash++;
		if (!(flags & REHASH))
			goto done;  	
		p = line;
		for_ = next_arg(p, &p);
		serversay(1, from_server, "%s", convert_output_format(fget_string_var(FORMAT_SERVER_NOTICE_REHASH_FSET), "%s %s", update_clock(GET_TIME), for_));
	}
	else if (wild_match("% added K-Line for *", line))
	{
		char *serv = NULL;
		serv_klines++;

		if (!(flags & KLINE))
			goto done;  	

		p = line;
		for_ = next_arg(p, &p);
		if (!strncmp(p, "from", 4))
		{
			next_arg(p, &p);
			serv = next_arg(p, &p);
		}
		p += 17;
		temp2 = next_arg(p, &temp);
		if (++temp2)
			chop(temp2, 1);
		if (serv)
			serversay(1, from_server, "%s", convert_output_format(fget_string_var(FORMAT_SERVER_NOTICE_GLINE_FSET), "%s %s %s %s %s", update_clock(GET_TIME), for_, temp2, serv, temp));
		else
			serversay(1, from_server, "%s", convert_output_format(fget_string_var(FORMAT_SERVER_NOTICE_KLINE_FSET), "%s %s %s %s", update_clock(GET_TIME), for_, temp2, temp));
	}
	else if (!strncmp(line, "Rejecting vlad/joh/com bot:", 27) || !strncmp(line+14, "Rejecting eggdrop bot:", 20) || !strncmp(line, "Rejecting ojnk/annoy bot", 24))
	{
		client_bot++;
		if (!(flags & POSSIBLE_BOT))
			goto done;
		p = line + 10;
		temp2 = next_arg(p, &p);
		for_ = p + 4;
		serversay(1, from_server, "%s", convert_output_format(fget_string_var(FORMAT_SERVER_NOTICE_BOT_FSET), "%s %s %s", update_clock(GET_TIME), for_, temp2));
	}
	else if (!strncmp(line, "Possible bot ", 13))
	{
		client_bot++;
		if (!(flags & POSSIBLE_BOT))
			goto done;  	
		p = line + 13;
		for_ = next_arg(p, &p);
		if ((temp2 = next_arg(p, &p)))
			chop(temp2, 1);
		serversay(1, from_server, "%s", convert_output_format(fget_string_var(FORMAT_SERVER_NOTICE_BOT_FSET), "%s %s %s", update_clock(GET_TIME), for_, temp2));
	}
	else if (wild_match("Possible % bot *", line))
	{
		char *possible = NULL;
		client_bot++;
		if (!(flags & POSSIBLE_BOT))
			goto done;  	
		p = line;
		possible = next_arg(p, &p);
		next_arg(p, &p);
		for_ = next_arg(p, &p);
		if ((temp2 = next_arg(p, &p)))
		{
			chop(temp2, 1);
			*temp2 = ' ';
		}
		serversay(1, from_server, "%s", convert_output_format(fget_string_var(FORMAT_SERVER_NOTICE_BOT1_FSET), "%s %s %s %s", update_clock(GET_TIME), possible?possible:"Unknown", for_, temp2));
	}
	else if (wild_match("% % is now operator*", line))
	{
		oper_requests++;
		if (!(flags & OPER_MODE))
			goto done;  	
		p = line;
		fr = next_arg(p, &p);
		if ((temp2 = next_arg(p, &p)))
		{
			chop(temp2, 1);
			if (*temp2 == '(')
				*temp2 = ' ';
		}
		serversay(1, from_server, "%s", convert_output_format(fget_string_var(FORMAT_OPER_FSET), "%s %s %s", update_clock(GET_TIME), fr, temp2));
	} 
	else if (!strncmp(line, "Received SQUIT", 14))
	{
		serv_squits++;
		if (!(flags & SQUIT))
			goto done;  	
		p = line + 14;
		fr = next_arg(p, &p);
		p += 5;
		for_ = next_arg(p, &temp2);
		if (temp2)
		{
			chop(temp2, 1);
			if (*temp2 == '(')
				temp2++;
		}
		serversay(1, from_server, "%s", convert_output_format(" SQUIT of $1 from $2 %K[%R$3-%K]", "%s %s %s %s", update_clock(GET_TIME), for_, fr, temp2));
	} 
	else if (!strncmp(line, "Received SERVER", 15))
	{
		serv_squits++;
		if (!(flags & SERVER_CONNECT))
			goto done;  	
		p = line + 15;
		fr = next_arg(p, &p);
		p += 5;
		for_ = next_arg(p, &temp2);
		if (temp2)
		{
			chop(temp2, 1);
			if (*temp2 == '(')
				temp2++;
		}
		serversay(1, from_server, "%s", convert_output_format(" Received SERVER %c$1%n from %c$2%n %K[%W$3-%K]", "%s %s %s %s", update_clock(GET_TIME), fr, for_, temp2));
	} 
	else if (!strncmp(line, "Sending SQUIT", 13))
	{
	    serv_squits++;
	    if (!(flags & SQUIT)) goto done;
	    p = line + 13;
	    fr = next_arg(p, &temp2);
	    if (temp2)
	    {
		chop(temp2, 1);
		if (*temp2 == '(') temp2++;
	    }
	    serversay(1, from_server, "%s", convert_output_format(" Sending SQUIT %c$1%n %K[%R$2-%K]", "%s %s %s", update_clock(GET_TIME), fr, temp2));
	}
	else if (!strncmp(line, "Sending SERVER", 14))
	{
	    serv_squits++;
	    if (!(flags & SERVER_CONNECT)) goto done;
	    p = line + 14;
	    fr = next_arg(p, &temp2);
	    if (temp2)
	    {
		chop(temp2, 1);
		if (*temp2 == '(') temp2++;
	    }
	    serversay(1, from_server, "%s", convert_output_format(" Sending SERVER %c$1%n %K[%W$2-%K]", "%s %s %s", update_clock(GET_TIME), fr, temp2));
	}
	else if (!strncmp(line, "WALLOPS :Remote CONNECT", 23))
	{
		serv_connects++;
		if (!(flags & SERVER_CONNECT))
			goto done;  	
		p = line + 23;
		for_ = next_arg(p, &p);
		fr = next_arg(p, &p);
		next_arg(p, &temp2);
		serversay(1, from_server, "%s", convert_output_format(" Remote Connect of $1:$2 from $3", "%s %s %s %s", update_clock(GET_TIME), for_, fr, temp2));
	}
	else if (!strncmp(line, "Client connecting", 17) || !strncmp(line, "Client exiting", 14))
	{
		char *q = strchr(line, ':');
		char *port = empty_string;
		int conn = !strncmp(line+7, "connect", 7) ? 1 : 0;
		int dalnet = 0, ircnet = 0;

		if (strlen(line) >= 19 && line[18] == ':')
			q = NULL;
		else
			dalnet = (q == NULL);

		if (!dalnet)
		{
		    if ((q = strchr(q + 1, ' ')))
		    {
			    q++;
			    if (conn)
				ircnet = !strcmp(q, "is ");
			    else
				ircnet = !strcmp(q, "was ");
		    }
		}

		if (conn)
			client_connects++;
		else 
			client_exits++;

		if (!(flags & CLIENT_CONNECT))
			goto done;  	
		p = line;
		next_arg(p, &p); next_arg(p, &p);
		if (ircnet)
		{
		    for_ = LOCAL_COPY(p);
		    fr = LOCAL_COPY(p);
		    temp = LOCAL_COPY(p);
		    temp2 = LOCAL_COPY(p);

		    if (conn) sscanf(p, "%s is %s from %s", for_, fr, temp);
		    else sscanf(p, "%s was %s from %s", for_, fr, temp);

		    q = p;
		    sprintf(q, "%s@%s", fr, temp);
		    if (!conn) 
		    {
			port = strstr(temp2, "reason:");
			port += 8;
		    }
		} 
		else if (dalnet && !conn)
		{
			for_ = next_arg(p, &p);			
			q = temp2 = p;
			if (temp2)
			{
				chop(temp2, 1);
				q = temp2+1;
			}
		} 
		else if (conn && dalnet)
		{
			next_arg(p, &p); next_arg(p, &p); 
			port = next_arg(p, &p);
			if (!(for_ = next_arg(p, &p)))
				for_ = port;
			{
				q = temp2 = p;
				chop(port, 1);
				if (temp2)
				{
					chop(temp2, 1);
					q = temp2+1;
				}
			}
		}
		else /* hybrid */
		{
		    for_ = q;
		    if ((q = strchr(q, ' ')))
		    {
			    *q = 0;
			    q += 2;
		    }
		    if ((port = strchr(q, ' ')))
		    {
			    *port = 0;
			    port++;
			    chop(q, 1);
		    }
		}
		
		serversay(1, from_server, "%s", convert_output_format(fget_string_var(conn ? FORMAT_SERVER_NOTICE_CLIENT_CONNECT_FSET : FORMAT_SERVER_NOTICE_CLIENT_EXIT_FSET), "%s %s %s %s", update_clock(GET_TIME), for_, q ? q : empty_string, port ? port : empty_string));
	}
	else if (!strncmp(line, "Terminating client for excess", 29))
	{
		char *q;

		client_floods++;
		if (!(flags & TERM_FLOOD))
			goto done;  	

		p = line + 29;
		for_ = next_arg(p, &p);
		q = temp2 = p;
		if (temp2)
		{
			chop(temp2, 1);
			q = temp2+1;
		}
		serversay(1, from_server, "%s", convert_output_format(fget_string_var(FORMAT_SERVER_NOTICE_CLIENT_TERM_FSET), "%s %s %s", update_clock(GET_TIME), for_, q));
	}
	else if (!strncmp(line, "Invalid username:", 17))
	{

		client_invalid++;
		if (!(flags & INVALID_USER))
			goto done;  	
		p = line + 17;
		for_ = next_arg(p, &p);
		if ((temp2 = next_arg(p, &p)))
			chop(temp2, 1);
		serversay(1, from_server, "%s", convert_output_format(fget_string_var(FORMAT_SERVER_NOTICE_CLIENT_INVALID_FSET), "%s %s %s", update_clock(GET_TIME), for_, temp2));
	}
	else if (!strncmp(line, "STATS ", 6))
	{

		stats_req++;
		if (!(flags & STATS_REQUEST))
			goto done;  	
		p = line + 6;
		temp = next_arg(p, &p);
		p += 12;
		for_ = next_arg(p, &p);
		if ( (temp2 = ++p) )
			chop(temp2, 1);
		serversay(1, from_server, "%s", convert_output_format(fget_string_var(FORMAT_SERVER_NOTICE_STATS_FSET), "%s %s %s %s", update_clock(GET_TIME), temp, for_, temp2));
	}
	else if (!strncmp(line, "Nick flooding detected by:", 26))
	{

		if (!(flags & NICK_FLOODING))
			goto done;  	
		p = line + 26;
		serversay(1, from_server, "%s", convert_output_format(" Nick Flooding %K[%B$1-%K]", "%s %s", update_clock(GET_TIME), for_));
	}
	else if (!strncmp(line, "Kill line active for", 20) || !strncmp(line+14, "K-line active for", 17))
	{

		if (!(flags & KILL_ACTIVE))
			goto done;  	
		if (!strncmp(line + 14,"Kill", 4))
			for_ = line + 20;
		else
			for_ = line + 17;
		serversay(1, from_server, "%s", convert_output_format(" Kill line for $1 active", "%s %s", update_clock(GET_TIME), for_));
	}
	else 
	{
		if (!(flags & SERVER_CRAP))
			goto done;  	
		serversay(1, from_server, "%s", convert_output_format(fget_string_var(FORMAT_SERVER_NOTICE_FSET), "%s %s %s", update_clock(GET_TIME), from, stripansicodes(line)));
		add_last_type(&last_servermsg[0], MAX_LAST_MSG, NULL, NULL, NULL, line);
	} 
done:
	reset_display_target();
	return done_one;
}
#endif

static	void parse_server_notice(char *from, char *line)
{
	int	flag = 0;
	int	up_status = 0;
const	char	*f;	
				
	f = from;
	if (!f || !*f)
		if (!(f = get_server_itsname(from_server)))
			f = get_server_name(from_server);
			
	if (*line != '*'  && *line != '#' && strncmp(line, "MOTD ", 4))
		flag = 1;
	else
		flag = 0;

	if (do_hook(SERVER_NOTICE_LIST, flag?"%s *** %s":"%s %s", f, line))
	{
#ifdef WANT_OPERVIEW
		if (handle_oper_vision(f, line, &up_status))
			reset_display_target();
		else 
#endif
		if (strstr(line, "***"))
		{
			set_display_target(NULL, LOG_SNOTE);
#ifdef WANT_OPERVIEW
			if (get_int_var(OV_VAR) && !(get_server_ircop_flags(from_server) & SERVER_CRAP))
				goto done1;
#endif
			if (do_hook(SERVER_NOTICE_LIST, flag ? "%s *** %s" : "%s %s", f, line))
			{
				char *for_;
				for_ = next_arg(line,&line);
				serversay(1, from_server, "%s", 
				convert_output_format(fget_string_var(FORMAT_SERVER_NOTICE_FSET), "%s %s %s", update_clock(GET_TIME), 
				f, stripansicodes(line)));
				add_last_type(&last_servermsg[0], MAX_LAST_MSG, NULL, NULL, NULL, line);
			}
		}
		else
		{
			set_display_target(NULL, LOG_SNOTE);
#ifdef WANT_OPERVIEW
			if (get_int_var(OV_VAR) && !(get_server_ircop_flags(from_server) & SERVER_CRAP))
				goto done1;
#endif
			if (do_hook(SERVER_NOTICE_LIST, flag ? "%s *** %s" : "%s %s", f, line))
				serversay(1, from_server, "%s", 
				convert_output_format(fget_string_var(FORMAT_SERVER_NOTICE_FSET), "%s %s %s", update_clock(GET_TIME), 
				f, stripansicodes(line)));
			add_last_type(&last_servermsg[0], MAX_LAST_MSG, NULL, NULL, NULL, line);
		}
	}
	if (up_status)
		update_all_status(current_window, NULL, 0);
done1:
	reset_display_target();
}

int check_ignore_notice(char *from, char *to, unsigned long type, char *line, char **high)
{
int flag;
	switch ((flag = check_ignore(from, FromUserHost, to, type, line)))
	{
		case IGNORED:
		{
			doing_notice = 0;
			return flag;
		}
		case HIGHLIGHTED:
			*high = highlight_char;
			break;
		default:
			*high = empty_string;
	}
	return flag;
}

void parse_notice(char *from, char **Args)
{
	int	type;
	char	*to,
		*high = empty_string,
		*target,
		*line;

	NickList *nick = NULL;	
	ChannelList *tmpc = NULL;
	char *newline = NULL;

		
	PasteArgs(Args, 1);
	to = Args[0];
	line = Args[1];
	if (!to || !line)
		return;
	if (!*to)
	{
		put_it("*** obsolete notice recieved. [%s]", line+1);
		return;
	}

	if (!from || !*from || !strcmp(get_server_itsname(from_server), from))
	{
		parse_server_notice(from, line);
		return;
	}


	
	if (is_channel(to))
	{
		target = to;
		type = PUBLIC_NOTICE_LIST;
		if ((tmpc = lookup_channel(to, from_server, CHAN_NOUNLINK)))
			nick = find_nicklist_in_channellist(from, tmpc, 0);
	}
	else
	{
		target = from;
		type = NOTICE_LIST;
	}

	update_stats(NOTICELIST, nick, tmpc, 0);		

	set_display_target(target, LOG_NOTICE);
	doing_notice = 1;

	if ((check_ignore_notice(from, to, IGNORE_NOTICES, line, &high) == IGNORED))
		goto notice_cleanup;

	if (!check_flooding(from, NOTICE_FLOOD, line, NULL))
		goto notice_cleanup;

	if (!strchr(from, '.'))
	{
		notify_mark(from, FromUserHost, 1, 0);
		line = do_notice_ctcp(from, to, line);
		if (!*line)
			goto notice_cleanup;
	}
		
	if (sed && !do_hook(ENCRYPTED_NOTICE_LIST, "%s %s %s", from, to, line))
	{
#if 0
		put_it("%s", convert_output_format(fget_string_var(FORMAT_ENCRYPTED_NOTICE_FSET), "%s %s %s %s", update_clock(GET_TIME), from, FromUserHost, line));
#endif
		sed = 0;
		goto notice_cleanup;
	}

	
	{
		char *free_me = NULL;
		char *s;
		free_me = newline = stripansi(line);
		if (wild_match("[*Wall*", line))
		{
			char *channel = NULL, *p, *q;
			q = p = next_arg(newline, &newline);
			if ((p = strchr(p, '/')))
			{
				p++;
				if (*p && *p == '\002')
					p++;
				channel = m_strdup(p);
				if ((p = strchr(channel, ']')))
					*p++ = 0;
				q = channel;
				if (*q && q[strlen(q)-1] == '\002')
					q[strlen(q)-1] = 0;
			} 
			if (channel && *channel)
				set_display_target(channel, LOG_WALL);
			else
				set_display_target(target, LOG_WALL);
			if (do_hook(type, "%s %s", from, line))
			{
				s = convert_output_format(fget_string_var(FORMAT_BWALL_FSET), "%s %s %s %s %s", update_clock(GET_TIME), q, from, FromUserHost, newline);
				if (tmpc)
					add_to_log(tmpc->msglog_fp, now, s, logfile_line_mangler);
				put_it("%s", s);
			}
			add_last_type(&last_wall[0], 1, from, FromUserHost, NULL, line);
			logmsg(LOG_WALL, from, 0, "%s", line);
/*			addtabkey(from, "wall", 0);*/
			new_free(&channel);
		}
		else
		{
			if (type == PUBLIC_NOTICE_LIST)
			{
				s = convert_output_format(fget_string_var(check_auto_reply(line)?FORMAT_PUBLIC_NOTICE_AR_FSET:FORMAT_PUBLIC_NOTICE_FSET), "%s %s %s %s %s", update_clock(GET_TIME), from, FromUserHost, to, newline);
				if (do_hook(type, "%s %s %s", from, to, line))
					put_it("%s", s);
			}
			else
			{
				s = convert_output_format(fget_string_var(FORMAT_NOTICE_FSET), "%s %s %s %s", update_clock(GET_TIME), from, FromUserHost, newline);
				if (do_hook(type, "%s %s", from, line))
					put_it("%s", s);

			}
			if (tmpc)
				add_to_log(tmpc->msglog_fp, now, s, logfile_line_mangler);
			logmsg(LOG_NOTICE, from, 0, "%s", line);
			add_last_type(&last_notice[0], MAX_LAST_MSG, from, FromUserHost, to, line);
		}
		new_free(&free_me);
	}

 notice_cleanup:
	if (beep_on_level & LOG_NOTICE)
		beep_em(1);
	reset_display_target();
	doing_notice = 0;
}


int loading_savefile = 0;

void load_scripts(void)
{
	extern char *new_script;
	static int done = 0;
#if !defined(WINNT) && !defined(__EMX__)
	char buffer[BIG_BUFFER_SIZE+1];
	int old_display = window_display;
#endif
	if (!done++)
	{
		never_connected = 0;
#if !defined(WINNT) && !defined(__EMX__)
		window_display = 0;
		sprintf(buffer, "%s/bxglobal", SCRIPT_PATH);
		loading_global = 1;
		load("LOAD", buffer, empty_string, NULL);
		loading_global = 0;
		window_display = old_display;
#endif


		if (!quick_startup)
		{
			loading_savefile++;
			reload_save(NULL, NULL, empty_string, NULL);
			loading_savefile--;
			/* read the newscript/.bitchxrc/.ircrc file */
			if (new_script && !access(new_script, R_OK))
				load("LOAD", new_script, empty_string, NULL);
			else if (!access(bircrc_file, R_OK))
				load("LOAD", bircrc_file, empty_string, NULL);
			else if (!access(ircrc_file, R_OK))
				load("LOAD", ircrc_file, empty_string, NULL);
		}
	}
	if (get_server_away(from_server))
		set_server_away(from_server, get_server_away(from_server), 1);
}

/*
 * got_initial_version_28: this is called when ircii gets the serial
 * number 004 reply.  We do this becuase the 004 numeric gives us the
 * server name and version in a very easy to use fashion, and doesnt
 * rely on the syntax or construction of the 002 numeric.
 *
 * Hacked as neccesary by jfn, May 1995
 */
extern struct in_addr nat_address;
extern int use_nat_address;

void get_nat_address(UserhostItem *stuff, char *nick, char *args)
{
char *h;
	if (!stuff || !stuff->nick || !nick || !strcmp(stuff->user, "<UNKNOWN"))
		return;
	if (isdigit((unsigned char)*stuff->host))
		h = stuff->host;
	else
		h = host_to_ip(stuff->host);
	nat_address.s_addr = inet_addr(h); 
	bitchsay("using NAT address for DCC");
}

void got_initial_version_28 (char **ArgList)
{
	char *server, *sversion, *user_modes, *channel_modes;

	server = ArgList[0];
	sversion = ArgList[1];
	user_modes = ArgList[2];
	channel_modes = ArgList[3];
		
	if (sversion)
	{
		if (!strncmp(sversion, "2.8", 3))
		{
			if (strstr(sversion, "mu") || strstr(sversion, "me"))
				set_server_version(from_server, Server_u2_8);
			else if (strstr(sversion, "hybrid-6"))
				set_server_version(from_server, Server2_8hybrid6);
			else if (strstr(sversion, "hybrid"))
				set_server_version(from_server, Server2_8hybrid);
			else if (strstr(sversion, "comstud"))
				set_server_version(from_server, Server2_8comstud);
			else if (strstr(channel_modes, "che"))
				set_server_version(from_server, Server2_8ts4); 
			else
				set_server_version(from_server, Server2_8);
		}
		else if (!strncmp(sversion, "2.9", 3))
		        set_server_version(from_server, Server2_9);
		else if (!strncmp(sversion, "2.10", 4))
	        	set_server_version(from_server, Server2_10);
		else if (!strncmp(sversion, "u2.9", 4))
			set_server_version(from_server, Server_u2_9);
		else if (!strncmp(sversion, "u2.10", 4))
			set_server_version(from_server, Server_u2_10);
		else if (!strncmp(sversion, "u3.0", 4))
			set_server_version(from_server, Server_u3_0);
		else
			set_server_version(from_server, Server2_8);
	} else
		set_server_version(from_server, Server2_8);

	set_server_version_string(from_server, sversion ? sversion : "2.8");
	set_server_itsname(from_server, server);
	reconnect_all_channels(from_server);

	reset_display_target();
	reinstate_user_modes();
	if (use_nat_address == 1)
		userhostbase(get_server_nickname(from_server), get_nat_address, 1, "%s", get_server_nickname(from_server));
	update_all_status(current_window, NULL, 0);
	do_hook(CONNECT_LIST, "%s %d %s", get_server_name(from_server), get_server_port(from_server), get_server_itsname(from_server));
}

#define IGNORE_DONT 1


void setup_ov_mode(int on, int hide, int log)
{
#ifdef WANT_OPERVIEW 

char *default_oper = "wsckf";
Window *win = NULL;

	if (on)
	{
		if ((win = get_window_by_name("oper_view")))
		{
			if (win->log)
			do_log(0, NULL, &win->log_fp);
			delete_window(win);
			update_all_windows();
			set_input_prompt(current_window, get_string_var(INPUT_PROMPT_VAR), 0);
			cursor_to_input();
		}
		send_to_server("MODE %s -%s%s", get_server_nickname(from_server), get_string_var(OPER_MODES_VAR)?get_string_var(OPER_MODES_VAR):default_oper, send_umode);
	} 
	else 
	{
		Window *tmp = NULL;
		win = current_window;
		if ((tmp = new_window(current_window->screen)))
		{
			malloc_strcpy(&tmp->name, "oper_view");
			tmp->double_status = 0;
			if (hide)
				hide_window(tmp);
			else
				resize_window(1, tmp, -5);
			tmp->window_level = LOG_WALLOP|LOG_OPNOTE|LOG_SNOTE;
			tmp->absolute_size = 1;
			tmp->skip = 1;
			set_wset_string_var(tmp->wset, STATUS_FORMAT1_WSET, fget_string_var(FORMAT_OV_FSET));
			build_status(tmp, NULL, 0);
			update_all_windows();
			set_input_prompt(current_window, get_string_var(INPUT_PROMPT_VAR), 0);
			cursor_to_input();
			send_to_server("MODE %s +%s", get_server_nickname(from_server), get_string_var(OPER_MODES_VAR)?get_string_var(OPER_MODES_VAR):default_oper);
			set_screens_current_window(win->screen, win);
			tmp->mangler = operlog_line_mangler;
			if (log != -1)
			{
				tmp->log = log;
				if (tmp->log)
					do_log(log, "~/.BitchX/operview.log", &tmp->log_fp);
			}
		}
	}
#endif
}


#ifdef WANT_OPERVIEW 
char *opflags[] = {"COLLIDE", "KILLS", "MISMATCH", "HACK", "IDENTD", "FAKES",
		"UNAUTHS", "CLIENTS", "TRAFFIC", "REHASH", "KLINE", "BOTS", 
		"OPER", "SQUIT", "SERVER", "CONNECT", "FLOOD", "USER", "STATS",
		"NICK",	"ACTIVEK", "CRAP", NULL};

char	all[] = "ALL",
	none[] = "NONE";

unsigned long ircop_str_to_flags(unsigned org_flags, char *str)
{
unsigned long flag = org_flags;
int neg = 0;
char *ptr;
int i, j;
	while ((ptr = next_in_comma_list(str, &str)))
	{
		if (!ptr || !*ptr)
			return flag;
		switch(*ptr)
		{
			case '-':
				neg = IGNORE_DONT;
				ptr++;
				break;
			default:
				neg = 0;
		}
		upper(ptr);
		if (!strcmp(ptr, all))
		{
			switch(neg)
			{
				case IGNORE_DONT:
					flag &= (~-1);
					break;
				default:
					flag = -1;
					break;
				}
		}
		if (!strcmp(ptr, none))
			return 0;
		for (i = 0, j = 1; opflags[i]; i++, j <<= 1 )
		{
			if (!strcmp(ptr, opflags[i]))
			{
				switch(neg)
				{
					case IGNORE_DONT:
						flag &= (~j);
						break;
					default:
						flag |= j;
						break;
				}
				break;
			}
		}
	}
	return flag;
}

char *ircop_flags_to_str(long flag)
{
int p, i;
char *buffer = new_malloc(BIG_BUFFER_SIZE+1);

	for (i = 0, p = 1; opflags[i]; i++, p <<= 1)
	{
		if (flag & p)
		{
			strmcat(buffer, opflags[i], BIG_BUFFER_SIZE);
			strmcat(buffer, ",", BIG_BUFFER_SIZE);
		}
	}
	if (*buffer)
		chop(buffer, 1);
	return buffer;
}

void print_ircop_flags(int server)
{
long flag;
char *buffer = NULL;
	flag = get_server_ircop_flags(server);
	buffer = ircop_flags_to_str(flag);
	put_it("%s", convert_output_format("$G %bOper%BView%n: $0-", "%s", *buffer ? flag == -1 ? "ALL" : buffer : "NONE"));
	new_free(&buffer);
}

void convert_swatch(Window *win, char *str, int unused)
{
unsigned long flag;
char *p;
	if (from_server != -1)
	{
		flag = ircop_str_to_flags(get_server_ircop_flags(from_server), str);
		set_server_ircop_flags(from_server, flag);
	}
	else
		flag = ircop_str_to_flags(default_swatch, str);
	default_swatch = flag;
	p = ircop_flags_to_str(flag);
	set_string_var(SWATCH_VAR, p);
	new_free(&p);
}

void set_operview_flags(int server, unsigned long flags, int neg)
{
unsigned long old = get_server_ircop_flags(server);
	switch(neg)
	{
		case IGNORE_DONT:
			set_server_ircop_flags(server, old & (~flags));
			break;
		default:
			set_server_ircop_flags(server, old | flags);
			break;
	}
}

BUILT_IN_COMMAND(s_watch)
{
unsigned long flag = 0;
unsigned long old_flags;
int gotargs = 0;

	if (from_server == -1)
	{
		put_it("%s", convert_output_format("$G Try connecting to a server first", NULL, NULL));
		return;
	}
	if (args && *args)
		gotargs = 1;
	old_flags = get_server_ircop_flags(from_server);	
	flag = ircop_str_to_flags(old_flags, args);
	if (flag != old_flags)
		set_server_ircop_flags(from_server, flag);
	else if (gotargs && old_flags != -1)
	{
		char buffer[BIG_BUFFER_SIZE+1];
		int i;
		strcpy(buffer, all);
		for (i = 0; opflags[i]; i++)
		{
			strmcat(buffer, space, BIG_BUFFER_SIZE);
			strmcat(buffer, opflags[i], BIG_BUFFER_SIZE);
		}
		strmcat(buffer, space, BIG_BUFFER_SIZE);
		strmcat(buffer, none, BIG_BUFFER_SIZE);
		bitchsay("You must specify from the following:");
		put_it("\t%s", buffer);
		/*ALL COLLIDE KILLS MISMATCH HACK IDENTD FAKES UNAUTHS CLIENTS TRAFFIC CRAP REHASH KLINE BOTS OPER SQUIT SERVER CONNECT FLOOD USER STATS NICK ACTIVEK NONE");*/
		return;
	}
	print_ircop_flags(from_server);
}

extern int old_ov_mode;
BUILT_IN_COMMAND(ov_window)
{
char *arg;
int ov = get_int_var(OV_VAR);
static int hide = DEFAULT_OPERVIEW_HIDE;
int count = 0;
int old_hide = hide;  
char *number = NULL;
int log = 0;
	while ((arg = next_arg(args, &args)))
	{
		if (!my_stricmp(arg, on))
			ov = 1;
		else if (!my_stricmp(arg, off))
			ov = 0;
		else if (!my_stricmp(arg, "+HIDE"))
			hide = 1;
		else if (!my_stricmp(arg, "-HIDE"))
			hide = 0;
		else if (!my_stricmp(arg, "HIDE"))
			hide = 1;
		else if (!my_stricmp(arg, "GROW"))
			number = next_arg(args, &args);
		else if (!my_stricmp(arg, "LOG"))
			log ^= 1;
		count++;
	}
	if (count == 0)
		put_it("%s", convert_output_format("$G %BOper%bView%n is %K[%W$0%K]", "%s", on_off(get_int_var(OV_VAR))));
	else
	{
		if ((!ov && get_int_var(OV_VAR)) || (ov && !get_int_var(OV_VAR)))
		{
			setup_ov_mode(ov ?  0 : 1, hide, log);
			old_ov_mode = ov;
			set_int_var(OV_VAR, ov);
			put_it("%s", convert_output_format("$G %BOper%bView%n is now toggled %K[%W$0%K]", "%s", on_off(get_int_var(OV_VAR))));
			return;
		} 
		if (get_int_var(OV_VAR) && old_hide != hide)
		{
			Window *tmp = get_window_by_name("oper_view");
			Window *old_window = current_window;
			if (tmp)
			{
				if (!old_hide && hide)
					hide_window(tmp);
				else
				{
					show_window(tmp);
					resize_window(1, tmp, -5);
				}
				update_all_windows();
				cursor_to_input();
				if (old_window)
					set_screens_current_window(old_window->screen, old_window);
			}
			return;
		}
		if (hide == 0 && number && is_number(number))
		{
			Window *tmp = get_window_by_name("oper_view");
			if (tmp)
				resize_window(1, tmp, my_atol(number));
			update_all_windows();
			return;
		}
 		put_it("%s", convert_output_format("$G %BOper%bView%n is already %K[%W$0%K]", "%s", on_off(get_int_var(OV_VAR))));
		hide = old_hide;
		{
			char buffer[BIG_BUFFER_SIZE] = "~/.BitchX/operview.log";
			Window *tmp = get_window_by_name("oper_view");
			if (tmp)
			{
				tmp->log = log;
				do_log(tmp->log, buffer, &tmp->log_fp); 
				if (!tmp->log_fp)
					tmp->log = 0;
				tmp->mangler = operlog_line_mangler;
			}
		}
		
	}
}
#endif
