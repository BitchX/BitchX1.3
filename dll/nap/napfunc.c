#define IN_MODULE
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
#include "list.h"
#include "modval.h"
#include "./napster.h"
#include "md5.h"

#define NEMPTY ""
#define EMPTY_STRING m_strdup(NEMPTY)
#define RETURN_EMPTY return EMPTY_STRING
#define RETURN_IF_EMPTY(x) if (empty( x )) RETURN_EMPTY
#define GET_INT_ARG(x, y) {RETURN_IF_EMPTY(y); x = atol(new_next_arg(y, &y));}
#define GET_STR_ARG(x, y) {RETURN_IF_EMPTY(y); x = new_next_arg(y, &y);RETURN_IF_EMPTY(x);}
#define RETURN_STR(x) return m_strdup(x ? x : EMPTY)
#define RETURN_INT(x) return m_strdup(ltoa(x))


/* 
 * given a "time in seconds" we convert this to a pretty time format 
 */

BUILT_IN_FUNCTION(func_mp3_time)
{
unsigned long t;
	t = my_atol(input);
	return m_strdup(mp3_time(t));
}

/*
 * return topic for the channel or empty
 */
 
BUILT_IN_FUNCTION(func_topic)
{
char *chan;
ChannelStruct *ch;
	GET_STR_ARG(chan, input);
	ch = (ChannelStruct *)find_in_list((List **)&nchannels, chan, 0);
	return ch ? m_strdup(ch->topic) : m_strdup("");
}

/*
 * are we on "channel"
 */
 
BUILT_IN_FUNCTION(func_onchan)
{
char *chan;
ChannelStruct *ch;
	GET_STR_ARG(chan, input);
	ch = (ChannelStruct *)find_in_list((List **)&nchannels, chan, 0);
	return ch ? m_strdup("1") : m_strdup("0");
}

/*
 * given a channel, returns the nicks on that channel.
 * given a channel and a "nick speed files" returns the nick else empty.
 */
BUILT_IN_FUNCTION(func_onchannel)
{
char *chan;
ChannelStruct *ch;
NickStruct *n;
char *nick = NULL;
	GET_STR_ARG(chan, input);
	if ((ch = (ChannelStruct *)find_in_list((List **)&nchannels, chan, 0)))
	{
		char *ret = NULL;
		char buffer[200];
		if (input && *input)
		{
			while ((nick = next_arg(input, &input)))
			{
				for (n = ch->nicks; n; n = n->next)
				{
					if (!my_stricmp(nick, n->nick))
					{
						sprintf(buffer, "%s %d %lu", n->nick, n->speed, n->shared);
						m_s3cat(&ret, " ", buffer);
					}			
				}
			}
		}
		else
		{
			for (n = ch->nicks; n; n = n->next)
				m_s3cat(&ret, " ", n->nick);
		}
		return ret ? ret : m_strdup("");
	}
	RETURN_EMPTY;
}

BUILT_IN_FUNCTION(func_connected)
{
	if (nap_socket > -1)
	{
		int len;
		struct sockaddr_in name;
		len = sizeof (name);
		if (getpeername(nap_socket, (struct sockaddr *)&name, &len))
			return m_strdup("-1");
		return m_sprintf("%s %d", inet_ntoa(name.sin_addr), ntohs(name.sin_port));
	}
	return m_strdup("");
}

BUILT_IN_FUNCTION(func_hotlist)
{
char *nick = NULL;
char *ret = NULL;
NickStruct *n;
	if (input && *input)
	{
		char buffer[200];
		while ((nick = next_arg(input, &input)))
		{
			for (n = nap_hotlist; n; n = n->next)
			{
				if (!my_stricmp(nick, n->nick))
				{
					sprintf(buffer, "%s %d %lu", n->nick, n->speed, n->shared);
					m_s3cat(&ret, " ", buffer);
				}
			}
		}
	}
	else
	{
		for (n = nap_hotlist; n; n = n->next)
			m_s3cat(&ret, " ", n->nick);

	}
	return ret ? ret : m_strdup("");
}

BUILT_IN_FUNCTION(func_raw)
{
int l;
_N_DATA n_data = {0, 0};
	GET_INT_ARG(l, input);
	n_data.command = l;
	if (input && *input)
		n_data.len = strlen(input);
	if (nap_socket < 0)
		return m_strdup("-1");
	write(nap_socket, &n_data, 4);
	if (n_data.len)
		return m_strdup(ltoa(write(nap_socket, input, n_data.len)));
	else
		return m_strdup("0");
}

BUILT_IN_FUNCTION(func_napchannel)
{
	return nap_current_channel ? m_strdup(nap_current_channel) : m_strdup(empty_string);
}

BUILT_IN_FUNCTION(func_md5)
{
int l;
unsigned long size = 0;
	GET_INT_ARG(l, input);
	if (input && *input)
		size = my_atol(input);
	return calc_md5(l, size);
}
