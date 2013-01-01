BitchX tcl now includes a special mode similar to what eggdrop bots have. ie
we can setup a BitchX client as both a client and/or a bot. 

     Client A -----\					/----- Client C
		     ----- Bot A ----------- Bot B -----
     Client B -----/				        \----- Client D

Bot A and Bot B are both BitchX clients. In Bot A attempts to link in a Bot
link to Bot B then Bot B needs to have Bot A in it's /botlist with a
password. ie /addbot BotA #channel 0 0 password  where channel, auto-op and
prot levels don't matter. /clink BotB BotA password is then used to link the
two clients together. From this point on, anything coming over a dcc chat
link or a dcc bot link are passed back and forth over the appropriate links.
Clients dcc chatting with either BotA or BotB will receive information about
joins and parts as well as things the other clients say to one another. BotA
and BotB can also partake in this link as well. The bots in this case should
not try and /dcc chat with anyone. let the Client's attempt the chat.

Commands from Bots

	/clink  client_to_attempt_link_with  
		Client_nick_to_link_with password <port>
		- attempt to link two clients together in a bot link.
		- password can also be /set bot_passwd which bypasses the
		  Botlist password.

	/csay   msg 
		- send msg over all dcc chat links and any bot links as well.

	/cwho	<bot>
		- shows who is on the current bot, or specified bot

	/cwhom
		- shows who is on every bot.

	/cboot	[nick] <reason>
		- boot nick from the network.

	/cmsg   [nick> msg
		- not fully implemented yet.

Commands for /dcc chat
*note: these are also available in regular dcc chat with someone else. So,
some of these have userlevels applied to them to prevent others from doing
things to you or causing you todo things you wouldnt want them todo.

	.act [what] 		- sends action to chat line
	.boot [nick] <msg> 	- boot nick from network
	.chat			- once a dcc is started send .chat to enter
				  the party line
	.cmsg [nick] msg 	-  send a privmsg
	.echo			-  toggle echo back to you on/off
	.help <topic> 		- help for commands
	.invite nick		- invites nick to chat with the host
	.msg [nick] [msg] 	- send a privmsg to someone
	.say [msg] 		- say something on the channel
	.quit			- quit partyline, but not the chat.
	.tcl [tcl_command] 	- tcl command
	.who <botnick>		- who is on bot or this bot
	.whom 			- all users on network

Commands for tcl scripting
*note: Most of these follow the eggdrop syntax. Some are unique to BitchX
however. On differance is that instead of a letter for the userlevel, a
numeric userlevel is applied instead.

	ircii		- causes a ircii command to execute
	validuser	- 
	pushmode
	flushmode
	lvarpop
	lempty
	lmatch
	keyldel
        keylget
        keylkeys
        keylset
	maskhost
	onchansplit
	servers
		server_list[i].name, server_list[i].port, server_list[i].connected
	chanstruct
		c->channel, c->server, c->mode, c->s_mode, c->limit,
		c->chop, c->voice, c->channel_create, c->join_time, c->stats_ops,
		c->stats_dops, c->stats_bans, c->stats_unbans, c->stats_sops,
		c->stats_sdops, c->stats_sbans, c->stats_sunbans, c->stats_topics,
		c->stats_kicks, c->stats_pubs, c->stats_parts, c->stats_signoffs,
		c->stats_joins, c->set_ainv, c->set_auto_rejoin,
		c->set_deop_on_deopflood, c->set_deop_on_kickflood,
		c->set_deopflood, c->set_deopflood_time, c->set_hacking,
		c->set_kick_on_deopflood, c->set_kick_on_joinflood,
		c->set_kick_on_kickflood, c->set_kick_on_nickflood,
		c->set_kick_on_pubflood, c->set_kickflood, c->set_kickflood_time,
		c->set_nickflood, c->set_nickflood_time, c->set_joinflood,
		c->set_joinflood_time, c->set_pubflood, c->set_pubflood_time,
		c->set_pubflood_ignore, c->set_userlist, c->set_shitlist, c->set_lamelist,
		c->set_kick_if_banned, c->totalnicks, c->maxnicks, c->maxnickstime,
		c->totalbans, c->maxbans, c->maxbanstime, c->set_aop, c->bitch_mode);

	channel
	channels
	isop
	getchanhost
	matchattr
	finduser
	findbot
	findshit
	date
	time
	ctime
	dccstats
                                dccnum,
                                read,
                                dcc_types[flags&DCC_TYPES],
                                user,
                                status  DCC_DELETE ? "Closed" :
                                DCC_ACTIVE ? "Active" :
                                DCC_WAIT ? "Waiting" :
                                DCC_OFFER ? "Offered" : "Unknown",
                                starttime.tv_sec,
                                filesize,
                                bytes_sent,
                                bytes_read,
                                filesize,
                                description,
                                encrypt or ""

	onchan
	chanlist
	unixtime
	putlog
	putloglev
	rand
	timer
	killtimer
	utimer
	killutimer
	timers
	utimers
	putserv
	putscr
	putdcc
	putbot
	putallbots
	bind
	binds
	unbind
	strftime
	cparse
	userhost	- Attempt to find nick on irc. Any number of nicks
			  may be specified.
	getchanmode

Tcl variables BitchX sets.
	botnick
	nick
	realname
	username
	server
	version
	curchan
	userhost	- variable is set in response to the userhost command
	un-userhost	- variable is set in response to the userhost command

Special considerations.

tcl bind raw  functions that return 1 will bypass the internal ircII
routines. returning 0 or nothing will have the tcl and the ircII internal
routine execute.


BitchX's bot mode acts very similar to eggdrop's and in fact thanks to Robey
for defining the protocol necessary to achieve this.
