/*
 * funny.c: Well, I put some stuff here and called it funny.  So sue me. 
 *
 * written by michael sandrof
 *
 * copyright(c) 1990 
 *
 * see the copyright file, or do a help ircii copyright 
 */


#include "irc.h"
static char cvsrevision[] = "$Id: funny.c 130 2011-05-16 13:16:25Z keaston $";
CVS_REVISION(funny_c)
#include "struct.h"

#include "ircaux.h"
#include "hook.h"
#include "vars.h"
#include "funny.h"
#include "names.h"
#include "server.h"
#include "lastlog.h"
#include "ircterm.h"
#include "output.h"
#include "numbers.h"
#include "parse.h"
#include "status.h"
#include "misc.h"
#include "screen.h"
#define MAIN_SOURCE
#include "modval.h"

static	char	*match_str = NULL;

static	int	funny_min;
static	int	funny_max;
static	int	funny_flags;

void funny_match(char  *stuff)
{
	malloc_strcpy(&match_str, stuff);
}

void set_funny_flags(int min, int max, int flags)
{
	funny_min = min;
	funny_max = max;
	funny_flags = flags;
}

struct	WideListInfoStru
{
	char	*channel;
	int	users;
};

typedef	struct WideListInfoStru WideList;

static	WideList **wide_list = NULL;
static	int	wl_size = 0;
static	int	wl_elements = 0;

static	int	funny_widelist_users (WideList **, WideList **);
static	int	funny_widelist_names (WideList **, WideList **);

static	int funny_widelist_users(WideList **left, WideList **right)
{
	if ((**left).users > (**right).users)
		return -1;
	else if ((**right).users > (**left).users)
		return 1;
	else
		return my_stricmp((**left).channel, (**right).channel);
}

static	int funny_widelist_names(WideList **left, WideList **right)
{
	int	comp;

	if (!(comp = my_stricmp((**left).channel, (**right).channel)))
		return comp;
	else if ((**left).users > (**right).users)
		return -1;
	else if ((**right).users > (**left).users)
		return 1;
	else
		return 0;
}


void funny_print_widelist(void)
{
	int	i;
	char	buffer1[BIG_BUFFER_SIZE];
	char	buffer2[BIG_BUFFER_SIZE];
	char	*ptr;

	if (!wide_list)
		return;

	if (funny_flags & FUNNY_NAME)
		qsort((void *) wide_list, wl_elements, sizeof(WideList *),
		(int (*) (const void *, const void *)) funny_widelist_names);
	else if (funny_flags & FUNNY_USERS)
		qsort((void *) wide_list, wl_elements, sizeof(WideList *),
		(int (*) (const void *, const void *)) funny_widelist_users);

	set_display_target(NULL, LOG_CRAP);
	*buffer1 = '\0';
	for (i = 1; i < wl_elements; i++)
	{
		sprintf(buffer2, "%s(%d) ", wide_list[i]->channel,
				wide_list[i]->users);
		ptr = strchr(buffer1, '\0');
		if (strlen(buffer1) + strlen(buffer2) > current_term->TI_cols - 5)
		{
			if (do_hook(WIDELIST_LIST, "%s", buffer1))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_WIDELIST_FSET), "%s %s", update_clock(GET_TIME), buffer1));
			*buffer1 = 0;
			strcat(buffer1, buffer2);
		}
		else
			strcpy(ptr, buffer2);
	}
	if (*buffer1 && do_hook(WIDELIST_LIST, "%s", buffer1))
		put_it("%s", convert_output_format(fget_string_var(FORMAT_WIDELIST_FSET), "%s %s", update_clock(GET_TIME), buffer1));

	reset_display_target();
	for (i = 0; i < wl_elements; i++)
	{
		new_free(&wide_list[i]->channel);
		new_free((char **)&wide_list[i]);
	}
	new_free((char **)&wide_list);
	wl_elements = wl_size = 0;
}

