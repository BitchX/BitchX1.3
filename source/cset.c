/************
 *  cset.c  *
 ************
 *
 * My code for creating all the "Channel Sets" on a per channel
 * basis. We manage them here, and then just insert/remove/return them 
 * when a channel calls for them.
 * Code for creating and modifying "Window Sets" on a per window
 * basis. 
 *
 * Note: Notice the shameless use of typecasting to get these functions
 *	 to work as painlessly as possible.
 *
 * Written by Scott H Kilau
 * Modified by Colten D Edwards for /wset
 *
 * Copyright(c) 1997
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT
 */

#if 0
[Sheik(~sheik@spartan.pei.edu)] notice in struct.h that the cset struct *does*
          have a ->next pointer.. thats for the future when we may want to
                    daisy chain cset's that don't have channels yet..  ie  a user can
                              /cset *warez*  and know that all warez channels would get defaulted
#endif                              

#include "irc.h"
static char cvsrevision[] = "$Id: cset.c 108 2011-02-02 12:13:54Z keaston $";
CVS_REVISION(cset_c)
#include "struct.h"
#include "vars.h"    /* for do_boolean() and var_settings[] */
#include "window.h"  /* for get_current_channel_by_refnum() */
#include "screen.h"  /* for curr_scr_win */
#include "names.h"   /* for lookup_channel() */
#include "log.h"
#include "ircaux.h"
#include "status.h"
#include "server.h"
#include "output.h"
#include "misc.h"
#include "list.h"
#include "cset.h"
#include "hash2.h"
#define MAIN_SOURCE
#include "modval.h"

void log_channel(CSetArray *, CSetList *);
void limit_channel(CSetArray *, CSetList *);
void set_msglog_channel_level(CSetArray *, CSetList *);

/*
 * Array of structures for each of the cset vars... allows us to
 * get an offset into the CSetList struct to that var
 */
static CSetArray cset_array[] = {
{ "AINV",			INT_TYPE_VAR,	offsetof(CSetList, set_ainv) ,NULL, 0 },
{ "ANNOY_KICK",			BOOL_TYPE_VAR,	offsetof(CSetList, set_annoy_kick) ,NULL, 0 },
{ "AOP",			BOOL_TYPE_VAR,	offsetof(CSetList, set_aop) ,NULL, 0 },
{ "AUTO_JOIN_ON_INVITE",	BOOL_TYPE_VAR,	offsetof(CSetList, set_auto_join_on_invite), NULL, 0 },
{ "AUTO_LIMIT",			INT_TYPE_VAR,	offsetof(CSetList, set_auto_limit), limit_channel, 0 },
{ "AUTO_REJOIN",		INT_TYPE_VAR,   offsetof(CSetList, set_auto_rejoin) ,NULL, 0 },
{ "BANTIME",			INT_TYPE_VAR,	offsetof(CSetList, set_bantime), NULL, 0 },
{ "BITCH",			BOOL_TYPE_VAR,	offsetof(CSetList, bitch_mode) ,NULL, 0 },
{ "CHANMODE",			STR_TYPE_VAR,	offsetof(CSetList, chanmode), NULL, 0 },
{ "CHANNEL_LOG",		BOOL_TYPE_VAR,	offsetof(CSetList, channel_log) ,log_channel, 0 },
{ "CHANNEL_LOG_FILE",		STR_TYPE_VAR,	offsetof(CSetList, channel_log_file), NULL, 0 },
{ "CHANNEL_LOG_LEVEL",		STR_TYPE_VAR,	offsetof(CSetList, log_level), set_msglog_channel_level, 0 },
{ "COMPRESS_MODES",		BOOL_TYPE_VAR,	offsetof(CSetList, compress_modes) ,NULL, 0 },
{ "CTCP_FLOOD_BAN",		BOOL_TYPE_VAR,	offsetof(CSetList, set_ctcp_flood_ban), NULL, 0 },
{ "DEOPFLOOD",			BOOL_TYPE_VAR,	offsetof(CSetList, set_deopflood) ,NULL, 0 },
{ "DEOPFLOOD_TIME",		INT_TYPE_VAR,	offsetof(CSetList, set_deopflood_time) ,NULL, 0 },
{ "DEOP_ON_DEOPFLOOD",		INT_TYPE_VAR,	offsetof(CSetList, set_deop_on_deopflood) ,NULL, 0 },
{ "DEOP_ON_KICKFLOOD",		INT_TYPE_VAR,	offsetof(CSetList, set_deop_on_kickflood) ,NULL, 0 },
{ "HACKING",			INT_TYPE_VAR,	offsetof(CSetList, set_hacking) ,NULL, 0 },
{ "JOINFLOOD",			BOOL_TYPE_VAR,	offsetof(CSetList, set_joinflood) ,NULL, 0 },
{ "JOINFLOOD_TIME",		INT_TYPE_VAR,	offsetof(CSetList, set_joinflood_time) ,NULL, 0 },
{ "KICKFLOOD",			BOOL_TYPE_VAR,	offsetof(CSetList, set_kickflood) ,NULL, 0 },
{ "KICKFLOOD_TIME",		INT_TYPE_VAR,	offsetof(CSetList, set_kickflood_time) ,NULL, 0 },
{ "KICK_IF_BANNED",		BOOL_TYPE_VAR,  offsetof(CSetList, set_kick_if_banned) ,NULL, 0 },
{ "KICK_ON_DEOPFLOOD",		INT_TYPE_VAR,   offsetof(CSetList, set_kick_on_deopflood) ,NULL, 0 },
{ "KICK_ON_JOINFLOOD",		INT_TYPE_VAR,	offsetof(CSetList, set_kick_on_joinflood) ,NULL, 0 },
{ "KICK_ON_KICKFLOOD",		INT_TYPE_VAR,   offsetof(CSetList, set_kick_on_kickflood) ,NULL, 0 },
{ "KICK_ON_NICKFLOOD",		INT_TYPE_VAR,   offsetof(CSetList, set_kick_on_nickflood) ,NULL, 0 },
{ "KICK_ON_PUBFLOOD",		INT_TYPE_VAR,   offsetof(CSetList, set_kick_on_pubflood) ,NULL, 0 },
{ "KICK_OPS",			BOOL_TYPE_VAR,	offsetof(CSetList, set_kick_ops), NULL, 0 },
{ "LAMEIDENT",			BOOL_TYPE_VAR,	offsetof(CSetList, set_lame_ident), NULL, 0},
{ "LAMELIST",			BOOL_TYPE_VAR,  offsetof(CSetList, set_lamelist) ,NULL, 0 },
{ "NICKFLOOD",			BOOL_TYPE_VAR,	offsetof(CSetList, set_nickflood) ,NULL, 0 },
{ "NICKFLOOD_TIME",		INT_TYPE_VAR,	offsetof(CSetList, set_nickflood_time) ,NULL, 0 },
{ "PUBFLOOD",			BOOL_TYPE_VAR,	offsetof(CSetList, set_pubflood) ,NULL, 0 },
{ "PUBFLOOD_IGNORE_TIME",	INT_TYPE_VAR,	offsetof(CSetList, set_pubflood_ignore) ,NULL, 0 },
{ "PUBFLOOD_TIME",		INT_TYPE_VAR,	offsetof(CSetList, set_pubflood_time) ,NULL, 0 },
{ "SHITLIST",			BOOL_TYPE_VAR,  offsetof(CSetList, set_shitlist) ,NULL, 0 },
{ "USERLIST",			BOOL_TYPE_VAR,  offsetof(CSetList, set_userlist) ,NULL, 0 },
{ NULL,				0,		0, NULL, 0 }
};

