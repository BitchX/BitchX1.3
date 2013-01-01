/*
 * commands.c: This is really a mishmash of function and such that deal with IRCII
 * commands (both normal and keybinding commands) 
 *
 * Written By Michael Sandrof
 * Portions are based on EPIC.
 * Modified by panasync (Colten Edwards) 1995-97
 * Copyright(c) 1990 
 * Random Ircname by Christian Deimel
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#include "irc.h"
static char cvsrevision[] = "$Id: commands.c 206 2012-06-13 12:34:32Z keaston $";
CVS_REVISION(commands_c)

/* These headers are for the local-address-finding routines. */
#ifdef IPV6

#include <net/if.h>
#include <ifaddrs.h>

#else /* IPV6 */

#include <sys/ioctl.h>
#include <sys/socket.h>

#ifndef SIOCGIFCONF
#include <sys/sockio.h>
#endif /* SIOCGIFCONF */

/* Some systems call it SIOCGIFCONF, some call it OSIOCGIFCONF */
#if defined(OSIOCGIFCONF)
    #define SANE_SIOCGIFCONF OSIOCGIFCONF
#elif defined(SIOCGIFCONF)
    #define SANE_SIOCGIFCONF SIOCGIFCONF
#endif

#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif /* HAVE_NET_IF_H */

#endif /* IPV6 */

#include <sys/stat.h>
#include "struct.h"
#include "commands.h"
#include "parse.h"
#include "server.h"
#include "chelp.h"
#include "encrypt.h"
#include "vars.h"
#include "ircaux.h"
#include "lastlog.h"
#include "log.h"
#include "window.h"
#include "screen.h"
#include "ircterm.h"
#include "who.h"
#include "hook.h"
#include "input.h"
#include "ignore.h"
#include "keys.h"
#include "names.h"
#include "alias.h"
#include "history.h"
#include "funny.h"
#include "ctcp.h"
#include "dcc.h"
#include "output.h"
#include "exec.h"
#include "notify.h"
#include "numbers.h"
#include "status.h"
#include "if.h"
#include "help.h"
#include "stack.h"
#include "queue.h"
#include "timer.h"
#include "list.h"
#include "userlist.h"
#include "misc.h"
#include "alist.h"
#include "tcl_bx.h"
#include "hash2.h"
#include "cset.h"
#include "notice.h"

#ifdef TRANSLATE
#include "translat.h"
#endif

#ifdef WANT_CD
#include "cdrom.h"
#endif

#define MAIN_SOURCE
#include "modval.h"

#ifdef WANT_TCL
Tcl_Interp *tcl_interp = NULL;
#endif

#define COMMENT_HACK 

extern	int	doing_notice;

/* Used to handle and catch breaks and continues */
	int     will_catch_break_exceptions = 0;
	int     will_catch_continue_exceptions = 0;
	int     will_catch_return_exceptions = 0;
	int     break_exception = 0;
	int     continue_exception = 0;
	int     return_exception = 0;



static	void	oper_password_received (char *, char *);

int	no_hook_notify = 0;
int	load_depth = -1;

extern char	cx_function[];

/* used with /save */
#define	SFLAG_ALIAS	0x0001
#define	SFLAG_ASSIGN	0x0002
#define	SFLAG_BIND	0x0004
#define	SFLAG_ON	0x0008
#define	SFLAG_SET	0x0010
#define	SFLAG_NOTIFY	0x0020
#define SFLAG_SERVER	0x0040
#define	SFLAG_DIGRAPH	0x0080
#define SFLAG_WATCH	0x0100
#define SFLAG_ALL	0xffff

/* The maximum number of recursive LOAD levels allowed */
#define MAX_LOAD_DEPTH 10

	int	interactive = 0;
	

typedef	struct	WaitCmdstru
{
	char	*stuff;
	struct	WaitCmdstru	*next;
}	WaitCmd;

static	WaitCmd	*start_wait_list = NULL,
		*end_wait_list = NULL;

char	lame_wait_nick[] = "***LW***";
char	wait_nick[] = "***W***";

#ifdef WANT_DLL
	IrcCommandDll *find_dll_command (char *, int *);
	IrcCommandDll *dll_commands = NULL;
#endif


AJoinList *ajoin_list = NULL;

/*
 * irc_command: all the availble irc commands:  Note that the first entry has
 * a zero length string name and a null server command... this little trick
 * makes "/ blah blah blah" to always be sent to a channel, bypassing queries,
 * etc.  Neato.  This list MUST be sorted.
 */

BUILT_IN_COMMAND(debug_user);
BUILT_IN_COMMAND(debugmsg);
BUILT_IN_COMMAND(debughook);
BUILT_IN_COMMAND(evalserver);
BUILT_IN_COMMAND(purge);
BUILT_IN_COMMAND(debugfunc);
BUILT_IN_COMMAND(url_grabber);
BUILT_IN_COMMAND(breakcmd);
BUILT_IN_COMMAND(returncmd);
BUILT_IN_COMMAND(continuecmd);
BUILT_IN_COMMAND(check_clones);
BUILT_IN_COMMAND(usleepcmd);
BUILT_IN_COMMAND(pastecmd);
BUILT_IN_COMMAND(oper);
BUILT_IN_COMMAND(epichelp);
BUILT_IN_COMMAND(spam);
BUILT_IN_COMMAND(do_mtopic);
BUILT_IN_COMMAND(xevalcmd);
BUILT_IN_COMMAND(send_kline);
BUILT_IN_COMMAND(show_revisions);

#ifdef GUI
BUILT_IN_COMMAND(os2popupmenu);
BUILT_IN_COMMAND(os2popupmsg);
BUILT_IN_COMMAND(pmpaste);
BUILT_IN_COMMAND(pmprop);
BUILT_IN_COMMAND(os2menu);
BUILT_IN_COMMAND(os2menuitem);
BUILT_IN_COMMAND(os2submenu);
#endif

#ifdef __EMXPM__
BUILT_IN_COMMAND(pmcodepage);
#endif

#ifdef WANT_CHAN_NICK_SERV
BUILT_IN_COMMAND(e_server);
#endif

#ifdef ALLOW_DETACH
BUILT_IN_COMMAND(detachcmd);
#endif


char relay_help[] = "%R[%n-list|-kick|-wall|-wallop|-msg|-notice|-topic|-kboot|-ansi|-kill|-help%R]%n %Y<%n#|channel|nick%Y> <%nchannel|nick%Y>";
char scripting_command[] = "- Scripting command";