void funny_list(char *from, char **ArgList)
{
	char	*channel,
		*user_cnt,
		*line;
	WideList **new_list;
	int	cnt;
	static	char	format[30];
	static	int	last_width = -1;

	if (last_width != get_int_var(CHANNEL_NAME_WIDTH_VAR))
	{
		if ((last_width = get_int_var(CHANNEL_NAME_WIDTH_VAR)) != 0)
			snprintf(format, 25, "%%s %%-%u.%us %%-5s %%s", /*thing_ansi,*/
				(unsigned char) last_width,
				(unsigned char) last_width);
		else
			snprintf(format, 25, "%%s %%s %%-5s %%s"/*, thing_ansi*/);
	}
	channel = ArgList[0];
	user_cnt = ArgList[1];
	line = PasteArgs(ArgList, 2);
	if (funny_flags & FUNNY_TOPIC && !(line && *line))
			return;
	cnt = my_atol(user_cnt);
	if (funny_min && (cnt < funny_min))
		return;
	if (funny_max && (cnt > funny_max))
		return;
	if ((funny_flags & FUNNY_PRIVATE) && (*channel != '*'))
		return;
	if ((funny_flags & FUNNY_PUBLIC) && ((*channel == '*') || (*channel == '@')))
		return;
	if (match_str)
	{
		if (wild_match(match_str, channel) == 0)
			return;
	}
	if (funny_flags & FUNNY_WIDE)
	{
		if (wl_elements >= wl_size)
		{
			new_list = (WideList **) new_malloc(sizeof(WideList *) *
			    (wl_size + 50));
			memset(new_list, 0, sizeof(WideList *) * (wl_size + 50));
			if (wl_size)
				memcpy(new_list, wide_list, sizeof(WideList *) * wl_size);
			wl_size += 50;
			new_free((char **)&wide_list);
			wide_list = new_list;
		}
		wide_list[wl_elements] = (WideList *)
			new_malloc(sizeof(WideList));
		wide_list[wl_elements]->channel = NULL;
		wide_list[wl_elements]->users = cnt;
		malloc_strcpy(&wide_list[wl_elements]->channel,
				(*channel != '*') ? channel : "Prv");
		wl_elements++;
		return;
	}
	set_display_target(channel, LOG_CRAP);
	if (do_hook(current_numeric, "%s %s %s %s", from,  channel, user_cnt,
	    line) && do_hook(LIST_LIST, "%s %s %s", channel, user_cnt, line))
	{
		if (channel && user_cnt)
			put_it("%s", convert_output_format(fget_string_var(FORMAT_LIST_FSET),"%s %s %s %s", update_clock(GET_TIME), *channel == '*'?"Prv":channel, user_cnt, line));
	}
	reset_display_target();
}

/* print_funny_names
 *
 * This handles presenting the output of a NAMES reply.  It is a cut-down
 * version of the /SCAN output formatting.
 */
void print_funny_names(char *line)
{
    char *t;
    int count = 0;
    char buffer[BIG_BUFFER_SIZE];
    int cols = get_int_var(NAMES_COLUMNS_VAR);

	if (!cols)
		cols = 1;

	if (line && *line)
	{	
		*buffer = 0;

		while ((t = next_arg(line, &line))) {
            char *nick;
            char *nick_format;
            char nick_buffer[BIG_BUFFER_SIZE];

			if (!count && fget_string_var(FORMAT_NAMES_BANNER_FSET))
				strlcat(buffer, convert_output_format(
                    fget_string_var(FORMAT_NAMES_BANNER_FSET), NULL, NULL), 
                    sizeof buffer);

            /* Seperate the nick and the possible status presets that might 
             * preceede it. */
            nick = t + strspn(t, "@%+~-");
            nick_format = fget_string_var(isme(nick) ? 
                FORMAT_NAMES_NICK_ME_FSET : FORMAT_NAMES_NICK_FSET);
            strlcpy(nick_buffer, 
                convert_output_format(nick_format, "%s", nick), 
                sizeof nick_buffer);

            if (nick != t)
            {
                char special = *t;

                if (special == '+')
				    strlcat(buffer, 
                        convert_output_format(
                            fget_string_var(FORMAT_NAMES_USER_VOICE_FSET),
                            "%c %s", special, nick_buffer), sizeof buffer);
                else
				    strlcat(buffer, 
                        convert_output_format(
                            fget_string_var(FORMAT_NAMES_USER_CHANOP_FSET),
                            "%c %s", special, nick_buffer), sizeof buffer);
            }
			else
				strlcat(buffer, 
                    convert_output_format(
                        fget_string_var(FORMAT_NAMES_USER_FSET), ". %s", 
                        nick_buffer), sizeof buffer);

			strlcat(buffer, space, sizeof buffer);

			if (++count >= cols)
			{
				put_it("%s", buffer);
				*buffer = 0;
				count = 0;
			}
		}

		if (count)
			put_it("%s", buffer);
	}
}