static WSetArray wset_array[] = {
{ "STATUS_AWAY",		STR_TYPE_VAR,	offsetof(WSet, status_away),		offsetof(WSet, away_format), BX_build_status },
{ "STATUS_CDCCCOUNT",		STR_TYPE_VAR,	offsetof(WSet, status_cdcccount),	offsetof(WSet, cdcc_format), BX_build_status },
{ "STATUS_CHANNEL",		STR_TYPE_VAR,	offsetof(WSet, status_channel),		offsetof(WSet, channel_format), BX_build_status },
{ "STATUS_CHANOP",		STR_TYPE_VAR,	offsetof(WSet, status_chanop),		-1, BX_build_status },
{ "STATUS_CLOCK",		STR_TYPE_VAR,	offsetof(WSet, status_clock),		offsetof(WSet, clock_format), BX_build_status },
{ "STATUS_CPU_SAVER",		STR_TYPE_VAR,	offsetof(WSet, status_cpu_saver), 	offsetof(WSet, cpu_saver_format), BX_build_status },
{ "STATUS_DCCCOUNT",		STR_TYPE_VAR,	offsetof(WSet, status_dcccount),	offsetof(WSet, dcccount_format), BX_build_status },
{ "STATUS_FLAG",		STR_TYPE_VAR,   offsetof(WSet, status_flag),		offsetof(WSet, flag_format), BX_build_status },
{ "STATUS_FORMAT",              STR_TYPE_VAR,   offsetof(WSet, format_status),		-1, BX_build_status },
{ "STATUS_FORMAT1",             STR_TYPE_VAR,   offsetof(WSet, format_status[1]),	-1, BX_build_status },
{ "STATUS_FORMAT2",             STR_TYPE_VAR,   offsetof(WSet, format_status[2]),	-1, BX_build_status },
{ "STATUS_FORMAT3",             STR_TYPE_VAR,   offsetof(WSet, format_status[3]),	-1, BX_build_status },
{ "STATUS_HALFOP",		STR_TYPE_VAR,	offsetof(WSet, status_halfop),		-1, BX_build_status },
{ "STATUS_HOLD",		STR_TYPE_VAR,	offsetof(WSet, status_hold),		-1, BX_build_status },
{ "STATUS_HOLD_LINES",		STR_TYPE_VAR,	offsetof(WSet, status_hold_lines),	offsetof(WSet, hold_lines_format), BX_build_status },
{ "STATUS_LAG",			STR_TYPE_VAR,	offsetof(WSet, status_lag),		offsetof(WSet, lag_format), BX_build_status },
{ "STATUS_MAIL",		STR_TYPE_VAR,	offsetof(WSet, status_mail),		offsetof(WSet, mail_format), BX_build_status },
{ "STATUS_MODE",		STR_TYPE_VAR,	offsetof(WSet, status_mode),		offsetof(WSet, mode_format), BX_build_status },
{ "STATUS_MSGCOUNT",		STR_TYPE_VAR,	offsetof(WSet, status_msgcount),	offsetof(WSet, msgcount_format), BX_build_status },
{ "STATUS_NICK",		STR_TYPE_VAR,	offsetof(WSet, status_nick),		offsetof(WSet, nick_format), BX_build_status },
{ "STATUS_NOTIFY",		STR_TYPE_VAR,	offsetof(WSet, status_notify),		offsetof(WSet, notify_format), BX_build_status },
{ "STATUS_OPER_KILLS",		STR_TYPE_VAR,	offsetof(WSet, status_oper_kills),	offsetof(WSet, kills_format), BX_build_status },
{ "STATUS_QUERY",		STR_TYPE_VAR,	offsetof(WSet, status_query),		offsetof(WSet, query_format), BX_build_status },
{ "STATUS_SCROLLBACK",		STR_TYPE_VAR,	offsetof(WSet, status_scrollback),	-1, BX_build_status },
{ "STATUS_SERVER",		STR_TYPE_VAR,	offsetof(WSet, status_server),		offsetof(WSet, server_format), BX_build_status },
{ "STATUS_TOPIC",		STR_TYPE_VAR,	offsetof(WSet, status_topic),		offsetof(WSet, topic_format), BX_build_status },
{ "STATUS_UMODE",		STR_TYPE_VAR,	offsetof(WSet, status_umode),		offsetof(WSet, umode_format), BX_build_status },
{ "STATUS_USER0",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats0),	-1, BX_build_status },
{ "STATUS_USER1",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats1),	-1, BX_build_status },
{ "STATUS_USER10",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats10),	-1, BX_build_status },
{ "STATUS_USER11",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats11),	-1, BX_build_status },
{ "STATUS_USER12",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats12),	-1, BX_build_status },
{ "STATUS_USER13",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats13),	-1, BX_build_status },
{ "STATUS_USER14",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats14),	-1, BX_build_status },
{ "STATUS_USER15",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats15),	-1, BX_build_status },
{ "STATUS_USER16",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats16),	-1, BX_build_status },
{ "STATUS_USER17",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats17),	-1, BX_build_status },
{ "STATUS_USER18",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats18),	-1, BX_build_status },
{ "STATUS_USER19",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats19),	-1, BX_build_status },
{ "STATUS_USER2",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats2),	-1, BX_build_status },
{ "STATUS_USER20",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats20),	-1, BX_build_status },
{ "STATUS_USER21",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats21),	-1, BX_build_status },
{ "STATUS_USER22",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats22),	-1, BX_build_status },
{ "STATUS_USER23",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats23),	-1, BX_build_status },
{ "STATUS_USER24",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats24),	-1, BX_build_status },
{ "STATUS_USER25",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats25),	-1, BX_build_status },
{ "STATUS_USER26",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats26),	-1, BX_build_status },
{ "STATUS_USER27",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats27),	-1, BX_build_status },
{ "STATUS_USER28",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats28),	-1, BX_build_status },
{ "STATUS_USER29",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats29),	-1, BX_build_status },
{ "STATUS_USER3",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats3),	-1, BX_build_status },
{ "STATUS_USER30",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats30),	-1, BX_build_status },
{ "STATUS_USER31",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats31),	-1, BX_build_status },
{ "STATUS_USER32",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats32),	-1, BX_build_status },
{ "STATUS_USER33",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats33),	-1, BX_build_status },
{ "STATUS_USER34",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats34),	-1, BX_build_status },
{ "STATUS_USER35",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats35),	-1, BX_build_status },
{ "STATUS_USER36",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats36),	-1, BX_build_status },
{ "STATUS_USER37",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats37),	-1, BX_build_status },
{ "STATUS_USER38",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats38),	-1, BX_build_status },
{ "STATUS_USER39",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats39),	-1, BX_build_status },
{ "STATUS_USER4",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats4),	-1, BX_build_status },
{ "STATUS_USER5",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats5),	-1, BX_build_status },
{ "STATUS_USER6",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats6),	-1, BX_build_status },
{ "STATUS_USER7",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats7),	-1, BX_build_status },
{ "STATUS_USER8",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats8),	-1, BX_build_status },
{ "STATUS_USER9",		STR_TYPE_VAR,	offsetof(WSet, status_user_formats9),	-1, BX_build_status },
{ "STATUS_USERS",		STR_TYPE_VAR,	offsetof(WSet, status_users),		offsetof(WSet, status_users_format), BX_build_status },
{ "STATUS_VOICE",		STR_TYPE_VAR,	offsetof(WSet, status_voice),		-1, BX_build_status },
{ "STATUS_WINDOW",		STR_TYPE_VAR,	offsetof(WSet, status_window),		-1, BX_build_status },
{ NULL,				0,		0, -1, NULL } 
};