IrcCommand irc_command[] =
{
	{ "",		empty_string,	do_send_text,		0,	NULL },
	{ "#",		NULL,		comment, 		0,	NULL },
	{ "4OP",	NULL,		do_4op,			SERVERREQ,	"%Y<%Cnick%Y>%n\n- Sets mode +oooo on %Y<%Cnick%Y>%n"},
	{ ":",		NULL,		comment, 		0,	NULL },
        { "ABORT",      NULL,           abortcmd,               0,	"- Saves IRCII settings then exits IRC" },
	{ "ABOUT",	NULL,		about,			0,	"- Shows yet another ansi screen with greets to people who have contributed to the %PBitchX%n project"},
	{ "ADDFORWARD",	"AddForward", 	do_forward,		0,	"%Y<%Bchannel%G|%Cnick%Y>%n\n - Forward all messages to %Y<%Bchannel%G|%Cnick%Y>%n" },
	{ "ADDIDLE",	"AddIdle",	addidle,		0,	"%Y<%Bchannel%Y>%n %R[%nseconds%R]%n\n- Adds %Y<%Bchannel%Y>%n as idle channel with %R[%nseconds%R]%n" },
	{ "ADDLAMENICK","AddLameNick",	add_bad_nick,		0,	"%Y<%Cnick%G|%Cnick%C nick nick%Y>%n\n - Adds %Y<%Cnick%Y>%n to your lame nicklist, bans nick!*@*"},
	{ "ADDNOFLOOD",	"AddNoFlood",	add_no_flood,		0,	"%Y<%Cnick%Y>%n\n - Adds %Cnick%n to your no flood list" },
#ifdef WANT_USERLIST
	{ "ADDSHIT",	"AddShit",	add_shit,		0,	"%Y<%Cnick%G|%Cnick%G!%nuser%Y@%nhostname%Y>%n %Y<%Bchannel%G|%Y*>%n %R[%nshitlevel%R]%n %R[%nreason%R]%n\n- Add %Y<%Cnick%G|%Cnick%G!%nuser%Y@%nhostname%Y>%n to your shitlist on %Y<%Bchannel%G|%Y*>%n with %R[%nshitlevel%R]%n and for %R[%nreason%R]%n" },
	{ "ADDUSER",	"AddUser",	add_user,		0,	"%Y<%Cnick%G|%Cnick%G!%nuser%Y@%nhostname%Y>%n %Y<%Bchannel%G|%Y*>%n %R[%nuserlevel%R]%n %R[%nauto-op%R]%n %R[%nprotection%R]%n %R[%npassword%R]%n\n - Adds %Y<%Cnick%G|%Cnick%G!%nuser%Y@%nhostname%Y>%n on %Y<%Bchannel%G|%Y*>%n with %R[%nuserlevel%R]%n %R[%nauto-op%R]%n and %R[%nprotection%R]%n levels to your userlist" },
#endif
	{ "ADDWORD",	NULL,		add_ban_word,		0,	"%Y<%Bchannel%G|%Y*>%n %Y<%nword%y(%ns%y)%Y>%n\n- Adds %Y<%nword%y(%ns%y)%Y>%n to wordlist, anyone saying it in %Y<%Bchannel%G|%Y*>%n will be kicked" },
	{ "ADMIN",	"ADMIN",	send_comm, 		SERVERREQ,	"%R[%nserver%R]%n\n- Shows the iRC Administration information on current server or %R[%nserver%R]%n" },
	{ "AJOIN",	"AJoin",	auto_join,		0,	"%Y<%Bchannel%y(%Bs%y)%Y>%n\n- Add %Y<%Bchannel%y(%Bs%y)%Y>%n to AJoin list%P;%n it joins %Y<%Bchannel%y(%Bs%y)%Y>%n on startup or change of server" },
	{ "AJOINLIST",	"AJoinList",	auto_join,		0,	"- Display list of ajoin channels" },
	{ "ALIAS",	zero,		aliascmd,		0,	scripting_command },
	{ "ASSIGN",	one,		assigncmd,		0,	scripting_command },
	{ "AWAY",	"Away",		away,			SERVERREQ,	"%R[%nreason%R]%n\n- Sets you away on server if %R[%nreason%R]%n else sets you back" },
	{ "AWAYLOG",	"AwayLog",	awaylog,		0,	"%R[%nALL|MSGS|NOTICES|...%R]%n\n- Displays or changes what you want logged in your lastlog. This is a comma separated list" },
	{ "AWAYMSG",	"AwayMsg",	away,			0,	"Sets your auto-away msg" },
	{ "B",		NULL,		ban,			SERVERREQ,	"See %YBAN%n" },
	{ "BACK",	"Back",		back,			SERVERREQ,	"- Sets you back from being away" },
	{ "BAN",	NULL,		ban,			0,	"%Y<%Cnick%G|%Cnick%G!%nuser%y@%nhostname%Y>%n\n- Ban %Y<%Cnick%G|%Cnick%G!%nuser%y@%nhostname%Y>%n from current channel" },
	{ "BANSTAT",	NULL,		banstat,		0,	"%R[%Bchannel%R]%n\n- Show bans on current channel or %R[%Bchannel%R]%n" },
	{ "BANTYPE",	NULL,		bantype,		0,	"%W/%nbantype %Y<%nNormal%G|%nBetter%G|%nHost%G|%nDomain%G|%nScrew%G|%nIP%Y>%n\n- When a ban is done on a nick, it uses %Y<%nbantype%Y>%n" },
	{ "BANWORDS",	NULL,		add_ban_word,		0,	"%Y<%nchannel|*%Y>%n word(s)\n- Adds word or words to the banned words list for %Y<%nchannel%Y>" },
	{ "BEEP",	NULL,		beepcmd,		0,	"- Creates a beep noise" },
#ifdef WANT_CHELP
	{ "BHELP",	"BHELP",	chelp,			0,	"%Y<%nhelp%W|%nindex%W|%nother%Y>%n\n - BitchX help command" },
#endif
	{ "BIND",	NULL,		bindcmd,		0,	"- Command to bind a key to a function" },
	{ "BK",		NULL,		kickban,		SERVERREQ,	"%Y<%Cnick%Y>%n %R[%nreason%R]%n\n- Deops, bans and kicks %Y<%Cnick%Y>%n for %R[%nreason%R]%n" },
	{ "BKI",	"BKI",		kickban,		SERVERREQ,	"%Y<%Cnick%Y>%n %R[%nreason%R]%n\n- Deops, bans, kicks and ignores %Y<%Cnick%Y>%n for %R[%nreason%R]%n" },
	{ "BLESS",	NULL,		blesscmd,		0,	scripting_command },
	{ "BREAK",	NULL,		breakcmd,		0,	NULL },
	{ "BYE",	"QUIT",		e_quit,			0,	"- Quit Irc"},
	{ "C",		"MODE",		send_mode,		0,	"%Y<%nnick|channel%Y> <%nmode%Y>%n\n- Sets a mode for a channel or nick"},
	{ "CALL",	NULL,		e_call,			0,	scripting_command },
#ifndef BITCHX_LITE
	{ "CBOOT",	"Cboot",	cboot,			0,	"%Y<%nnick%Y>%n %R[%nreason%R]%n\n- Kicks a %Y<%nnick%Y>%n off the TwilightZone with %R[%nreason%R]" },
#endif
	{ "CD",		NULL,		cd,			0,	"%Y<%ndir%Y>%n\n - Changes current directory to %Y<%ndir%Y>%n or prints current directory" },
#ifdef WANT_CDCC
	{ "CDCC",	NULL,		cdcc,			0,	"%Y<%ncommand%Y>%n\n- Channel, DOffer, Echo, Load, List, MinSpeed, Notice, Offer, PList, Queue, ReSend, Save, Type Describe Note" },
#endif
#ifdef WANT_CD
	{ "CDEJECT",	NULL,		cd_eject,		0,	NULL },
	{ "CDHELP",	NULL,		cd_help,		0,	NULL },
	{ "CDLIST",	NULL,		cd_list,		0,	NULL },
	{ "CDPAUSE",	NULL,		cd_pause,		0,	NULL },
	{ "CDPLAY",	NULL,		cd_play,		0,	NULL },
	{ "CDSTOP",	NULL,		cd_stop,		0,	NULL },
	{ "CDVOL",	NULL,		cd_volume,		0,	NULL },
#endif
	{ "CHANNEL",	"JOIN",		e_channel,		SERVERREQ,	"- Shows information on the channels, modes and server you are on" },
#ifdef WANT_CHAN_NICK_SERV
	{ "CHANSERV",	"CHANSERV",	send_comm,		0,	NULL },
#endif
	{ "CHANST",	NULL,		channel_stats,		SERVERREQ,	"%Y<%n-ALL%Y> %R[%Bchannel%R]%n\n- Shows statistics on current channel or %R[%Bchannel%R]%n" },
	{ "CHAT",	"Chat",		chat,			SERVERREQ,	"%Y<%nNick%Y>%n\n- Attempts to dcc chat nick" },
	{ "CHATOPS",	"CHATOPS",	e_wall,			SERVERREQ,	NULL },
#ifdef WANT_USERLIST
	{ "CHGAOP",	"ChgAop",	change_user,		0,	"%Y<%Cnick%Y>%n <auto-oplevel>" },
	{ "CHGCHAN",	"ChgChan",	change_user,		0,	"%Y<%Cnick%Y>%n %Y<%Bchannel%Y>%n" },
	{ "CHGLEVEL",	"ChgLevel",	change_user,		0,	"%Y<%Cnick%Y>%n %Y<%nlevel%Y>%n" },
	{ "CHGPASS",	"ChgPass",	change_user,		0,	"%Y<%Cnick%Y>%n %Y<%nprotection-level%Y>%n" },
	{ "CHGPROT",	"ChgProt",	change_user,		0,	"%Y<%Cnick%Y>%n %Y<%nprotection-level%Y>%n" },
	{ "CHGUH",	"ChgUH",	change_user,		0,	"%Y<%Cnick%Y>%n %Y<%nuserhost%Y>%n" },
#endif
	{ "CHOPS",	"Chops",	users,			SERVERREQ,	"%R[%Bchannel%R]%n\n- Shows, in a full format, all the nicks with op status" },
	{ "CLEAR",	NULL,		my_clear,		0,	"- Clears the screen" },
	{ "CLEARAUTO",	"CLEARAUTO",	clear_tab,		0,	"- Clears all the nicks in the auto-response list" },
	{ "CLEARLOCK",	"ClearLock",	mode_lock,		0,	"%Y<%Cchannel%G|%Y*>%n\n- Unlocks the mode lock for %Y<%Cchannel%G|%Y*>%n" },
	{ "CLEARTAB",	NULL,		clear_tab,		0,	"- Clears the nicks in the tabkey list" },
#ifndef BITCHX_LITE
	{ "CLINK",	"Clink",	clink,			0,	"%Y<%Cnick%Y>%n %Y<%ntext%Y>%n" },
	{ "CLONES",	"Clones",	check_clones,		SERVERREQ,	NULL },
	{ "CMSG",	"Cmsg",		cmsg,			0,	"%Y<%Cnick%Y>%n %Y<%ntext%Y>%n\n- While in the TwilightZone, a private message will be sent to %Y<%Cnick%Y>%n with %Y<%ntext%Y>%n" },
#else
	{ "CLONES",	"Clones",	check_clones,		SERVERREQ,	NULL },
#endif
#ifdef __EMXPM__
        { "CODEPAGE",	"CODEPAGE",	pmcodepage,		0,	"OS/2 - Changes the current VIO Codepage" },
#endif
	{ "CONNECT",	"CONNECT",	send_comm,		0,	"%Y<%nserver1%Y>%n %Y<%nport%Y>%n %R[%nserver2%R]%n\n%Y*%n Requires irc operator status" },
	{ "CONTINUE",	NULL,		continuecmd,		0,	NULL },
#ifndef BITCHX_LITE
	{ "CSAY",	"Csay",		csay,			0,	"%Y<%ntext%Y>%n" },
#endif
	{ "CSET",	"Cset",		cset_variable,		0,	"%R[%n*|channel|chan*%R]%n\n- changes variables that affect a channel or displays them" },
	{ "CTCP",	NULL,		ctcp,			SERVERREQ,	"%Y<%Cnick%Y>%n %Y<%nrequest%Y>%n\n- CTCP sends %Y<%Cnick%Y>%n with %Y<%nrequest%Y>%n" },
#ifndef BITCHX_LITE
	{ "CTOGGLE",	NULL,		toggle_xlink,		0,	NULL },
	{ "CWHO",	"Cwho",		cwho,			0,	"- Lists the clients and bots connected to the TwilightZone" },
	{ "CWHOM",	"Cwhom",	cwho,			0,	"- Lists the clients and bots connected to the TwilightZone" },
#endif
	{ "CYCLE",	NULL,		cycle,			SERVERREQ,	"%R[%Bchannel%R]%n\n- Leaves current channel or %R[%Bchannel%R]%n and immediately rejoins" },
	{ "D",		NULL,		describe,		SERVERREQ,	"See %YDESCRIBE%n" },
	{ "DATE",	"TIME",		send_comm,		SERVERREQ,	"- Shows current time and date from current server" },
	{ "DBAN",	NULL,		unban,			SERVERREQ,	"- Clears all bans on current channel" },
	{ "DC",		NULL,		init_dcc_chat,		SERVERREQ,	"%Y<%Cnick%Y>%n\n- Starts a DCC CHAT to %Y<%Cnick%Y>%n" },
	{ "DCA",	"Dca",		dcx,			0,	"- Kills all dcc connections" },
	{ "DCC",	NULL,		dcc,			0,	"Displays a active DCC list. For more info /dcc help" },
	{ "DCG",	"Dcg",		dcx,			0,	"%Y<%nnick%Y>%n\n- Kills all dcc gets to %Y<%nnick%Y>" },
	{ "DCS",	"Dcs",		dcx,			0,	"%Y<%nnick%Y>%n\n- Kills all dcc sends to %Y<%nnick%Y>" },
	{ "DCX",	"Dcx",		dcx,			0,	"%Y<%nnick%Y>%n\n- Kills all dcc chats to %Y<%nnick%Y>" },
	{ "DEBUG",	NULL,		e_debug,		0,	"%R[%nflags%R]%n\n- Sets various debug flags in the client" },
	{ "DEBUGFUNC",	NULL,		debugfunc,		0,	NULL },
	{ "DEBUGHASH",	NULL,		show_hash,		0,	NULL },
	{ "DEBUGMSG",	NULL,		debugmsg,		0,	NULL },
#ifdef WANT_USERLIST
	{ "DEBUGUSER",	NULL,		debug_user,		0,	NULL },
#endif
	{ "DEHOP",	"h",		dodeop,			SERVERREQ,	"%Y<%C%nnick(s)%Y>%n\n- Removes halfops from %Y<%Cnick%y(%Cs%y)%Y>%n" },
	{ "DEOP",	"o",		dodeop,			SERVERREQ,	"%Y<%C%nnick(s)%Y>%n\n- Deops %Y<%Cnick%y(%Cs%y)%Y>%n" },
	{ "DEOPER",	NULL,		deop,			SERVERREQ,	"%Y*%n Requires irc operator status\n- Removes irc operator status" },
	{ "DESCRIBE",	NULL,		describe,		SERVERREQ,	"%Y<%Cnick%G|%Bchannel%Y>%n %Y<%naction%Y>%n\n- Describes to %Y<%Cnick%G|%Bchannel%Y>%n with %Y<%naction%Y>%n" },
#ifdef ALLOW_DETACH
	{ "DETACH",	NULL,		detachcmd,		0,	NULL },
#endif
	{ "DEVOICE",	"v",	dodeop,			SERVERREQ,	"%Y<%C%nnick(s)%Y>%n\n- de-voices %Y<%Cnick%y(%Cs%y)%Y>%n" },
#if !defined(WINNT) && !defined(__EMX__)
	{ "DF",		"df",		exec_cmd,		0,	"- Show disk space usage" },
#endif
	{ "DIE",	"DIE",		send_comm,		SERVERREQ,	"%Y*%n Requires irc operator status\n- Kills the IRC server you are on" },
#ifdef TRANSLATE 
	{ "DIGRAPH",	NULL,		digraph,		0,	NULL },
#endif
	{ "DISCONNECT",	NULL,		disconnectcmd,		0,	"- Disconnects you from the current server" },
	{ "DLINE",	"DLINE",	send_kline,		SERVERREQ,	NULL },
	{ "DME",	"dme",		me,			SERVERREQ,	"<action>\n- Sends an action to current dcc" },
	{ "DNS",	"NSlookup",	nslookup,		0,	"%YDNS<%nnick|hostname%y>%n\n- Attempts to nslookup on nick or hostname"},
	{ "DO",		NULL,		docmd,			0,	scripting_command },
	{ "DOP",	"o",		dodeop,			SERVERREQ,	"- See deop" },
	{ "DS",		NULL,		dcc_stat_comm,		0,	"- Displays some dcc file transfer stats" },
	{ "DUMP",	"Dump",		dumpcmd,		0,	"%Y<%ntype%Y>%n\n- Dumps %Y<%ntype%Y>%n to screen\n%Y<%ntype%Y>%n:\n%YAlias%n  %YAll%n  %YBind%n  %YChstats%n  %YFsets%n\n%YFile%n  %YOn%n  %YVar%n  %YTimers%n  %YWsets%n  %YCsets%n" },
	{ "ECHO",	NULL,		echocmd,		0,	"<text>\n- Echos text to the screen" },
#ifdef WANT_EPICHELP
	{ "EHELP",	NULL,		epichelp,		0,	NULL },
#endif
	{ "ENCRYPT",	NULL,		encrypt_cmd,		0,	NULL },
	{ "EVAL",	"EVAL",		evalcmd,		0,	scripting_command },
	{ "EVALSERVER", NULL,		evalserver,		0,	"<refnum> <command> sends to <refnum> server command" },
#if !defined(WINNT)
	{ "EXEC",	NULL,		execcmd,		0,	"%Y<%ncommand%Y>%n\n- Executes %Y<%ncommand%Y>%n with the shell set from %PSHELL%n" },
#endif
	{ "EXIT",	"QUIT",		e_quit,			0,	"- Quits IRC" },
	{ "FE",		"Fe",		fe,			0,	scripting_command },
	{ "FEC",	"Fec",		fe,			0,	scripting_command },
#ifdef GUI
	{ "FILEDIALOG",	NULL,		filedialog,		0,	"GUI - File dialog" },
#endif
	{ "FINGER",	NULL,		finger,			0,	"%Y<%Cnick%Y>%n\n- Fetches finger info on %Y<%Cnick%Y>%n" },
	{ "FK",		"FK",		masskick,		SERVERREQ,	"%Y<%Cnick%G!%nuser%Y@%nhostname%Y>%n%R[%nreason%R]%n\nFinds clienTs matching %Y<%Cnick%G!%nuser%Y@%nhostname%Y>%n and immediately kicks them from current channel for %R[%nreason%R]%n" },
	{ "FLUSH",	NULL,		flush,			0,	"- Flush ALL server output" },
#ifdef GUI
	{ "FONTDIALOG",	NULL,		fontdialog,		0,	"GUI - Font Dialog" },
#endif
	{ "FOR",	NULL,		forcmd,			0,	scripting_command },
	{ "FOREACH",	NULL,		foreach,		0,	scripting_command },
	{ "FORWARD",	"Forward", 	do_forward,		0,	"%Y<%Cnick%Y>%n\n- Forwards all messages to %Y<%Cnick%Y>%n" },
	{ "FPORTS",	NULL,		findports,		0,	"%Y<%nhostname%Y>%n %R[%Y<%nlowport%Y>%n %Y<%nhighport%Y>%R]%n\n- Attempts to find ports on %Y<%nhostname%Y>%n on %R[%Y<%nlowport%Y>%n %Y<%n %Y<%nhighport%Y>%R]%n" },
	{ "FPROT",	"FProt",	tog_fprot,		0,	"- Toggles flood protection to be either on or off" },
	{ "FSET",	"Fset",		fset_variable,		0,	"- Setting various output formats" },
#ifdef WANT_FTP
	{ "FTP",	"Ftp",		init_ftp,		0,	"%Y<%nhostname%Y> %R[%nlogin%R] [%npassword%R] [%n-p port%R]%n\n- Open a ftp connection to a host" },
#endif
	{ "FUCK",	"Fuck",		kickban,		SERVERREQ,	"%Y<%Cnick%Y>%n %R[%nreason%R]%n\n- Deops, bans and kicks %Y<%Cnick%Y>%n for %R[%nreason%R]%n" },
	{ "FUCKEM",	NULL,		fuckem,			SERVERREQ,	NULL },
	{ "GONE",	"Gone",		away,			SERVERREQ,	"%R[%nreason%R]%n\n- Sets you away on server if %R[%nreason%R]%n else sets you back. Does not announce to your channels." },
	{ "HASH",	"HASH",		send_comm,		SERVERREQ,	"Server command"},
	{ "HELP",	NULL,		help,			0,	NULL },
#ifdef WANT_CHAN_NICK_SERV
	{ "HELPOP",	"HELPOP",	send_comm,		SERVERREQ,	NULL },
	{ "HELPSERV",	"HELPSERV",	send_comm,		SERVERREQ,	NULL },
#endif
	{ "HISTORY",	NULL,		history,		0,	"- Shows recently typed commands" },
	{ "HOOK",	NULL,		hookcmd,		0,	scripting_command },
	{ "HOP",		"h",		doop,			SERVERREQ,	"%Y<%Cnick%Y>%n\n- Gives %Y<%Cnick%Y>%n +h" },
	{ "HOST",	"USERHOST",	userhostcmd,		0,	"- Shows host of yourself or %R[%Cnick%R]%n" },
	{ "HOSTNAME",	"HOSTNAME",	e_hostname,		0,	"%Y<%nhostname%Y>%n\n- Shows list of possible hostnames with option to change it on virtual hosts" },
	{ "I",		"INVITE",	do_invite,		SERVERREQ,	"- See %YINVITE%n" },
	{ "IF",		"IF",		ifcmd,			0,	scripting_command },
	{ "IG",		"Ig",		do_ig,			0,	"+%G|%n-%Y<%Cnick%Y>%n\n- Ignores ALL except crap and public of nick!host matching %Y<%Cnick%Y>%n" },
	{ "IGH",	"IgH",		do_ig,			0,	"+%G|%n-%Y<%Cnick%Y>%n\n- Ignores ALL except crap and public of hostname matching %Y<%Cnick%Y>%n" },
	{ "IGHT",	"IgHt",		do_ig,			0,	"+%G|%n-%Y<%Cnick%Y>%n\n- Ignores ALL except crap and public of hostname matching %Y<%Cnick%Y>%n and expires this on a timer"},
	{ "IGNORE",	NULL,		ignore,			0,	NULL },
	{ "IGT",	"Igt",		do_ig,			0,	"+%G|%n-%Y<%Cnick%Y>%n\n- Ignores ALL except crap and public of nick!host matching %Y<%Cnick%Y>%n and expires this on a timer"  },
	{ "INFO",	"INFO",		info,			0,	"- Shows current client info" },
	{ "INPUT",	"Input",	inputcmd,		0,	scripting_command  },
	{ "INPUT_CHAR", "Input_Char",	inputcmd,		0,	scripting_command },
	{ "INVITE",	"INVITE",	do_invite,		SERVERREQ,	"%Y<%Cnick%Y>%n %R[%Bchannel%R]%n\n- Invites %Y<%Cnick%Y>%n to current channel or %R[%Bchannel%R]%n" },
#ifdef WANT_CHAN_NICK_SERV
	{ "IRCIIHELP",	"IRCIIHELP",	send_comm,		SERVERREQ,	NULL },
#endif
	{ "IRCHOST",	"HOSTNAME",	e_hostname,		0,	"%Y<%nhostname%Y>%n\n- Shows list of possible hostnames with option to change it on virtual hosts"  },
	{ "IRCNAME",	NULL,		realname_cmd,		0,	NULL },
	{ "IRCUSER",	NULL,		set_username,		0,	"<username>\n- Changes your <username>" },
	{ "ISON",	"ISON",		isoncmd,		SERVERREQ,	NULL },
	{ "J",		"JOIN",		e_channel,		SERVERREQ,	"%Y<%nchannel%Y> %R[%nkey%R]%n\n- Joins a %Y<%nchannel%Y>" },
	{ "JNW",	"Jnw",		jnw,			SERVERREQ,	"%Y<%nchannel%Y> %R[%nkey%R]%n %R[%n-hide%R]%n\n- Joins a %Y<%nchannel%Y>%n in a new window"},
	{ "JOIN",	"JOIN",		e_channel,		SERVERREQ,	NULL },
	{ "K",		NULL,		dokick,			SERVERREQ,	"%Y<%Cnick%Y>%n %R[%nreason%R]%n\n- Kicks %Y<%Cnick%Y>%n for %R[%nreason%R]%n on current channel" },
	{ "KB",		"KB",		kickban,		SERVERREQ,	"%Y<%Cnick%Y>%n %R[%nreason%R]%n\n- Deops, kicks and bans %Y<%Cnick%Y>%n for %R[%nreason%R]%n" },
	{ "KBI",	"KBI",		kickban,		SERVERREQ,	"%Y<%Cnick%Y>%n %R[%nreason%R]%n\n- Deops, kicks and bans %Y<%Cnick%Y>%n for %R[%nreason%R]%n" },
	{ "KICK",	"KICK",		send_kick,		SERVERREQ,	"%Y<%Bchannel%G|%Y*>%n %Y<%Cnick%Y>%n %R[%nreason%R]%n" },
	{ "KICKIDLE",	"KickIdle",	kickidle,		SERVERREQ,	"%Y<%Bchannel%Y>%n\n- Kicks all idle people on %Y<%Bchannel%Y>%n" },
	{ "KILL",	"KILL",		send_kill,		SERVERREQ,	"%Y<%Cnick%Y>%n %R[%nreason%R]%n\n%Y*%n Requires irc operator status\n- Kills %Y<%Cnick%Y>%n for %R[%nreason%R]%n" },
	{ "KLINE",	"KLINE",	send_kline,		SERVERREQ,	"%Y<%nuser%y@%nhost%Y> <%nreason%Y>%n\n%Y*%n Requires irc operator status\n- Adds a K-line to the server configfile" },
	{ "KNOCK",	"KNOCK",	send_channel_com,	SERVERREQ,	NULL },
	{ "L",		"PART",		do_getout,		SERVERREQ,	"- See %YLEAVE" },
	{ "LAMENICKLIST",NULL,		add_bad_nick,		0,	"- Lists nicks in your lame nick list" },
	{ "LASTLOG",	NULL,		lastlog,		0,	"-file filename%G|%n-clear%G|%n-max #%G|%n-liternal pat%G|%n-beep%G|%nlevel" },
	{ "LEAVE",	"PART",		send_comm,		SERVERREQ,	"%Y<%Bchannel%Y>%n\n- Leaves current channel or %Y<%Bchannel%Y>%n" },
	{ "LINKS",	"LINKS",	send_comm,		SERVERREQ,	"- Shows servers and links to other servers" },
	{ "LIST",	"LIST",		funny_stuff,		SERVERREQ,	"- Lists all channels" },
#ifdef WANT_DLL
	{ "LISTDLL",	"LISTDLL",	dll_load,		0,	"- List currently loaded plugins" },
#endif
	{ "LK",		"LameKick",	LameKick,		SERVERREQ,	"%R[%nreason%R]%n\n- Kicks all non +o people on current channel with %R[%nreason%R]%n" },
	{ "LKW",	"Lkw",		lkw,			SERVERREQ,	"%R[%Bchannel%R]%n\n- Leave the current channel, killing the window in current channel or%R[%Bchannel%R]%n as well" },
	{ "LLOOK",	NULL,		linklook,		SERVERREQ,	"%Y*%n Requires set %YLLOOK%n %RON%n\n- Lists all the servers which are current split from the IRC network" },
	{ "LOAD",	"LOAD",		BX_load,			0,	"filename\n- Loads filename as a irc script" },
#ifdef WANT_DLL
	{ "LOADDLL",	NULL,		dll_load,		0,	"filename.so\n- Loads filename as a dll"},
#endif
#ifdef WANT_TCL
	{ "LOADTCL",	NULL,		tcl_load,		0,	"filename.tcl\n- Loads a tcl file" },
#endif

	{ "LOCAL",	"2",		localcmd,		0,	scripting_command },
	{ "LOCOPS",	"LOCOPS",	e_wall,			SERVERREQ,	NULL },
#if !defined(WINNT)
	{ "LS",		"ls",		exec_cmd,		0,	"- Displays list of file" },
#endif
	{ "LUSERS",	"LUSERS",	send_comm,		SERVERREQ,	"- Shows stats on current server" },
	{ "M",		"PRIVMSG",	e_privmsg,		0,	"See %YMSG%n" },
	{ "MAP",	NULL,		do_map,			0,	"- Displays a map of the current servers"},
	{ "MB",		NULL,		massban,		SERVERREQ,	"- Mass bans everybody on current channel" },
	{ "MD",		NULL,		massdeop,		SERVERREQ,	"- Mass deops current channel" },
	{ "MDOP",	NULL,		massdeop,		SERVERREQ,	"- Mass deops current channel" },
	{ "MDVOICE",	"MDVoice",	massdeop,		SERVERREQ,	"- Mass de-voices current channel" },
	{ "ME",		NULL,		me,			SERVERREQ,	"<action>\n- Sends an action to current channel" },
#ifdef WANT_CHAN_NICK_SERV
	{ "MEMOSERV",	"MEMOSERV",	send_comm,		SERVERREQ,	NULL },
#endif
#ifdef GUI
	{ "MENU",	NULL,		os2menu,		0,	"GUI - Creates or removes a menu" },
	{ "MENUITEM",	NULL,		os2menuitem,		0,	"GUI - Adds a menuitem to a menu" },
#endif
	{ "MESG",	NULL,		extern_write,		0,	"<YesB|BNoB||BOnB|BOff>\n- Turns mesg <Yes|No|On|Off>" },
	{ "MK",		NULL,		masskick,		SERVERREQ,	"- Mass kicks current channel" },
	{ "MKB",	"MKB",		masskickban,		SERVERREQ,	"- Mass kick bans current channel" },
	{ "MKNU",	NULL,		mknu,			SERVERREQ,	"- Mass kick bans non-userlist users on current channel" },
	{ "MODE",	"MODE",		send_channel_com,	SERVERREQ,	"%Y<%n#channel%W|%nnick%Y>%B +-%Y<%nmode%Y>%n" },
	{ "MODELOCK",	"ModeLock",	mode_lock,		0,	"%Y<%Bchannel%Y>%n +%G|%n-%Y<%ninstampkl #%Y>%n\n- Locks %Y<%Bchannel%Y>%n with +%G|%n-%Y<%nmodes%Y>%n" },
	{ "MOP",	NULL,		massop,			SERVERREQ,	"- Mass ops everybody on current channel" },
	{ "MORE",	"More",		readlog,		0,	"%R[%nfilename%R]%n\n- displays file in pages" },
	{ "MOTD",	"MOTD",		send_comm,		SERVERREQ,	"%R[%nserver%R]%n\n- Shows MOTD on current server %R[%nserver%R]%n" },
	{ "MSAY",	"MSay",		do_msay,		0,	NULL },
	{ "MSG",	"PRIVMSG",	e_privmsg,		0,	"%Y<%C[=]nick%Y>%n %Y<%ntext%Y>%n\n- Send %Y<%Cnick%Y>%n a message with %Y<%ntext%Y>%n" },
	{ "MTOPIC",	"MTopic",	do_mtopic,		0,	NULL },
	{ "MUB",	NULL,		unban,			SERVERREQ,	"- Mass unbans current channel" },
	{ "MULT",	NULL,		multkick,		SERVERREQ,	"%Y<%nnick(s)%Y>%n %R[%nreason%R]%n\n- Kicks all %Y<%nnick(s)%Y>%n with %R[%nreason%R]" },
	{ "MVOICE",	"MVoice",	massop,			SERVERREQ,	"%R[%Cnick%G|%Bchannel%G|%Cnick%Y!%Cuser%Y@%nhostname%Y]%n\n- Mass voice nicks matching %R[%Cnick%G|%Bchannel%G|%Cnick%Y!%Cuser%Y@%nhostname%Y]%n" },
	{ "N",		"NAMES",	do_mynames,		0,	NULL },
	{ "NAMES",	"NAMES",	funny_stuff,		SERVERREQ,	"%R[%Bchannel%R]%n\n- Shows names on current channel or %R[%Bchannel%R]%n" },
	{ "NEWNICK",	NULL,		newnick,		0,	NULL },
	{ "NEWUSER",	NULL,		newuser,		0,	NULL },
	{ "NICK",	"NICK",		e_nick,			0,	"-Changes nick to specified nickname" },
#ifdef WANT_CHAN_NICK_SERV
	{ "NICKSERV",	"NICKSERV",	send_comm,		SERVERREQ,	NULL },
#endif
	{ "NOCHAT",	"NoChat",	chat,			0,	"%Y<%nnick%Y>%n\n- Kills chat reqest from %Y<%nnick%Y>" },
	{ "NOPS",	"Nops",		users,			0,	"%R[%Bchannel%R]%n\n- Shows, in a full format, all the nicks without ops in %R[%Bchannel%R]%n or channel" },
	{ "NOTE",	"NOTE",		send_comm,		SERVERREQ,	NULL },
	{ "NOTICE",	"NOTICE",	e_privmsg,		0,	"%Y<%Cnick%G|%Bchannel%Y> %Y<%ntext%Y>%n\n- Sends a notice to %Y<%Cnick%G|%Bchannel%Y> with %Y<%ntext%Y>%n" },
	{ "NOTIFY",	NULL,		notify,			0,	"%Y<%Cnick|+|-nick%Y>%n\n- Adds/displays/removes %Y<%Cnick%Y>%n to notify list" },
	{ "NSLOOKUP",	"NSLookup",	nslookup,		0,	"%Y<%nhostname%Y>%n\n- Returns the IP adress and IP number for %Y<%nhostname%Y>%n" },
	{ "NWHOIS",	NULL,		nwhois,			0,	"%Y<%Cnick|channel%Y>%n\n- Shows internal statistics for %Y<%Cnick%Y>%n" },
	{ "NWHOWAS",	NULL,		whowas,			0,	"- Displays internal whowas info for all channels. This information expires after 20 minutes for users on internal list, 10 minutes for others" },
	{ "OFFERS",	"Offers",	do_offers,		0,	NULL },
	{ "ON",		NULL,		oncmd,			0,	scripting_command },
	{ "OOPS",	NULL,		do_oops,		SERVERREQ,	"%Y<%Cnick%Y>%n\n- Sends a oops message to last recipient of a message and sends the correct message to %Y<%Cnick%Y>%n" },
	{ "OP",		"o",		doop,			SERVERREQ,	"%Y<%Cnick%Y>%n\n- Gives %Y<%Cnick%Y>%n +o" },
	{ "OPER",	"OPER",		oper,			SERVERREQ,	"%Y*%n Requires irc operator status\n%Y<%Cnick%Y>%n %R[%npassword%R]%n" },
#ifdef WANT_CHAN_NICK_SERV
	{ "OPERSERV",	"OPERSERV",	send_comm,		SERVERREQ,	NULL },
#endif
	{ "OPERWALL",	"OPERWALL",	e_wall,			0,	NULL },
	{ "ORIGNICK",	"OrigNick",	orig_nick,		0,	"%Y<%Cnick%Y>%n\n- Attempts to regain a nick that someone else has taken from you." },
	{ "OSTAT",	NULL, 		serv_stat,		0,	"-Displays some internal statistics gathered about the server" },
#ifdef WANT_OPERVIEW
	{ "OV",		NULL,		ov_window,		0,	"%R[%non|off%R]%n %R[%-hide|+hide|hide%R]%n\n- turns on/off oper view window as well as creating hidden window" },
#endif
	{ "P",		"Ping",		pingcmd,		0,	"%Y<%nnick%Y>%n\n- Pings %Y<%nnick%Y>" },
	{ "PARSEKEY",	NULL,		parsekeycmd,		0,	scripting_command },
	{ "PART",	"PART",		send_2comm,		0,	"- Leaves %Y<%nchannel%Y>%n" },
	{ "PARTALL",	"PARTALL",	do_getout,		0,	"- Leaves all channels" },
	{ "PASTE",	NULL,		pastecmd,		0,	NULL },
	{ "PAUSE",	NULL,		e_pause,		0,	scripting_command },
	{ "PING",	NULL, 		pingcmd,		0,	"%Y<%nnick%Y>%n\n- Pings %Y<%nnick%Y>" },
#ifdef GUI
        { "PMPASTE",    NULL,           pmpaste,                0,      "GUI - Clipboard Paste Command" },
#endif
	{ "POP",	NULL,		pop_cmd,		0,	scripting_command },
#ifdef GUI
	{ "POPUPMENU",	NULL,		os2popupmenu,		0,	"GUI - Popup Menu Command" },
	{ "POPUPMSG",	NULL,		os2popupmsg,		0,	"GUI - Popup Message Command" },
#endif
	{ "PRETEND",	NULL,		pretend_cmd,		0,	scripting_command },
#ifdef GUI
        { "PROPERTIES", NULL,           pmprop,                 0,      "GUI - Properies Dialog" },
#endif
	{ "PS",		"ps",		exec_cmd,		0,	"- Displays process table" },
	{ "PURGE",	NULL,		purge,			0,	"<variable>\n- Removes all traces of variable(s) specified"},
	{ "PUSH",	NULL,		push_cmd,		0,	scripting_command },
	{ "Q",		NULL,		query,			0,	"%Y<%n-cmd cmdname%Y> <%Cnick%Y>%n\n- Starts a query to %Y<%Cnick%Y>%n" },
	{ "QME",	"qme",		me,			0,	"- Sends a action to a query" },
	{ "QUERY",	NULL,		query,			0,	"%Y<%n-cmd cmdname%Y> <%Cnick%Y>%n\n- Starts a query to %Y<%Cnick%Y>%n" },
	{ "QUEUE",      NULL,           queuecmd,               0,	scripting_command },
	{ "QUIT",	"QUIT",		e_quit,			0,	"- Quit BitchX" },
	{ "QUOTE",	"QUOTE",	quotecmd,		0,	"%Y<%ntext%Y>%n\n- Sends text directly to the server" },
	{ "RANDOMNICK",	NULL,		randomnick,		0,	"%Y<%Cnick%Y>%n\n- Changes your nick to a random nick. If nick is specified is is used as a prefix" },
	{ "RBIND",	NULL,		rbindcmd,		0,	"- Removes a key binding" },
	{ "READLOG",	"ReadLog",	readlog,		0,	"- Displays current away log" },
	{ "RECONNECT",	NULL,		reconnect_cmd,		0,	"- Reconnects you to current server" },
	{ "REDIRECT",	NULL,		redirect,		0,	"%Y<%Cnick%G|%Bchannel%Y> <command>\n- Redirects <command> to %Y<%Cnick%G|%Bchannel%Y>" },
	{ "REHASH",	"REHASH",	send_comm,		0,	"%Y*%n Requires irc operator status\n- Rehashs ircd.conf for new configuration" },
	{ "REINIT",	NULL,		init_vars,		0,	"- Reinitializes internal variables" },
	{ "REINITSTAT",	NULL,		init_window_vars,	0,	"- Reinitializes window variables" },

	{ "REL",	"Rel",		do_dirlasttype,		0,	relay_help  },

	{ "RELC",	"RelC",		do_dirlasttype,		0,	relay_help },
	{ "RELCR",	"RelCR",	do_dirlasttype,		0,	relay_help  },
	{ "RELCRT",	"RelCRT",	do_dirlasttype,		0,	relay_help  },
	{ "RELCT",	"RelCT",	do_dirlasttype,		0,	relay_help  },
	{ "RELD",	"RelD",		do_dirlasttype,		0,	relay_help  },
	{ "RELDT",	"RelDT",	do_dirlasttype,		0,	relay_help  },

	{ "RELI",	"RelI",		do_dirlasttype,		0,	relay_help  },
	{ "RELIT",	"RelIT",	do_dirlasttype,		0,	relay_help  },
	{ "RELM",	"RelM",		do_dirlasttype,		0,	relay_help  },
	{ "RELMT",	"RelMT",	do_dirlasttype,		0,	relay_help  },
	{ "RELN",	"RelN",		do_dirlasttype,		0,	relay_help  },
	{ "RELNT",	"RelNT",	do_dirlasttype,		0,	relay_help  },

	{ "RELOAD",	NULL,		reload_save,		0,	"- Reloads BitchX.sav" },

	{ "RELS",	"RelS",		do_dirlasttype,		0,	relay_help  },
	{ "RELSD",	"RelSd",	do_dirlasttype,		0,	relay_help  },
	{ "RELSDT",	"RelSdT",	do_dirlasttype,		0,	relay_help  },
	{ "RELSM",	"RelSM",	do_dirlasttype,		0,	relay_help  },
	{ "RELSMT",	"RelSMT",	do_dirlasttype,		0,	relay_help  },

	{ "RELSN",	"RelSN",	do_dirlasttype,		0,	relay_help  },
	{ "RELSNT",	"RelSNT",	do_dirlasttype,		0,	relay_help  },
	{ "RELST",	"RelST",	do_dirlasttype,		0,	relay_help  },
	{ "RELSTT",	"RelSTT",	do_dirlasttype,		0,	relay_help  },

	{ "RELSW",	"RelSW",	do_dirlasttype,		0,	relay_help  },
	{ "RELSWT",	"RelSWT",	do_dirlasttype,		0,	relay_help  },

	{ "RELT",	"RelT",		do_dirlasttype,		0,	relay_help  },
	{ "RELTT",	"RelTT",	do_dirlasttype,		0,	relay_help  },
	{ "RELW",	"RelW",		do_dirlasttype,		0,	relay_help  },
	{ "RELWT",	"RelWT",	do_dirlasttype,		0,	relay_help  },


	{ "REMLOG",	"RemLog",	remove_log,		0,	"- Removes logfile" },
	{ "REPEAT", 	NULL, 		repeatcmd,		0,	"%Y<%ntimes%Y>%n %Y<%ncommand%Y>%n\n- Repeats %Y<%ntimes%Y>%n %Y<%ncommand%Y>%n" },
	{ "REQUEST",	NULL,		ctcp,			0,	"%Y<%Cnick%G|%Bchannel%Y>%n %Y<%nrequest%Y>%n\n- Sends CTCP %Y<%nrequest%Y>%n to %Y<%Cnick%G|%Bchannel%Y>%n" },
	{ "RESET",	NULL,		reset,			0,	"- Fixes flashed terminals" },
	{ "RESTART",	"RESTART",	send_comm,		0,	"%Y*%n Requires irc operator status\n- Restarts server" },
	{ "RETURN",	NULL,		returncmd,		0,	NULL },
	{ "REVISIONS",	NULL,  	show_revisions,	0,	"- Shows CVS revisions of source files." },
	/* nuxx requested this command */
	{ "RMAP",	"MAP",		send_2comm,		0,	"- Sends out a /map command to the server%n" },
	{ "RPING",	"RPING",	send_comm,		0,	NULL },
	{ "SAVE",	"SaveAll",	savelists,		0,	"- Saves ~/.BitchX/BitchX.sav" },
	{ "SAVEIRC",	NULL,		save_settings,		0,	"- Saves ~/.bitchxrc" },
	{ "SAVELIST",	NULL, 		savelists,		0,	"- Saves ~/.BitchX/BitchX.sav" },
	{ "SAY",	empty_string,	do_send_text,		0,	"-%Y<%ntype%Y>%n Says whatever you write" },
	{ "SC",		"NAMES",	do_mynames,		0,	NULL },
	{ "SCAN",	"scan",		do_scan,		0,	"%R[%Bchannel%R]%n\n- Scans %R[%Bchannel%R]%n or current channel for all nicks" },
	{ "SCANF",	"ScanF",	do_scan,		0,	"%R[%Bchannel%R]%n\n- Scans %R[%Bchannel%R]%n or current channel for friends" },
	{ "SCANI",	"ScanI",	do_scan,		0,	"%R[%Bchannel%R]%n\n- Scans %R[%Bchannel%R]%n or current channel for ircops" },
	{ "SCANN",	"ScanN",	do_scan,		0,	"%R[%Bchannel%R]%n\n- Scans %R[%Bchannel%R]%n or current channel for non-ops" },
	{ "SCANO",	"ScanO",	do_scan,		0,	"%R[%Bchannel%R]%n\n- Scans %R[%Bchannel%R]%n or current channel for ops" },
	{ "SCANS",	"ScanS",	do_scan,		0,	"%R[%Bchannel%R]%n\n- Scans %R[%Bchannel%R]%n or current channel for shitlisted nicks" },
	{ "SCANV",	"ScanV",	do_scan,		0,	"%R[%Bchannel%R]%n\n- Scans %R[%Bchannel%R]%n or current channel for voice" },
	{ "SEND",	NULL,		do_send_text,		0,	NULL },
	{ "SENDLINE",	empty_string,	sendlinecmd,		0,	NULL },
	{ "SERVER",	NULL,		servercmd,		0,	"%Y<%nserver%Y>%n\n- Changes to %Y<%nserver%Y>%n" },
	{ "SERVLIST",	"SERVLIST",	send_comm,		0,	"- Displays available services" },
	{ "SET",	NULL,		setcmd,			0,	"- Set Variables" },
	{ "SETAR",	NULL,		set_autoreply,		0,	"%Y<%n-|d|pat1 pat2 ..%Y>%n\n- Adds or removes from your auto-response" },
	{ "SETENV",	NULL,		setenvcmd,		0,	NULL },
	{ "SHIFT",	NULL,		shift_cmd,		0,	scripting_command },
#ifdef WANT_USERLIST
	{ "SHITLIST",	NULL,		showshitlist,		0,	"- Displays internal shitlist" },
#endif
	{ "SHOOK",	NULL,		shookcmd,		0,	NULL },
	{ "SHOWIDLE",	"ShowIdle",	showidle,		0,	"%Y<-sort %R[none|time|host|nick%R]%Y> %R[%Bchannel%R]%n\n- Shows idle people on current channel or %R[%Bchannel%R]%n" },
	{ "SHOWLOCK",	"ShowLock",	mode_lock,		0,	"- Show the mode lock on the channel" },
	{ "SHOWSPLIT",	NULL,		linklook,		0,	"- Shows split servers" },
	{ "SHOWWORDKICK",NULL,		show_word_kick,		0,	"- Shows the internal banned word list" },

#ifdef WANT_CHAN_NICK_SERV
	{ "SILENCE",	"SILENCE",	send_comm,		0,	NULL },
#endif
	{ "SLEEP",	NULL,		sleepcmd,		0,	scripting_command },
	{ "SPAM",	NULL,		spam,			0,	NULL },
	{ "SPING",	"Sping",	sping,			0,	"%Y<%nserver|.|-clear%Y>%n\n- Checks how lagged you are to %Y<%nserver%Y>%n" },
	{ "SQUERY",	"SQUERY",	send_2comm,		0,	"%Y<%nservice%Y> <%ntext%Y>%n\n - Sends %Y<%ntext%Y>%n to %Y<%nservice%Y>%n" },
	{ "SQUIT",	"SQUIT",	send_2comm,		0,	"%Y<%nserver1%Y>%n %R[%nserver2%R]%n\n%Y*%n Requires irc operator status\n- Disconnects %Y<%nserver1%Y>%n from current server or %R[%nserver2%R]%n" },
	{ "STACK",	NULL,		stackcmd,		0,	scripting_command },
	{ "STATS",	"STATS",	do_stats,		0,	NULL },
	{ "STUB",	"Stub",		stubcmd,		0,	"(alias|assign) <name> <file> [<file ...]" },
#ifdef GUI
        { "SUBMENU",	NULL,		os2submenu,		0,      "GUI - Adds a submenu to a menu" },
#endif
	{ "SUMMON",	"SUMMON",	send_comm,		0,	"%Y<%nuser%Y> %R[%nserver%R [%nchannel%R]]%n\n- calls %Y<%nuser%Y>%n on %R[%nserver%R]%n to %R[%nchannel%R]%n" },
	{ "SV",		"Sv",		show_version,		0,	"%R[%nnick|#channel%R]%n\nShow Client Version information" },
	{ "SWALLOP",	"SWALLOPS",	e_wall,			0,	NULL },
#ifdef WANT_OPERVIEW
	{ "SWATCH",	NULL,		s_watch,		0,	"- Display or Modify what messages you want to see from the server"},
#endif
	{ "SWITCH",	"SWITCH",	switchcmd,		0,	scripting_command },
	{ "T", 		"TOPIC",	e_topic,		0,	"%Y<%ntext%Y>%n\n- Sets %Y<%ntext%Y>%n as topic on current channel"},
	{ "TBAN",	NULL,		tban,			0,	"- Interactive channel ban delete" },
	{ "TBK",	"TBK",		kickban,		0,	"%Y<%Cnick%Y>%n %R[%ntime%R]%n\n- Deops, bans and kicks %Y<%Cnick%Y>%n for %R[%ntime%R]%n" },
	{ "TCL",	NULL,		tcl_command,		0,	"<-version> <command>" },
	{ "TIGNORE",	NULL,		tignore,		0,	NULL },
	{ "TIME",	"TIME",		send_comm,		0,	"- Shows time and date of current server" },
	{ "TIMER",	"TIMER",	timercmd,		0,	NULL },
	{ "TKB",	"TKB",		kickban,		0,	"%Y<%Cnick%Y>%n %R[%ntime%R]%n\n- Deops, kicks, and bans %Y<%Cnick%Y>%n for %R[%ntime%R]%n" },
	{ "TKLINE",	"TKLINE",	send_kline,		0,	"%Y<%nuser%y@%nhost%Y> <%nreason%Y>%n\n%Y*%n Requires irc operator status\n- Adds a temporary K-line" },
	{ "TLOCK",	"TLock",	topic_lock,		0,	"%Y<%Bchannel%Y>%n %R[%non%G|%noff%R]%n\n- [Un]Locks %Y<%Bchannel%Y>%n with the current topic" },
	{ "TOGGLE",	NULL,		do_toggle,		0,	"- Various Client toggles which can be change" },
	{ "TOPIC",	"TOPIC",	e_topic,		0,	"- Change or show the current or specified channel topic" },
	{ "TRACE",	"TRACE",	do_trace,		0,	"<argument> <server>\n- Without a specified server it shows the current connections on local server\n- Arguments: -s -u -o   trace for servers, users, ircops" },
	{ "TRACEKILL",	"TraceKill",	tracekill,		0,	NULL },
	{ "TRACESERV",	"TraceServ",	traceserv,		0,	NULL },
	{ "TYPE",	NULL,		type,			0,	scripting_command },
	{ "U",		NULL,		users,			0,	"- Shows users on current channel" },
	{ "UB",		NULL,		unban,			0,	"%R[%Cnick%R]%n\n- Removes ban on %R[%Cnick%R]%n" },
	{ "UMODE",	"MODE",		umodecmd,		0,	"%Y<%nmode%Y>%n\n- Sets %Y<%nmode%Y>%n on yourself" },
	{ "UNAJOIN",	"UnAjoin",	auto_join,		0,	"%Y<%Bchannel%Y>%n\n- Removes autojoin %Y<%Bchannel%Y>%n from list" },
	{ "UNBAN",	NULL,		unban,			0,	NULL },
	{ "UNBANWORD",	"UnWordKick",	add_ban_word,		0,	"- Removes a banned word" },
	{ "UNFORWARD",  "NoForward",	do_forward,		0,	"%Y<%Cnick%G|%Bchannel%Y>\n- Remove %Y<%Cnick%G|%Bchannel%Y> from forward list" },
	{ "UNIDLE",	"UnIdle",	addidle,		0,	"%Y<%Bchannel%Y>%n\n- Removes %Y<%Bchannel%Y>%n from idle list" },
	{ "UNIG",	"UnIg",		do_ig,			0,	"%Y<%Cnick%Y>%n\n- UnIgnores %Y<%Cnick%Y>%n"},
	{ "UNIGH",	"UnIgH",	do_ig,			0,	"%Y<%Cnick%Y>%n\n- Removes %Y<%Cnick%Y>%n's host from the ignore list" },
	{ "UNKEY",	NULL,		do_unkey,		0,	"- Removes channel key from current channel" },
	{ "UNKLINE",	"UNKLINE",	send_kline,		0,	"<host|host,host2> remove klines for host(s)" },
	{ "UNLAMENICK","UnLameNick",	add_bad_nick,		0,	"%Y<%nnick(s)%Y>%n\n- Remove nicks from you lame nick list"},
	{ "UNLESS",	"UNLESS",	ifcmd,			0,	scripting_command },
	{ "UNLOAD",	NULL,		unload,			0,	"- Unloads all loaded scripts" },
#ifdef WANT_DLL
	{ "UNLOADDLL",	NULL,		unload_dll,		0,	"- Unloads loaded a plugin" },
#endif
	{ "UNSCREW",	NULL,		do_unscrew,		0,	"%Y<%Cnick%Y>%n\n- Unscrew %Y<%Cnick%Y>%n" },
	{ "UNSHIFT",	NULL,		unshift_cmd,		0,	scripting_command },
#ifdef WANT_USERLIST
	{ "UNSHIT",	"UnShit",	add_shit,		0,	"%Y<%nnick%Y> <%nchannel%Y>%n" },
#endif
	{ "UNTIL",	"UNTIL",	whilecmd,		0,	scripting_command },
	{ "UNTOPIC",	"UNTOPIC",	untopic,		0,	"- Unsets a channel topic" },
#ifdef WANT_USERLIST
	{ "UNUSER",	"UnUser",	add_user,		0,	"%Y<%nnick%W|%nnick!user@hostname%Y> <%n#channel%W|%n*%Y>%n" },
#endif
	{ "UNVOICE",	"v",	dodeop,			0,	NULL },
	{ "UNWORDKICK",	"UnWordKick",	add_ban_word,		0,	"%Y<%n#channel%Y> <%nword%Y>%n" },
	{ "UPING",	"uPing",	pingcmd,		0,	NULL },
	{ "UPTIME",	NULL,		do_uptime,		0,	NULL },
	{ "URL",	NULL,		url_grabber,		0,	NULL },
	{ "USER",	NULL,		users,			0,	NULL },
	{ "USERHOST",	NULL,		userhostcmd,		0,	NULL },
#ifdef WANT_USERLIST
	{ "USERINFO",	NULL,		set_user_info,		0,	NULL },
#endif
	{ "USERIP",	"USERIP",	useripcmd,		0,	NULL },
#ifdef WANT_USERLIST
	{ "USERLIST",	NULL,		showuserlist,		0,	NULL },
#endif
	{ "USERS",	"USERS",	send_comm,		0,	"%R[%nserver%R]%n\n- Show users on %R[%nserver%R]%n (as finger @host)" },
#ifdef WANT_USERLIST
	{ "USERSHOW",	"UserShow",	set_user_info,		0,	NULL },
#endif
	{ "USLEEP",	NULL,		usleepcmd,		0,	NULL },
	{ "USRIP",	"USRIP",	usripcmd,		0,	NULL },
	{ "VER",	"Version",	ctcp_version,		0,	NULL },
	{ "VERSION",	"VERSION",	version1,		0,	NULL },
	{ "VOICE",	"v",	doop,			0,	NULL },
	{ "W",		"W",		whocmd,			0,	NULL },
	{ "WAIT",	NULL,		waitcmd,		0,	scripting_command },
	{ "WALL",	"WALL",		ChanWallOp,		0,	NULL },
	{ "WALLCHOPS",	"WALLCHOPS",	send_2comm,		0,	NULL },
	{ "WALLMSG",	NULL,		ChanWallOp,		0,	NULL },
	{ "WALLOPS",	"WALLOPS",	e_wall,			0,	NULL },
	{ "WATCH",	"WATCH",	watchcmd,		0,	NULL },
	{ "WHICH",	"WHICH",	BX_load,			0,	"- Shows which script would be loaded without loading it" },
	{ "WHILE",	"WHILE",	whilecmd,		0,	scripting_command },
	{ "WHO",	"Who",		whocmd,			SERVERREQ,	NULL },
	{ "WHOIS",	"WHOIS",	whois,			SERVERREQ,	NULL },
	{ "WHOKILL",	"WhoKill",	whokill,		SERVERREQ,	"%Y<%Cnick%G!%nuser%Y@%nhostname%Y>%n %R[%nreason%R]%n\n%Y*%n Requires irc operator status\n- Kills multiple clients matching the filter %Y<%Cnick%G!%nuser%Y@%nhostname%Y>%n with %R[%n%W:%nreason%R]%n" },
	{ "WHOLEFT",	"WhoLeft",	whowas,			SERVERREQ,	NULL },
	{ "WHOWAS",	"WhoWas",	whois,			SERVERREQ,	NULL },
	{ "WI",		"Whois",	whois,			SERVERREQ,	NULL },
	{ "WII",	NULL,		my_whois,		SERVERREQ,	NULL },
	{ "WILC",	"WILC",		my_whois,		0,	NULL },
	{ "WILCR",	"WILCR",	my_whois,		0,	NULL },
	{ "WILM",	"WILM",		my_whois,		0,	NULL },
	{ "WILN",	"WILN",		my_whois,		0,	NULL },
	{ "WINDOW",	NULL,		windowcmd,		0,	"Various window commands. use /window help" },
	{ "WORDLIST",	NULL,		show_word_kick,		0,	NULL },
	{ "WSET",	NULL,		wset_variable,		0,	NULL },
	{ "WW",		"WhoWas",	whois,			0,	NULL },
	{ "XDEBUG",	NULL,		xdebugcmd,		0,	NULL },
	{ "XECHO",	"XECHO",	echocmd,		0,	NULL },
	{ "XEVAL",	"XEVAL",	xevalcmd,		0,	NULL },
	{ "XQUOTE",	"XQUOTE",	quotecmd,		0,	NULL },
	{ "XTRA",	"XTRA",		e_privmsg,		0,	NULL },
	{ "XTYPE",	NULL,		xtypecmd,		0,	NULL },
	{ NULL,		NULL,		comment,		0,	NULL }
};

