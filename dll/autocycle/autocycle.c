/*
 * dll/autocycle/autocycle.c - Auto-cycle plugin for BitchX
 * Copyright (c) David Walluck 1999
 */

#include "irc.h"
#include "struct.h"
#include "server.h"
#include "vars.h"
#include "misc.h"
#include "module.h"
#include "hook.h"
#ifndef INIT_MODULE
#define INIT_MODULE
#endif
#include "modval.h"

#define MODULE_NAME	"Autocycle"
#define _MODULE_VERSION	"0.01"

int auto_cycle(IrcCommandDll *interp, char *command, char *args, char *subargs)
{
	char *		channel	= current_window->current_channel;
	int		netsplit = (int)next_arg(args, &args);
	int		this_server = current_window->server;
	ChannelList *	chan = lookup_channel(channel, this_server, 0);
	NickList *	tmp = NULL;
	int		counter = 0;

	/*
	 * This may look a little odd, but I had a hard time assuring otherwise
	 * that there's only one person left on the channel
	 */
	for (tmp = next_nicklist(chan, NULL); tmp && counter != 2; tmp = next_nicklist(chan, tmp), counter++);
	
	if (get_dllint_var("AUTO_CYCLE"))
	{
		if (counter == 1 && (!netsplit || get_dllint_var("AUTO_CYCLE") > 1) && !is_chanop(channel, get_server_nickname(from_server)) && channel[0] != '+')
		{
			put_it("%s", convert_output_format("$G Auto-cycling $0 to gain ops", "%s", channel));
			my_send_to_server(from_server, "PART %s\nJOIN %s%s%s",chan->channel, chan->channel, chan->key ? " " : "", chan->key ? chan->key : "");
			return 1;
		}
	}
	return 0;
}

int Autocycle_Cleanup(IrcCommandDll **interp, Function_ptr *global_table)
{
	remove_module_proc(VAR_PROC, MODULE_NAME, NULL, NULL);
	remove_module_proc(HOOK_PROC, MODULE_NAME, NULL, NULL);
	put_it("%s", convert_output_format("$G $0 $1 by DavidW2 unloaded","%s %s", MODULE_NAME, _MODULE_VERSION));
	return 2;
}

int Autocycle_Init(IrcCommandDll **interp, Function_ptr *global_table)
{
	initialize_module(MODULE_NAME);
	add_module_proc(VAR_PROC, MODULE_NAME, "AUTO_CYCLE", NULL, INT_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(HOOK_PROC, MODULE_NAME, NULL, "*", NETSPLIT_LIST, 1, NULL, auto_cycle);
	put_it("%s", convert_output_format("$G $0 $1 by DavidW2 loaded", "%s %s", MODULE_NAME, _MODULE_VERSION));
	return 0;
}