CSetList *cset_queue = NULL;

/*
 * Returns the address of the requested field within the given CSetList.
 */
static void *get_cset_var_address(CSetList *tmp, int var)
{
	void *ptr = ((char *)tmp + cset_array[var].offset);
	return ptr;
}

/*
 * returns the requested int from the cset struct
 * Will work fine with either BOOL or INT type of csets.
 */
int BX_get_cset_int_var(CSetList *tmp, int var)
{
	int *ptr = get_cset_var_address(tmp, var);
	return *ptr;
}

char *BX_get_cset_str_var(CSetList *tmp, int var)
{
	char **ptr = get_cset_var_address(tmp, var);
	return *ptr;
}

/*
 * sets the requested int from the cset struct
 * Will work fine with either BOOL or INT type of csets.
 */
void BX_set_cset_int_var(CSetList *tmp, int var, int value)
{
	int *ptr = get_cset_var_address(tmp, var);
	*ptr = value;
}

void BX_set_cset_str_var(CSetList *tmp, int var, const char *value)
{
	char **ptr = get_cset_var_address(tmp, var);
	if (value)
		malloc_strcpy(ptr, value);
	else
		new_free(ptr);
}

/*
 * Next few functions ripped from vars.c, as we are really doing the same
 * thing as those functions did, with a couple changes... The nice
 * thing about this is that we will get the same "feel" now with
 * SET's and CSET's
 */
static int find_cset_variable(CSetArray *array, char *org_name, int *cnt)
{
	CSetArray *v, *first;
	int     len, var_index;
	char    *name = NULL;

	len = strlen(org_name);
	name = alloca(len + 1);
	strcpy(name, org_name);

	upper(name);
	var_index = 0;
	for (first = array; first->name; first++, var_index++) 
	{
		if (strncmp(name, first->name, len) == 0) 
		{
			*cnt = 1;
			break;
		}
	}
	if (first->name) 
	{
		if (strlen(first->name) != len) 
		{
			v = first;
			for (v++; v->name; v++, (*cnt)++) 
			{
				if (strncmp(name, v->name, len) != 0)
					break;
			}
		}
		return (var_index);
	}
	else 
	{
		*cnt = 0;
		return (-1);
	}
}


static void set_cset_var_value(CSetList *tmp, int var_index, char *value)
{
	char    *rest;
	CSetArray *var;

	var = &(cset_array[var_index]);

	switch (var->type) 
	{
	case BOOL_TYPE_VAR:
        	if (value && *value && (value = next_arg(value, &rest))) {
			if (do_boolean(value, (int *)get_cset_var_address(tmp, var_index)))
			{
				say("Value must be either ON, OFF, or TOGGLE");
				break;
			}
			if (var->func)
				var->func(var, tmp);
			put_it("%s", convert_output_format(fget_string_var(FORMAT_CSET_FSET), "%s %s %s", var->name, tmp->channel, get_cset_int_var(tmp, var_index)?var_settings[ON] : var_settings[OFF]));
		}
		else 
			put_it("%s", convert_output_format(fget_string_var(FORMAT_CSET_FSET), "%s %s %s", var->name, tmp->channel, get_cset_int_var(tmp, var_index)?var_settings[ON] : var_settings[OFF]));
		break;

	case INT_TYPE_VAR:
		if (value && *value && (value = next_arg(value, &rest))) 
		{
			int     val;

			if (!is_number(value)) 
			{
				say("Value of %s must be numeric!", var->name);
				break;
			}
			if ((val = atoi(value)) < 0) 
			{
				say("Value of %s must be greater than 0", var->name);
				break;
			}
			set_cset_int_var(tmp, var_index, val);
			if (var->func)
				var->func(var, tmp);
			put_it("%s", convert_output_format(fget_string_var(FORMAT_CSET_FSET), "%s %s %d", var->name, tmp->channel, get_cset_int_var(tmp, var_index)));
		}
		else 
			put_it("%s", convert_output_format(fget_string_var(FORMAT_CSET_FSET), "%s %s %d", var->name, tmp->channel, get_cset_int_var(tmp, var_index)));
		break;
	case STR_TYPE_VAR:
		if (value && *value)
		{
			set_cset_str_var(tmp, var_index, value);
			if (var->func)
				var->func(var, tmp);
			put_it("%s", convert_output_format(fget_string_var(FORMAT_CSET_FSET), "%s %s %s", var->name, tmp->channel, get_cset_str_var(tmp, var_index) ));
		}
		else
			put_it("%s", convert_output_format(fget_string_var(FORMAT_CSET_FSET), "%s %s %s", var->name, tmp->channel, (char *)get_cset_str_var(tmp, var_index)));
	}
}

CSetList *check_cset_queue(char *channel, int add)
{
	CSetList *c = NULL;
	int found = 0;
	if (!strchr(channel, '*') && !(c = (CSetList *)find_in_list((List **)&cset_queue, channel, 0)))
	{
		if (!add) 
		{
			for (c = cset_queue; c; c = c->next)
				if (!my_stricmp(c->channel, channel) || wild_match(c->channel, channel))
					return c;
			return NULL;
		}
		c = create_csets_for_channel(channel);
		add_to_list((List **)&cset_queue, (List *)c);
		found++;
	}
	if (c)
		return c;
	if (add && !found)
	{
		for (c = cset_queue; c; c = c->next)
			if (!my_stricmp(c->channel, channel))
				return c;
		c = create_csets_for_channel(channel);
		c->next = cset_queue;
		cset_queue = c;
		return c;
	}
	return NULL;
}