/* number of entries in irc_command array */
#define	NUMBER_OF_COMMANDS (sizeof(irc_command) / sizeof(IrcCommand)) - 2

BUILT_IN_COMMAND(about)
{
#ifdef GUI
       static char *about_text =
               "Grtz To: Trench, HappyCrappy, Yak, Zircon, Otiluke, Masonry,\n\
BuddahX, Hob, Lifendel, JondalaR, JVaughn, suicide, NovaLogic,\n\
Jordy, BigHead,Ananda, Hybrid, Reefa, BlackJac, GenX, MHacker,\n\
PSiLiCON, hop, Sheik, psykotyk, oweff, icetrey, Power, sideshow,\n\
Raistlin, Mustang, [Nuke], Rosmo, Sellfone, Drago and bark0de!\n\n\
Mailing list is at <bitchx-devel@lists.sourceforge.net>\n";
       gui_about_box(about_text);
       return;
#else
	int i = strip_ansi_in_echo;
	strip_ansi_in_echo = 0; 

#if defined(WINNT) || defined(__EMX__)
	put_it("[0;25;35;40m [0m");
#else
	charset_ibmpc();
	put_it("[0;25;35;40m [0m");
#endif
#ifdef ASCII_LOGO
	put_it("                                                                   ,");
	put_it("                                           .                     ,$");
	put_it("                 .                                              ,$'");
	put_it("                                           .        .          ,$'");
	put_it("                 :      ,g$p,              .         $,       ,$'");
	put_it("               y&$       `\"` .,.           $&y       `$,     ,$'");
	put_it("               $$$     o oooy$$$yoo o      $$$        `$,   ,$' -acidjazz");
	put_it("         .     $$$%%yyyp, gyp`$$$'gyyyyyyp, $$$yyyyp,   `$, ,$'     .");
	put_it("       . yxxxx $$$\"`\"$$$ $$$ $$$ $y$\"`\"$$$ $$$\"`\"$$$ xxx`$,$'xxxxxxy .");
	put_it("         $     $$7   l$$ $$$ $$$ $$7   \"\"\" $$7   ly$     .$'       $");
	put_it("         $     $$b   dy$ $$$ $y$ $$b   $$$ $$b   d$$    ,$`$,      $");
	put_it("       . $xxxx $$$uuu$$$ $$$ $$$ $$$uuu$$$ $$$   $$$ x ,$'x`$, xxxx$ .");
	put_it("         .           \"\"\" \"\"\" \"\"\"       \"\"\"       \"\"\"  ,$'   `$,    .");
	put_it("           b i t c h    -      x                     ,$'     `$,");
	put_it("                                                     $'       `$,");
	put_it("                                                    '          `$,");
	put_it("                                                                `$,");
	put_it("                                                                 `$");
	put_it("                                                                   `");
	put_it(empty_string);
	put_it("[0;30;47m                                                                            [0m");
	put_it("[0;30;47m Grtz To: Trench, HappyCrappy, Yak, Zircon, Otiluke, Masonry, BuddahX, Hob, [0m");
	put_it("[0;30;47m          Lifendel, JondalaR, JVaughn, Suicide, NovaLogic, Jordy, BigHead,  [0m");
	put_it("[0;30;47m          Ananda, Hybrid, Reefa, BlackJac, GenX, MHacker, PSiLiCON,         [0m");
	put_it("[0;30;47m          hop, Sheik, psykotyk, oweff, icetrey, Power, sideshow, Raistlin,  [0m");   
	put_it("[0;30;47m          [Nuke], Rosmo and Bark0de!                                        [0m");
	put_it("[0;30;47m A special thanks to ccm.net for co-locating BitchX.com                     [0m");
	put_it("[0;30;47m Mailing list is at <bitchx-devel@lists.sourceforge.net>                    [0m");
	put_it(empty_string);
#else
	put_it("[35m    ‹‹‹‹€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€ﬂ€€[1;45m∞±≤[0;35;40m  [1;45m≤±[0;35;40mﬂ[1;32m‹[42m±≤[40m‹[0;35;40m [1;45m≤[40mﬁ[0m");
	put_it("[35m [1;31m‹ﬂ[0m   [1;35;45m≤[0;35;40m€€€€€€€€€€€€[1;31;41m≤[40mﬂ[0m   [1;35;45m≤[0;35;40m€€€€€€€€€€€€€€€€€€[1;31;41m≤[40mﬂ[0m   [1;35;45m∞[0;35;40m€€€€€€€€ﬂﬂ[1;32m‹›[0;35;40mﬁ€€[1;45m∞±[0;35;40m  [1mﬂ[0;32;40m‹[1;42m∞±≤€[40mﬂ[0;35;40m [1;45m±[0;35;40mﬁ[0m");
	put_it("[35mﬁ[1;31;41m±[0m    [35m‹[1;31m‹[0;32;40m   [35mﬂ€[1;31;41m≤[40mﬂ[0;32;40m  [1;35m‹[45m≤[31;41m±[0m    [35m‹‹‹‹[1;45mﬂ[0;35;40m€€[1;31;45m‹[40mﬂ[0;32;40m   [35m‹[1;31m‹[0;32;40m  [1;35m ﬂ[45m‹[31;41m±[0m    [35m‹[1;31m‹[0;32;40m   ‹‹‹‹[1;42m∞±≤[0m [35m€€€€[1;45m∞[0;35;40m [32m‹[1;42m∞±[0;32;40mﬂﬂ[1mﬂ[35m‹[45m≤[0;35;40m [1;45m∞[0;35;40mﬁ[0m");
	put_it("[35m€[1;31;41m∞[0m    [1;35;45m∞[31;41m€[0m    [1;35;45m [31m‹‹[0;35;40mﬂﬂﬂ€[1;31;41m∞[0m    [1;35;45m∞[31;41m€[40mﬂ[0;32;40m   [1;35;45m∞[31;41m€[0m    [1;35;45m∞[31;41m≤[0m    [1;35;45m≤[31;41m∞[0m    [1;35;45m∞[31;41m€[0m    [35m‹[32mﬂﬂﬂﬂ[1;42m±≤[40m‹[0;35;40mﬂ[1;45m∞[0;35;40mﬂ[32m‹ﬂﬂ[1;35m‹‹[45m≤[40m›[0;35;40m [1;45m≤±[0;35;40m €ﬁ[0m");
	put_it("[35m€[1;31;41m [0m    [1;35;45m±[31;41m≤[0m    [1;35;45m∞[31;41m≤[0m    [1;35;45m∞[31;41m [0m    [1;35;45m±[31;41m≤[0m    [1;35;45m±[31;41m≤[0m    [1;35;45m±[31m‹[0;35;40mﬂﬂﬂﬂ[1;45m‹[31;41m [0m    [1;35;45m±[31;41m≤[0m    [1;35;45m∞[0;35;40m€€€€‹‹‹[32mﬂ[1m˛[0;32;40m‹[35m˛  [1;45m≤±±[0;35;40m› [1;45m±∞[0;35;40m €[1;30mﬁ[0m");
	put_it("[35m€[31;45m≤[0m    [1;35;45m≤[31;41m±[0m    [1;35;45m±[31;41m±[0m    [1;35;45m±[0;31;45m≤[0m    [1;35;45m≤[31;41m±[0m    [1;35;45m≤[31;41m±[0m    [1;35;45m≤[31;41m±[0m    [1;35;45m±[0;31;45m≤[0m    [1;35;45m≤[31;41m±[0m    [1;35;45m±[0;35;40m€€€€€ﬂ[32m‹ﬂ[35m‹‹[1;32mﬂ‹‹[0;32;40m‹[35mﬂﬂ› [1;45m∞[0;35;40mÀ €ﬁ[0m");
	put_it("[35m€[31;45m±[0m    [1;35;45m€[31;41m∞[0m    [1;35;45m≤[31;41m∞[0m    [1;35;45m≤[0;31;45m±[0m    [1;35;45m€[31;41m∞[0m    [1;35;45m€[31;41m∞[0m    [1;35;45m€[31;41m∞[0m    [1;35;45m≤[0;31;45m±[0m    [1;35;45m€[31;41m∞[0m    [1;35;45m≤[0;35;40m€€€ﬂ[32m‹[1;42m∞[0;32;40m›[35mﬁ€[1;30;45m∞[0;35;40m›[1;32mﬁ€[42m≤±∞[0;32;40m‹‹‹[35mﬂ €ﬁ[0m");
	put_it("[35m€[31;45m∞[0m    [1;35mﬂ[0;31;40mﬂ[1;34m   [35m‹[45m€[31;41m [0m   [1;35m‹[45m€[0;31;45mﬂ[40m‹[1;34m   [35mﬂ[0;31;40mﬂ[1;34m   [35m‹[45mﬂ[0;31;45mﬂ[40m‹[1;34m   [35mﬂ[0;31;40mﬂ[1;34m   [35m‹[45mﬂ[0;31;45m∞[0m   [1;35m‹[45m€[31;41m [0m   [1;35m‹[45m€[0;35;40m€ﬂ[1;32m‹[42m±∞[0;32;40m€[0m [35m€€€€[34m [1;32;42m≤±±[0;32;40m€[1;42m∞[0;32;40m€€€‹‹[1;30mﬁ[0m");
	put_it("[35mﬁ€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€ﬂ[1;32m‹[42m≤±∞[0;32;40m€›[35mﬁ€[1;30;45m∞[0;35;40m€[1;30;45m∞[0;35;40m [34m [1;32;42m±∞∞[0;32;40m€€€ﬂﬂ[1;30m‹[0;35;40mﬁ");
	put_it(" ﬂ€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€›[1;32mﬁ[42m≤±∞[0;32;40m€€[0m [35m€€€[1;30;45m∞±[0;35;40m  [34m [1;32;42m∞[0;32;40mﬂﬂ[35m [1;30m‹[45m≤[0;35;40m [1;30;45m≤[40mﬁ[0;35;40m");
	put_it("    ﬂﬂﬂﬂ€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€‹[1;32mﬂ[0;32;40mﬂﬂﬂ[35m‹[30;45mrE[35;40m€[1;30;45m∞±≤[0;35;40m  [1;30;45m≤[40m‹[45m≤[40m›[0;35;40m [1;30;45m≤±[0;35;40m [1;30;45m±[40mﬁ[0m");
	put_it(empty_string);
	put_it("[1m‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹≤[0m‹");
	put_it("[1;47m€[0;30;47m                                                                             [0m");
	put_it("[1;47m≤[0;30;47m Grtz To: Trench, HappyCrappy, Yak, Zircon, Otiluke, Masonry, BuddahX, Hob, [1m∞[0m");
	put_it("[1;47m±[0;30;47m          Lifendel, JondalaR, JVaughn, suicide, NovaLogic, Jordy, BigHead,  [1m±[0m");
	put_it("[1;47m∞[0;30;47m          Ananda, Hybrid, Reefa, BlackJac, GenX, MHacker, PSiLiCON,         [1m≤[0m");
	put_it("[1;47m∞[0;30;47m          hop, Sheik, psykotyk, oweff, icetrey, Power, sideshow, Raistlin,  [1m≤[0m");
	put_it("[1;47m∞[0;30;47m          Mustang, [Nuke], Rosmo, Sellfone, Drago and bark0de!              [1m≤[0m");
	put_it("[1;47m∞[0;30;47m Mailing list is at <bitchx-devel@lists.sourceforge.net>                    [1m≤[0m");
	put_it("ﬂ[1;30m≤ﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂ[0m");
	put_it(empty_string);
#endif
#if !defined(WINNT) && !defined(__EMX__)
#if defined(LATIN1)
	charset_lat1();
#elif defined(CHARSET_CUSTOM)
	charset_cst();
#endif
#endif
	strip_ansi_in_echo = i; 
#endif
}

BUILT_IN_COMMAND(dcc_stat_comm)
{
	dcc_stats(NULL, NULL);
}

void handle_dcc_chat(UserhostItem *stuff, char *nick, char *args)
{
	if (!stuff || !stuff->nick || !nick || !strcmp(stuff->user, "<UNKNOWN>") || !strcmp(stuff->host, "<UNKNOWN>"))
	{
		bitchsay("No such nick %s", nick);
		return;
	}
	dcc_chat(NULL, args);
}

BUILT_IN_COMMAND(init_dcc_chat)
{
char *nick = NULL;
	if (args && *args)
	{
		char *n;
		while ((nick = next_arg(args, &args)))
		{
			while ((n = next_in_comma_list(nick, &nick)))
			{
				if (!n || !*n) break;
				if (!my_stricmp(n, get_server_nickname(from_server))) continue;
				userhostbase(n, handle_dcc_chat, 1, n, NULL);
			}
		}
	}
}

#ifdef WANT_FTP
BUILT_IN_COMMAND(init_ftp)
{
	if (args && *args)
		dcc_ftpopen(NULL, args);
}
#endif

char *glob_commands(char *name, int *cnt, int pmatch)
{
IrcCommand *var = NULL;
char *loc_match;
char *mylist = NULL;
	*cnt = 0;
	/* let's do a command completion here */
	if (!(var = find_command(name, cnt)))
		return m_strdup(empty_string);
	loc_match = alloca(strlen(name)+2);
	sprintf(loc_match, "%s*", (name && *name) ? name : empty_string);
	while (wild_match(loc_match, var->name))
	{
		m_s3cat(&mylist, space, var->name);
		var++;
	}
	return m_strdup(mylist ? mylist : empty_string);
}

/*
 * find_command: looks for the given name in the command list, returning a
 * pointer to the first match and the number of matches in cnt.  If no
 * matches are found, null is returned (as well as cnt being 0). The command
 * list is sorted, so we do a binary search.  The returned commands always
 * points to the first match in the list.  If the match is exact, it is
 * returned and cnt is set to the number of matches * -1.  Thus is 4 commands
 * matched, but the first was as exact match, cnt is -4.
 */
IrcCommand *BX_find_command(char *com, int *cnt)
{
	IrcCommand *retval;
	int loc;

	/*
	 * As a special case, the empty or NULL command is send_text.
	 */
	if (!com || !*com)
	{
		*cnt = -1;
		return irc_command;
	}

	retval = (IrcCommand *)find_fixed_array_item ((void *)irc_command, sizeof(IrcCommand), NUMBER_OF_COMMANDS + 1, com, cnt, &loc);
	return retval;
}

#ifdef WANT_DLL
IrcCommandDll * find_dll_command(char *com, int *cnt)
{
	int len = 0;
	
	if (com && (len = strlen(com)) && dll_commands)
	{
		int	min,
			max;
		IrcCommandDll *old, *old_next = NULL;
		
		*cnt = 0;
		min = 1;
		max = 0;
		for (old = dll_commands; old; old = old->next)
			max++;
		
		old = dll_commands;
		while (1)
		{
			if (!my_strnicmp(com, old->name, len))
			{
				if (!old_next)
					old_next = old;
				(*cnt)++;
			}
			if (old->next == NULL)
			{
				if (old_next && strlen(old_next->name) == len)
					*cnt *= -1;
				return (old_next);
			}
			else
				old = old->next;
		}
	}
	else
	{
		*cnt = -1;
		return (NULL);
	}
}
#endif

/* IRCUSER command. Changes your userhost on the fly.  Takes effect
 * the next time you connect to a server 
 */
BUILT_IN_COMMAND(set_username)
{
        char *blah;
	
	if ((blah = next_arg(args, &args)))
	{
		if (!strcmp(blah, "-"))
			strmcpy(username, empty_string, NAME_LEN);
		else 
			strmcpy(username, blah, NAME_LEN);
		say("Username has been changed to '%s'",username);
	}
}

/* This code is courtesy of Richie B. (richie@morra.et.tudelft.nl)
 * Modified by Christian Deimel (defender@gmx.net), now, if no argument is
 * given, a random name is chosen
 */
/*
 * REALNAME command. Changes the current realname. This will only be parsed
 * to the server when the client is connected again.
 */