void funny_namreply(char *from, char **Args)
{
char	*type,
	*channel;
static	char	format[40];
static	int	last_width = -1;
register char	*ptr;
register char	*line;
int user_count = 0;

	PasteArgs(Args, 2);
	type = Args[0];
	channel = Args[1];
	line = Args[2];

	/* protocol violation by server */
	if (!channel || !line)
		return;

	ptr = line;
	while (*ptr)
	{
		while (*ptr && (*ptr != ' '))
			ptr++;
		user_count++;
		while (*ptr && (*ptr == ' '))
			ptr++;
	}

	if (in_join_list(channel, from_server))
	{
		set_display_target(channel, LOG_CRAP);
		if (do_hook(current_numeric, "%s %s %s %s", from, type, channel,line) 
			&& do_hook(NAMES_LIST, "%s %s", channel, line)
			&& get_int_var(SHOW_CHANNEL_NAMES_VAR))
		{
			put_it("%s", convert_output_format(fget_string_var(FORMAT_NAMES_FSET), "%s %s %d %d",update_clock(GET_TIME), channel, user_count, user_count));
			print_funny_names(line);
		} 
		if ((user_count == 1) && (*line == '@'))
		{
			ChannelList *chan;
			if ((chan = lookup_channel(channel, from_server, CHAN_NOUNLINK)))
				if ((ptr = get_cset_str_var(chan->csets, CHANMODE_CSET)))
					my_send_to_server(from_server, "MODE %s %s", channel, ptr);
		}
		got_info(channel, from_server, GOTNAMES);
		reset_display_target();
		return;
	}
	if (last_width != get_int_var(CHANNEL_NAME_WIDTH_VAR))
	{
		if ((last_width = get_int_var(CHANNEL_NAME_WIDTH_VAR)) != 0)
			sprintf(format, "%%s: %%-%u.%us %%s",
				(unsigned char) last_width,
				(unsigned char) last_width);
		else
			strcpy(format, "%s: %s\t%s");
	}
	if (funny_min && (user_count < funny_min))
		return;
	else if (funny_max && (user_count > funny_max))
		return;
	if ((funny_flags & FUNNY_PRIVATE) && (*type == '='))
		return;
	if ((funny_flags & FUNNY_PUBLIC) && ((*type == '*') || (*type == '@')))
		return;
	if (type && channel)
	{
		if (match_str)
		{
			if (wild_match(match_str, channel) == 0)
				return;
		}
		if (do_hook(current_numeric, "%s %s %s %s", from, type, channel, line) && do_hook(NAMES_LIST, "%s %s", channel, line))
		{
			set_display_target(channel, LOG_CRAP);
			if (fget_string_var(FORMAT_NAMES_FSET))
			{
				put_it("%s", convert_output_format(fget_string_var(FORMAT_NAMES_FSET), "%s %s %d %d", update_clock(GET_TIME), channel, user_count, user_count));
				print_funny_names(line);
			} 
			else
			{
				switch (*type)
				{
				case '=':
					if (last_width &&(strlen(channel) > last_width))
					{
						channel[last_width-1] = '>';
						channel[last_width] = (char) 0;
					}
					put_it(format, "Pub", channel, line);
					break;
				case '*':
					put_it(format, "Prv", channel, line);
					break;
				case '@':
					put_it(format, "Sec", channel, line);
					break;
				}
			}
			reset_display_target();
		}
	}
}

void funny_mode(char *from, char **ArgList)
{
	char	*mode, *channel;
	ChannelList *chan = NULL;
		
	if (!ArgList[0]) return;

	channel = ArgList[0];
	mode = ArgList[1];
	PasteArgs(ArgList, 1);

	if (in_join_list(channel, from_server))
	{
		update_channel_mode(from, channel, from_server, mode, chan);
		update_all_status(current_window, NULL, 0);
		got_info(channel, from_server, GOTMODE);
	}
	else
	{
		set_display_target(channel, LOG_CRAP);
		if (do_hook(current_numeric, "%s %s %s", from, channel, mode))
			put_it("%s", convert_output_format(fget_string_var(FORMAT_MODE_CHANNEL_FSET), "%s %s %s %s %s", update_clock(GET_TIME), from, *FromUserHost ? FromUserHost:"ÿ", channel, mode));
		reset_display_target();
	}
}

void update_user_mode(char *modes)
{
	int	onoff = 1;
	char	*p_umodes = get_possible_umodes(from_server);

	for (; *modes; modes++)
	{
		if (*modes == '-')
			onoff = 0;
		else if (*modes == '+')
			onoff = 1;

		else if   ((*modes >= 'a' && *modes <= 'z')
			|| (*modes >= 'A' && *modes <= 'Z'))
		{
			size_t 	idx;
			int 	c = *modes;

			idx = ccspan(p_umodes, c);
			if (p_umodes[idx] == 0)
				ircpanic("Invalid user mode referenced");
			set_server_flag(from_server, idx, onoff);

			if (c == 'o' || c == 'O')
				set_server_operator(from_server, onoff);
#if 0
			char c = tolower(*modes);
			size_t idx = (size_t) (strchr(umodes, c) - umodes);

			set_server_flag(from_server, USER_MODE << idx, onoff);

			if (c == 'o' || c == 'O')
				set_server_operator(from_server, onoff);
#endif
		}
	}
}

void	reinstate_user_modes (void)
{
	char *modes = get_umode(from_server);
	if (modes && *modes)
		send_to_server("MODE %s +%s", get_server_nickname(from_server), modes);
}
