/*
 * edit.h: header for edit.c 
 *
 */
#ifndef __edit_h_
#define __edit_h_

#include "irc_std.h"

extern	char	*sent_nick;
extern	char	*sent_body;
extern	char	*recv_nick;

	void	BX_send_text (const char *, const char *, char *, int, int);
	void	eval_inputlist (char *, char *);
	int	BX_parse_command (char *, int, char *);
	void	BX_parse_line (const char *, char *, const char *, int, int, int);
	void	edit_char (unsigned char);
	void	execute_timer (void);
	void	ison_now (char *, char *);
	void	quote_char (char, char *);
	void	type_text (char, char *);
	void	parse_text (char, char *);
	void	irc_clear_screen (char, char *);
	int	check_wait_command (char *);
	void	ExecuteTimers (void);
	int	check_mode_lock (char *, char *, int);
	void	destroy_call_stack (void);
	void	unwind_stack (void);
	void	wind_stack (char *);
	void	redirect_text (int, const char *, const char *, char *, int, int);
	int	command_exist (char *);
			


/* a few advance declarations */
extern	void	my_clear 		(char *, char *, char *, char *);
extern	void	reconnect_cmd		(char *, char *, char *, char *);
extern	void	e_hostname		(char *, char *, char *, char *);
extern	void	BX_load			(char *, char *, char *, char *);
extern	void	query			(char *, char *, char *, char *);
extern	void	unquery			(char *, char *, char *, char *);
extern	void	away			(char *, char *, char *, char *);
extern	void	e_quit			(char *, char *, char *, char *);
extern	void	repeatcmd		(char *, char *, char *, char *);
extern	void	do_unkey		(char *, char *, char *, char *);
extern	void	do_unscrew		(char *, char *, char *, char *);
extern	void	do_getout		(char *, char *, char *, char *);
extern	void	do_mynames		(char *, char *, char *, char *);
extern	void	my_whois		(char *, char *, char *, char *);
extern	void	do_4op			(char *, char *, char *, char *);
extern	void	umodecmd		(char *, char *, char *, char *);
extern	void	do_invite		(char *, char *, char *, char *);
extern	void	do_forward		(char *, char *, char *, char *);
extern	void	do_oops			(char *, char *, char *, char *);
extern	void	sendlinecmd		(char *, char *, char *, char *);
extern	void	do_send_text		(char *, char *, char *, char *);
extern	void	funny_stuff		(char *, char *, char *, char *);
extern	void	cd			(char *, char *, char *, char *);
extern	void	e_wall			(char *, char *, char *, char *);
extern	void	send_2comm		(char *, char *, char *, char *);
extern	void	send_comm		(char *, char *, char *, char *);
extern	void	untopic			(char *, char *, char *, char *);
extern	void	e_topic			(char *, char *, char *, char *);
extern	void	send_kick		(char *, char *, char *, char *);
extern	void	send_channel_com	(char *, char *, char *, char *);
extern	void	quotecmd		(char *, char *, char *, char *);
extern	void	e_privmsg		(char *, char *, char *, char *);
extern	void	flush			(char *, char *, char *, char *);
extern	void	oper			(char *, char *, char *, char *);
extern	void	e_channel		(char *, char *, char *, char *);
extern	void	who			(char *, char *, char *, char *);
extern	void	whois			(char *, char *, char *, char *);
extern	void	ison			(char *, char *, char *, char *);
extern	void	userhostcmd		(char *, char *, char *, char *);
extern	void	info			(char *, char *, char *, char *);
extern	void	e_nick			(char *, char *, char *, char *);
extern	void	comment			(char *, char *, char *, char *);
extern	void	sleepcmd		(char *, char *, char *, char *);
extern	void	version1		(char *, char *, char *, char *);
extern	void	ctcp			(char *, char *, char *, char *);
extern	void	rctcp			(char *, char *, char *, char *);
extern	void	dcc			(char *, char *, char *, char *);
extern	void	deop			(char *, char *, char *, char *);
extern	void	echocmd			(char *, char *, char *, char *);
extern	void	save_settings		(char *, char *, char *, char *);
extern	void	redirect		(char *, char *, char *, char *);
extern	void	waitcmd			(char *, char *, char *, char *);
extern	void	describe		(char *, char *, char *, char *);
extern	void	me			(char *, char *, char *, char *);
extern	void	evalcmd			(char *, char *, char *, char *);
extern	void	hookcmd			(char *, char *, char *, char *);
extern	void	inputcmd		(char *, char *, char *, char *);
extern	void	pingcmd			(char *, char *, char *, char *);
extern	void	xtypecmd		(char *, char *, char *, char *);
extern	void	beepcmd			(char *, char *, char *, char *);
extern	void	abortcmd		(char *, char *, char *, char *);
extern	void	e_debug			(char *, char *, char *, char *);
extern	void	do_scan			(char *, char *, char *, char *);
extern	void	push_cmd		(char *, char *, char *, char *);
extern	void	pop_cmd			(char *, char *, char *, char *);
extern	void	unshift_cmd		(char *, char *, char *, char *);
extern	void	shift_cmd		(char *, char *, char *, char *);
extern	void	exec_cmd		(char *, char *, char *, char *);
extern	void	auto_join		(char *, char *, char *, char *);
extern	void	dcc_crash		(char *, char *, char *, char *);
extern	void	do_msay			(char *, char *, char *, char *);
extern	void	send_mode		(char *, char *, char *, char *);
extern	void	do_offers		(char *, char *, char *, char *);
extern	void	ctcp_version		(char *, char *, char *, char *);
extern	void	about			(char *, char *, char *, char *);
extern	void	dcc_stat_comm		(char *, char *, char *, char *);
extern	void	sping			(char *, char *, char *, char *);
extern  void    realname_cmd		(char *, char *, char *, char *);
extern  void    set_username		(char *, char *, char *, char *);
extern  void    e_call			(char *, char *, char *, char *);
extern	void	do_toggle 		(char *, char *, char *, char *);
extern	void	e_quit 			(char *, char *, char *, char *);
extern	void	do_ig 			(char *, char *, char *, char *);
extern	void	do_listshit		(char *, char *, char *, char *);
extern	void	savelists		(char *, char *, char *, char *);
extern	void	mknu			(char *, char *, char *, char *);
extern	void	reconnect_cmd		(char *, char *, char *, char *);
extern  void    LameKick		(char *, char *, char *, char *);
extern  void    ChanWallOp		(char *, char *, char *, char *);
extern  void    NewUser			(char *, char *, char *, char *);
extern  void    ReconnectServer		(char *, char *, char *, char *);
extern  void    MegaDeop		(char *, char *, char *, char *);
extern  void    do_flood		(char *, char *, char *, char *);
extern  void    cycle			(char *, char *, char *, char *);
extern  void    bomb			(char *, char *, char *, char *);
extern  void    finger			(char *, char *, char *, char *);
extern  void    multkick		(char *, char *, char *, char *);
extern  void    massdeop		(char *, char *, char *, char *);
extern  void    doop			(char *, char *, char *, char *);
extern  void    dodeop			(char *, char *, char *, char *);
extern  void    massop			(char *, char *, char *, char *);
extern  void    whokill			(char *, char *, char *, char *);
extern  void 	ban			(char *, char *, char *, char *);
extern  void 	kickban			(char *, char *, char *, char *);
extern  void 	massban			(char *, char *, char *, char *);
extern  void 	dokick			(char *, char *, char *, char *);
extern  void 	nslookup		(char *, char *, char *, char *);
extern  void 	masskick		(char *, char *, char *, char *);
extern  void 	do_flood		(char *, char *, char *, char *);
extern  void 	reset			(char *, char *, char *, char *);
extern  void 	users			(char *, char *, char *, char *);
extern	void	my_ignorehost		(char *, char *, char *, char *);
extern	void	my_ignore		(char *, char *, char *, char *);
extern	void	unban			(char *, char *, char *, char *);
extern	void	masskickban		(char *, char *, char *, char *);
extern  void    linklook		(char *, char *, char *, char *);
extern  void	do_dump			(char *, char *, char *, char *);
extern  void	do_dirlasttype		(char *, char *, char *, char *);
extern	void	do_dirlistmsg		(char *, char *, char *, char *);
extern  void	do_dirlastmsg		(char *, char *, char *, char *);
extern  void	do_dirlastctcp		(char *, char *, char *, char *);
extern  void	do_dirlastctcpreply	(char *, char *, char *, char *);
extern  void	do_dirlastinvite	(char *, char *, char *, char *);
extern	void 	readlog			(char *, char *, char *, char *);
extern	void 	remove_log		(char *, char *, char *, char *);
extern	void 	add_user		(char *, char *, char *, char *);
extern	void	bot			(char *, char *, char *, char *);
extern	void	do_uptime		(char *, char *, char *, char *);
extern	void	cdcc			(char *, char *, char *, char *);
extern	void	extern_write		(char *, char *, char *, char *);
extern	void	showuserlist		(char *, char *, char *, char *);
extern	void	init_dcc_chat		(char *, char *, char *, char *);
extern	void	add_shit		(char *, char *, char *, char *);
extern	void	showshitlist		(char *, char *, char *, char *);
extern	void	channel_stats		(char *, char *, char *, char *);
extern	void	my_clear		(char *, char *, char *, char *);
extern	void	stubcmd			(char *, char *, char *, char *);
extern	void	addidle			(char *, char *, char *, char *);
extern	void	showidle		(char *, char *, char *, char *);
extern	void	kickidle		(char *, char *, char *, char *);
extern	void	usage			(char *, char *, char *, char *);
extern	void	reload_save		(char *, char *, char *, char *);
extern	void	cset_variable		(char *, char *, char *, char *);
extern	void	banstat			(char *, char *, char *, char *);
extern	void	nwhois			(char *, char *, char *, char *);
extern	void	statkgrep		(char *, char *, char *, char *);
extern	void	tban			(char *, char *, char *, char *);
extern	void	bantype			(char *, char *, char *, char *);
extern	void	whowas			(char *, char *, char *, char *);
extern	void	findports		(char *, char *, char *, char *);
extern	void	add_ban_word		(char *, char *, char *, char *);
extern	void	show_word_kick		(char *, char *, char *, char *);
extern	void	clear_tab		(char *, char *, char *, char *);
extern	void	topic_lock		(char *, char *, char *, char *);
extern	void	mode_lock		(char *, char *, char *, char *);
extern	void	randomnick		(char *, char *, char *, char *);
extern	void	topic_lock		(char *, char *, char *, char *);
extern	void	show_version		(char *, char *, char *, char *);
extern	void	chat			(char *, char *, char *, char *);
extern	void	back			(char *, char *, char *, char *);
extern	void	tog_fprot		(char *, char *, char *, char *);
extern	void	ftp			(char *, char *, char *, char *);
extern	void	do_dirsentlastnotice	(char *, char *, char *, char *);
extern	void	do_dirsentlastmsg	(char *, char *, char *, char *);
extern	void	do_dirlastwall		(char *, char *, char *, char *);
extern	void	do_dirlasttopic		(char *, char *, char *, char *);
extern	void	do_dirsentlastwall	(char *, char *, char *, char *);
extern	void	do_dirsentlasttopic	(char *, char *, char *, char *);
extern	void	do_dirlastserver	(char *, char *, char *, char *);
extern	void	botlink			(char *, char *, char *, char *);
extern	void	jnw			(char *, char *, char *, char *);
extern	void	lkw			(char *, char *, char *, char *);
extern	void	whokill			(char *, char *, char *, char *);
extern	void	csay			(char *, char *, char *, char *);
extern	void	clink			(char *, char *, char *, char *);
extern	void	cwho			(char *, char *, char *, char *);
extern	void	cboot			(char *, char *, char *, char *);
extern	void	cmsg			(char *, char *, char *, char *);
extern	void	toggle_xlink		(char *, char *, char *, char *);
extern	void	dcx			(char *, char *, char *, char *);
extern	void	orig_nick		(char *, char *, char *, char *);
extern	void	print_structs		(char *, char *, char *, char *);
extern	void	pretend_cmd		(char *, char *, char *, char *);
extern	void	e_pause			(char *, char *, char *, char *);
extern	void	add_bad_nick		(char *, char *, char *, char *);
extern	void	serv_stat		(char *, char *, char *, char *);
extern	void	fuckem			(char *, char *, char *, char *);
extern	void	tracekill		(char *, char *, char *, char *);
extern	void	traceserv		(char *, char *, char *, char *);
extern	void	dll_load		(char *, char *, char *, char *);
extern	void	tignore			(char *, char *, char *, char *);
extern	void	dumpcmd			(char *, char *, char *, char *);
extern	void	aliascmd		(char *, char *, char *, char *);
extern	void	set_autoreply		(char *, char *, char *, char *);
extern	void	init_ftp		(char *, char *, char *, char *);
extern	void	xdebugcmd		(char *, char *, char *, char *);
extern	void	blesscmd		(char *, char *, char *, char *);
extern	void	do_trace		(char *, char *, char *, char *);
extern	void	do_stats		(char *, char *, char *, char *);
extern	void	setenvcmd		(char *, char *, char *, char *);
extern	void	send_kill		(char *, char *, char *, char *);
extern	void	set_user_info		(char *, char *, char *, char *);
extern	void	init_vars		(char *, char *, char *, char *);
extern	void	init_window_vars	(char *, char *, char *, char *);
extern	void	show_hash		(char *, char *, char *, char *);
extern	void	unload			(char *, char *, char *, char *);
extern	void	do_map			(char *, char *, char *, char *);
extern	void	add_no_flood		(char *, char *, char *, char *);
extern	void	s_watch			(char *, char *, char *, char *);
extern	void	awaylog			(char *, char *, char *, char *);
extern	void	newnick			(char *, char *, char *, char *);
extern	void	newuser			(char *, char *, char *, char *);