BUILT_IN_COMMAND(realname_cmd)
{
	
        if (*args)
	{
                strmcpy(realname, args, REALNAME_LEN);
		say("Realname at next server connnection: %s", realname);
	}
	else
	{
		char *s = NULL;
		s = get_realname(get_server_nickname(from_server));
		strmcpy(realname, s, REALNAME_LEN);
		say("Randomly chose a new ircname: %s", realname);
	}
}

BUILT_IN_COMMAND(pop_cmd)
{
	extern char *function_pop(char *, char *);
        char *blah = function_pop(NULL, args);
        new_free(&blah);
}

BUILT_IN_COMMAND(push_cmd)
{
	extern char *function_push(char *, char *);
        char *blah = function_push(NULL, args);
        new_free(&blah);
}

BUILT_IN_COMMAND(shift_cmd)
{
	extern char *function_shift(char *, char *);
        char *blah = function_shift(NULL, args);
	
        new_free(&blah);
}

BUILT_IN_COMMAND(unshift_cmd)
{
	extern char *function_unshift(char *, char *);
        char *blah = function_unshift(NULL, args);
	
        new_free(&blah);
}

BUILT_IN_COMMAND(do_forward)
{
	
	if (command && (!my_stricmp(command, "NOFORWARD") || !my_stricmp(command, "UNFORWARD")))
	{
		if (forwardnick)
		{
			bitchsay("No longer forwarding messages to %s", forwardnick);
		        send_to_server("NOTICE %s :%s is no longer forwarding to you",
				forwardnick, get_server_nickname(from_server));
		} 
		new_free(&forwardnick);
		return;
	}
	if (args && *args)
	{
		char *q;
		malloc_strcpy(&forwardnick, args);
		if ((q = strchr(forwardnick, ' ')))
			*q = 0;
		send_to_server("NOTICE %s :%s is now forwarding messages to you",
			forwardnick, get_server_nickname(from_server));
		bitchsay("Now forwarding messages to %s", forwardnick);
	}
	return;
}

BUILT_IN_COMMAND(blesscmd)
{
	bless_local_stack();
}

BUILT_IN_COMMAND(do_oops)
{
	const char      *to = next_arg(args, &args);
	const char      *sent_nick = get_server_sent_nick(from_server);
	const char      *sent_body = get_server_sent_body(from_server);

	if (sent_nick && sent_body && to && *to)
	{
		send_to_server("PRIVMSG %s :Oops, that /msg wasn't for you", sent_nick);
		send_to_server("PRIVMSG %s :%s", to, sent_body);
		if (window_display && do_hook(SEND_MSG_LIST, "%s %s", to, sent_body))
			put_it("%s", convert_output_format(fget_string_var(FORMAT_SEND_MSG_FSET), "%s %s %s %s", update_clock(GET_TIME),
				to, get_server_nickname(from_server), sent_body));
	}
}

/*
 * RECONNECT command. Closes the server, and then reconnects again.
 * Works also while connected to multiple servers. It only reconnects to the
 * current server number (which is stored in from_server). 
 * This command puts the REALNAME command in effect.
 */
BUILT_IN_COMMAND(reconnect_cmd)
{
	char scommnd[6];
		
	if (from_server == -1)
	{
		bitchsay("Try connecting to a server first.");
		return;
	}
	if (do_hook(DISCONNECT_LIST, "Reconnecting to server"))
		put_it("%s", convert_output_format("$G Reconnecting to server %K[%W$1%K]", "%s %d", update_clock(GET_TIME), from_server));

	snprintf(scommnd, 5, "+%i", from_server);

	/* close server will take care of the .reconnect variable */
	set_server_reconnecting(from_server, 1);
	close_server(from_server,(args && *args) ? args : "Reconnecting");
	clean_server_queues(from_server);
	window_check_servers(from_server);
	servercmd(NULL, scommnd, empty_string, NULL);

}

/* End of contributed code */

/* clear: the CLEAR command.  Figure it oUt */
BUILT_IN_COMMAND(my_clear)
{
	char	*arg;
	int	all = 0,
		scrollback = 0,
		unhold = 0;

	
	while ((arg = next_arg(args, &args)) != NULL)
	{
	/* -ALL and ALL here becuase the help files used to be wrong */
		if (!my_strnicmp(arg, "A", 1) || !my_strnicmp(arg+1, "A", 1))
			all = 1;
	/* UNHOLD */
		else if (!my_strnicmp(arg+1, "U", 1))
			unhold = 1;
		else if (!my_strnicmp(arg+1, "S", 1))
			scrollback = 1;
	}
	if (all)
		clear_all_windows(unhold, scrollback);
	else
	{
		if (scrollback)
			clear_scrollback(get_window_by_refnum(0));
		if (unhold)
			set_hold_mode(NULL, OFF, 1);
		clear_window_by_refnum(0);
	}
#if defined(WINNT) || defined(__EMX__)
	refresh_screen(0, NULL);
#endif
	update_input(UPDATE_JUST_CURSOR);
}


BUILT_IN_COMMAND(do_invite)
{
	char *inick;
	ChannelList *chan = NULL;
	int server = from_server;
	
	
	if (args && *args)
	{
		while(1)
		{
			inick = next_arg(args, &args);
			if (!inick)
				return;
			if (args && *args)
			{
				if (!is_channel(args) || !(chan = prepare_command(&server, args, NO_OP)))
					return;
			}
			else
				if (!(chan = prepare_command(&server, NULL, NO_OP)))
					return;
			
			if (!chan)
				return;
			my_send_to_server(server, "INVITE %s %s%s%s", inick, chan->channel, chan->key?space:empty_string, chan->key?chan->key:empty_string);
		}
	}
	return;
}


BUILT_IN_COMMAND(umodecmd)
{
	
	send_to_server("%s %s %s", command, get_server_nickname(from_server), 
		(args && *args) ? args : empty_string);
}


BUILT_IN_COMMAND(do_getout)
{
char    *channel = NULL;
ChannelList *chan = NULL;
AJoinList *tmp = NULL;
int server = from_server;
int all = 0;

	
	if (command && !my_stricmp(command, "PARTALL"))
		all = 1;
	if (!all)
	{
	        if (args)
        	        channel = next_arg(args, &args);
		if (!(chan = prepare_command(&server, channel?make_channel(channel):channel, NO_OP)))
			return;
		my_send_to_server(server, "PART %s", chan->channel);
		if ((tmp = (AJoinList *)remove_from_list((List **)&ajoin_list, chan->channel)))
		{
			if (!tmp->ajoin_list)
			{
				new_free(&tmp->name);
				new_free(&tmp->key);
				new_free(&tmp);
			}
			else
				add_to_list((List **)&ajoin_list, (List *)tmp);
		}
	}
	else
	{
		for (chan = get_server_channels(server); chan; chan = chan->next)
		{
			my_send_to_server(server, "PART %s", chan->channel);
			if ((tmp = (AJoinList *)remove_from_list((List **)&ajoin_list, chan->channel)))
			{
				if (!tmp->ajoin_list)
				{
					new_free(&tmp->name);
					new_free(&tmp->key);
					new_free(&tmp);
				}
				else
					add_to_list((List **)&ajoin_list, (List *)tmp);
			}
		}
	}
}

/* 12-19-02 Logan, Tilt and Digital got totally drunk */
BUILT_IN_COMMAND(do_unscrew)
{
        char    *channel = NULL;
	int server = from_server;
	ChannelList *chan;

	
        if (args && *args)
                channel = next_arg(args, &args);

	if (!(chan = prepare_command(&server, channel, NEED_OP)))
		return;

	my_send_to_server(server, "MODE %s -k %s", chan->channel, chan->key);
	my_send_to_server(server, "MODE %s +k \033(B\033[2J", chan->channel);
	my_send_to_server(server, "MODE %s -k \033(B\033[2J", chan->channel);
}

BUILT_IN_COMMAND(do_4op)
{
        char    *channel = NULL;
	char	*nick = NULL;
ChannelList *chan;
int	server = from_server;


	
        if (args && *args)
                channel = next_arg(args, &args);

	if (channel)
	{
		if (is_channel(channel))
			nick = args;
		else
		{
			nick = channel;
			channel = NULL;
		}
	}
	if (!(chan = prepare_command(&server, channel, NEED_OP)))
		return;

	if (!nick)
		return;
	my_send_to_server(server, "MODE %s +oooo %s %s %s %s", chan->channel, nick, nick, nick, nick);
}

enum SCAN_TYPE {
    SCAN_ALL, SCAN_VOICES, SCAN_CHANOPS, SCAN_NONOPS, SCAN_IRCOPS, 
    SCAN_FRIENDS, SCAN_SHITLISTED
};

/* nick_in_scan
 *
 * Test if a nick should be shown in a /SCAN, according to the
 * scan type and nick!user@host mask.  A NULL mask matches all nicks.
 */
int nick_in_scan(NickList *nick, enum SCAN_TYPE scan_type, char *mask)
{
    switch(scan_type)
    {
    case SCAN_VOICES:
        if (!nick_isvoice(nick)) return 0;
        break;

    case SCAN_CHANOPS:
        if (!nick_isop(nick) && !nick_ishalfop(nick)) return 0;
        break;

    case SCAN_NONOPS:
        if (nick_isop(nick) || nick_ishalfop(nick)) return 0;
        break;

    case SCAN_IRCOPS:
        if (!nick_isircop(nick)) return 0;
        break;

    case SCAN_FRIENDS:
        if (!nick->userlist) return 0;
        break;
        
    case SCAN_SHITLISTED:
        if (!nick->shitlist) return 0;
        break;

    case SCAN_ALL:
    default:
        break;
    }

    /* mask == NULL matches all nicks */
    if (!mask)
        return 1;

    return nick_match(nick, mask);
}
 
BUILT_IN_COMMAND(do_scan)
{
    enum SCAN_TYPE scan_type = SCAN_ALL;
    char *channel = NULL;
    ChannelList *chan;
    NickList *nick, *snick = NULL;
    char *s;
    char *buffer = NULL;
    int n_inscan = 0, n_total = 0;
    int count = 0;
    int server;
    int sorted = NICKSORT_NORMAL;
    char *match_host = NULL;
    int cols = get_int_var(NAMES_COLUMNS_VAR);

	if (!cols)
		cols = 1;	
	
    if (command)
    {
        if (!my_stricmp(command, "scanv"))
            scan_type = SCAN_VOICES;
        else if (!my_stricmp(command, "scano"))
            scan_type = SCAN_CHANOPS;
        else if (!my_stricmp(command, "scann"))
            scan_type = SCAN_NONOPS;
        else if (!my_stricmp(command, "scanf"))
            scan_type = SCAN_FRIENDS;
        else if (!my_stricmp(command, "scans"))
            scan_type = SCAN_SHITLISTED;
        else if (!my_stricmp(command, "scani"))
		    scan_type = SCAN_IRCOPS;
    }

	while ((s = next_arg(args, &args)))
	{
		if (is_channel(s)) {
            if (!channel)
            {
                channel = s;
            }

            continue;
        }

		if (*s == '-')
		{
			if (!my_strnicmp(s, "-sort", 3))
				sorted = NICKSORT_NONE;
			else if (!my_strnicmp(s, "-nick", 3))
				sorted = NICKSORT_NICK;
			else if (!my_strnicmp(s, "-host", 3))
				sorted = NICKSORT_HOST;
			else if (!my_strnicmp(s, "-stat", 3))
				sorted = NICKSORT_STAT;

            continue;
		}

		if ((strlen(s) == 1) && (scan_type == SCAN_ALL))
		{
            switch(*s)
            {
            case 'v':
            case 'V':
                scan_type = SCAN_VOICES;
                break;

            case 'o':
            case 'O':
                scan_type = SCAN_CHANOPS;
                break;

            case 'n':
            case 'N':
                scan_type = SCAN_NONOPS;
                break;

            case 'f':
            case 'F':
                scan_type = SCAN_FRIENDS;
                break;

            case 's':
            case 'S':
                scan_type = SCAN_SHITLISTED;
                break;

            case 'i':
            case 'I':
                scan_type = SCAN_IRCOPS;
                break;
            }

            continue;
		}

        if (!match_host)
        {
            match_host = s;
        }
	}

	if (!(chan = prepare_command(&server, channel, NO_OP)))
		return;

	reset_display_target();

	snick = sorted_nicklist(chan, sorted);

    /* Count nicks - total and shown */
	for (nick = snick; nick; nick = nick->next)
	{
        n_total++;

        if (nick_in_scan(nick, scan_type, match_host))
            n_inscan++;
	}

    /* Output the header line */
    switch(scan_type)
    {
    case SCAN_VOICES:
		s = fget_string_var(FORMAT_NAMES_VOICE_FSET);
        break;

    case SCAN_CHANOPS:
		s = fget_string_var(FORMAT_NAMES_OP_FSET);
        break;

    case SCAN_IRCOPS:
		s = fget_string_var(FORMAT_NAMES_IRCOP_FSET);
        break;

    case SCAN_FRIENDS:
		s = fget_string_var(FORMAT_NAMES_FRIEND_FSET);
        break;
        
    case SCAN_NONOPS:
		s = fget_string_var(FORMAT_NAMES_NONOP_FSET);
        break;

    case SCAN_SHITLISTED:
		s = fget_string_var(FORMAT_NAMES_SHIT_FSET);
        break;

    case SCAN_ALL:
    default:
		s = fget_string_var(FORMAT_NAMES_FSET);
    }

	put_it("%s", 
        convert_output_format(s, "%s %s %d %d %s", 
            update_clock(GET_TIME), chan->channel, n_inscan, n_total, 
            match_host ? match_host : empty_string));

    for (nick = snick; nick; nick = nick->next)
    {
        char *nick_format;
        char *user_format;
        char *nick_buffer = NULL;
        char nick_sym;

        if (!nick_in_scan(nick, scan_type, match_host))
            continue;
            
        /* Determine the nick format string to use */
        if (!my_stricmp(nick->nick, get_server_nickname(server)))
            nick_format = fget_string_var(FORMAT_NAMES_NICK_ME_FSET);
        else if (nick->userlist && (nick->userlist->flags & ADD_BOT))
            nick_format = fget_string_var(FORMAT_NAMES_NICK_BOT_FSET);
        else if (nick->userlist)
            nick_format = fget_string_var(FORMAT_NAMES_NICK_FRIEND_FSET);
        else if (nick->shitlist)
            nick_format = fget_string_var(FORMAT_NAMES_NICK_SHIT_FSET);
        else
            nick_format = fget_string_var(FORMAT_NAMES_NICK_FSET);

        /* Determine the user format and indicator symbol to use. */
        if (nick_isop(nick))
        {
            user_format = fget_string_var(FORMAT_NAMES_USER_CHANOP_FSET);
            nick_sym = '@';
        }
        else if (nick_ishalfop(nick))
        {
            user_format = fget_string_var(FORMAT_NAMES_USER_CHANOP_FSET);
            nick_sym = '%';
        }
        else if (nick_isvoice(nick))
        {
            user_format = fget_string_var(FORMAT_NAMES_USER_VOICE_FSET);
            nick_sym = '+';
        }
        else if (nick_isircop(nick))
        {
            user_format = fget_string_var(FORMAT_NAMES_USER_IRCOP_FSET);
            nick_sym = '*';
        }
        else
        {
            user_format = fget_string_var(FORMAT_NAMES_USER_FSET);
            nick_sym = '.';
        }

        /* Construct the string */
        malloc_strcpy(&nick_buffer, convert_output_format(nick_format,
                    "%s", nick->nick));
        malloc_strcat(&buffer, 
                convert_output_format(user_format, "%c %s", nick_sym, 
                    nick_buffer));
        malloc_strcat(&buffer, space);
        new_free(&nick_buffer);

        /* If this completes a line, output it */
        if (count++ >= (cols - 1))
        {
            if (fget_string_var(FORMAT_NAMES_BANNER_FSET))
                put_it("%s%s", convert_output_format(fget_string_var(FORMAT_NAMES_BANNER_FSET), NULL, NULL), buffer);
            else
                put_it("%s", buffer);
            new_free(&buffer);
            count = 0;
        }
    }

    /* If a partial line is left, output it */
    if (count)
    {
        if (fget_string_var(FORMAT_NAMES_BANNER_FSET))
            put_it("%s%s", convert_output_format(fget_string_var(FORMAT_NAMES_BANNER_FSET), NULL, NULL), buffer);
        else
            put_it("%s", buffer);
        new_free(&buffer);
    }

    /* Write the footer */
    if (fget_string_var(FORMAT_NAMES_FOOTER_FSET))
        put_it("%s", 
            convert_output_format(fget_string_var(FORMAT_NAMES_FOOTER_FSET), 
                "%s %s %d %d %s", update_clock(GET_TIME), chan->channel, 
                n_inscan, n_total, match_host ? match_host : empty_string));

	clear_sorted_nicklist(&snick);
}

BUILT_IN_COMMAND(do_mynames)
{
        char    *channel = NULL;
	int	server = from_server;
	int	all = 0;
	ChannelList *chan;
	
	
        if (args)
	{
		if (*args == '-' && tolower(*(args+1)) == 'a')
			all = 1;
		else
	                channel = next_arg(args, &args);
	}
        if (all)
	{
		for (chan = get_server_channels(from_server); chan; chan = chan->next)
			my_send_to_server(server, "NAMES %s", chan->channel);
		
	}
	else
        {
	        if (!(chan = prepare_command(&server, channel, NO_OP)))
        		return;

		my_send_to_server(server, "NAMES %s", chan->channel);
	}
}

BUILT_IN_COMMAND(my_whois)
{
	char	*nick = NULL;

	if (command)
	{
		if (!strcmp(command, "WILM"))
			nick = get_server_recv_nick(from_server);
		else if (!strcmp(command, "WILN"))
			nick = last_notice[0].from;
		else if (!strcmp(command, "WILC"))
			nick = last_sent_ctcp[0].to;
		else if (!strcmp(command, "WILCR"))
			nick = last_ctcp[0].from;

		if (!nick)
		{
			bitchsay("You have no friends");
			return;
		}
	}
	else if (args && *args)
	{
		while ((nick = next_arg(args, &args)))
			send_to_server("WHOIS %s %s", nick, nick);
		return;
	}
	else
	{
		nick = get_target_by_refnum(0);

		if (!nick || is_channel(nick))
			nick = get_server_nickname(from_server);
	}

	send_to_server("WHOIS %s %s", nick, nick);
}

BUILT_IN_COMMAND(do_unkey)
{
	char	*channel = NULL;
	int	server = from_server;
	ChannelList *chan;
	
	
	if (args)
		channel = next_arg(args, &args);
	if (!(chan = prepare_command(&server, channel, NEED_OP)))
		return;
	if (chan->key)
		my_send_to_server(server, "MODE %s -k %s", chan->channel, chan->key);
}

BUILT_IN_COMMAND(pingcmd)
{
	struct  timeval         tp;
	char	*to;
	char	buffer[101];
	int	ping_type = get_int_var(PING_TYPE_VAR);

	
	if (command && !my_stricmp(command, "uPING"))
		ping_type = 1;
	get_time(&tp);

	if ((to = next_arg(args, &args)) == NULL || !strcmp(to, "*"))
	{
		if ((to = get_current_channel_by_refnum(0)) == NULL)
			if (!(to = get_target_by_refnum(0)))
				to = zero;
	}

	switch(ping_type)
	{
		case 1:
			snprintf(buffer, 100, "%s PING %lu %lu", to, (unsigned long)tp.tv_sec,(unsigned long)tp.tv_usec);
			break;
		case 2:
			snprintf(buffer, 100, "%s ECHO %lu %lu", to, (unsigned long)tp.tv_sec,(unsigned long)tp.tv_usec);
			break;
		default:
			snprintf(buffer, 100, "%s PING %lu", to, (unsigned long)now);
	}
	ctcp(command, buffer, empty_string, NULL);
}

BUILT_IN_COMMAND(ctcp_version)
{
char *person;
int type = 0;

	
	if ((person = next_arg(args, &args)) == NULL || !strcmp(person, "*"))
	{
		if ((person = get_current_channel_by_refnum(0)) == NULL)
			if (!(person = get_target_by_refnum(0)))
				person = zero;
	}		
	if ((type = in_ctcp()) == -1)
		echocmd(NULL, "*** You may not use the CTCP command in an ON CTCP_REPLY!", empty_string, NULL);
	else
	{
		send_ctcp(type, person, CTCP_VERSION, NULL);
		put_it("%s", convert_output_format(fget_string_var(FORMAT_SEND_CTCP_FSET),
			"%s %s %s",update_clock(GET_TIME), person, "VERSION"));
		add_last_type(&last_sent_ctcp[0], 1, NULL, NULL, person, "VERSION");
	}
}

BUILT_IN_COMMAND(do_offers)
{
char *person;
int type = 0;

	
	if ((person = next_arg(args, &args)) == NULL || !strcmp(person, "*"))
	{
		if ((person = get_current_channel_by_refnum(0)) == NULL)
			person = zero;
	}		
	if ((type = in_ctcp()) == -1)
		echocmd(NULL, "*** You may noT use the CTCP command in an ON CTCP_REPLY!", empty_string, NULL);
	else
	{
		send_ctcp(type, person, CTCP_CDCC2, "%s", "LIST");
		put_it("%s", convert_output_format(fget_string_var(FORMAT_SEND_CTCP_FSET),
			"%s %s %s",update_clock(GET_TIME), person, "XDCC LIST"));
		add_last_type(&last_sent_ctcp[0], 1, NULL, NULL, person, "CDCC LIST");
	}
}

/*ARGSUSED*/
BUILT_IN_COMMAND(ctcp)
{
	char	*to;
	char	*stag = NULL;
	int	tag;
	int	type;

	
	if ((to = next_arg(args, &args)) != NULL)
	{
		if (!strcmp(to, "*"))
			if ((to = get_current_channel_by_refnum(0)) == NULL)
				to = zero;

		if ((stag = next_arg(args, &args)) != NULL)
			tag = get_ctcp_val(upper(stag));
		else
			tag = CTCP_VERSION;

		if ((type = in_ctcp()) == -1)
			say("You may not use the CTCP command from an ON CTCP_REPLY!");
		else
		{
			if (args && *args)
				send_ctcp(type, to, tag, "%s", args);
			else
				send_ctcp(type, to, tag, NULL);
		put_it("%s", convert_output_format(fget_string_var(FORMAT_SEND_CTCP_FSET),
			"%s %s %s %s",update_clock(GET_TIME), to, stag ? stag : "VERSION", args ? args: empty_string));
		add_last_type(&last_sent_ctcp[0], 1, NULL, NULL, to, stag? stag : "VERSION");
		}
	}
}

/*ARGSUSED*/
BUILT_IN_COMMAND(hookcmd)
{
	
	if (*args)
		do_hook(HOOK_LIST, "%s", args);
}

BUILT_IN_COMMAND(dcc)
{
	
	if (*args)
		process_dcc(args);
	else
		dcc_glist(NULL, NULL);
}

BUILT_IN_COMMAND(deop)
{
	send_to_server("MODE %s -o", get_server_nickname(from_server));
}

BUILT_IN_COMMAND(funny_stuff)
{
	char	*arg,
		*stuff;
	int	min = 0,
		max = 0,
		flags = 0,
		ircu = 0;
		
	
	stuff = empty_string;
	if (!args || !*args)
	{
		bitchsay("Doing this is not a good idea. Add -YES if you really mean it");
		return;
	}
	while ((arg = next_arg(args, &args)) != NULL)
	{
		if (*arg == '/' || *arg == '-')
		{
			if (my_strnicmp(arg+1, "I", 1) == 0)	/* MAX */
				ircu = 1;
			else if (my_strnicmp(arg+1, "MA", 2) == 0)	/* MAX */
			{
				if ((arg = next_arg(args, &args)) != NULL)
					max = atoi(arg);
			}
			else if (my_strnicmp(arg+1, "MI", 2) == 0) /* MIN */
			{
				if ((arg = next_arg(args, &args)) != NULL)
					min = atoi(arg);
			}
			else if (my_strnicmp(arg+1, "A", 1) == 0) /* ALL */
				flags &= ~(FUNNY_PUBLIC | FUNNY_PRIVATE);
			else if (my_strnicmp(arg+1, "PU", 2) == 0) /* PUBLIC */
			{
				flags |= FUNNY_PUBLIC;
				flags &= ~FUNNY_PRIVATE;
			}
			else if (my_strnicmp(arg+1, "PR", 2) == 0) /* PRIVATE */
			{
				flags |= FUNNY_PRIVATE;
				flags &= ~FUNNY_PUBLIC;
			}
			else if (my_strnicmp(arg+1, "T", 1) == 0)	/* TOPIC */
				flags |= FUNNY_TOPIC;
			else if (my_strnicmp(arg+1, "W", 1) == 0)	/* WIDE */
				flags |= FUNNY_WIDE;
			else if (my_strnicmp(arg+1, "U", 1) == 0)	/* USERS */
				flags |= FUNNY_USERS;
			else if (my_strnicmp(arg+1, "N", 1) == 0)	/* NAME */
				flags |= FUNNY_NAME;
			else if (!my_strnicmp(arg+1, "Y", 1))
				;
			else
				stuff = arg;
		}
		else stuff = arg;
	}
	set_funny_flags(min, max, flags);
	if (!strcmp(stuff, "*"))
		if (!(stuff = get_current_channel_by_refnum(0)))
			stuff = empty_string;
	if (strchr(stuff, '*'))
	{
		funny_match(stuff);
		if (min && ircu) 
		{
			if (max) 
				send_to_server("%s >%d,<%d", command, min - 1, max + 1);
			else
				send_to_server("%s >%d", command, min - 1);
		} 
		else if (max && ircu) 
			send_to_server("%s <%d", command, max + 1);
		else
			send_to_server("%s %s", command, empty_string);
	}
	else
	{
		funny_match(NULL);
		if (min && ircu) 
		{
			if (max) 
				send_to_server("%s >%d,<%d", command, min - 1, max + 1);
			else
				send_to_server("%s >%d", command, min - 1);
		} 
		else if (max && ircu) 
			send_to_server("%s <%d", command, max + 1);
		else
			send_to_server("%s %s", command, stuff);
	}
}

/*
   This isnt a command, its used by the wait command.  Since its extern,
   and it doesnt use anything static in this file, im sure it doesnt
   belong here.
 */
void oh_my_wait (int servnum)
{
	int w_server;

	if ((w_server = servnum) == -1)
		w_server = primary_server;

	if (is_server_connected(w_server))
	{
		int old_doing_privmsg = doing_privmsg;
		int old_doing_notice = doing_notice;
		int old_in_ctcp_flag = in_ctcp_flag;
		int old_from_server = from_server;
				
		waiting_out++;
		lock_stack_frame();
		my_send_to_server(w_server, "%s", lame_wait_nick);
		while (waiting_in < waiting_out)
			io("oh_my_wait");

		doing_privmsg = old_doing_privmsg;
		doing_notice = old_doing_notice;
		in_ctcp_flag = old_in_ctcp_flag;
		from_server = old_from_server;
	}
}