static inline void cset_variable_case1(char *channel, int var_index, char *args)
{
	ChannelList *chan = NULL;
	int tmp = 0;
	int count = 0;
	/* 
	 * implement a queue for channels that don't exist... later...
	 * go home if user doesn't have any channels.
	 */
	if (current_window->server != -1)
	{
		for (chan = get_server_channels(current_window->server); chan; chan = chan->next) 
		{
			tmp = var_index;
			if (wild_match(channel, chan->channel))
			{
				set_cset_var_value(chan->csets, tmp, args);
				count++;
			}
		}
	}
	/* no channel match. lets check the queue */
/*	if (!count)*/
	{
		CSetList *c = NULL;
		if (!count)
			check_cset_queue(channel, 1);
		for (c = cset_queue; c; c = c->next)
		{
			tmp = var_index;
			if (!my_stricmp(channel, c->channel) || wild_match(channel, c->channel))
			{
				set_cset_var_value(c, tmp, args);
				count++;
			}
		}
		if (!count)
			say("CSET_VARIABLE: No match in cset queue for %s", channel);
	}
}

static inline void cset_variable_casedef(char *channel, int cnt, int var_index, char *args)
{
	ChannelList *chan = NULL; 
	int tmp, tmp2;
	int count = 0;
	
	if (current_window->server != -1)
	{
		for (chan = get_server_channels(current_window->server); chan; chan = chan->next) 
		{
			tmp = var_index;
			tmp2 = cnt;
			if (wild_match(channel, chan->channel)) 
			{
				for (tmp2 += tmp; tmp < tmp2; tmp++)
					set_cset_var_value(chan->csets, tmp, empty_string);
				count++;
			}
		}
	}
/*	if (!count) */
	{
		CSetList *c = NULL;
		if (!count)
			check_cset_queue(channel, 1);
		for (c = cset_queue; c; c = c->next)
		{
			tmp = var_index;
			tmp2 = cnt;
			if (!my_stricmp(channel, c->channel) || wild_match(channel, c->channel))
			{
				for (tmp2 +=tmp; tmp < tmp2; tmp++)
					set_cset_var_value(c, tmp, empty_string);
				count++;
			}
		}
		if (!count)
			say("CSET_VARIABLE: No match in cset queue for %s", channel);
		return;
	}

}

static inline void cset_variable_noargs(char *channel)
{
	int var_index = 0;
	ChannelList *chan = NULL; 
	int count = 0;

	if (current_window->server != -1)
	{
		for (chan = get_server_channels(current_window->server); chan; chan = chan->next) 
		{
			if (wild_match(channel, chan->channel)) 
			{	
				for (var_index = 0; var_index < NUMBER_OF_CSETS; var_index++)
					set_cset_var_value(chan->csets, var_index, empty_string);
				count++;
			}
		}
	}
/*	if (!count) */
	{
		CSetList *c = NULL;
		if (!count)
			check_cset_queue(channel, 1);
		for (c = cset_queue; c; c = c->next)
		{
			if (!wild_match(channel, c->channel))
				continue;
			for (var_index = 0; var_index < NUMBER_OF_CSETS; var_index++)
				set_cset_var_value(c, var_index, empty_string);
			count++;
		}
		if (!count)
			say("CSET_VARIABLE: No match in cset queue for %s", channel ? channel : empty_string);
		return;
	}

}

char *set_cset(char *var, ChannelList *chan, char *value)
{
int var_index, cnt = 0;
	var_index = find_cset_variable(cset_array, var, &cnt);
	if (cnt == 1)
	{
		CSetArray *var;
		var = &(cset_array[var_index]);
		switch(var->type)
		{
			case BOOL_TYPE_VAR:
			{
				int i = my_atol(value);
				set_cset_int_var(chan->csets, var_index, i? 1 : 0);
				break;
			}
			case INT_TYPE_VAR:
			{
				int i = my_atol(value);
				set_cset_int_var(chan->csets, var_index, i);
				break;
			}
			case STR_TYPE_VAR:
			{
				set_cset_str_var(chan->csets, var_index, value);
				break;
			}
			default:
				return NULL;
		}
		return value;
	}
	return NULL;
}

char *get_cset(char *var, ChannelList *chan, char *value)
{
int var_index, cnt = 0;
	var_index = find_cset_variable(cset_array, var, &cnt);
	if (cnt == 1)
	{
		char s[81];
		CSetArray *var;
		var = &(cset_array[var_index]);
		*s = 0;
		switch (var->type)
		{
			case BOOL_TYPE_VAR:
			{
				strcpy(s, get_cset_int_var(chan->csets, var_index)?var_settings[ON] : var_settings[OFF]);
				if (value)
				{
					int val = -1;
					if (!my_stricmp(value, on))
						val = 1;
					else if (!my_stricmp(value, off))
						val = 0;
					else
					{
						if (isdigit((unsigned char)*value))
							val = (int)(*value - '0');
					}
					if (val != -1)
						set_cset_int_var(chan->csets, var_index, val);
				}
				break;
			}
			case INT_TYPE_VAR:
			{
				strncpy(s, ltoa(get_cset_int_var(chan->csets, var_index)), 30);
				if (value && isdigit((unsigned char)*value))
					set_cset_int_var(chan->csets, var_index, my_atol(value)); 
				break;
			}
			case STR_TYPE_VAR:
			{
				char *t;
				t = m_strdup(get_cset_str_var(chan->csets, var_index));
				if (value)
					set_cset_str_var(chan->csets, var_index, value);
				return t;
			}
		}		
		return m_strdup(s && *s ? s : empty_string);
	}
	return m_strdup(empty_string);
}

BUILT_IN_COMMAND(cset_variable)
{
	char    *var, *channel = NULL;
	int     no_args = 1, cnt, var_index, hook = 1;

	if (from_server != -1 && current_window->server != -1)
	{
		if (args && *args && (is_channel(args) || *args == '*'))
			channel = next_arg(args, &args);
		else
			channel = get_current_channel_by_refnum(0);
	}
	else if (args && *args && (is_channel(args) || *args == '*'))
		channel = next_arg(args, &args);
		
	if (!channel)
		return;

	if ((var = next_arg(args, &args)) != NULL) 
	{
		if (*var == '-') 
		{
			var++;
			args = NULL;
		}
		var_index = find_cset_variable(cset_array, var, &cnt);
		
		if (hook)
		{
			switch (cnt) 
			{
				case 0:
					say("No such variable \"%s\"", var);
					return;
				case 1:
					cset_variable_case1(channel, var_index, args);
					return;
				default:
					say("%s is ambiguous", var);
					cset_variable_casedef(channel, cnt, var_index, args);
					return;
			}
		}
	}
	if (no_args)
		cset_variable_noargs(channel);
}

