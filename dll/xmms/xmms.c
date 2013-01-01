#if !defined(WINNT) && !defined(EMX)
/*
 * xmms.c: This file handles all the XMMS routines, in BitchX
 *
 * Written by André Camargo
 * based on the code of Tom Zickel a.k.a. IceBreak on the IRC
 * and Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 
 * 4Front Technologies
 *
 * Copyleft (1999) "CASSIUS IN THE HOUSE!" :)
 *
 * BitchX plugin v0.01 by DavidW2
 * BitchX plugin v1.00 by caf
 */

#include "irc.h"
#include "struct.h"
#include "server.h"
#include "ircaux.h"
#include "output.h"
#include "misc.h"
#include "vars.h"
#include "status.h"
#include "hook.h"
#include "module.h"
#include "ctcp.h"
#ifndef INIT_MODULE
#define INIT_MODULE
#endif
#include "modval.h"

#include <xmms/xmmsctrl.h>

#define MODULE_NAME	"Xmms"
#define _MODULE_VERSION	"1.00"

static int check_xmms(void);

static char xmms_format[] = "%K[%G" MODULE_NAME " %g" _MODULE_VERSION 
                            "%K] %n$*";
static char xmms_format2[] = "%K[%G" MODULE_NAME " %g" _MODULE_VERSION 
                                " %K|%B $0 %K] %n$1-";
static char xmms_format_vol[] = "%K[%G" MODULE_NAME " %g" _MODULE_VERSION 
                                " %K|%B $0 %K|%W$[10]1%K] %n$2%%";
static char xmms_format_track[] = "%K[%G" MODULE_NAME " %g" _MODULE_VERSION 
                                " %K|%B $0 %K|%W$[-4]1%n/%W$[-4]2%K] %n$3-";

static char xmms_volbar[] = "==========";
#define VOLBAR(n) ((n)>9?(xmms_volbar + 10 - (n)/10):".")

static int xmms_session = 0;