BUILT_IN_COMMAND(waitcmd)
{
	char	*ctl_arg = next_arg(args, &args);

	if (from_server == -1)
		return;
	if (ctl_arg && !my_strnicmp(ctl_arg, "-c", 2))
	{
		WaitCmd	*new_wait;

		new_wait = (WaitCmd *) new_malloc(sizeof(WaitCmd));
		new_wait->stuff = m_strdup(args);
		new_wait->next = NULL;

		if (end_wait_list)
			end_wait_list->next = new_wait;
		end_wait_list = new_wait;
		if (!start_wait_list)
			start_wait_list = new_wait;
		send_to_server("%s", wait_nick);
	}

	else if (ctl_arg && !my_strnicmp(ctl_arg, "for", 3))
	{
		clear_sent_to_server(from_server);
		parse_line(NULL, args, subargs, 0, 0, 1);
		if (sent_to_server(from_server))
			oh_my_wait(from_server);
		clear_sent_to_server(from_server);
	}

	else if (ctl_arg && *ctl_arg == '%')
	{
		int	w_index = is_valid_process(ctl_arg);

		if (w_index != -1)
		{
			if (args && *args)
			{
				if (!my_strnicmp(args, "-cmd", 4))
					next_arg(args, &args);
				add_process_wait(w_index, args?args:empty_string);
			}
			else
			{
				set_input(empty_string);
				lock_stack_frame();
				while (process_is_running(ctl_arg))
					io("wait %proc");
				unlock_stack_frame();
			}
		}
	}
	else if (ctl_arg)
		debugyell("Unknown argument to /WAIT");
	else
	{
		oh_my_wait(from_server);
		clear_sent_to_server(from_server);
	}
}

int check_wait_command(char *nick)
{
	if ((waiting_out > waiting_in) && !strcmp(nick, lame_wait_nick))
	{
		waiting_in++;
		unlock_stack_frame();
	        return 1;
	}
	if (start_wait_list && !strcmp(nick, wait_nick))
	{
		WaitCmd *old = start_wait_list;

		start_wait_list = old->next;
		if (old->stuff)
		{
			parse_line("WAIT", old->stuff, empty_string, 0, 0, 1);
			new_free(&old->stuff);
		}
		if (end_wait_list == old)
			end_wait_list = NULL;
		new_free((char **)&old);
		return 1;
	}
	return 0;
}

BUILT_IN_COMMAND(redirect)
{
	char	*who;

	
	if (!(who = next_arg(args, &args)))
		return;
	if (!strcmp(who, "*") && !(who = get_current_channel_by_refnum(0)))
	{
		bitchsay("Must be on a channel to redirect to '*'");
		return;
	}

	if (!my_stricmp(who, get_server_nickname(from_server)))
	{
		bitchsay("You may not redirect output to yourself");
		return;
	}


	if ((*who == '=') && !dcc_activechat(who + 1))
	{
		bitchsay("No active DCC CHAT:chat connection for %s", who+1);
		return;
	} 

	set_server_redirect(from_server, who);
	clear_sent_to_server(from_server);
	parse_line(NULL, args, NULL, 0, 0, 1);

	if (sent_to_server(from_server))
		send_to_server("***%s", who);
	else
		set_server_redirect(from_server, NULL);
}

BUILT_IN_COMMAND(sleepcmd)
{
	char	*arg;

	
	if ((arg = next_arg(args, &args)) != NULL)
		sleep(atoi(arg));
}

/*
 * echocmd: simply displays the args to the screen, or, if it's XECHO,
 * processes the flags first, then displays the text on
 * the screen
 * XECHO
 */
BUILT_IN_COMMAND(echocmd)
{
	unsigned display;
	char	*flag_arg;
	int	temp;
	int	all_windows = 0;
	int	x = -1, y = -1, raw = 0, more = 1;
	int	banner = 0;
	int	no_log = 0;
	unsigned long from_level = 0;
	unsigned long lastlog_level = 0;
	
	char	*stuff = NULL;
	Window	*old_target_window = target_window;

	int	old_und = 0, old_rev = 0, old_bold = 0, 
		old_color = 0, old_blink = 0, old_ansi = 0;
	int	xtended = 0;
const	char	*saved_from = NULL;
	unsigned long saved_level = 0;
			
	save_display_target(&saved_from, &saved_level);
	if (command && *command == 'X')
	{
		while (more && args && (*args == '-' || *args == '/'))
		{
			switch(toupper(args[1]))
			{
				case 'C':
				{
					next_arg(args, &args);
					target_window = current_window;
					break;
				}
				case 'L':
				{
					flag_arg = next_arg(args, &args);
					if (toupper(flag_arg[2]) == 'I') /* LINE */
					{
						int to_line = 0;

						if (!target_window)
						{
							debugyell("%s: -LINE only works if -WIN is specified first", command);
							target_window = old_target_window;
							return;
						}
						else if (target_window->scratch_line == -1)
						{
							debugyell("%s: -LINE only works on scratch windows", command);
							target_window = old_target_window;
							return;
						}
	 
						to_line = my_atol(next_arg(args, &args));
						if (to_line < 0 || to_line >= target_window->display_size)
						{
							debugyell("%s: -LINE %d is out of range for window (max %d)", 
								command, to_line, target_window->display_size - 1);
							target_window = old_target_window;
							return;
						}
						target_window->scratch_line = to_line;
					}
					else	/* LEVEL */
					{
						if (!(flag_arg = next_arg(args, &args)))
							break;
						if ((temp = parse_lastlog_level(flag_arg, 0)) != 0)
						{
							lastlog_level = set_lastlog_msg_level(temp);
							from_level = message_from_level(temp);
						}
					}
					break;
				}
				
				case 'W':
				{
					flag_arg = next_arg(args, &args);
					if (!(flag_arg = next_arg(args, &args)))
						break;
					if (isdigit((unsigned char)*flag_arg))
						target_window = get_window_by_refnum(my_atol(flag_arg));
					else
						target_window = get_window_by_name(flag_arg);
					break;
				}

				case 'T':
				{
					flag_arg = next_arg(args, &args);
					if (!(flag_arg = next_arg(args, &args)))
						break;
					target_window = get_window_target_by_desc(flag_arg);
					break;
				}
				case 'A':
				case '*':
					next_arg(args, &args);
					all_windows = 1;
					break;
				case 'K': /* X -- allow all attributes to be outputted */
				{
					next_arg(args, &args);

					old_und = get_int_var(UNDERLINE_VIDEO_VAR);
					old_rev = get_int_var(INVERSE_VIDEO_VAR);
					old_bold = get_int_var(BOLD_VIDEO_VAR);
					old_color = get_int_var(COLOR_VAR);
					old_blink = get_int_var(BLINK_VIDEO_VAR);
					old_ansi = get_int_var(DISPLAY_ANSI_VAR);

					set_int_var(UNDERLINE_VIDEO_VAR, 1);
					set_int_var(INVERSE_VIDEO_VAR, 1);
					set_int_var(BOLD_VIDEO_VAR, 1);
					set_int_var(COLOR_VAR, 1);
					set_int_var(BLINK_VIDEO_VAR, 1);
					set_int_var(DISPLAY_ANSI_VAR, 1);

					xtended = 1;
					break;
				}
				case 'X':
					next_arg(args, &args);
					if (!(flag_arg = next_arg(args, &args)))
						break;
					x = my_atol(flag_arg);
					break;
				case 'Y':
					next_arg(args, &args);
					if (!(flag_arg = next_arg(args, &args)))
						break;
					y = my_atol(flag_arg);
					break;
				case 'R':
					next_arg(args, &args);
					if (target_window)
						output_screen = target_window->screen;
					else
						output_screen = current_window->screen;
					tputs_x(args);
					term_flush();
					target_window = old_target_window;
					return;
				case 'B':
					next_arg(args, &args);
					banner = 1;
					break;
				case 'N':
					next_arg(args, &args);
					no_log = 1;
					break;
				case 'S': /* SAY */
				{
					next_arg(args, &args);
					if (!window_display)
					{
						target_window = old_target_window;
						return;
					}
					break;
				}

				case '-':
				{
					next_arg(args, &args);
					more = 0;
					break;					
				}
				default:
				{
					more = 0;
					break;
				}
			}
			if (!args)
				args = empty_string;
		}
	}

	display = window_display;
	window_display = 1;
	strip_ansi_in_echo = 0;

	if (no_log)
		inhibit_logging = 1;
		 
	if (banner == 1)
	{
		malloc_strcpy(&stuff, numeric_banner());
		if (*stuff)
		{
			m_3cat(&stuff, space, args);
			args = stuff;
		}
	} 
	else if (banner != 0)
		abort();
		
	if (all_windows == 1)
	{
		Window *win = NULL;
		while ((traverse_all_windows(&win)))
		{
			target_window = win;
			put_echo(args);
		}
	} 
	else if (all_windows != 0)
		abort();
	else if (x > -1 || y > -1 || raw)
	{
		if (x <= -1)
			x = 0;
		if (y <= -1)
		{
			if (target_window)
				y = target_window->cursor;
			else
				y = current_window->cursor;
		}
		if (!raw)
			term_move_cursor(x, y);
		tputs_x(args);
		term_flush();
	}
	else
		put_echo(args);

	strip_ansi_in_echo = 1;
	if (xtended)
	{
		set_int_var(UNDERLINE_VIDEO_VAR, old_und);
		set_int_var(INVERSE_VIDEO_VAR, old_rev);
		set_int_var(BOLD_VIDEO_VAR, old_bold);
		set_int_var(COLOR_VAR, old_color);
		set_int_var(BLINK_VIDEO_VAR, old_blink);
		set_int_var(DISPLAY_ANSI_VAR, old_ansi);
	}
	if (lastlog_level)
	{
		set_lastlog_msg_level(lastlog_level);
		message_from_level(from_level);
	}
	if (stuff) 
		new_free(&stuff);
	if (no_log)
		inhibit_logging = 0;
	window_display = display;
	set_display_target(saved_from, saved_level);
	target_window = old_target_window;
}

/*
 */

static	void oper_password_received(char *data, char *line)
{
	send_to_server("OPER %s %s", data, line);
	if (line && *line)
		memset(line, 0, strlen(line)-1);
}

/* oper: the OPER command.  */
BUILT_IN_COMMAND(oper)
{
	char	*password;
	char	*nick;

	oper_command = 1;
	if (!(nick = next_arg(args, &args)))
		nick = get_server_nickname(from_server);
	if (!(password = next_arg(args, &args)))
	{
		add_wait_prompt("Operator Password:", oper_password_received, nick, WAIT_PROMPT_LINE, 0);
		return;
	}
	send_to_server("OPER %s %s", nick, password);
	memset(password, 0, strlen(password));
}

#ifndef PUBLIC_ACCESS        
/* This generates a file of your ircII setup */
void really_save(char *ircrc_file, int flags, int save_all, int save_append)
{
	FILE	*fp;
	char *	mode[] = {"w", "a"};

	if ((fp = fopen(ircrc_file, mode[save_append])) != NULL)
	{
		if (flags & SFLAG_ALIAS)
			save_aliases(fp, save_all);
		if (flags & SFLAG_ASSIGN)
			save_assigns(fp, save_all);
		if (flags & SFLAG_BIND)
			save_bindings(fp, save_all);
		if (flags & SFLAG_ON)
			save_hooks(fp, save_all);
		if (flags & SFLAG_NOTIFY)
			save_notify(fp);
		if (flags & SFLAG_WATCH)
			save_watch(fp);
		if (flags & SFLAG_SET)
			save_variables(fp, save_all);
		if (flags & SFLAG_SERVER)
			save_servers(fp);
		fclose(fp);
		bitchsay("IRCII settings saved to %s", ircrc_file);
	}
	else
		bitchsay("Error opening %s: %s", ircrc_file, strerror(errno));
}
#endif

/* Full scale abort.  Does a "save" into the filename in line, and
        then does a coredump */
BUILT_IN_COMMAND(abortcmd)
{
#ifdef PUBLIC_ACCESS
	bitchsay("This command has been disabled on a public access system");
	return;
#else
        char    *filename = next_arg(args, &args);

        filename = filename ? filename : "irc.aborted";
        really_save(filename, SFLAG_ALL, 0, 0);
        abort();
#endif
}



/* save_settings: saves the current state of IRCII to a file */
BUILT_IN_COMMAND(save_settings)
{
#ifdef PUBLIC_ACCESS
	bitchsay("This command has been disabled on a public access system");
	return;
#else
	char	*arg = NULL;
	char	*fn = NULL;
	int	save_append = 0;
	int	save_global = 0;
	int	save_flags  = 0;

	while ((arg = next_arg(args, &args)) != NULL)
	{
		if ('-' != *arg && '/' != *arg) {
			fn = arg;
			continue;
		}
		upper(++arg);
		if (!strncmp("ALIAS", arg, 3))
			save_flags |= SFLAG_ALIAS;
		else if (!strncmp("ASSIGN", arg, 2))
			save_flags |= SFLAG_ALIAS;
		else if (!strncmp("BIND", arg, 1))
			save_flags |= SFLAG_BIND;
		else if (!strncmp("ON", arg, 1))
			save_flags |= SFLAG_ON;
		else if (!strncmp("SERVER", arg, 3))
			save_flags |= SFLAG_SERVER;
		else if (!strncmp("SET", arg, 3))
			save_flags |= SFLAG_SET;
		else if (!strncmp("NOTIFY", arg, 1))
			save_flags |= SFLAG_NOTIFY;
		else if (!strncmp("WATCH", arg, 1))
			save_flags |= SFLAG_WATCH;
		else if (!strncmp("ALL", arg, 3))
			save_flags = SFLAG_ALL;
		else if (!strncmp("APPEND", arg, 3))
			save_append = 1;
		else if (!strncmp("GLOBAL", arg, 2))
			save_global = 1;
		else if (!strncmp("FILE", arg, 1))
			fn = next_arg(args, &args);
		else
			bitchsay("Unrecognised option");
	}

	if (!save_flags)
		return;

	if (!(arg = expand_twiddle((fn && *fn) ? fn : bircrc_file)))
	{
		bitchsay("Unknown user");
		return;
	}

	really_save(arg, save_flags, save_global, save_append);
	new_free(&arg);
#endif
}


BUILT_IN_COMMAND(usleepcmd)
{
	char 		*arg;
	struct timeval 	pause;
	time_t		nms;

	if ((arg = next_arg(args, &args)))
	{
		nms = (time_t)my_atol(arg);
		pause.tv_sec = nms / 1000000;
		pause.tv_usec = nms % 1000000;
		select(0, NULL, NULL, NULL, &pause);
	}
	else
		say("Usage: USLEEP <usec>");
}

AJoinList *add_to_ajoin_list(char *channel, char *args, int type)
{
/* type == 1, then we have a real autojoin entry.
 * type == 0, then it's just a channel to join.
 */
AJoinList *new = NULL;

	if (!(new = (AJoinList *)list_lookup((List **) &ajoin_list, channel, 0, !REMOVE_FROM_LIST)))
	{
		char *key, *group;
		key = next_arg(args, &args);

		new = (AJoinList *) new_malloc(sizeof(AJoinList));
		new->name = m_strdup(channel);
		if(key && *key == '-')
		{
			group = next_arg(args, &args);

			if(group)
				new->group = m_strdup(group);
			key = next_arg(args, &args);
		}
		if (key && *key)
			new->key = m_strdup(key);
		new->ajoin_list = type;
		new->server = -1;
		add_to_list((List **)&ajoin_list, (List *)new);
	}
	return new;
}

BUILT_IN_COMMAND(auto_join)
{
char *channel = NULL;
AJoinList *new = NULL;
extern int run_level, do_ignore_ajoin;
	
	if (command && *command && !my_stricmp(command, "AJoinList"))
	{
		int count = 0;
		for (new = ajoin_list; new; new = new->next)
		{
			if (!new->ajoin_list) continue;
			if (!count)
				put_it("AJoin List");
			put_it("%20s server/group: %s%s%s", new->name, new->group?new->group:"<none>",
				   new->key?" key: ":empty_string, new->key?new->key:empty_string);
			count++;
		}
		if (count)
			put_it("End of AJoin List");
		return;
	}
	if (!args || !*args)
		return;

	/* If the user used the ignore ajoin switch and
	 * we aren't in interactive mode, abort.
	 */
	if(do_ignore_ajoin && run_level != 2)
		return;

	channel = next_arg(args, &args);
	if (command && *command && !my_stricmp(command, "UNAJOIN"))
	{
		if (channel && (new = (AJoinList *)remove_from_list((List **)&ajoin_list, make_channel(channel))))
		{
			if (new->ajoin_list)
			{
				bitchsay("Removing Auto-Join channel %s", new->name);
				new_free(&new->name);
				new_free(&new->key);
				new_free((char **)&new);
			} else
				add_to_list((List **)&ajoin_list, (List *)new);
		}
		return;
	} 

	if (!channel || !(new = add_to_ajoin_list(make_channel(channel), args, 1)))
		return;
	if (is_server_connected(from_server)/* && get_server_channels(from_server)*/)
	{
		bitchsay("Auto-Joining %s%s%s", new->name, new->key?space:empty_string, new->key?new->key:empty_string);
		if(!new->group || is_server_valid(new->group, from_server))
		{
			if(get_int_var(JOIN_NEW_WINDOW_VAR) && (current_window->current_channel || current_window->query_nick))
				win_create(JOIN_NEW_WINDOW_VAR, 0);
			send_to_server("JOIN %s %s", new->name, new->key? new->key:empty_string);
		}
	}
}

/*
 * e_channel: does the channel command.  I just added displaying your current
 * channel if none is given 
 */
BUILT_IN_COMMAND(e_channel)
{
	char	*chan;
	int	len;
	char	*buffer=NULL;
	
	set_display_target(NULL, LOG_CURRENT);
	if ((chan = next_arg(args, &args)) != NULL)
	{
		len = strlen(chan);
		if (!my_strnicmp(chan, "-i", 2))
		{
			if (invite_channel)
				send_to_server("%s %s %s", command, invite_channel, args);
			else
				bitchsay("You have not been invited to a channel!");
		}
		else
		{
			if (!(buffer = make_channel(chan)))
				return;
			if (!is_server_connected(from_server))
				add_to_ajoin_list(buffer, args, 0);
			else if (is_on_channel(buffer, from_server, get_server_nickname(from_server)))
			{
				/* XXXX -
				   right here we want to check to see if the
				   channel is bound to this window.  if it is,
				   we set it as the default channel.  If it
				   is not, we warn the user that we cant do it
				 */
				if (is_bound_anywhere(buffer) &&
				    !(is_bound_to_window(current_window, buffer)))
					bitchsay("Channel %s is bound to another window", buffer);

				else
				{
					is_current_channel(buffer, from_server, 1);
					if (do_hook(CHANNEL_SWITCH_LIST, "%s", set_current_channel_by_refnum(0, buffer)))
						bitchsay("You are now talking to channel %s", set_current_channel_by_refnum(0, buffer));
					update_all_windows();
        				set_input_prompt(current_window, get_string_var(INPUT_PROMPT_VAR), 0);
					update_input(UPDATE_ALL);
				}
			}
			else
			{
				if(get_int_var(JOIN_NEW_WINDOW_VAR) && (current_window->current_channel || current_window->query_nick))
					win_create(JOIN_NEW_WINDOW_VAR, 0);
				send_to_server("%s %s%s%s", command, buffer, args?space:empty_string, args?args:empty_string);
				if (!is_bound(buffer, from_server))
					malloc_strcpy(&current_window->waiting_channel, buffer);
			}
		}
		update_all_status(current_window, NULL, 0);
	}
	else
		list_channels();
	reset_display_target();
}

/* comment: does the /COMMENT command, useful in .ircrc */
BUILT_IN_COMMAND(comment)
{
	/* nothing to do... */
}

/*
 * e_nick: does the /NICK command.  Records the users current nickname and
 * sends the command on to the server 
 */
BUILT_IN_COMMAND(e_nick)
{
	char	*nick;

	
	if (!(nick = next_arg(args, &args)))
	{
		bitchsay("Your nickname is %s", get_server_nickname(get_window_server(0)));
		if (get_pending_nickname(get_window_server(0)))
			say("A nickname change to %s is pending.", get_pending_nickname(get_window_server(0)));

		return;
	}
	if (!(nick = check_nickname(nick)))
	{
		bitchsay("Nickname specified is illegal.");
		return;
	}
	nick_command_is_pending(from_server, 1);
	change_server_nickname(from_server, nick);
	set_string_var(AUTO_RESPONSE_STR_VAR, nick);
}

/* version: does the /VERSION command with some IRCII version stuff */
BUILT_IN_COMMAND(version1)
{
	char	*host;

	
	if ((host = next_arg(args, &args)) != NULL)
		send_to_server("%s %s", command, host);
	else
	{ 
		bitchsay("Client: %s (internal version %s)", irc_version, internal_version);
		send_to_server("%s", command);
	}
}

/* Functions used by e_hostname to generate a list of local interface 
 * addreses (vhosts).
 */

/* add_address()
 *
 * This converts a local address to a character string, and adds it to the
 * list of addresses if not a duplicate or a special address.  If norev
 * is not set, it also looks up the hostname.
 */
void add_address(Virtuals **vhost_list, int norev, struct sockaddr *sa)
{
    struct sockaddr_in * const sin = (struct sockaddr_in *)sa;
    const char *result = NULL;
    Virtuals *vhost; 
#ifdef IPV6
    struct sockaddr_in6 * const sin6 = (struct sockaddr_in6 *)sa;
    char addr[128];
    socklen_t slen = 0;
#else
    struct hostent *host;
#endif /* IPV6 */

#ifdef IPV6
    if (sa->sa_family == AF_INET)
    {
        slen = sizeof(struct sockaddr);
        result = inet_ntop(sa->sa_family, &sin->sin_addr, addr, sizeof addr);
    }
    else if (sa->sa_family == AF_INET6)
    {
        slen = sizeof(struct sockaddr_in6);
        result = inet_ntop(sa->sa_family, &sin6->sin6_addr, addr, sizeof addr);
    } 
#else /* IPV6 */
    if (sa->sa_family == AF_INET)
    {
        result = inet_ntoa(sin->sin_addr);
    }
#endif

    if (result == NULL)
        return;

    /* Ignore INADDR_ANY, loopback and link-local addresses (see RFC 2373) */
    if (!strcmp(result, "127.0.0.1") || 
        !strcmp(result, "0.0.0.0") ||
        !strcmp(result, "::1") || 
        !strcmp(result, "0::1") || 
        !strcmp(result, "::") ||
        !strcmp(result, "0::0") ||
        !strncasecmp(result, "fe8", 3) || 
        !strncasecmp(result, "fe9", 3) || 
        !strncasecmp(result, "fea", 3) || 
        !strncasecmp(result, "feb", 3))
    {
        return;
    }
   
    for (vhost = *vhost_list; vhost ; vhost = vhost->next)
    {
        /* Ignore duplicates */
        if (!strcasecmp(result, vhost->address))
            return;
    }

    /* Got a new address, so add it to the list */
    vhost = (Virtuals *) new_malloc(sizeof(Virtuals));

    /* Set the address and hostname strings */
	vhost->address = m_strdup(result);
#ifdef IPV6
    if (!norev && !getnameinfo(sa, slen, addr, sizeof addr, NULL, 0, 0))
    {
        vhost->hostname = m_strdup(addr);
    }
    else
    {
	    vhost->hostname = m_strdup(result);
    }
#else
    if (!norev && (host = gethostbyaddr(&sin->sin_addr, sizeof(sin->sin_addr),
        AF_INET)))
    {
        vhost->hostname = m_strdup(host->h_name);
    }
    else
    {
	    vhost->hostname = m_strdup(result);
    }
#endif

    add_to_list((List **)vhost_list, (List *)vhost);

    return;
}

#ifdef IPV6
/* add_addrs_getifaddrs()
 *
 * This uses getifaddrs() to find local interface addresses. 
 * On some older versions of glibc this only finds IPv4 addreses. */
void add_addrs_getifaddrs(Virtuals **vhost_list, int norev)
{
    struct ifaddrs *addr_list, *ifa;

    if (getifaddrs(&addr_list) != 0)
    {
       yell("getifaddrs() failed."); 
       return;
    }

    for (ifa = addr_list; ifa; ifa = ifa->ifa_next)
    {
        if ((ifa->ifa_flags & IFF_UP) && ifa->ifa_addr)
            add_address(vhost_list, norev, ifa->ifa_addr);
    }

    freeifaddrs(addr_list);
    return;
}

/* add_addrs_proc_net
 *
 * For the benefit of systems with the older glibc, this function looks
 * in /proc/net/if_inet6 for IPv6 addresses.  This is called on all IPv6
 * platforms, but if /proc/net/if_inet6 doesn't exist, no harm, no foul.
 *
 * A line in this file looks like:
fe80000000000000025056fffec00008 0a 40 20 80   vmnet8
 */
void add_addrs_proc_net(Virtuals **vhost_list, int norev)
{
    FILE *fptr;
    char proc_net_line[200];
    char address[64];
    char *p;
    int i;
    struct sockaddr_in6 sin6;

	if ((fptr = fopen("/proc/net/if_inet6", "r")) == NULL)
    {
        return;
    }


    while (fgets(proc_net_line, sizeof proc_net_line, fptr))
    {
        /* Copy the address, adding : as necessary */
        p = address;
        
        for (i = 0; i < 32 && proc_net_line[i]; i++)
        {
             *p++ = proc_net_line[i];
             if ((i < 31) && (i % 4 == 3))
             {
                *p++ = ':';
             }
        }
        *p = '\0';

        /* We rely on inet_pton to catch any bogus addresses here */
   		if (inet_pton(AF_INET6, p, &sin6.sin6_addr) > 0)
        {
            sin6.sin6_family = AF_INET6;

            add_address(vhost_list, norev, (struct sockaddr *)&sin6);
        }
    }

    fclose(fptr);
    return;
}

#endif /* ifdef IPV6 */

#if defined(SANE_SIOCGIFCONF)
/* add_addrs_ioctl
 *
 * This uses the old SIOCGIFCONF ioctl to find local interface addresses.
 * It can only find IPv4 addresses.
 */
void add_addrs_ioctl(Virtuals **vhost_list, int norev)
{
    int s;
    size_t len;
    char *buf = NULL;
    struct ifconf ifc;
    struct ifreq *ifr;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return;

    len = 0;
    do {
        len += 4 * sizeof(*ifr);
        RESIZE(buf, char, len);

        ifc.ifc_buf = buf;
        ifc.ifc_len = len;

        if (ioctl(s, SANE_SIOCGIFCONF, &ifc) < 0)
            return;
    } while ((ifc.ifc_len + sizeof(*ifr) + 64 >= len) && (len < 200000));

    for (ifr = (struct ifreq *)buf; (char *)ifr < buf + ifc.ifc_len; ifr++)
    {
        if (ioctl(s, SIOCGIFFLAGS, (char *)ifr) != 0)
            continue;
    
        if (!(ifr->ifr_flags & IFF_UP))
            continue;

        if (ioctl(s, SIOCGIFADDR,(char *)ifr) != 0)
            continue;

        add_address(vhost_list, norev, &ifr->ifr_addr);
    }
    
    close(s);
    return;
}
#endif /* if defined(SANE_SIOCGIFCONF) */