CSetList *create_csets_for_channel(char *channel)
{
	CSetList *tmp; 
#ifdef VAR_DEBUG
 	int i;
	
	for (i = 1; i < NUMBER_OF_CSETS - 1; i++)
		if (strcmp(cset_array[i-1].name, cset_array[i].name) >= 0)
			ircpanic("Variable [%d] (%s) is out of order.", i, cset_array[i].name);
#endif

	if (check_cset_queue(channel, 0))
	{
		if ((tmp = (CSetList *)find_in_list((List **)&cset_queue, channel, 0)))
			return tmp;
		for (tmp = cset_queue; tmp; tmp = tmp->next)
			if (!my_stricmp(tmp->channel, channel) || wild_match(tmp->channel, channel))
				return tmp;
	}		
	tmp = (CSetList *) new_malloc(sizeof(CSetList));
	/* use default settings. */
	tmp->set_aop = get_int_var(AOP_VAR);
	tmp->set_annoy_kick = get_int_var(ANNOY_KICK_VAR);
	tmp->set_ainv = get_int_var(AINV_VAR);
	tmp->set_auto_join_on_invite = get_int_var(AUTO_JOIN_ON_INVITE_VAR);
	tmp->set_auto_rejoin = get_int_var(AUTO_REJOIN_VAR);
	tmp->set_bantime = get_int_var(BANTIME_VAR);
	tmp->compress_modes = get_int_var(COMPRESS_MODES_VAR);
	tmp->bitch_mode = get_int_var(BITCH_VAR);
	tmp->channel_log = 0;
	
	tmp->log_level = m_strdup("ALL");	
	set_msglog_channel_level(cset_array, tmp); 
#if defined(WINNT) || defined(__EMX__)
	tmp->channel_log_file = m_sprintf("~/bx-conf/%s.log", channel+1);
#else
	tmp->channel_log_file = m_sprintf("~/.BitchX/%s.log", channel+1);
#endif
	tmp->set_joinflood = get_int_var(JOINFLOOD_VAR);
	tmp->set_joinflood_time = get_int_var(JOINFLOOD_TIME_VAR);
	tmp->set_ctcp_flood_ban = get_int_var(CTCP_FLOOD_BAN_VAR);
	tmp->set_deop_on_deopflood = get_int_var(DEOP_ON_DEOPFLOOD_VAR);
	tmp->set_deop_on_kickflood = get_int_var(DEOP_ON_KICKFLOOD_VAR);
	tmp->set_deopflood = get_int_var(DEOPFLOOD_VAR);
	tmp->set_deopflood_time = get_int_var(DEOPFLOOD_TIME_VAR);

	tmp->set_hacking = get_int_var(HACKING_VAR);

	tmp->set_kick_on_deopflood = get_int_var(KICK_ON_DEOPFLOOD_VAR);
	tmp->set_kick_on_joinflood = get_int_var(KICK_ON_JOINFLOOD_VAR);
	tmp->set_kick_on_kickflood = get_int_var(KICK_ON_KICKFLOOD_VAR);
	tmp->set_kick_on_nickflood = get_int_var(KICK_ON_NICKFLOOD_VAR);
	tmp->set_kick_on_pubflood = get_int_var(KICK_ON_PUBFLOOD_VAR);

	tmp->set_kickflood = get_int_var(KICKFLOOD_VAR);
	tmp->set_kickflood_time = get_int_var(KICKFLOOD_TIME_VAR);
	tmp->set_kick_ops = get_int_var(KICK_OPS_VAR);
	tmp->set_nickflood = get_int_var(NICKFLOOD_VAR);
	tmp->set_nickflood_time = get_int_var(NICKFLOOD_TIME_VAR);

	tmp->set_pubflood = get_int_var(PUBFLOOD_VAR);
	tmp->set_pubflood_time = get_int_var(PUBFLOOD_TIME_VAR);
	tmp->set_pubflood_ignore = 60;
	tmp->set_userlist = get_int_var(USERLIST_VAR);
	tmp->set_shitlist = get_int_var(SHITLIST_VAR);
	tmp->set_lamelist = get_int_var(LAMELIST_VAR);
	tmp->set_lame_ident = get_int_var(LAMEIDENT_VAR);
	tmp->set_kick_if_banned = get_int_var(KICK_IF_BANNED_VAR);
	tmp->set_auto_limit = get_int_var(AUTO_LIMIT_VAR);

	
	malloc_strcpy(&tmp->chanmode, get_string_var(CHANMODE_VAR));
	malloc_strcpy(&tmp->channel, channel);

	return tmp;
}

void remove_csets_for_channel(CSetList *tmp)
{
	new_free(&tmp->channel);
	new_free(&tmp->channel_log_file);
	new_free(&tmp->chanmode);
	new_free(&tmp);
}



static int find_wset_variable(WSetArray *array, char *org_name, int *cnt)
{
	WSetArray *v, *first;
	int     len, var_index;
	char    *name = NULL;

	len = strlen(org_name);
	name = alloca(len + 1);
	strcpy(name, org_name);

	upper(name);
	var_index = 0;
	for (first = array; first->name; first++, var_index++) 
	{
		if (strncmp(name, first->name, len) == 0) 
		{
			*cnt = 1;
			break;
		}
	}
	if (first->name) 
	{
		if (strlen(first->name) != len) 
		{
			v = first;
			for (v++; v->name; v++, (*cnt)++) 
			{
				if (strncmp(name, v->name, len) != 0)
					break;
			}
		}
		return (var_index);
	}
	else 
	{
		*cnt = 0;
		return (-1);
	}
}




/*
 * returns the requested int from the cset struct
 * Will work fine with either BOOL or INT type of csets.
 */
char *BX_get_wset_string_var(WSet *tmp, int var)
{
	char **ptr = (char **)((unsigned long)tmp + wset_array[var].offset);
	return *ptr;
}

/*
 * returns the requested int ADDRESS from the wset struct
 * Will work fine with STR_TYPE_VAR type of csets.
 */
static char ** get_wset_str_var_address(WSet *tmp, int var)
{
	char **ptr = (char **)((unsigned long)tmp + wset_array[var].offset);
	return ptr;
}

/*
 * returns the requested int ADDRESS from the wset struct
 * Will work fine with STR_TYPE_VAR type of csets.
 */
char ** get_wset_format_var_address(WSet *tmp, int var)
{
	char **ptr = NULL;
	if (wset_array[var].format_offset != -1)
		ptr = (char **)((unsigned long)tmp + wset_array[var].format_offset);
	return ptr;
}

/*
 * sets the requested string from the wset struct
 */

void BX_set_wset_string_var(WSet *tmp, int var, char *value)
{
	char **ptr = (char **)((unsigned long)tmp + wset_array[var].offset);
	if (value && *value)
		malloc_strcpy(ptr, value);
	else
		new_free(ptr);
}