extern	void	os2menu			(char *, char *, char *, char *);
extern	void	os2menuitem             (char *, char *, char *, char *);
extern	void	os2submenu       	(char *, char *, char *, char *);
extern	void	fontdialog         	(char *, char *, char *, char *);
extern	void	filedialog         	(char *, char *, char *, char *);

extern	void	ame			(char *, char *, char *, char *);

#ifdef WANT_DLL
extern	void	unload_dll		(char *, char *, char *, char *);
#endif

	IrcCommand *BX_find_command (char *, int *);
	char	*glob_commands(char *, int *, int);
		
#define AWAY_ONE 0
#define AWAY_ALL 1

#define STACK_POP 0
#define STACK_PUSH 1
#define STACK_SWAP 2

#define TRACE_OPER	0x01
#define TRACE_SERVER	0x02
#define TRACE_USER	0x04

#define STATS_LINK	0x001
#define STATS_CLASS	0x002
#define STATS_ILINE	0x004
#define STATS_TKLINE	0x008
#define STATS_YLINE	0x010
#define STATS_OLINE	0x020
#define STATS_HLINE	0x040
#define STATS_UPTIME	0x080
#define STATS_MLINE	0x100
#define STATS_KLINE	0x200

#define NONOVICEABBREV	0x0001
#define NOINTERACTIVE	0x0002
#define NOSIMPLESCRIPT	0x0004
#define NOCOMPLEXSCRIPT	0x0008
#define SERVERREQ	0x0010

#ifdef WANT_DLL
extern IrcCommandDll *dll_commands;
#endif

extern        int     will_catch_break_exceptions;
extern        int     will_catch_continue_exceptions;
extern        int     will_catch_return_exceptions;
extern        int     break_exception;
extern        int     continue_exception;
extern        int     return_exception;


#endif /* __edit_h_ */