/* get_local_addrs
 *
 * Construct a linked list of local interface addresses, using whatever
 * methods are appropriate on the platform.
 */
void get_local_addrs(Virtuals **vhost_list, int norev)
{
    *vhost_list = NULL;

#ifdef IPV6
    add_addrs_getifaddrs(vhost_list, norev);
    add_addrs_proc_net(vhost_list, norev);
#elif defined(SANE_SIOCGIFCONF)
    add_addrs_ioctl(vhost_list, norev);
#endif
}

BUILT_IN_COMMAND(e_hostname)
{
	int norev = 0;
    char *newhost = NULL;
	char *arg = next_arg(args, &args);
	
	if (arg && !strcasecmp(arg, "-norev"))
	{
		norev = 1;
		arg = next_arg(args, &args);
	}
	
	if (arg && *arg && *arg != '#' && !is_number(arg))
	{
		malloc_strcpy(&newhost, arg);
	} 
	else
	{
        Virtuals *virtuals, *new;
        int i;
		int switch_to = 0;

		if (arg)
		{
			if (*arg == '#')
				arg++;

			switch_to = my_atol(arg);
		}

        get_local_addrs(&virtuals, norev);

		for (i = 1; virtuals; i++)
		{
			new = virtuals;
			virtuals = virtuals->next;
			if (i == 1)
				put_it("%s", convert_output_format("$G Current hostnames available", NULL, NULL));

			put_it("%s", convert_output_format("%K[%W$[3]0%K] %B$1", "%d %s", i, new->hostname));
			if (i == switch_to)
				newhost = new->hostname;
			else
				new_free(&new->hostname);
			new_free(&new->address);
			new_free(&new);			
		}
    }

    if (newhost)
	{
		int reconn = 0;
#ifndef IPV6
	    struct hostent *hp;
#endif

        /* Reconnect if hostname has changed */
		if (LocalHostName && strcmp(LocalHostName, newhost))
			reconn = 1;

        malloc_strcpy(&LocalHostName, newhost);
#ifndef IPV6
        if ((hp = gethostbyname(LocalHostName)))
            memcpy((void *)&LocalHostAddr.sf_addr, hp->h_addr, sizeof(struct in_addr));
#endif

        bitchsay("Local host name is now [%s]", LocalHostName);
        new_free(&newhost);

		if (reconn)
            reconnect_cmd(NULL, NULL, NULL, NULL);
    }
}

extern void display_bitchx (int);

BUILT_IN_COMMAND(info)
{
	
	if (!args || !*args)
	{
		display_bitchx(-1);
		return;
	} 
	else if (isdigit((unsigned char)*args))
	{
		display_bitchx(my_atol(args));
		return;
	}
	send_to_server("%s %s", command, args);
}

BUILT_IN_COMMAND(setenvcmd)
{
	char *env_var;

	if ((env_var = next_arg(args, &args)) != NULL)
		bsd_setenv(env_var, args, 1);
	else
		say("Usage: SETENV <var-name> <value>");
}

/*
 * whois: the WHOIS and WHOWAS commands.  This translates the 
 * to the whois handlers in whois.c 
 */
BUILT_IN_COMMAND(whois)
{
char *nick;
	if (!my_stricmp(command, "WHOWAS"))
	{
		char *word_one = next_arg (args, &args);
#if 0
		if (args && *args)
			malloc_sprintf(&stuff, "%s %s", word_one, args);
		else if (word_one && *word_one)
			malloc_sprintf(&stuff, "%s %d", word_one, get_int_var(NUM_OF_WHOWAS_VAR));
		else
			malloc_sprintf(&stuff, "%s %d", get_server_nickname(from_server), get_int_var(NUM_OF_WHOWAS_VAR));
#endif
		send_to_server("WHOWAS %s %s", (word_one && *word_one) ? 
			word_one : get_server_nickname(from_server), 
			(args && *args) ? args : ltoa(get_int_var(NUM_OF_WHOWAS_VAR)));
		return;
	}
	if (!(nick = next_arg(args, &args))) /* whois command */
	{
		char *target;
		target = get_target_by_refnum(0);
		if (target && !is_channel(target))
			send_to_server("WHOIS %s", target);
		else
			send_to_server("WHOIS %s", get_server_nickname(from_server));
	}
	else
		send_to_server("WHOIS %s%s%s", nick, (args && *args) ? space : empty_string, (args && *args)? args : empty_string);
}


/*
 * query: the /QUERY command.  Works much like the /MSG, I'll let you figure
 * it out.
 */
BUILT_IN_COMMAND(query)
{
	if(args && *args)
		win_create(QUERY_NEW_WINDOW_VAR, 0);
	window_query(current_window, &args, NULL);
	update_window_status(current_window, 0);
}

void read_away_log(char *stuff, char *line)
{
	
	if (!line || !*line || (line && toupper(*line) == 'Y'))
		readlog(NULL, NULL, NULL, NULL);
	update_input(UPDATE_ALL);
}

BUILT_IN_COMMAND(back)
{
int minutes = 0;
int hours = 0;
int seconds = 0;
int silent = 0;
	
	if (!current_window || from_server == -1)
		return;
	if (command && !my_stricmp(command, "gone"))
		silent = 1;
	if (get_server_awaytime(from_server))
	{
		time_t current_t = now - get_server_awaytime(from_server);
		int old_server = from_server;
		from_server = current_window->server;
		
		hours = current_t / 3600;
		minutes = (current_t - (hours * 3600)) / 60;
		seconds = current_t % 60;
		

		if (get_int_var(SEND_AWAY_MSG_VAR) && !silent)
		{
			bitchsay("You were /away for %i hours %i minutes and %i seconds. [\002BX\002-MsgLog %s]",
				hours, minutes, seconds,
				on_off(get_int_var(MSGLOG_VAR)));
			{
				char str[BIG_BUFFER_SIZE+1];
				char reason[BIG_BUFFER_SIZE+1];
				char fset[BIG_BUFFER_SIZE+1];
				*reason = 0;
				quote_it(args ? args : get_server_away(from_server), NULL, reason);
				if(fget_string_var(FORMAT_BACK_FSET))
				{
					quote_it(stripansicodes(convert_output_format(fget_string_var(FORMAT_BACK_FSET), "%s %d %d %d %d %s",
																  update_clock(GET_TIME),	hours, minutes, seconds,
																  get_int_var(MSGCOUNT_VAR), reason)), NULL, fset);
					snprintf(str, BIG_BUFFER_SIZE,
							 "PRIVMSG %%s :ACTION %s", fset);
					send_msg_to_channels(get_server_channels(from_server), from_server, str);
				}
			}
		}
		set_server_away(from_server, NULL, silent);
		from_server = old_server;
	}
	else
		set_server_away(from_server, NULL, silent);
	if (get_int_var(MSGLOG_VAR))
		log_toggle(0, NULL);

	set_server_awaytime(from_server, 0);
	if (get_int_var(MSGLOG_VAR))
	{
		char tmp[100];
		sprintf(tmp, " read /away msgs (%d msg%s) log [Y/n]? ", get_int_var(MSGCOUNT_VAR), plural(get_int_var(MSGCOUNT_VAR)));
		add_wait_prompt(tmp, read_away_log, empty_string, WAIT_PROMPT_LINE, 1); 
	}
	set_int_var(MSGCOUNT_VAR, 0);
	update_all_status(current_window, NULL, 0);
}

/*
 * away: the /AWAY command.  Keeps track of the away message locally, and
 * sends the command on to the server.
 */
BUILT_IN_COMMAND(away)
{
	int	len;
	char	*arg = NULL;
	int	flag = AWAY_ONE;
	int	i,
		silent = 0;

	
	if (command && !my_stricmp(command, "awaymsg"))
	{
		extern char *awaymsg;
		if (args && *args)
		{
			malloc_strcpy(&awaymsg, args);
			bitchsay("Your auto-away msg has been set to \"%s\"", awaymsg);
		}
		else
		{
			new_free(&awaymsg);
			bitchsay("Your auto-away msg has been unset");
		}
		return;
	}
	if (*args)
	{
		if ((*args == '-') || (*args == '/'))
		{
			arg = strchr(args, ' ');
			if (arg)
				*arg++ = '\0';
			else
				arg = empty_string;
			len = strlen(args);
			if (!my_strnicmp(args+1, "A", 1)) /* all */
			{
				flag = AWAY_ALL;
				args = arg;
			}
			else if (!my_strnicmp(args+1, "O", 1))/* one */
			{
				flag = AWAY_ONE;
				args = arg;
			}
			else
				return;
		}
	}
	if (command && !my_stricmp(command, "gone"))
		silent = 1;
	if (flag == AWAY_ALL)
	{
		for (i = 0; i < server_list_size(); i++)
			set_server_away(i, args, silent);
	}
	else
	{
		if (args && *args)
			set_server_away(from_server, args, silent);
		else
			back(command, args, NULL, NULL);
	}
	update_all_status(current_window, NULL, 0);
}

#if 0
#ifndef RAND_MAX
#define RAND_MAX 213234444
#endif
#endif

static void real_quit (char *dummy, char *ptr)
{
	
	if (ptr && *ptr)
	{
		if (*ptr == 'Y' || *ptr == 'y')
		{
			irc_exit(1, dummy, "%s", convert_output_format(fget_string_var(FORMAT_SIGNOFF_FSET), "%s %s %s %s", update_clock(GET_TIME), get_server_nickname(get_window_server(0)), m_sprintf("%s@%s", username, hostname), dummy));
		}
	}
	bitchsay("Excelllaaant!!");
}

/* e_quit: The /QUIT, /EXIT, etc command */
BUILT_IN_COMMAND(e_quit)
{
	int	old_server = from_server;
	char	Reason[BIG_BUFFER_SIZE];
	int active_dcc = 0;
	
	
	if (args && *args)
		strcpy(Reason, args);
	else
		strcpy(Reason, get_signoffreason(get_server_nickname(from_server)));
	if (!*Reason)
		strcpy(Reason, irc_version);
	
	active_dcc = get_active_count();
	if (active_dcc)
		add_wait_prompt("Active DCC\'s. Really Quit [y/N] ? ", real_quit, Reason, WAIT_PROMPT_LINE, 1);
	else
	{
		from_server = old_server;
		irc_exit(1, Reason, "%s", convert_output_format(fget_string_var(FORMAT_SIGNOFF_FSET), "%s %s %s %s", update_clock(GET_TIME), get_server_nickname(get_window_server(0)), m_sprintf("%s@%s", username, hostname), Reason));
	}
}

/* flush: flushes all pending stuff coming from the server */
BUILT_IN_COMMAND(flush)
{
	
	if (get_int_var(HOLD_MODE_VAR))
		flush_everything_being_held(NULL);
	bitchsay("Standby, Flushing server output...");
	flush_server();
	bitchsay("Done");
}

/* e_wall: used for WALLOPS */
BUILT_IN_COMMAND(e_wall)
{
	
	if ((!args || !*args))
		return;
	set_display_target(NULL, LOG_WALLOP);
	send_to_server("%s :%s", command, args);
	if (get_server_flag(from_server, USER_MODE_W))
		put_it("!! %s", args);
	add_last_type(&last_sent_wall[0], 1, get_server_nickname(from_server), NULL, "*", args);
	reset_display_target();
}

/*
 * e_privmsg: The MSG command, displaying a message on the screen indicating
 * the message was sent.  Also, this works for the NOTICE command. 
 */
BUILT_IN_COMMAND(e_privmsg)
{
	char	*nick;

	
	if ((nick = next_arg(args, &args)) != NULL)
	{
		if (!strcmp(nick, "."))
		{
			if (!(nick = get_server_sent_nick(from_server)))
			{
				bitchsay("You have not sent a message to anyone yet");
				return;
			}
		}
		else if (!strcmp(nick, ","))
		{
			if (!(nick = get_server_recv_nick(from_server)))
			{
				bitchsay("You have not received a message from anyone yet");
				return;
			}
		}
		else if (!strcmp(nick, "*") && (!(nick = get_current_channel_by_refnum(0))))
				nick = zero;
		send_text(nick, args, command, window_display, 1);
	}
}

/*
 * quote: handles the QUOTE command.  args are a direct server command which
 * is simply send directly to the server 
 */
BUILT_IN_COMMAND(quotecmd)
{
	int	old_from_server = from_server;
	int	all_servers = 0;
	
	if (command && *command == 'X')
	{
		while (args && (*args == '-' || *args == '/'))
		{
			char *flag = next_arg(args, &args);
			if (!my_strnicmp(flag + 1, "S", 1)) /* SERVER */
			{
				int sval = my_atol(next_arg(args, &args));
				if (is_server_open(sval))
					from_server = sval;
			} 
			else if (!my_strnicmp(flag+1, "ALL", 3) && args && *args)
			{
				next_arg(args, &args);
				all_servers++;
			}
		}
	}


	if (args && *args)
	{
		char	*comm = new_next_arg(args, &args);
		protocol_command *p;
		int	cnt;
		int	loc;

		upper(comm);
		p = (protocol_command *)find_fixed_array_item(
			(void *)rfc1459, sizeof(protocol_command),
			num_protocol_cmds + 1, comm, &cnt, &loc);

		/*
		 * If theyre dispatching some protocol commands we
		 * dont know about, then let them, without complaint.
		 */
#if 0
		if (cnt < 0 && (rfc1459[loc].flags & PROTO_NOQUOTE))
		{
			yell("Doing /QUOTE %s is not permitted.  Use the client's built in command instead.", comm);
			from_server = old_from_server;
			return;
		}
#endif
		/*
		 * If we know its going to cause a problem in the 
		 * future, whine about it.
		 */
#if 0
		if (cnt < 0 && (rfc1459[loc].flags & PROTO_DEPREC))
			yell("Doing /QUOTE %s is discouraged because it will break the client in the future.  Use the client's built in command instead.", comm);
#endif
		if (all_servers)
		{
			int i;
			for (i = 0; i < server_list_size(); i++)
			{
				if (is_server_connected(i))
				{
					from_server = i;
					send_to_server("%s %s", comm, args ? args:empty_string);
					rfc1459[loc].bytes += strlen(args);
					rfc1459[loc].count++;
				}
			}
		}
		else
		{
			send_to_server("%s %s", comm, args ? args : empty_string);
			rfc1459[loc].bytes += strlen(comm) + (args? strlen(args) : 0);
			rfc1459[loc].count++;
		}
	}

	from_server = old_from_server;
}

/*
 * send_comm: the generic command function.  Uses the full command name found
 * in 'command', combines it with the 'args', and sends it to the server 
 */
BUILT_IN_COMMAND(send_comm)
{
	
	if (args && *args)
		send_to_server("%s %s", command, args);
	else
		send_to_server("%s", command);
}

BUILT_IN_COMMAND(do_trace)
{
char *flags = NULL, *serv = NULL;
	
	set_server_trace_flag(from_server, 0);
	while (args && *args)
	{
		flags = next_arg(args, &args);
		if (flags && *flags=='-')
		{
			switch(toupper(*(flags+1)))
			{
				case 'O':
					set_server_trace_flag(from_server, get_server_trace_flag(from_server) | TRACE_OPER);
					break;
				case 'U':
					set_server_trace_flag(from_server, get_server_trace_flag(from_server) | TRACE_USER);
					break;
				case 'S':
					set_server_trace_flag(from_server, get_server_trace_flag(from_server) | TRACE_SERVER);
				default:
					break;
			}
			continue;
		}
		if (flags && *flags != '-')
			serv = flags;
		else
			serv = next_arg(args, &args);
	}
	if (get_server_trace_flag(from_server) & TRACE_OPER)
		bitchsay("Tracing server %s%sfor Operators", serv?serv:empty_string,serv?space:empty_string);
	if (get_server_trace_flag(from_server) & TRACE_USER)
		bitchsay("Tracing server %s%sfor Users", serv?serv:empty_string,serv?space:empty_string);
	if (get_server_trace_flag(from_server) & TRACE_SERVER)
		bitchsay("Tracing server %s%sfor servers", serv?serv:empty_string,serv?space:empty_string);
	send_to_server("%s%s%s", command, serv?space:empty_string, serv?serv:empty_string);
}

BUILT_IN_COMMAND(do_stats)
{
char *flags = NULL, *serv = NULL;
char *new_flag = empty_string;
char *str = NULL;
	
	set_server_stat_flag(from_server, 0);
	flags = next_arg(args, &args);
	if (flags)
	{
		switch(*flags)
		{
			case 'o':
				set_server_stat_flag(from_server, get_server_stat_flag(from_server) | STATS_OLINE);
				str = "%GType   Host                      Nick";
				break;
			case 'y':
				set_server_stat_flag(from_server, get_server_stat_flag(from_server) | STATS_YLINE);
				str = "%GType   Class  PingF  ConnF  MaxLi      SendQ";
				break;
			case 'i':
				set_server_stat_flag(from_server, get_server_stat_flag(from_server) | STATS_ILINE);
				str = "%GType   Host                      Host                       Port Class";
				break;
			case 'h':
				set_server_stat_flag(from_server, get_server_stat_flag(from_server) | STATS_HLINE);
				str = "%GType    Server Mask     Server Name";
				break;
			case 'l':
				set_server_stat_flag(from_server, get_server_stat_flag(from_server) | STATS_LINK);
				str = "%Glinknick|server               sendq smsgs sbytes rmsgs rbytes timeopen";
				break;
			case 'c':
				set_server_stat_flag(from_server, get_server_stat_flag(from_server) | STATS_CLASS);
				str = "%GType   Host                      Host                       Port Class";
				break;
			case 'u':
				set_server_stat_flag(from_server, get_server_stat_flag(from_server) | STATS_UPTIME);
				break;
			case 'k':
				set_server_stat_flag(from_server, get_server_stat_flag(from_server) | STATS_TKLINE);
				str = "%GType   Host                      UserName    Reason";
				break;
			case 'K':
				set_server_stat_flag(from_server, get_server_stat_flag(from_server) | STATS_KLINE);
				str = "%GType   Host                      UserName    Reason";
				break;
			case 'm':
				set_server_stat_flag(from_server, get_server_stat_flag(from_server) | STATS_MLINE);
			default:
				break;
		}
		new_flag = flags;
	}
	if (args && *args)
		serv = args;
	else
		serv = get_server_itsname(from_server);
	if (str)
		put_it("%s", convert_output_format(str, NULL, NULL));
	send_to_server("%s %s %s", command, new_flag, serv);
}

BUILT_IN_COMMAND(untopic)
{
	char *channel = NULL;
	ChannelList *chan;
	int server;
	
	args = sindex(args, "^ ");

	if (is_channel(args))
		channel = next_arg(args, &args);

	if (!(chan = prepare_command(&server, channel, PC_TOPIC)))
		return;

	my_send_to_server(server, "TOPIC %s :", chan->channel);
}

BUILT_IN_COMMAND(e_topic)
{
	char *channel = NULL;
	ChannelList *chan;
	int server;

	args = sindex(args, "^ ");

	if (is_channel(args))
	{
		channel = next_arg(args, &args);
		args = sindex(args, "^ ");
	}

	if (!(chan = prepare_command(&server, channel, args ? PC_TOPIC : NO_OP)))
		return;

	if (args)
	{
		add_last_type(&last_sent_topic[0], 1, NULL, NULL, chan->channel, args);
		my_send_to_server(server, "TOPIC %s :%s", chan->channel, args);
	}
	else
		my_send_to_server(server, "TOPIC %s", chan->channel);
}

/* MTOPIC by singe_ stolen from an idea by MHacker 
 * the code is basically from below with a few mods,
 *  and some added bloatcode(tm). Further modified by panasync
 */
BUILT_IN_COMMAND(do_mtopic)
{
	ChannelList *chan;
	
	if (from_server != -1)
	{
		int count = 0;
		for (chan  = get_server_channels(from_server); chan; chan = chan->next)
		{
			if (!chan->have_op)
				continue;
		 	send_to_server("TOPIC %s :%s", chan->channel, args ? args : empty_string);
			count++;
		}
		if (!count)
			bitchsay("You're not an op on any channels");
	} else
		bitchsay("No server for this window");
}

BUILT_IN_COMMAND(send_2comm)
{
	char	*reason = NULL;

	
#ifdef CDE
	args = next_arg(args, &reason);
	if (!args)
		args = empty_string;
#endif
	if (!(args = next_arg(args, &reason)))
		args = empty_string;
	if (!reason || !*reason)
		reason = empty_string;

	if (!args || !*args || !strcmp(args, "*"))
	{
		args = get_current_channel_by_refnum(0);
		if (!args || !*args)
			args = "*";     /* what-EVER */
	}
	if (reason && *reason)
		send_to_server("%s %s :%s", command, args, reason);
	else
		send_to_server("%s %s", command, args);
}

BUILT_IN_COMMAND(send_kill)
{
	char	*reason = NULL;
	char	*r_file;
	char	*nick;
	char	*arg = NULL, *save_arg = NULL;
		
#if 0
	args = next_arg(args, &reason);
	if (!args)
		args = empty_string;
#endif

#if defined(WINNT)
	r_file = m_sprintf("%s/BitchX.kil",get_string_var(CTOOLZ_DIR_VAR));
#else
	r_file = m_sprintf("%s/BitchX.kill",get_string_var(CTOOLZ_DIR_VAR));
#endif

	if ((reason = strchr(args, ' ')) != NULL)
		*reason++ = '\0';
	else
		if ((reason = get_reason(args, r_file)))
			reason = empty_string;
	new_free(&r_file);
	
	if (!strcmp(args, "*"))
	{
		ChannelList *chan = NULL;
		NickList *n = NULL;
		arg = get_current_channel_by_refnum(0);
		if (!arg || !*arg)
			return;     /* what-EVER */
		else
		{
			if (!(chan = lookup_channel(arg, from_server, 0)))
				return;
			arg = NULL;
			for (n = next_nicklist(chan, NULL); n; n = next_nicklist(chan, n))
			{
				if (isme(n->nick)) continue;
				m_s3cat(&arg, ",",  n->nick);
			}
			save_arg = arg;
		}
	}
	else
	{
		malloc_strcpy(&arg, args);
		save_arg = arg;
	}
	while ((nick = next_in_comma_list(arg, &arg)))
	{
		if (!nick || !*nick)
			break;
		if (reason && *reason)
			send_to_server("%s %s :%s", command, nick, reason);
		else
			send_to_server("%s %s", command, nick);
	}
	new_free(&save_arg);
}

/*
 * send_kick: sends a kick message to the server.  Works properly with
 * kick comments.
 */

BUILT_IN_COMMAND(send_kick)
{
	char	*kickee,
		*comment,
		*channel = NULL;
ChannelList *chan;
int server = from_server;
 	
	
        if (!(channel = next_arg(args, &args)))
		return;
        
        if (!(kickee = next_arg(args, &args)))
 		return;

        comment = args?args:get_reason(kickee, NULL);

	reset_display_target();
	if (!(chan = prepare_command(&server, (channel && !strcmp(channel, "*"))?NULL:channel, NEED_OP)))
		return;

	my_send_to_server(server, "%s %s %s :%s", command, chan->channel, kickee, comment);
}

BUILT_IN_COMMAND(send_mode)
{
char *channel;
	
	if ((channel = get_current_channel_by_refnum(0)))
		send_to_server("%s %s %s", command, channel, args? args: empty_string);
	else if (args && *args)
		send_to_server("%s %s", command, args);
}

/*
 * send_channel_com: does the same as send_com for command where the first
 * argument is a channel name.  If the first argument is *, it is expanded
 * to the current channel (a la /WHO).
 */
BUILT_IN_COMMAND(send_channel_com)
{
	char	*ptr,
		*s;
	
        ptr = next_arg(args, &args);

	if (ptr && !strcmp(ptr, "*"))
	{
		if ((s = get_current_channel_by_refnum(0)) != NULL)
			send_to_server("%s %s %s", command, s, args?args:empty_string);
		else
			say("%s * is not valid since you are not on a channel", command);
	}
	else if (ptr)
			send_to_server("%s %s %s", command, ptr, args?args:empty_string);
}

/*
 * Returns 1 if the given built in command exists,
 * Returns 0 if not.
 */
int	command_exist (char *command)
{
	int num;
	char buf[BIG_BUFFER_SIZE];

	if (!command || !*command)
		return 0;

	strcpy(buf, command);
	upper(buf);

	if (find_command(buf, &num))
		return (num < 0) ? 1 : 0;

	return 0;
}

void	redirect_text (int to_server, const char *nick_list, const char *text, char *command, int hook, int log)
{
static	int 	recursion = 0;
	int 	old_from_server = from_server;
	int	allow = 0;

	from_server = to_server;
	if (recursion++ == 0)
		allow = do_hook(REDIRECT_LIST, "%s %s", nick_list, text);

	/* 
	 * Dont hook /ON REDIRECT if we're being called recursively
	 */
	if (allow)
		send_text(nick_list, text, command, hook, log);

	recursion--;
	from_server = old_from_server;
}




struct target_type
{
	char *nick_list;
	char *message;
	int  hook_type;
	char *command;
	char *format;
	unsigned long level;
	char *output;
	char *other_output;
};

int current_target = 0;

/*
 * The whole shebang.
 *
 * The message targets are parsed and collected into one of 4 buckets.
 * This is not too dissimilar to what was done before, except now i 
 * feel more comfortable about how the code works.
 *
 * Bucket 0 -- Unencrypted PRIVMSGs to nicknames
 * Bucket 1 -- Unencrypted PRIVMSGs to channels
 * Bucket 2 -- Unencrypted NOTICEs to nicknames
 * Bucket 3 -- Unencrypted NOTICEs to channels
 *
 * All other messages (encrypted, and DCC CHATs) are dispatched 
 * immediately, and seperately from all others.  All messages that
 * end up in one of the above mentioned buckets get sent out all
 * at once.
 */