static void set_wset_var_value(Window *win, int var_index, char *value)
{
	WSetArray *var;

	var = &(wset_array[var_index]);

	switch (var->type) 
	{
	case STR_TYPE_VAR:
		{
			char **val = NULL;
			if ((val = get_wset_str_var_address(win->wset, var_index))) 
			{
				if (value)
				{
					if (*value)
						malloc_strcpy(val, value);
					else
					{
						put_it("%s", convert_output_format(fget_string_var(FORMAT_SET_FSET), "%s %s", var->name, *val?*val:empty_string));
						return;
					}
				} else
					new_free(val);
				if (var->func)
					(var->func) (win, *val, 0);
				say("Value of %s set to %s", var->name, *val ?
					*val : "<EMPTY>");
			}
		}
		break;
	default:
		say("WSET_type not supported");
	}
}

static inline void wset_variable_case1(Window *win, char *name, int var_index, char *args)
{
Window *tmp = NULL;
int count = 0;
int i;
	if (!name)
	{
		set_wset_var_value(win, var_index, args);
		return;
	}
	while ((traverse_all_windows(&tmp)))
	{
		i = var_index;
		if (*name == '*')
		{
			set_wset_var_value(tmp, i, args);
			count++;
		}
		else if ((tmp->name && wild_match(name, tmp->name)) || (tmp->refnum == my_atol(name)))
		{
			set_wset_var_value(tmp, i, args);
			count++;
		}
	}
	if (!count)
		say("No such window name [%s]", name);
}

static inline void wset_variable_casedef(Window *win, char *name, int cnt, int var_index, char *args)
{
Window *tmp = NULL;
int count = 0;
int c, i;
	if (!name)
	{
		for (cnt +=var_index; var_index < cnt; var_index++)
			set_wset_var_value(win, var_index, args);
		return;
	}
	while ((traverse_all_windows(&tmp)))
	{
		c = cnt;
		i = var_index;
		if (*name == '*')
		{
			for (c += i; i < c; i++)
				set_wset_var_value(tmp, i, empty_string);
			count++;
		}
		else if ((tmp->name && wild_match(name, tmp->name)) || (tmp->refnum == my_atol(name)) ) 
		{
			for (c += i; i < c; i++)
				set_wset_var_value(tmp, i, empty_string);
			count++;
		}
	}
	if (!count)
		say("No such window name [%s]", name);
}

static inline void wset_variable_noargs(Window *win, char *name)
{
Window *tmp = NULL;
int var_index = 0;
	if (!name)
	{
		for (var_index = 0; var_index < NUMBER_OF_WSETS; var_index++)
			set_wset_var_value(win, var_index, empty_string);
	} 
	else
	{
		int count = 0;
		while ((traverse_all_windows(&tmp)))
		{
			if (*name == '*')
			{
				for (var_index = 0; var_index < NUMBER_OF_WSETS; var_index++)
					set_wset_var_value(tmp, var_index, empty_string);
				count++;
			}
			else if ((tmp->name && wild_match(name, tmp->name)) || (tmp->refnum == my_atol(name)))
			{
				for (var_index = 0; var_index < NUMBER_OF_WSETS; var_index++)
					set_wset_var_value(tmp, var_index, empty_string);
				count++;
			}
		}
		if (!count)
			say("No such window name [%s]", name);
	}
}


BUILT_IN_COMMAND(wset_variable)
{
	char    *var;
	char	*possible;
	char 	*name = NULL;
	int     no_args = 1, cnt, var_index;
	Window *win = screen_list->window_list;

	if (args)
	{
		if (*args == '*')
			name = next_arg(args, &args);
		else if (args && *args)
		{
			possible = LOCAL_COPY(args);
			name = next_arg(possible, &possible);
			if (!(win = get_window_by_name(name)))
			{
				win = get_window_by_refnum(my_atol(name));
				if (!win)
				{
					if (isdigit((unsigned char)*name))
					{
						say("No such window refnum %d", my_atol(name));
						return;
					}
				} 
				else if (isdigit((unsigned char)*name))
					var = next_arg(args, &args);
				name = NULL;
			}
			else
				var = next_arg(args, &args);
		}
	}
	else 
		win = current_window;

	
	if ((var = next_arg(args, &args)) != NULL) 
	{
		if (*var == '-') 
		{
			var++;
			args = NULL;
		}
		var_index = find_wset_variable(wset_array, var, &cnt);
		switch (cnt) 
		{
			case 0:
				say("No such variable \"%s\"", var);
				return;
			case 1:
				wset_variable_case1(win, name, var_index, args);
				update_all_status(win, NULL, 0);
				update_all_windows();
				return;
			default:
				say("%s is ambiguous", var);
				wset_variable_casedef(win, name, cnt, var_index, args);
				return;
		}
	}
	if (no_args) 
		wset_variable_noargs(win, name);
}