BUILT_IN_DLL(xmms_cmd)
{
	char *arg;
	arg = next_arg(args, &args);

	if (!arg)
		arg = "HELP";

	if (*arg == '-' || *arg == '/')
		arg++;

	if (!my_stricmp(arg, "PLAY"))
	{
		if (check_xmms())
		{
			if ((arg = next_arg(args, &args)))
				if (atoi(arg) < xmms_remote_get_playlist_length(xmms_session))
					xmms_remote_set_playlist_pos(xmms_session, atoi(args));
			xmms_remote_play(xmms_session);
		}		
	}
	else if (!my_stricmp(arg, "PAUSE"))
	{
		if (check_xmms())
		{
			if (!xmms_remote_is_paused(xmms_session))
				xmms_remote_pause(xmms_session);
		}
	}
	else if (!my_stricmp(arg, "NEXT"))
	{
		if (check_xmms())
			xmms_remote_playlist_next(xmms_session);
	}
	else if (!my_stricmp(arg, "PREV"))
	{
		if (check_xmms())
			xmms_remote_playlist_prev(xmms_session);
	}
	else if (!my_stricmp(arg, "STOP"))
	{
		if (check_xmms())
			xmms_remote_stop(xmms_session);
	}
	else if (!my_stricmp(arg, "LIST"))
	{
		int i;

		if (check_xmms())
		{
			char *title;
			int pl_len = xmms_remote_get_playlist_length(xmms_session);
			for (i = 0; i < pl_len; i++) {
				title = xmms_remote_get_playlist_title(xmms_session, i);
				put_it("%s", convert_output_format(xmms_format_track, 
				       "playlist %d %d %s", i + 1, pl_len, title));
			}
		}
	}
	else if (!my_stricmp(arg, "VOL"))
	{
		int v;

		if (check_xmms())
		{
			if ((arg = next_arg(args, &args)))
			{
				v = atoi(arg);
				if (v > 100)
					v = 100;
				if (v < 0)
					v = 0;
				xmms_remote_set_main_volume(xmms_session, v);
    			} else {
	      			v = xmms_remote_get_main_volume(xmms_session);
			}
    			put_it("%s", convert_output_format(xmms_format_vol,
			       "volume %s %d", VOLBAR(v), v)); 
   		}
	}
	else if (!my_stricmp(arg, "VOLUP"))
	{
		int vol;
		if (check_xmms())
		{
			vol = xmms_remote_get_main_volume(xmms_session) + 10;
			if (vol > 100)
				vol = 100;
			xmms_remote_set_main_volume(xmms_session, vol);
    			put_it("%s", convert_output_format(xmms_format_vol,
			       "volume %s %d", VOLBAR(vol), vol)); 
		}
	}
	else if (!my_stricmp(arg, "VOLDOWN"))
	{
		int vol;
		if (check_xmms())
		{
			vol = xmms_remote_get_main_volume(xmms_session) - 10;
			if (vol < 0)
				vol = 0;
			xmms_remote_set_main_volume(xmms_session, vol);
    			put_it("%s", convert_output_format(xmms_format_vol,
			       "volume %s %d", VOLBAR(vol), vol)); 
		}
	}
	else if (!my_stricmp(arg, "BALANCE"))
	{
		int bal;
		arg = next_arg(args, &args);

		if (arg && check_xmms()) {
			bal = atoi(arg);
			if (bal > 100)
				bal = 100;
			if (bal < -100)
				bal = -100;
			xmms_remote_set_balance(xmms_session, bal);
		}
	}
	else if (!my_stricmp(arg, "TOGSHUFFLE"))
	{
		if (check_xmms())
			xmms_remote_toggle_shuffle(xmms_session);
	}
	else if (!my_stricmp(arg, "TOGREPEAT"))
	{
		if (check_xmms())
			xmms_remote_toggle_repeat(xmms_session);
	}
	else if (!my_stricmp(arg, "HELP"))
	{
		put_it("%s", convert_output_format(xmms_format, "%s",
		       "Usage: /XMMS <command>"));
		put_it("   - PLAY            Play a song");
        	put_it("   - PAUSE           Pause/resume the playing song");
        	put_it("   - STOP            Stop the playing song");
        	put_it("   - NEXT            Scream to XMMS: next song!");
        	put_it("   - PREV            "
		       "Change to previous song in XMMS playlist");
        	put_it("   - VOL             "
		       "Set's the XMMS Volume (range from 0%% to 100%%)");
		put_it("   - VOLUP           Increase volume +10");
		put_it("   - VOLDOWN         Decrease volume -10");
		put_it("   - BALANCE         "
		       "Set balance, -100 (full left) to 100 (full right)");
		put_it("   - TOGREPEAT       Toggle repeat");
		put_it("   - TOGSHUFFLE      Toggle shuffle");
        	put_it("   - LIST            Show XMMS playlist songs");
		put_it("   - INFO            "
		       "Show what song is selected in the playlist");
        	put_it("   - INFOCHANNEL     "
		       "Spam the current channel with the song you're "
		       "listening to");
		put_it("   - SETTIME         "
		       "Jump to position in song (percentage)");
		put_it("   - QUIT            Close XMMS");
	}	
	else if (!my_stricmp(arg, "INFO"))
	{
		char *title;
		int pl_len, pl_pos;	
	
		if (check_xmms()) {
			pl_len = xmms_remote_get_playlist_length(xmms_session);
 			if (pl_len) {
				pl_pos = xmms_remote_get_playlist_pos(xmms_session);
				title = xmms_remote_get_playlist_title(xmms_session, pl_pos);
				put_it("%s", convert_output_format(xmms_format_track, 
				       "track %d %d %s", pl_pos + 1, pl_len, 
				       title));
			} else {
				put_it("%s", convert_output_format(xmms_format2,
				       "track (playlist empty)"));
			}
		}
	}
	else if (!my_stricmp(arg, "QUIT"))
	{
		if (check_xmms())
			xmms_remote_quit(xmms_session);
	}
	else if (!my_stricmp(arg, "SETTIME"))
	{
		int pl_pos;
		int full_time;
		int set_time;
		arg = next_arg(args, &args);
		
		if (arg && check_xmms())
		{
			pl_pos = xmms_remote_get_playlist_pos(xmms_session);
			full_time = xmms_remote_get_playlist_time(xmms_session,
					                          pl_pos);
			set_time = atoi(arg);
			if (set_time < 0)
				set_time = 0;
			if (set_time > 100)
				set_time = 100;
			
			xmms_remote_jump_to_time(xmms_session, 
			                         set_time * full_time / 100);
		}
	}
	else if (!my_stricmp(arg, "INFOCHANNEL"))
	{
		if (check_xmms())
		{
			char *target;
			char *title;
			int pl_pos = xmms_remote_get_playlist_pos(xmms_session);
			int pl_len = xmms_remote_get_playlist_length(xmms_session);
			target = get_current_channel_by_refnum(0);
			if (pl_len && target)
			{
				char message[512];
				char putbuf[IRCD_BUFFER_SIZE];

				title = xmms_remote_get_playlist_title(xmms_session, pl_pos);
				sprintf(message,
				        "[XMMS Playing: %s]", 
				        title);

				set_display_target(target, LOG_ACTION);
				
				do_hook(SEND_CTCP_LIST, "PRIVMSG %s ACTION %s",
					target, message);
				sprintf(putbuf, "%cACTION %s%c", 
					CTCP_DELIM_CHAR, message, 
					CTCP_DELIM_CHAR);
				send_text(target, putbuf, "PRIVMSG", 0, 0);
				
				if (do_hook(SEND_ACTION_LIST, "%s %s", target, message))
				{
					if (strchr("&#", *target))
						put_it("%s", convert_output_format(fget_string_var(FORMAT_SEND_ACTION_FSET), "%s %s %s %s", update_clock(GET_TIME), get_server_nickname(from_server), target, message));
					else
						put_it("%s", convert_output_format(fget_string_var(FORMAT_SEND_ACTION_OTHER_FSET), "%s %s %s %s", update_clock(GET_TIME), get_server_nickname(from_server), target, message));
				}
				reset_display_target();
			}
		}
	}
}

static int check_xmms(void)
{
	int xmms_running = xmms_remote_is_running(xmms_session);

	if (!xmms_running)
	{
		put_it("%s", convert_output_format(xmms_format, "%s",
		       "Could not find a running instance of XMMS.")); 
	}
	return xmms_running;
}

int Xmms_Cleanup(IrcCommandDll **interp, Function_ptr *global_table)
{
	remove_module_proc(COMMAND_PROC, MODULE_NAME, NULL, NULL);
	put_it("%s", convert_output_format(xmms_format, "%s",
	       "by André Camargo, DavidW2 and caf unloaded."));
	return 2;
}

int Xmms_Init(IrcCommandDll **interp, Function_ptr *global_table)
{
	initialize_module(MODULE_NAME);
	add_module_proc(COMMAND_PROC, "Xmms", "Xmms", NULL, 0, 0, xmms_cmd, empty_string);
	put_it("%s", convert_output_format(xmms_format, "%s",
	       "by André Camargo, DavidW2 and caf loaded.  "
	       "Type /XMMS for command help."));
	return 0;
}
#endif