void 	BX_send_text(const char *nick_list, const char *text, char *command, int hook, int log)
{
	int	i, 
		old_server,
		not_done = 1,
		is_current = 0,
		old_window_display = window_display;
	char	*current_nick,
		*next_nick,
		*free_nick,
		*line,
		*key = NULL;
static	int sent_text_recursion = 0;	
	        
struct target_type target[4] = 
{	
	{NULL, NULL, SEND_MSG_LIST,     "PRIVMSG", "*%s*> %s" , LOG_MSG, NULL, NULL },
	{NULL, NULL, SEND_PUBLIC_LIST,  "PRIVMSG", "%s> %s"   , LOG_PUBLIC, NULL, NULL },
	{NULL, NULL, SEND_NOTICE_LIST,  "NOTICE",  "-%s-> %s" , LOG_NOTICE, NULL, NULL }, 
	{NULL, NULL, SEND_NOTICE_LIST,  "NOTICE",  "-%s-> %s" , LOG_NOTICE, NULL, NULL }
};

	

	target[0].output = fget_string_var(FORMAT_SEND_MSG_FSET);
	target[1].output = fget_string_var(FORMAT_SEND_PUBLIC_FSET);
	target[1].other_output = fget_string_var(FORMAT_SEND_PUBLIC_OTHER_FSET);
	target[2].output = fget_string_var(FORMAT_SEND_NOTICE_FSET);
	target[3].output = fget_string_var(FORMAT_SEND_NOTICE_FSET);
	target[3].other_output = fget_string_var(FORMAT_SEND_NOTICE_FSET);

	if (sent_text_recursion)
		hook = 0;
	window_display = hook;
	sent_text_recursion++;
	free_nick = next_nick = m_strdup(nick_list);

	while ((current_nick = next_nick))
	{
		if ((next_nick = strchr(current_nick, ',')))
			*next_nick++ = 0;

		if (!*current_nick)
			continue;

		if (*current_nick == '%')
		{
			if ((i = get_process_index(&current_nick)) == -1)
				say("Invalid process specification");
			else
				text_to_process(i, text, 1);
		}
		/*
		 * This test has to be here because it is legal to
		 * send an empty line to a process, but not legal
		 * to send an empty line anywhere else.
		 */
		else if (!text || !*text)
			;
#ifdef WANT_FTP
		else if (*current_nick == '-')
			dcc_ftpcommand(current_nick+1, (char *)text);
#endif
		else if (*current_nick == '"')
			send_to_server("%s", text);
		else if (*current_nick == '/')
		{
			line = m_opendup(current_nick, space, text, NULL);
			parse_command(line, 0, empty_string);
			new_free(&line);
		}
		else if (*current_nick == '=')
		{
			if (!is_number(current_nick + 1) && 
				!dcc_activechat(current_nick + 1) && 
				!dcc_activebot(current_nick+1) && 
				!dcc_activeraw(current_nick+1))
			{
				yell("No DCC CHAT connection open to %s", current_nick + 1);
				continue;
			}
			if ((key = is_crypted(current_nick)))
			{
				char *breakage;
				breakage = LOCAL_COPY(text);
				line = crypt_msg(breakage, key);
			}
			else
				line = m_strdup(text);

			old_server = from_server;
			from_server = -1;
			if (dcc_activechat(current_nick+1))
			{
				dcc_chat_transmit(current_nick + 1, line, line, command, hook);
				if (hook)
					addtabkey(current_nick, "msg", 0);
			}
			else if (dcc_activebot(current_nick + 1))
				dcc_bot_transmit(current_nick + 1, line, command);
			else 
				dcc_raw_transmit(current_nick + 1, line, command);

			add_last_type(&last_sent_dcc[0], MAX_LAST_MSG, NULL, NULL, current_nick+1, (char *)text);
			from_server = old_server;
			new_free(&line);
		}
		else
		{
			char *copy = NULL;
			if (doing_notice)
			{
				say("You cannot send a message from within ON NOTICE");
				continue;
			}

			copy = LOCAL_COPY(text);

			if (!(i = is_channel(current_nick)) && hook)
				addtabkey(current_nick, "msg", 0);
				
			if (doing_notice || (command && !strcmp(command, "NOTICE")))
				i += 2;

			if ((key = is_crypted(current_nick)))
			{
				set_display_target(current_nick, target[i].level);

				line = crypt_msg(copy, key);
				if (hook && do_hook(target[i].hook_type, "%s %s", current_nick, copy))
					put_it("%s", convert_output_format(fget_string_var(target[i].hook_type == SEND_MSG_LIST?FORMAT_SEND_ENCRYPTED_MSG_FSET:FORMAT_SEND_ENCRYPTED_NOTICE_FSET),
					"%s %s %s %s",update_clock(GET_TIME), get_server_nickname(from_server),current_nick, text));

				send_to_server("%s %s :%s", target[i].command, current_nick, line);
				new_free(&line);
				reset_display_target();
			}
			else
			{
				if (target[i].nick_list)
					malloc_strcat(&target[i].nick_list, ",");
				malloc_strcat(&target[i].nick_list, current_nick);
				if (!target[i].message)
					target[i].message = (char *)text;
			}
		}
	}
	
	for (i = 0; i < 4; i++)
	{
		char *copy = NULL;
		char *s = NULL;
		ChannelList *chan = NULL;
		const char *old_mf;
		unsigned long old_ml;
		int blah_hook = 0;
				
		if (!target[i].message)
			continue;

		save_display_target(&old_mf, &old_ml);
		set_display_target(target[i].nick_list, target[i].level);
		copy = m_strdup(target[i].message);

		/* do we forward this?*/

		if (forwardnick && not_done)
		{
			send_to_server("NOTICE %s :-> *%s* %s", forwardnick, nick_list, copy);
			not_done = 0;
		}

		/* log it if logging on */
		if (log)
			logmsg(LOG_SEND_MSG, target[i].nick_list, 0, "%s", copy);
		/* save this for /oops */
	
		if (i == 1 || i == 3)
		{
			char *channel;
			is_current = is_current_channel(target[i].nick_list, from_server, 0);
			if ((channel = get_current_channel_by_refnum(0)))
			{
				NickList *nick = NULL;
				if ((chan = lookup_channel(channel, from_server, 0)))
				{
					nick = find_nicklist_in_channellist(get_server_nickname(from_server), chan, 0);
					update_stats((i == 1) ?PUBLIC_LIST : NOTICE_LIST, nick, chan, 0);
				}
			}
		}
		else
			is_current = 1;

		if (hook && (blah_hook = do_hook(target[i].hook_type, "%s %s", target[i].nick_list, copy)))
			;
		/* supposedly if this is called before the do_hook
		 * it can cause a problem with scripts.
		 */
		 
		if (blah_hook || chan)
			s = convert_output_format(is_current ? target[i].output : target[i].other_output,
				"%s %s %s %s",update_clock(GET_TIME), target[i].nick_list, get_server_nickname(from_server), copy);
		if (blah_hook)
			put_it("%s", s);
			
		if (chan)
			add_to_log(chan->msglog_fp, now, s, logfile_line_mangler);

		if ((i == 0))
		{
			set_server_sent_nick(from_server, target[0].nick_list);
			set_server_sent_body(from_server, copy);
			add_last_type(&last_sent_msg[0], MAX_LAST_MSG, NULL, NULL, target[i].nick_list, copy);
		}
		else if ((i == 2) || (i == 3))
			add_last_type(&last_sent_notice[0], MAX_LAST_MSG, target[i].nick_list, NULL, get_server_nickname(from_server), copy);

		send_to_server("%s %s :%s", target[i].command, target[i].nick_list, copy);
		new_free(&copy);
		new_free(&target[i].nick_list);
		target[i].message = NULL;
		restore_display_target(old_mf, old_ml);
	}
	
	if (hook && get_server_away(from_server) && get_int_var(AUTO_UNMARK_AWAY_VAR))
		parse_line(NULL, "AWAY", empty_string, 0, 0, 1);

	new_free(&free_nick);
	window_display = old_window_display;
	sent_text_recursion--;
}

BUILT_IN_COMMAND(do_send_text)
{
	char	*tmp;
	char	*text = args;
	char	*cmd = NULL;
	
	if (command)
		tmp = get_current_channel_by_refnum(0);
	else
	{
		tmp = get_target_by_refnum(0);
		cmd = get_target_cmd_by_refnum(0);
	}
	if (cmd)
	{
#ifdef WANT_DLL
		int dllcnt = -1;
		IrcCommandDll *dll = NULL;
		dll = find_dll_command(cmd, &dllcnt);
		if (dll && (dllcnt == -1 || dllcnt == 1))
			dll->func (dll_commands, dll->server_func, text, NULL, dll->help);
		else 
#endif
		if (!my_stricmp(cmd, "SAY") || !my_stricmp(cmd, "PRIVMSG") || !my_stricmp(cmd, "MSG"))
			send_text(tmp, text, NULL, 1, 1);
		else if (!my_stricmp(cmd, "NOTICE"))
			send_text(tmp, text, "NOTICE", 1, 1);
		else if (!my_strnicmp(cmd, "WALLO", 5))
			e_wall("WALLOPS", text, NULL, NULL);
		else if (!my_strnicmp(cmd, "SWALLO", 6))
			e_wall("SWALLOPS", text, NULL, NULL);
		else if(!my_strnicmp(cmd, "WALLC", 5))
			ChanWallOp(NULL, text, NULL, NULL);
#ifndef BITCHX_LITE
		else if (!my_stricmp(cmd, "CSAY"))
			csay(NULL, text, NULL, NULL);
		else if (!my_stricmp(cmd, "CMSG"))
			cmsg(NULL, text, NULL, NULL);
#endif
		else
			send_text(tmp, text, NULL, 1, 1);					
	}
	else
		send_text(tmp, text, NULL, 1, 1);
}

BUILT_IN_COMMAND(do_msay)
{
	char *channels = NULL;
	int i = get_window_server(0);
	ChannelList *chan;
	
	if (i != -1)
	{
		int old_from_server = from_server;
		for (chan  = get_server_channels(i); chan; chan = chan->next)
		{
			malloc_strcat(&channels, chan->channel);
			if (chan->next)
				malloc_strcat(&channels, ",");
		}
		from_server = i;
		if (channels)
			send_text(channels, args, NULL, 1, 1);
		new_free(&channels);
		from_server = old_from_server;
	} else
		bitchsay("No server for this window");
}

/*
 * command_completion: builds lists of commands and aliases that match the
 * given command and displays them for the user's lookseeing 
 */
char **get_builtins (char *, int *);
void command_completion(char unused, char *not_used)
{
	int	do_aliases = 1, 
		do_functions = 1;
	int	cmd_cnt = 0,
		alias_cnt = 0,
		dll_cnt = 0,
		function_cnt = 0,
		i,
		c;
	char	**aliases = NULL;
	char	**functions = NULL;
	
	char	*line = NULL,
		*com,
		*cmdchars,
		*rest,
		firstcmdchar[2] = "/";
	IrcCommand	*command;
#ifdef WANT_DLL
	IrcCommandDll	*dll = NULL;
#endif
	char	buffer[BIG_BUFFER_SIZE + 1];
	
	
	malloc_strcpy(&line, get_input());
	if ((com = next_arg(line, &rest)) != NULL)
	{
		if (!(cmdchars = get_string_var(CMDCHARS_VAR)))
			cmdchars = DEFAULT_CMDCHARS;
		if (*com == '/' || strchr(cmdchars, *com))
		{
			*firstcmdchar = *cmdchars;
			com++;
			if (*com && strchr(cmdchars, *com))
			{
				do_aliases = 0;
				do_functions = 0;
				alias_cnt = 0;
				function_cnt = 0;
				com++;
			}
			else if (*com && strchr("$", *com))
			{
				do_aliases = 0;
				alias_cnt = 0;
				do_functions = 1;
				com++;
			} else
				do_aliases = do_functions = 1;

			upper(com);
			if (do_aliases)
				aliases = glob_cmd_alias(com, &alias_cnt);

			if (do_functions)
				functions = get_builtins(com, &function_cnt);

			if ((command = find_command(com, &cmd_cnt)) != NULL)
			{
				if (cmd_cnt < 0)
					cmd_cnt *= -1;
				/* special case for the empty string */
				if (!*command->name)
				{
					command++;
					cmd_cnt = NUMBER_OF_COMMANDS;
				}
			}
#ifdef WANT_DLL
			if ((dll = find_dll_command(com, &dll_cnt)) != NULL)
			{
				if (dll_cnt < 0)
					dll_cnt *= -1;
			}
#endif
			if ((alias_cnt == 1) && (cmd_cnt == 0) && (dll_cnt <= 0))
			{
				snprintf(buffer, BIG_BUFFER_SIZE, "%s%s %s", firstcmdchar, *aliases, rest);
				set_input(buffer);
				new_free((char **)aliases);
				new_free((char **)&aliases);
				update_input(UPDATE_ALL);
			}
			else if (((cmd_cnt == 1) && (dll_cnt <= 0) && (alias_cnt == 0)) ||
			    ((cmd_cnt == 0) && (dll_cnt == 1) && (alias_cnt == 0)) ||
			    (((cmd_cnt == 1) && (alias_cnt == 1) &&
			    !strcmp(aliases[0], command[0].name))
#ifdef WANT_DLL
			    || ((dll_cnt == 1) && (alias_cnt == 1) && !strcmp(dll->name, aliases[0]))
#endif
			    ))
			{
				snprintf(buffer, BIG_BUFFER_SIZE, "%s%s%s %s",
					firstcmdchar,
					do_aliases ? empty_string : firstcmdchar,
#ifdef WANT_DLL
					cmd_cnt ? command[0].name : dll->name,
#else
					command[0].name,
#endif					
					rest);
				set_input(buffer);
				update_input(UPDATE_ALL);
			}
			else
			{
				*buffer = 0;
				if (command)
				{
					*buffer = 0;
					c = 0;
					for (i = 0; i < cmd_cnt; i++)
					{
						if (i == 0)
							bitchsay("Commands:");
						strmcat(buffer, command[i].name, BIG_BUFFER_SIZE);
						strmcat(buffer, space, BIG_BUFFER_SIZE);
						if (++c == 4)
						{
							put_it("%s", convert_output_format("$G $[15]0 $[15]1 $[15]2 $[15]3", "%s", buffer));
							*buffer = 0;
							c = 0;
						}
					}
					if (c)
						put_it("%s", convert_output_format("$G $[15]0 $[15]1 $[15]2 $[15]3", "%s", buffer));
#ifdef WANT_DLL
					if (dll_commands)
						bitchsay("Plugin Commands:");
					*buffer = 0;
					c = 0;
					for (dll = dll_commands; dll; dll = dll->next)
					{
						if (com && *com && my_strnicmp(com, dll->name, strlen(com)))
							continue;
						strmcat(buffer, dll->name, BIG_BUFFER_SIZE);
						strmcat(buffer, space, BIG_BUFFER_SIZE);
						if (++c == 4)
						{
							put_it("%s", convert_output_format("$G $[15]0 $[15]1 $[15]2 $[15]3", "%s", buffer));
							*buffer = 0;
							c = 0;
						}
					}
					if (c)
						put_it("%s", convert_output_format("$G $[15]0 $[15]1 $[15]2 $[15]3", "%s", buffer));
#endif
				}
				if (aliases)
				{
					*buffer = 0;
					c = 0;
					for (i = 0; i < alias_cnt; i++)
					{
						if (i == 0)
							bitchsay("Aliases:");
						strmcat(buffer, aliases[i], BIG_BUFFER_SIZE);
						strmcat(buffer, space, BIG_BUFFER_SIZE);
						if (++c == 4)
						{
							put_it("%s", convert_output_format("$G $[15]0 $[15]1 $[15]2 $[15]3", "%s", buffer));
							*buffer = 0;
							c = 0;
						}
						new_free(&(aliases[i]));
					}
					if (strlen(buffer) > 1)
						put_it("%s", convert_output_format("$G $[15]0 $[15]1 $[15]2 $[15]3", "%s", buffer));
					new_free((char **)&aliases);
				}
				if (functions)
				{
					*buffer = 0;
					c = 0;
					for (i = 0; i < function_cnt; i++)
					{
						if (i == 0)
							bitchsay("Functions:");
						strmcat(buffer, functions[i], BIG_BUFFER_SIZE);
						strmcat(buffer, space, BIG_BUFFER_SIZE);
						if (++c == 4)
						{
							put_it("%s", convert_output_format("$G $[15]0 $[15]1 $[15]2 $[15]3", "%s", buffer));
							*buffer = 0;
							c = 0;
						}
						new_free(&(functions[i]));
					}
					if (strlen(buffer) > 1)
						put_it("%s", convert_output_format("$G $[15]0 $[15]1 $[15]2 $[15]3", "%s", buffer));
					new_free((char **)&functions);
				}
				if (!*buffer)
					term_beep();
			}
		}
		else
			term_beep();
	}
	else
		term_beep();
	new_free(&line);
}


BUILT_IN_COMMAND(e_call)
{
	dump_call_stack();
}

static int oper_issued = 0;

/* parse_line: This is the main parsing routine.  It should be called in
 * almost all circumstances over parse_command().
 *
 * parse_line breaks up the line into commands separated by unescaped
 * semicolons if we are in non interactive mode. Otherwise it tries to leave
 * the line untouched.
 *
 * Currently, a carriage return or newline breaks the line into multiple
 * commands too. This is expected to stop at some point when parse_command
 * will check for such things and escape them using the ^P convention.
 * We'll also try to check before we get to this stage and escape them before
 * they become a problem.
 *
 * Other than these two conventions the line is left basically untouched.
 */
/* Ideas on parsing: Why should the calling function be responsible
 *  for removing {} blocks?  Why cant this parser cope with and {}s
 *  that come up?
 */
void BX_parse_line (const char *name, char *org_line, const char *args, int hist_flag, int append_flag, int handle_local)
{
	char	*line = NULL,
			*stuff,
			*s,
			*t;
	int	args_flag = 0,
		die = 0;

	

	if (handle_local)
		make_local_stack((char *)name);

	line = LOCAL_COPY(org_line);

	if (!*org_line)
		do_send_text(NULL, empty_string, empty_string, NULL);
	else if (args) do
	{
		while (*line == '{') 
               	{
                       	if (!(stuff = next_expr(&line, '{'))) 
			{
				error("Unmatched {");
				destroy_local_stack();
                               	return;
                        }
			if (((internal_debug & DEBUG_CMDALIAS) || (internal_debug & DEBUG_HOOK)) && alias_debug && !in_debug_yell)
				debugyell("%3d %s", debug_count++, stuff);
       	                parse_line(name, stuff, args, hist_flag, append_flag, handle_local);

			if ((will_catch_break_exceptions && break_exception) ||
			    (will_catch_return_exceptions && return_exception) ||
			    (will_catch_continue_exceptions && continue_exception))
			{
				die = 1;
				break;
			}

			while (line && *line && ((*line == ';') || (my_isspace(*line))))
				*line++ = '\0';
		}

		if (!line || !*line || die)
			break;

		stuff = expand_alias(line, args, &args_flag, &line);
		if (!line && append_flag && !args_flag && args && *args)
			m_3cat(&stuff, space, args);

		if (((internal_debug & DEBUG_CMDALIAS) || (internal_debug & DEBUG_HOOK)) && alias_debug && !in_debug_yell)
			debugyell("%3d %s", debug_count++, stuff);

		parse_command(stuff, hist_flag, (char *)args);
       	        new_free(&stuff);

		if ((will_catch_break_exceptions && break_exception) ||
		    (will_catch_return_exceptions && return_exception) ||
		    (will_catch_continue_exceptions && continue_exception))
			break;
	} while (line && *line);
        else
	{
		if (load_depth != -1) /* CDE should this be != 1 or > 0 */
			parse_command(line, hist_flag, (char *)args);
		else
		{
			while ((s = line))
			{
				if ((t = sindex(line, "\r\n")))
				{
					*t++ = '\0';
					line = t;
				}
				else
					line = NULL;
				parse_command(s, hist_flag, (char *)args);

				if ((will_catch_break_exceptions && break_exception) ||
				    (will_catch_return_exceptions && return_exception) ||
				    (will_catch_continue_exceptions && continue_exception))
					break;

			}
		}
		debug_count = 1;
	}
	if (oper_issued)
	{
		memset(org_line, 0, strlen(org_line));
		oper_issued = 0;
	}
	if (handle_local)
		destroy_local_stack();
	return;
}


/*
 * parse_command: parses a line of input from the user.  If the first
 * character of the line is equal to irc_variable[CMDCHAR_VAR].value, the
 * line is used as an irc command and parsed appropriately.  If the first
 * character is anything else, the line is sent to the current channel or to
 * the current query user.  If hist_flag is true, commands will be added to
 * the command history as appropriate.  Otherwise, parsed commands will not
 * be added. 
 *
 * Parse_command() only parses a single command.  In general, you will want
 * to use parse_line() to execute things.Parse command recognized no quoted
 * characters or anything (beyond those specific for a given command being
 * executed). 
 */
int BX_parse_command(register char *line, int hist_flag, char *sub_args)
{
static	unsigned int	 level = 0;
	unsigned int	display,
			old_display_var;
		char	*cmdchars;
	const	char	*com;
		char	*this_cmd = NULL;
		int	args_flag = 0,
			add_to_hist,
			cmdchar_used = 0;
		int	noisy = 1;

		int	old_alias_debug = alias_debug;
				
	if (!line || !*line)
		return 0;

	if (internal_debug & DEBUG_COMMANDS && !in_debug_yell)
		debugyell("Executing [%d] %s", level, line);
	level++;

	if (!(cmdchars = get_string_var(CMDCHARS_VAR)))
		cmdchars = DEFAULT_CMDCHARS;


	this_cmd = LOCAL_COPY(line);
	set_current_command(this_cmd);
	add_to_hist = 1;
	display = window_display;
	old_display_var = (unsigned) get_int_var(DISPLAY_VAR);
	for ( ;*line;line++)
	{
		if (*line == '^' && (!hist_flag || cmdchar_used))
		{
			if (!noisy)
				break;
			noisy = 0;
		}
		else if ((!hist_flag && *line == '/') || strchr(cmdchars, *line))
		{
			cmdchar_used++;
			if (cmdchar_used > 2)
				break;
		}
		else
			break;
	}

	if (!noisy)
		window_display = 0;
	com = line;

	/*
	 * always consider input a command unless we are in interactive mode
	 * and command_mode is off.   -lynx
	 */
	if (hist_flag && !cmdchar_used && !get_int_var(COMMAND_MODE_VAR))
	{
		do_send_text(NULL, line, empty_string, NULL);
		if (hist_flag && add_to_hist)
			add_to_history(this_cmd);
		/* Special handling for ' and : */
	}
	else if (*com == '\'' && get_int_var(COMMAND_MODE_VAR))
	{
		do_send_text(NULL, line+1, empty_string, NULL);
		if (hist_flag && add_to_hist)
			add_to_history(this_cmd);
	}
	else if ((*com == '@') || (*com == '('))
	{
		/* This kludge fixes a memory leak */
		char	*tmp;
		char	*my_line = LOCAL_COPY(line); 
		char	*l_ptr;
		/*
		 * This new "feature" masks a weakness in the underlying
		 * grammar that allowed variable names to begin with an
		 * lparen, which inhibited the expansion of $s inside its
		 * name, which caused icky messes with array subscripts.
		 *
		 * Anyhow, we now define any commands that start with an
		 * lparen as being "expressions", same as @ lines.
		 */
		if (*com == '(')
		{
			if ((l_ptr = MatchingBracket(my_line + 1, '(', ')')))
				*l_ptr++ = 0;
		}

		if ((tmp = parse_inline(my_line + 1, sub_args, &args_flag)))
			new_free(&tmp);

		if (hist_flag && add_to_hist)
			add_to_history(this_cmd);
	}
	else
	{
		char		*rest,
				*alias = NULL,
				*alias_name = NULL,
				*cline = NULL;
		IrcCommand	*command = NULL;
		void		*arglist = NULL; 
		int		cmd_cnt,
				alias_cnt = 0;
#ifdef WANT_DLL
		int		dll_cnt;
		IrcCommandDll	*dll = NULL;
#endif		
		
		if ((rest = strchr(line, ' ')))
		{
			cline = alloca((rest - line) + 1);
			strmcpy(cline, line, (rest - line));
			rest++;
		}
		else
		{
			cline = LOCAL_COPY(line);
			rest = empty_string;
		}

		upper(cline);
		if (cline && !strcmp(cline, "OPER"))
			oper_issued++;

		if (cmdchar_used < 2)
			alias = get_cmd_alias(cline, &alias_cnt, &alias_name, &arglist);

		if (alias && alias_cnt < 0)
		{
			if (hist_flag && add_to_hist && !oper_issued)
				add_to_history(this_cmd);
			call_user_alias(alias_name, alias, rest, arglist);
			new_free(&alias_name);
		}
		else
		{
			/* History */
			if (*cline == '!')
			{
				if ((cline = do_history(cline + 1, rest)) != NULL)
				{
					if (level == 1)
					{
						set_input(cline);
						update_input(UPDATE_ALL);
					}
					else
						parse_command(cline, 0, sub_args);
					new_free(&cline);
				}
				else
					set_input(empty_string);
			}
			else
			{
				char unknown[] = "Unknown command:";
				
				if (hist_flag && add_to_hist && !oper_issued)
					add_to_history(this_cmd);
				command = find_command(cline, &cmd_cnt);
#ifdef WANT_DLL
				dll = find_dll_command(cline, &dll_cnt);
				if ((dll && (dll_cnt < 0)) || ((alias_cnt == 0) && (dll_cnt == 1)))
				{
					
					strcpy(cx_function, dll->name);
					if (rest && *rest && !my_stricmp(rest, "-help") && dll->help)
						userage(cline, dll->help);
					else if (dll->func)
					{
						dll->func(dll_commands, dll->server_func, rest, sub_args, dll->help);
						if (dll->result)
							put_it("%s", dll->result);
					}
					else
						bitchsay("%s: command disabled", dll->name);
					*cx_function = 0;
				}
				else 
#endif
				if (cmd_cnt < 0 || (alias_cnt == 0 && cmd_cnt == 1))
				{
					if (rest && *rest && !my_stricmp(rest, "-help"))
					{
						if (command->help)
							userage(cline, command->help);
#ifdef WANT_CHELP
						else
							chelp(NULL, cline, NULL, NULL);
#endif
					} else if ((command->flags & SERVERREQ) && (connected_to_server == 0))
						bitchsay("%s: You are not connected to a server. Use /SERVER to connect.", com);
					else if (command->func)
						command->func(command->server_func, rest, sub_args, command->help);
					else
						bitchsay("%s: command disabled", command->name);
				}
				else if ((alias_cnt == 1 && cmd_cnt == 1 && (!strcmp(alias_name, command->name))) ||
					    (alias_cnt == 1 && cmd_cnt == 0))
				{
					will_catch_return_exceptions++;
					parse_line(alias_name, alias, rest, 0, 1, 1);
					will_catch_return_exceptions--;
					return_exception = 0;
				}
				else if (!my_stricmp(com, get_server_nickname(from_server)))
					me(NULL, rest, empty_string, NULL);
#ifdef WANT_TCL
				else
				{
					
					if (tcl_interp)
					{
						int err;
						err = Tcl_Invoke(tcl_interp, cline, rest);
						if (err == TCL_OK)
						{
							if (tcl_interp->result && *tcl_interp->result)
								bitchsay("%s %s", *tcl_interp->result?empty_string:unknown, *tcl_interp->result?tcl_interp->result:empty_string);
						}
						else
						{
							if (alias_cnt + cmd_cnt > 1)
								bitchsay("Ambiguous command: %s", cline);
							else if (get_int_var(DISPATCH_UNKNOWN_COMMANDS_VAR))
								send_to_server("%s %s", cline, rest);
							else if (tcl_interp->result && *tcl_interp->result)
							{
								if (check_help_bind(cline))
									bitchsay("%s", tcl_interp->result);
							}
							else
								bitchsay("%s %s", unknown, cline);
								
						}
					} else if (get_int_var(DISPATCH_UNKNOWN_COMMANDS_VAR))
						send_to_server("%s %s", cline, rest);
					else if (alias_cnt + cmd_cnt > 1)
						bitchsay("Ambiguous command: %s", cline);
					else
						bitchsay("%s %s", unknown, cline);
				}
#else
				else if (get_int_var(DISPATCH_UNKNOWN_COMMANDS_VAR))
					send_to_server("%s %s", cline, rest);
				else if (alias_cnt + cmd_cnt > 1)
					bitchsay("Ambiguous command: %s", cline);
				else
					bitchsay("%s %s", unknown, cline);
#endif
			}
			if (alias)
				new_free(&alias_name);
		}
	}

	if (oper_issued)
		memset(this_cmd, 0, strlen(this_cmd));
		
	if (old_display_var != get_int_var(DISPLAY_VAR))
		window_display = get_int_var(DISPLAY_VAR);
	else
		window_display = display;
	level--;
	unset_current_command();
	alias_debug = old_alias_debug;
	return 0;
}