WSet *create_wsets_for_window(Window *win)
{
	WSet *tmp; 
#ifdef VAR_DEBUG
 	int i;
	
	for (i = 1; i < NUMBER_OF_CSETS - 1; i++)
		if (strcmp(wset_array[i-1].name, wset_array[i].name) >= 0)
			ircpanic("Variable [%d] (%s) is out of order.", i, wset_array[i].name);
#endif

	tmp = (WSet *) new_malloc(sizeof(WSet));

	malloc_strcpy(&tmp->status_channel, get_string_var(STATUS_CHANNEL_VAR));
	malloc_strcpy(&tmp->status_chanop, get_string_var(STATUS_CHANOP_VAR));
	malloc_strcpy(&tmp->status_halfop, get_string_var(STATUS_HALFOP_VAR));
	malloc_strcpy(&tmp->status_clock, get_string_var(STATUS_CLOCK_VAR));
	malloc_strcpy(&tmp->status_hold_lines, get_string_var(STATUS_HOLD_LINES_VAR));
	malloc_strcpy(&tmp->status_hold, get_string_var(STATUS_HOLD_VAR));
	malloc_strcpy(&tmp->status_voice, get_string_var(STATUS_VOICE_VAR));
	malloc_strcpy(&tmp->status_dcccount, get_string_var(STATUS_DCCCOUNT_VAR));
	malloc_strcpy(&tmp->status_cdcccount, get_string_var(STATUS_CDCCCOUNT_VAR));
	malloc_strcpy(&tmp->status_cpu_saver, get_string_var(STATUS_CPU_SAVER_VAR));
	malloc_strcpy(&tmp->status_lag, get_string_var(STATUS_LAG_VAR));
	malloc_strcpy(&tmp->status_away, get_string_var(STATUS_AWAY_VAR));
	malloc_strcpy(&tmp->status_mail, get_string_var(STATUS_MAIL_VAR));
	malloc_strcpy(&tmp->status_mode, get_string_var(STATUS_MODE_VAR));
	malloc_strcpy(&tmp->status_notify, get_string_var(STATUS_NOTIFY_VAR));
	malloc_strcpy(&tmp->status_oper_kills, get_string_var(STATUS_OPER_KILLS_VAR));
	malloc_strcpy(&tmp->status_query, get_string_var(STATUS_QUERY_VAR));
	malloc_strcpy(&tmp->status_server, get_string_var(STATUS_SERVER_VAR));
	malloc_strcpy(&tmp->status_topic, get_string_var(STATUS_TOPIC_VAR));
	malloc_strcpy(&tmp->status_umode, get_string_var(STATUS_UMODE_VAR));
	malloc_strcpy(&tmp->status_users, get_string_var(STATUS_USERS_VAR));
	malloc_strcpy(&tmp->status_msgcount, get_string_var(STATUS_MSGCOUNT_VAR));
	malloc_strcpy(&tmp->status_nick, get_string_var(STATUS_NICK_VAR));
	malloc_strcpy(&tmp->status_flag, get_string_var(STATUS_FLAG_VAR));		
	malloc_strcpy(&tmp->status_scrollback, get_string_var(STATUS_SCROLLBACK_VAR));

	malloc_strcpy(&tmp->format_status[0], get_string_var(STATUS_FORMAT_VAR));
	malloc_strcpy(&tmp->format_status[1], get_string_var(STATUS_FORMAT1_VAR));
	malloc_strcpy(&tmp->format_status[2], get_string_var(STATUS_FORMAT2_VAR));
	malloc_strcpy(&tmp->format_status[3], get_string_var(STATUS_FORMAT3_VAR));

	malloc_strcpy(&tmp->status_user_formats0, get_string_var(STATUS_USER0_VAR));
	malloc_strcpy(&tmp->status_user_formats1, get_string_var(STATUS_USER1_VAR));
	malloc_strcpy(&tmp->status_user_formats10, get_string_var(STATUS_USER10_VAR));
	malloc_strcpy(&tmp->status_user_formats11, get_string_var(STATUS_USER11_VAR));
	malloc_strcpy(&tmp->status_user_formats12, get_string_var(STATUS_USER12_VAR));
	malloc_strcpy(&tmp->status_user_formats13, get_string_var(STATUS_USER13_VAR));
	malloc_strcpy(&tmp->status_user_formats14, get_string_var(STATUS_USER14_VAR));
	malloc_strcpy(&tmp->status_user_formats15, get_string_var(STATUS_USER15_VAR));
	malloc_strcpy(&tmp->status_user_formats16, get_string_var(STATUS_USER16_VAR));
	malloc_strcpy(&tmp->status_user_formats17, get_string_var(STATUS_USER17_VAR));
	malloc_strcpy(&tmp->status_user_formats18, get_string_var(STATUS_USER18_VAR));
	malloc_strcpy(&tmp->status_user_formats19, get_string_var(STATUS_USER19_VAR));
	malloc_strcpy(&tmp->status_user_formats2, get_string_var(STATUS_USER2_VAR));
	malloc_strcpy(&tmp->status_user_formats20, get_string_var(STATUS_USER20_VAR));
	malloc_strcpy(&tmp->status_user_formats21, get_string_var(STATUS_USER21_VAR));
	malloc_strcpy(&tmp->status_user_formats22, get_string_var(STATUS_USER22_VAR));
	malloc_strcpy(&tmp->status_user_formats23, get_string_var(STATUS_USER23_VAR));
	malloc_strcpy(&tmp->status_user_formats24, get_string_var(STATUS_USER24_VAR));
	malloc_strcpy(&tmp->status_user_formats25, get_string_var(STATUS_USER25_VAR));
	malloc_strcpy(&tmp->status_user_formats26, get_string_var(STATUS_USER26_VAR));
	malloc_strcpy(&tmp->status_user_formats27, get_string_var(STATUS_USER27_VAR));
	malloc_strcpy(&tmp->status_user_formats28, get_string_var(STATUS_USER28_VAR));
	malloc_strcpy(&tmp->status_user_formats29, get_string_var(STATUS_USER29_VAR));
	malloc_strcpy(&tmp->status_user_formats3, get_string_var(STATUS_USER3_VAR));
	malloc_strcpy(&tmp->status_user_formats30, get_string_var(STATUS_USER30_VAR));
	malloc_strcpy(&tmp->status_user_formats31, get_string_var(STATUS_USER31_VAR));
	malloc_strcpy(&tmp->status_user_formats32, get_string_var(STATUS_USER32_VAR));
	malloc_strcpy(&tmp->status_user_formats33, get_string_var(STATUS_USER33_VAR));
	malloc_strcpy(&tmp->status_user_formats34, get_string_var(STATUS_USER34_VAR));
	malloc_strcpy(&tmp->status_user_formats35, get_string_var(STATUS_USER35_VAR));
	malloc_strcpy(&tmp->status_user_formats36, get_string_var(STATUS_USER36_VAR));
	malloc_strcpy(&tmp->status_user_formats37, get_string_var(STATUS_USER37_VAR));
	malloc_strcpy(&tmp->status_user_formats38, get_string_var(STATUS_USER38_VAR));
	malloc_strcpy(&tmp->status_user_formats39, get_string_var(STATUS_USER39_VAR));
	malloc_strcpy(&tmp->status_user_formats4, get_string_var(STATUS_USER4_VAR));
	malloc_strcpy(&tmp->status_user_formats5, get_string_var(STATUS_USER5_VAR));
	malloc_strcpy(&tmp->status_user_formats6, get_string_var(STATUS_USER6_VAR));
	malloc_strcpy(&tmp->status_user_formats7, get_string_var(STATUS_USER7_VAR));
	malloc_strcpy(&tmp->status_user_formats8, get_string_var(STATUS_USER8_VAR));
	malloc_strcpy(&tmp->status_user_formats9, get_string_var(STATUS_USER9_VAR));
	malloc_strcpy(&tmp->status_window, get_string_var(STATUS_WINDOW_VAR));
	win->wset = tmp;	
	return tmp;
}

void remove_wsets_for_window(Window *tmp)
{
	new_free(&tmp->wset->status_channel);
	new_free(&tmp->wset->status_clock); 
	new_free(&tmp->wset->status_hold_lines);
	new_free(&tmp->wset->status_hold);
	new_free(&tmp->wset->status_voice);
	new_free(&tmp->wset->status_dcccount);
	new_free(&tmp->wset->status_cdcccount);
	new_free(&tmp->wset->status_lag); 
	new_free(&tmp->wset->status_away); 
	new_free(&tmp->wset->status_nick);
	new_free(&tmp->wset->status_flag);	
	new_free(&tmp->wset->status_mail);
	new_free(&tmp->wset->status_msgcount);
	new_free(&tmp->wset->status_cpu_saver);
	new_free(&tmp->wset->status_chanop);
	new_free(&tmp->wset->status_mode);
	
	new_free(&tmp->wset->status_notify);
	new_free(&tmp->wset->status_oper_kills);
	new_free(&tmp->wset->status_query);
	new_free(&tmp->wset->status_server);
	new_free(&tmp->wset->status_topic);
	new_free(&tmp->wset->status_umode);
	new_free(&tmp->wset->status_users);
	new_free(&tmp->wset->status_scrollback);
	new_free(&tmp->wset->status_halfop);	

	new_free(&tmp->wset->status_user_formats1);
	new_free(&tmp->wset->status_user_formats10);
	new_free(&tmp->wset->status_user_formats11);
	new_free(&tmp->wset->status_user_formats12);
	new_free(&tmp->wset->status_user_formats13);
	new_free(&tmp->wset->status_user_formats14);
	new_free(&tmp->wset->status_user_formats15);
	new_free(&tmp->wset->status_user_formats16);
	new_free(&tmp->wset->status_user_formats17);
	new_free(&tmp->wset->status_user_formats18);
	new_free(&tmp->wset->status_user_formats19);
	new_free(&tmp->wset->status_user_formats2);
	new_free(&tmp->wset->status_user_formats20);
	new_free(&tmp->wset->status_user_formats21);
	new_free(&tmp->wset->status_user_formats22);
	new_free(&tmp->wset->status_user_formats23);
	new_free(&tmp->wset->status_user_formats24);
	new_free(&tmp->wset->status_user_formats25);
	new_free(&tmp->wset->status_user_formats26);
	new_free(&tmp->wset->status_user_formats27);
	new_free(&tmp->wset->status_user_formats28);
	new_free(&tmp->wset->status_user_formats29);
	new_free(&tmp->wset->status_user_formats3);
	new_free(&tmp->wset->status_user_formats30);
	new_free(&tmp->wset->status_user_formats31);
	new_free(&tmp->wset->status_user_formats32);
	new_free(&tmp->wset->status_user_formats33);
	new_free(&tmp->wset->status_user_formats34);
	new_free(&tmp->wset->status_user_formats35);
	new_free(&tmp->wset->status_user_formats36);
	new_free(&tmp->wset->status_user_formats37);
	new_free(&tmp->wset->status_user_formats38);
	new_free(&tmp->wset->status_user_formats39);
	new_free(&tmp->wset->status_user_formats4);
	new_free(&tmp->wset->status_user_formats5);
	new_free(&tmp->wset->status_user_formats6);
	new_free(&tmp->wset->status_user_formats7);
	new_free(&tmp->wset->status_user_formats8);
	new_free(&tmp->wset->status_user_formats9);
	new_free(&tmp->wset->status_user_formats0);

	new_free(&tmp->wset->format_status[0]);
	new_free(&tmp->wset->format_status[1]);
	new_free(&tmp->wset->format_status[2]);
	new_free(&tmp->wset->format_status[3]); 

	new_free(&tmp->wset->status_format[0]);
	new_free(&tmp->wset->status_format[1]);
	new_free(&tmp->wset->status_format[2]);
	new_free(&tmp->wset->status_format[3]); 
	new_free(&tmp->wset->status_line[0]);
	new_free(&tmp->wset->status_line[1]);
	new_free(&tmp->wset->status_line[2]);

	new_free(&tmp->wset->mode_format);
	new_free(&tmp->wset->umode_format);
	new_free(&tmp->wset->topic_format);
	new_free(&tmp->wset->query_format);
	new_free(&tmp->wset->clock_format);
	new_free(&tmp->wset->hold_lines_format);
	new_free(&tmp->wset->channel_format);
	new_free(&tmp->wset->mail_format);
	new_free(&tmp->wset->server_format);
	new_free(&tmp->wset->notify_format);
	new_free(&tmp->wset->kills_format);
	new_free(&tmp->wset->lag_format);
	new_free(&tmp->wset->cpu_saver_format);
	new_free(&tmp->wset->msgcount_format);
	new_free(&tmp->wset->dcccount_format);
	new_free(&tmp->wset->cdcc_format);
	new_free(&tmp->wset->nick_format);
	new_free(&tmp->wset->away_format);
	new_free(&tmp->wset->flag_format);
	new_free(&tmp->wset->status_users_format);
	new_free(&tmp->wset->status_window);
	new_free(&tmp->wset->window_special_format);
	new_free(&tmp->wset);
}

void log_channel(CSetArray *var, CSetList *cs)
{
	ChannelList *chan;
	if (!cs->channel_log_file)
	{
		bitchsay("Try setting a channel log file first");
		set_cset_int_var(cs, CHANNEL_LOG_CSET, 0);
		return;
	}
	if ((chan = lookup_channel(cs->channel, from_server, 0)))
		do_log(cs->channel_log, cs->channel_log_file, &chan->msglog_fp);
}

void set_msglog_channel_level(CSetArray *var, CSetList *cs)
{
	ChannelList *chan;
	if ((chan = lookup_channel(cs->channel, from_server, 0)))
	{
		chan->log_level = parse_lastlog_level(cs->log_level, 1);
		set_cset_str_var(cs, CHANNEL_LOG_LEVEL_CSET, bits_to_lastlog_level(chan->log_level));
	}
}

void do_logchannel(unsigned long level, ChannelList *chan, char *format, ...)
{
	if (!chan || !get_cset_int_var(chan->csets, CHANNEL_LOG_CSET))
		return;
	if ((chan->log_level & level) && format)
	{
		char s[BIG_BUFFER_SIZE+1];
		va_list args;
		va_start(args, format);
		vsnprintf(s, BIG_BUFFER_SIZE, format, args);
		va_end(args);
		add_to_log(chan->msglog_fp, now, s, logfile_line_mangler);
	}
}

void set_channel_limit(ChannelList *channel, int currentlimit, int add, int numusers)
{
	if ((now - channel->limit_time) < 30)
		return;
	if (add && numusers)
	{
		if ((currentlimit - numusers) < (add / 2) && ((numusers + add) != currentlimit))
			my_send_to_server(channel->server, "MODE %s +l %d", channel->channel, numusers + add);
		else if ((currentlimit - numusers) > (currentlimit + (add / 2)) && ((numusers + add) != currentlimit))
			my_send_to_server(channel->server, "MODE %s +l %d", channel->channel, numusers + add);
	}
	else
		my_send_to_server(channel->server, "MODE %s -l", channel->channel);
	channel->limit_time = now;
}

void check_channel_limit(ChannelList *chan)
{
	if (chan && chan->csets && chan->csets->set_auto_limit && chan->have_op)
	{
		int count = 0;
		NickList *nick;
		for (nick = next_nicklist(chan, NULL); nick; nick = next_nicklist(chan, nick))
			count++;
		set_channel_limit(chan, chan->limit, chan->csets->set_auto_limit, count);
	}
}

void limit_channel(CSetArray *var, CSetList *cs)
{
ChannelList *chan;
	if ((chan = lookup_channel(cs->channel, from_server, 0)))
	{
		if (cs->set_auto_limit)
		{
			int count = 0;
			NickList *nick;
			for (nick = next_nicklist(chan, NULL); nick; nick = next_nicklist(chan, nick))
				count++;
			set_channel_limit(chan, chan->limit, cs->set_auto_limit, count);
		} else
			set_channel_limit(chan, chan->limit, 0, 0);
	}
}

#ifdef GUI
CSetArray *return_cset_var(int nummer)
{
   return &cset_array[nummer];
}

WSetArray *return_wset_var(int nummer)
{
   return &wset_array[nummer];
}
#endif