struct load_info
{
	char 	*filename;
	int	line;
	int	start_line;
} load_level[MAX_LOAD_DEPTH] = {{ NULL, 0 }};

void dump_load_stack (int onelevel)
{
	int i = load_depth;

	if (i == -1)
		return;

	debugyell("Right before line [%d] of file [%s]", load_level[i].line,
					  load_level[i].filename);

	if (!onelevel)
	{
		while (--i >= 0)
		{
			debugyell("Loaded right before line [%d] of file [%s]", 
				load_level[i].line, load_level[i].filename);
		}
	}
	return;
}

const char *current_filename (void) 
{ 
	if (load_depth == -1)
		return empty_string;
	else if (load_level[load_depth].filename)
		return load_level[load_depth].filename;
	else
		return empty_string;
}

int	current_line (void)
{ 
	if (load_depth == -1)
		return -1;
	else
		return load_level[load_depth].line;
}


/*
 * load: the /LOAD command.  Reads the named file, parsing each line as
 * though it were typed in (passes each line to parse_command). 
	Right now, this is broken, as it doesnt handle the passing of
	the '-' flag, which is meant to force expansion of expandos
	with the arguments after the '-' flag.  I think its a lame
	feature, anyhow.  *sigh*.
 */

BUILT_IN_COMMAND(BX_load)
{
#ifdef PUBLIC_ACCESS
	if (!never_connected)
	{
		bitchsay("This command is disabled for public access systems");
		return;
	}
#else
	FILE	*fp;
	char	*filename,
		*expanded = NULL;
	int	flag = 0;
        int     paste_level = 0;
	register char	*start;
	char	*current_row = NULL;
#define MAX_LINE_LEN 5 * BIG_BUFFER_SIZE
	char	buffer[MAX_LINE_LEN + 1];
	int	no_semicolon = 1;
	char	*irc_path;
	int	display = window_display;
        int     ack = 0;

	
	if (!(irc_path = get_string_var(LOAD_PATH_VAR)))
	{
		bitchsay("LOAD_PATH has not been set");
		return;
	}

	if (++load_depth == MAX_LOAD_DEPTH)
	{
		load_depth--;
		dump_load_stack(0);
		bitchsay("No more than %d levels of LOADs allowed", MAX_LOAD_DEPTH);
		return;
	}

	display = window_display;
	window_display = 0;
	status_update(0);

	/* 
	 * We iterate over the whole list -- if we use the -args flag, the
	 * we will make a note to exit the loop at the bottom after we've
	 * gone through it once...
	 */
	while (args && *args && (filename = next_arg(args, &args)))
	{
		/* 
		   If we use the args flag, then we will get the next
		   filename (via continue) but at the bottom of the loop
		   we will exit the loop 
		 */
		if (!my_strnicmp(filename, "-args", strlen(filename)))
		{
			flag = 1;
			continue;
		}
		if (!end_strcmp(filename, ".tcl", 4))
		{
#ifdef WANT_TCL
			if (Tcl_EvalFile(tcl_interp, filename) != TCL_OK)
				error("Unable to load filename %s[%s]", filename, tcl_interp->result);
#endif
			continue;
		}
		if (!(expanded = expand_twiddle(filename)))
		{
			error("Unknown user for file %s", filename);
			continue;
		}

		if (!(fp = uzfopen(&expanded, irc_path, 0)))
		{
			int owc = window_display;
			/* uzfopen emits an error if the file
			 * is not found, so we dont have to. */
			window_display = 1;
#ifdef WANT_DLL
			if (expanded)
			{
				dll_load(NULL, expanded, NULL, NULL);
				new_free(&expanded);
				window_display = owc;
				continue;
			}
#endif
			if (connected_to_server && !load_depth)
				bitchsay("Unable to open file %s", filename);
			new_free(&expanded);
			window_display = owc;
			continue;
		}

		if (command && *command == 'W')
		{
			yell ("%s", expanded);
			if (fp)
				fclose (fp);
			new_free(&expanded);
			continue;
		}
/* Reformatted by jfn */
/* *_NOT_* attached, so dont "fix" it */
		{
		int	in_comment 	= 0;
		int	comment_line 	= -1;
		int 	paste_line	= -1;

		current_row = NULL;
		load_level[load_depth].filename = expanded;
		load_level[load_depth].line = 1;
                                
		for (;;load_level[load_depth].line++)
		{
			int len;
			register char *ptr;
			
			if (!(fgets(buffer,MAX_LINE_LEN, fp)))
				break;

			for (start = buffer; my_isspace(*start); start++)
				;
			if (!*start || *start == '#')
				continue;

			len = strlen(start);

			/*
			 * this line from stargazer to allow \'s in scripts for continued
			 * lines <spz@specklec.mpifr-bonn.mpg.de>
			 *  If we have \\ at the end of the line, that
			 *  should indicate that we DONT want the slash to 
			 *  escape the newline (hop)
			 *  We cant just do start[len-2] because we cant say
			 *  what will happen if len = 1... (a blank line)
			 *  SO.... 
			 *  If the line ends in a newline, and
			 *  If there are at least 2 characters in the line
			 *  and the 2nd to the last one is a \ and,
			 *  If there are EITHER 2 characters on the line or
			 *  the 3rd to the last character is NOT a \ and,
			 *  If the line isnt too big yet and,
			 *  If we can read more from the file,
			 *  THEN -- adjust the length of the string
			 */

			while ( (start[len-1] == '\n') && 
				(len >= 2 && start[len-2] == '\\') &&
				(len < 3 || start[len-3] != '\\') && 
				(len < MAX_LINE_LEN) && 
				(fgets(&(start[len-2]), MAX_LINE_LEN - len, fp)))
			{
				len = strlen(start);
				load_level[load_depth].line++;
			}

			if (start[len - 1] == '\n')
				start[--len] = 0;
			if (start[len-1] == '\r')
				start[--len] = 0;

			while (start && *start)
			{
				char    *optr = start;

				/* Skip slashed brackets */
				while ((ptr = sindex(optr, "{};/")) && 
				      ptr != optr && ptr[-1] == '\\')
					optr = ptr+1;

				/* 
				 * if no_semicolon is set, we will not attempt
				 * to parse this line, but will continue
				 * grabbing text
				 */
				if (no_semicolon)
					no_semicolon = 0;
				else if ((!ptr || (ptr != start || *ptr == '/')) && current_row)
				{
					if (!paste_level)
					{
						parse_line(NULL, current_row, flag ? args :get_int_var(INPUT_ALIASES_VAR) ?empty_string: NULL, 0, 0, 1);
						new_free(&current_row);
					}
					else if (!in_comment)
						malloc_strcat(&current_row, ";");
				}

				if (ptr)
				{
					char    c = *ptr;

					*ptr = '\0';
					if (!in_comment)
						malloc_strcat(&current_row, start);
					*ptr = c;

					switch (c)
					{
		/* switch statement tabbed back */
case '/' :
{
	/* If we're in a comment, any slashes that arent preceeded by
	   a star is just ignored (cause its in a comment, after all >;) */
	if (in_comment)
	{
		/* ooops! cant do ptr[-1] if ptr == optr... doh! */
		if ((ptr > start) && (ptr[-1] == '*'))
		{
			in_comment = 0;
			comment_line = -1;
		}
		else
			break;
	}
	/* We're not in a comment... should we start one? */
	else
	{
		/* COMMENT_BREAKAGE_VAR */
		if ((ptr[1] == '*') && (!get_int_var(COMMENT_BREAKAGE_VAR) || ptr == start))
		{
			/* Yep. its the start of a comment. */
			in_comment = 1;
			comment_line = load_level[load_depth].line;
		}
		else
		{
			/* Its NOT a comment. Tack it on the end */
			malloc_strcat(&current_row, "/");

			/* Is the / is at the EOL? */
			if (ptr[1] == '\0')
			{
				/* If we are NOT in a block alias, */
				if (!paste_level)
				{
					/* Send the line off to parse_line */
					parse_line(NULL, current_row, flag ? args : get_int_var(INPUT_ALIASES_VAR) ? empty_string : NULL, 0, 0, 1);
					new_free(&current_row);
					ack = 0; /* no semicolon.. */
				}
				else
					/* Just add a semicolon and keep going */
					ack = 1; /* tack on a semicolon */
			}
		}


	}
	no_semicolon = 1 - ack;
	ack = 0;
	break;
}
case '{' :
{
	if (in_comment)
		break;

	/* If we are opening a brand new {} pair, remember the line */
	if (!paste_level)
		paste_line = load_level[load_depth].line;
		
	paste_level++;
	if (ptr == start)
		malloc_strcat(&current_row, " {");
	else
		malloc_strcat(&current_row, "{");
	no_semicolon = 1;
	break;
}
case '}' :
{
	if (in_comment)
		break;
        if (!paste_level)
		error("Unexpected } in %s, line %d", expanded, load_level[load_depth].line);
	else 
	{
        	--paste_level;

		if (!paste_level)
			paste_line = -1;
		malloc_strcat(&current_row, "}"); /* } */
		no_semicolon = ptr[1]? 1: 0;
	}
	break;
}
case ';' :
{
	if (in_comment)
		break;
	malloc_strcat(&current_row, ";");
	if (!ptr[1] && !paste_level)
	{
		parse_line(NULL, current_row, flag ? args : get_int_var(INPUT_ALIASES_VAR) ? empty_string : NULL, 0, 0, 1);
		new_free(&current_row);
	}
	else if (!ptr[1] && paste_level)
		no_semicolon = 0;
	else
		no_semicolon = 1;
	break;
}
	/* End of reformatting */
					}
					start = ptr+1;
				}
				else
				{
					if (!in_comment)
						malloc_strcat(&current_row, start);
					start = NULL;
				}
			}
		}
		if (in_comment)
			error("File %s ended with an unterminated comment in line %d", expanded, comment_line);
		if (current_row)
		{
			if (paste_level)
				error("Unexpected EOF in %s trying to match '[' at line %d",
						expanded, paste_line);
			else
				parse_line(NULL,current_row, flag ? args: get_int_var(INPUT_ALIASES_VAR) ? empty_string : NULL,0,0, 1);
			new_free(&current_row);
		}
		}
		new_free(&expanded);
		fclose(fp);
	}	/* end of the while loop that allows you to load
		   more then one file at a time.. */

	if (get_int_var(DISPLAY_VAR))
		window_display = display;

	status_update(1);
	load_depth--;
#endif
}



/* The SENDLINE command.. */
BUILT_IN_COMMAND(sendlinecmd)
{
	int	server;
	int	display;

	
	server = from_server;
	display = window_display;
	window_display = 1;
	if (args && *args)
		parse_line(NULL, args, get_int_var(INPUT_ALIASES_VAR) ?empty_string : NULL, 1, 0, 1);
	update_input(UPDATE_ALL);
	window_display = display;
	from_server = server;
}

/*
 * irc_clear_screen: the CLEAR_SCREEN function for BIND.  Clears the screen and
 * starts it if it is held 
 */
/*ARGSUSED*/
void irc_clear_screen(char key, char *ptr)
{
	
	set_hold_mode(NULL, OFF, 1);
	my_clear(NULL, empty_string, empty_string, NULL);
}

BUILT_IN_COMMAND(cd)
{
	char	*arg,
		*expand;
	char buffer[BIG_BUFFER_SIZE + 1];

	if ((arg = new_next_arg(args, &args)) != NULL && *arg)
	{
		if ((expand = expand_twiddle(arg)) != NULL)
		{
			if (chdir(expand))
			{
				bitchsay("CD: %s %s", arg, strerror(errno));
				new_free(&expand);
				return;
			}
			new_free(&expand);
		}
		else
		{
			bitchsay("CD: %s No such directory", arg);
			return;
		}
	}
	if (getcwd(buffer, BIG_BUFFER_SIZE))
		bitchsay("Current directory: %s", buffer);
	else
		bitchsay("CD: %s", strerror(errno));
}

BUILT_IN_COMMAND(exec_cmd)
{
	char	buffer[BIG_BUFFER_SIZE + 1];
	
	snprintf(buffer, BIG_BUFFER_SIZE, "%s %s", command, args);
	execcmd(NULL, buffer, subargs, helparg);
}

BUILT_IN_COMMAND(describe)
{
	char	*target;

	
	target = next_arg(args, &args);
	if (target && args && *args)
	{
		char	*message;

		message = args;
		if (!strcmp(target, "*"))
			if ((target = get_current_channel_by_refnum(0)) == NULL)
				target = zero;
		set_display_target(target, LOG_ACTION);
		send_ctcp(CTCP_PRIVMSG, target, CTCP_ACTION, "%s", message);

		if (do_hook(SEND_ACTION_LIST, "%s %s", target, message))
			put_it("%s", convert_output_format(fget_string_var(FORMAT_SEND_ACTION_OTHER_FSET), 
				"%s %s %s %s", update_clock(GET_TIME), get_server_nickname(from_server), target, message));
		reset_display_target();
	}
}

/*
 * New 'me' command.
 */

BUILT_IN_COMMAND(me)
{
	
	if (args && *args)
	{
		char	*target = NULL;
		if (!command)
			target = get_current_channel_by_refnum(0);
		else if (!my_stricmp(command, "dme"))
		{
			DCC_int *n = NULL;
			SocketList *s;
			int i;
			for (i = 0; i < get_max_fd()+1; i++)
			{
				if (!check_dcc_socket(i))
					continue;
				s = get_socket(i);
				n = (DCC_int *) s->info;
				if (!((s->flags & DCC_TYPES) == DCC_CHAT))
					continue;
				target = n->user;
				break;                                                                                                                
			}
		}
		else if (!my_stricmp(command, "qme"))
		{
			target = current_window->query_nick;
			if (!target)
			{
				Window *tmp = NULL;
				while((traverse_all_windows(&tmp)))
					if((target = tmp->query_nick))
						break;
			}
		}		
		else
			if (!(target = get_target_by_refnum(0)))
				target = get_current_channel_by_refnum(0);
		if (target)
		{
			char	*message;
			message = args;
			set_display_target(target, LOG_ACTION);
			send_ctcp(CTCP_PRIVMSG, target, CTCP_ACTION, "%s", message);

			if (do_hook(SEND_ACTION_LIST, "%s %s", target, message))
			{
				if (strchr("&#", *target))
					put_it("%s", convert_output_format(fget_string_var(FORMAT_SEND_ACTION_FSET), 
						"%s %s %s %s", update_clock(GET_TIME), get_server_nickname(from_server), target, message));
				else
					put_it("%s", convert_output_format(fget_string_var(FORMAT_SEND_ACTION_OTHER_FSET), 
						"%s %s %s %s", update_clock(GET_TIME), get_server_nickname(from_server), target, message));
			}
			reset_display_target();
		}
	}
}

/*
 * eval_inputlist:  Cute little wrapper that calls parse_line() when we
 * get an input prompt ..
 */

void eval_inputlist(char *args, char *line)
{
	
	parse_line(NULL, args, line ? line : empty_string, 0, 0, 1);
}

BUILT_IN_COMMAND(xevalcmd)
{
	char *	flag;
	int	old_from_server = from_server;
	Window *old_target_window = target_window;
	int	old_refnum = current_window->refnum;
	
	if (*command == 'X')
	{
		while (args && (*args == '-' || *args == '/'))
		{
			flag = next_arg(args, &args);
			if (!my_strnicmp(flag + 1, "S", 1)) /* SERVER */
			{
				int val = my_atol(next_arg(args, &args));
				if (is_server_open(val))
					from_server = val;
			}
			else if (!my_strnicmp(flag + 1, "W", 1)) /* WINDOW */
			{
				Window *win = get_window_by_desc(next_arg(args, &args));
				if (win)
				{
					current_window = win;
					target_window = win;
				}
 			}
		}
	}

	parse_line(NULL, args, subargs ? subargs : empty_string, 0, 0, 0);
	target_window = old_target_window;
	make_window_current_by_winref(old_refnum);
	from_server = old_from_server;
}

BUILT_IN_COMMAND(evalcmd)
{
	parse_line(NULL, args, subargs ? subargs : empty_string, 0, 0, 0);
}

/*
 * inputcmd:  the INPUT command.   Takes a couple of arguements...
 * the first surrounded in double quotes, and the rest makes up
 * a normal ircII command.  The command is evalutated, with $*
 * being the line that you input.  Used add_wait_prompt() to prompt
 * the user...  -phone, jan 1993.
 */

BUILT_IN_COMMAND(inputcmd)
{
	char	*prompt;
	int	wait_type;
	char	*argument;
	int	echo = 1;
	
	


	while (*args == '-')
	{
		argument = next_arg(args, &args);
		if (!my_stricmp(argument, "-noecho"))
			echo = 0;
	}

	if (!(prompt = new_next_arg(args, &args)))
	{
		say("Usage: %s \"prompt\" { commands }", command);
		return;
	}

	if (!my_stricmp(command, "INPUT"))
		wait_type = WAIT_PROMPT_LINE;
	else
		wait_type = WAIT_PROMPT_KEY;

	while (my_isspace(*args))
		args++;

	add_wait_prompt(prompt, eval_inputlist, args, wait_type, echo);
}

BUILT_IN_COMMAND(xtypecmd)
{
	char	*arg;

	
	if (args && (*args == '-' || *args == '/'))
	{
		char saved = *args;
		args++;
		if ((arg = next_arg(args, &args)) != NULL)
		{
			if (!my_strnicmp(arg, "L", 1))
			{
				for (; *args; args++)
					input_add_character(*args, empty_string);
			}
			else
				bitchsay ("Unknown flag -%s to XTYPE", arg);
			return;
		}
		input_add_character(saved, empty_string);
	}
	else
		type(command, args, empty_string, helparg);
	return;
}

BUILT_IN_COMMAND(beepcmd)
{
	term_beep();
}

BUILT_IN_COMMAND(e_debug)
{
	int x;
	char buffer[BIG_BUFFER_SIZE + 1];

	if (args && *args)
	{
		char *s, *copy;

		copy = LOCAL_COPY(args);
		upper(copy);
		while ((s = next_arg(copy, &copy)))
		{
			char *t, *c;
			x = 1;
			c = new_next_arg(copy, &copy);
			if (!c || !*c) 
				break;
			while ((t = next_in_comma_list(c, &c)))
			{
				if (!t || !*t) 
					break;
				if (*t == '-') 
				{ 
					t++; 
					x = 0; 
				}
				if (!my_stricmp(s, "ALIAS"))
					debug_alias(t, x);
				else if (!my_stricmp(s, "HOOK"))
					debug_hook(t, x);
			}
		}
		return;
	}
	*buffer = 0;
	for (x = 0; x < FD_SETSIZE; x++)
	{
		if (FD_ISSET(x, &readables))
		{
			strcat(buffer, space);
			strcat(buffer, ltoa(x));
		}
	}
	yell(buffer);
}

BUILT_IN_COMMAND(pretend_cmd)
{
	parse_server(args);
}

/*
 * This is a quick and dirty hack (emphasis on dirty) that i whipped up
 * just for the heck of it.  I feel really dirty about using the add_timer
 * call (bletch!) to fake a timeout for io().  The better answer would be
 * for io() to take an argument specifying the maximum threshold for a
 * timeout, but i didnt want to deal with that here.  So i just add a
 * dummy timer event that does nothing (wasting two function calls and
 * about 20 bytes of memory), and call io() until the whole thing blows
 * over.  Nice and painless.  You might want to try this instead of /sleep,
 * since this is (obviously) non-blocking.  This also calls time() for every
 * io event, so that might also start adding up.  Oh well, TIOLI.
 *
 * Without an argument, it waits for the user to press a key.  Any key.
 * and the key is accepted.  Thats probably not right, ill work on that.
 */
static        int     e_pause_cb_throw = 0;
static        void    e_pause_cb (char *u1, char *u2) { e_pause_cb_throw--; }

BUILT_IN_COMMAND(e_pause)
{
	char *sec;
	long milliseconds;
struct timeval start;

	if (!(sec = next_arg(args, &args)))
	{
		int	c_level = e_pause_cb_throw;

		add_wait_prompt(empty_string, e_pause_cb, 
				NULL, WAIT_PROMPT_DUMMY, 0);
		e_pause_cb_throw++;
		while (e_pause_cb_throw > c_level)
			io("pause");
		return;
	}

	milliseconds = (long)(atof(sec) * 1000);
	get_time(&start);
	start.tv_usec += (milliseconds * 1000);
	start.tv_sec += (start.tv_usec / 1000000);
	start.tv_usec %= 1000000;

	/* 
	 * I use comment here simply becuase its not going to mess
	 * with the arguments.
	 */
	add_timer(0, empty_string, milliseconds, 1, (int (*)(void *, char *))comment, NULL, NULL, get_current_winref(), "pause");
	while (BX_time_diff(get_time(NULL), start) > 0)
		io("e_pause");
}

#ifndef WANT_TCL

char tcl_versionstr[] = "";
BUILT_IN_COMMAND(tcl_command)
{
	bitchsay("This Client is not compiled with tcl support.");
}

BUILT_IN_COMMAND(tcl_load)
{
	tcl_command(NULL, NULL, NULL, NULL);
}

#else
BUILT_IN_COMMAND(tcl_load)
{
char *filename = NULL;
int result = 0;
	reset_display_target();
	if ((filename = next_arg(args, &args)))
	{
		char *bla = NULL;
		if (get_string_var(LOAD_PATH_VAR))
			bla = path_search(filename, get_string_var(LOAD_PATH_VAR));
		if ((result = Tcl_EvalFile(tcl_interp, bla?bla:filename)) != TCL_OK)
			put_it("Tcl:  [%s]",tcl_interp->result);
		else if (*tcl_interp->result)
			put_it("Tcl:  [%s]", tcl_interp->result);
	}
}


#endif


BUILT_IN_COMMAND(breakcmd)
{
	if (!will_catch_break_exceptions)
		say("Cannot BREAK here.");
	else
		break_exception++;
}

BUILT_IN_COMMAND(continuecmd)
{
	if (!will_catch_continue_exceptions)
		say("Cannot CONTINUE here.");
	else
		continue_exception++;
}

BUILT_IN_COMMAND(returncmd)
{
	if (!will_catch_return_exceptions)
		say("Cannot RETURN here.");
	else
	{
		if (args && *args)
		{
			int o_w_d = window_display;
			window_display = 0;
			add_local_alias("FUNCTION_RETURN", args);
			window_display = o_w_d;
		}
		return_exception++;
	}
}


BUILT_IN_COMMAND(help)
{
int cnt = 1;
int cntdll = 0;
IrcCommand *cmd = NULL;
#ifdef WANT_DLL
IrcCommandDll *cmddll = NULL;
#endif
int c, i;
char buffer[BIG_BUFFER_SIZE+1];
char *comm = NULL;
	reset_display_target();
	if (args && *args)
	{
		comm = next_arg(args, &args);
		cmd = find_command(upper(comm), &cnt);
#ifdef WANT_DLL
		cmddll = find_dll_command(comm, &cntdll);
		if (!cmd && !cmddll)
#else
		if (!cmd)
#endif
		{
			bitchsay("No such command [%s]", comm);
			return;
		}
		if (cmd == irc_command)
		{
			cmd = NULL;
			cnt = 0;
		}
		if (cnt < 0)
		{
			userage(cmd->name, cmd->help ? cmd->help : "No Help available Yet");
			return;
		}
#ifdef WANT_DLL
		if (cntdll < 0 && cmddll)
		{
			userage(cmddll->name, cmddll->help ? cmddll->help : "No Help available Yet");
			return;
		}
#endif
	}
#ifdef WANT_DLL
	if (!cmd && !cmddll)
#else
	if (!cmd)
#endif
	{
		cmd = irc_command; 
		cmd++; 
		cnt = NUMBER_OF_COMMANDS;
#ifdef WANT_DLL
		for (cmddll = dll_commands; cmddll; cmddll = cmddll->next)
			cntdll++;
		cmddll = dll_commands;
#endif
	} 
	else if (comm && *comm) 
	{
		int tcnt = 0;
		tcnt = cnt + cntdll;
		put_it("%s", convert_output_format("$G Found %K[%W$0%K]%n matches for %K[%W$1%K]", "%d %s", tcnt, comm));
	}
	*buffer = 0;
	c = 0;
	for (i = 0; i < cnt; i++)
	{
		strmcat(buffer, cmd[i].name, BIG_BUFFER_SIZE);
		strmcat(buffer, space, BIG_BUFFER_SIZE);
		if (++c == 5)
		{
			put_it("%s", convert_output_format("$G $[13]0 $[13]1 $[13]2 $[13]3 $[13]4", "%s", buffer));
			*buffer = 0;
			c = 0;
		}
	}
#ifdef WANT_DLL
	for (i = 0; i < cntdll && cmddll; cmddll = cmddll->next, i++)
	{
		strmcat(buffer, cmddll->name, BIG_BUFFER_SIZE);
		strmcat(buffer, space, BIG_BUFFER_SIZE);
		if (++c == 5)
		{
			put_it("%s", convert_output_format("$G $[13]0 $[13]1 $[13]2 $[13]3 $[13]4", "%s", buffer));
			*buffer = 0;
			c = 0;
		}
	}
#endif
	if (c)
		put_it("%s", convert_output_format("$G $[13]0 $[13]1 $[13]2 $[13]3 $[13]4", "%s", buffer));
	userage("help", "%R[%ncommand%R]%n or /command -help %n to get help on specific commands");
}


