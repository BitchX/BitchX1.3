/*
 * 'new' config.h:
 *	A configuration file designed to make best use of the abilities
 *	of ircII, and trying to make things more intuitively understandable.
 *
 * Done by Carl v. Loesch <lynx@dm.unirm1.it>
 * Based on the 'classic' config.h by Michael Sandrof.
 * Copyright(c) 1991 - See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * Warning!  You will most likely have to make changes to your .ircrc file to
 * use this version of IRCII!  Please read the INSTALL and New2.2 files
 * supplied with the distribution for details!
 *
 * @(#)$Id: config.h 160 2012-03-06 11:14:51Z keaston $
 */

#ifndef __config_h_
#define __config_h_

#include "defs.h"

#define OFF	0
#define ON	1

/*
 * Set your favorite default server list here.  This list should be a
 * whitespace separated hostname:portnum:password list (with portnums and
 * passwords optional).  This IS NOT an optional definition. Please set this
 * to your nearest servers.  However if you use a seperate 'ircII.servers'
 * file and the ircII can find it, this setting is overridden.
 */
#ifndef DEFAULT_SERVER
/* 
 * some caution is required here. the \ is a continuation char and is required
 * on any servers you add into this list. also the very last server should not 
 * have a continuation char.
 *
 * List last updated: 15-Aug-2009 (caf).
 */
#define DEFAULT_SERVER  "[efnet] "\
				"irc.eversible.com "\
				"irc.choopa.net "\
				"irc.easynews.com "\
				"irc.blessed.net "\
				"irc.servercentral.net "\
				"irc.umich.edu "\
				"irc.he.net "\
				"irc.mzima.net "\
				"irc.paraphysics.net "\
				"irc.shoutcast.com "\
				"irc.vel.net "\
				"irc.wh.verio.net "\
			"[xsirc] "\
				"irc.BitchX.org "\
				"ircd.ircii.org "\
				"irc.gibbed.net "\
			"[ircnet US] "\
				"ircnet.eversible.com "\
				"ircnet.choopa.net "\
				"us.ircnet.org "\
			"[ircnet EU] "\
				"irc.dotsrc.org "\
				"uk.ircnet.org "\
				"irc.xs4all.nl "\
				"irc.belwue.de "\
				"ircnet.nerim.fr "\
				"irc.eutelia.it "\
				"krakow.irc.pl "\
				"ircnet.netvision.net.il "\
			"[dalnet] "\
				"irc.dal.net "\
			"[Undernet US] "\
				"Dallas.TX.US.Undernet.org "\
				"mesa.az.us.undernet.org "\
				"newyork.ny.us.undernet.org "\
				"SantaAna.CA.US.Undernet.org "\
				"Tampa.FL.US.Undernet.org "\
			"[Undernet CA] "\
				"Vancouver.BC.CA.Undernet.org "\
			"[Undernet EU] "\
				"Diemen.NL.EU.Undernet.Org "\
				"Helsinki.FI.EU.Undernet.org "\
				"trondheim.no.eu.undernet.org "\
				"graz.at.Eu.UnderNet.org "\
				"Elsene.Be.Eu.undernet.org "\
				"bucharest.ro.eu.undernet.org "\
				"Ede.NL.EU.UnderNet.Org "\
				"oslo.no.eu.undernet.org "\
				"Zagreb.Hr.EU.UnderNet.org "\
				"Lelystad.NL.EU.UnderNet.Org "\
			"[Anynet] "\
				"irc.bluecherry.net "\
				"irc.irule.net "\
			"[AfterNet] "\
				"irc.afternet.org "\
			"[oftc] "\
				"irc.oftc.net "\
			"[SlashNET] "\
				"pinky.slashnet.org "\
				"blago.slashnet.org "\
				"moo.slashnet.org "\
				"coruscant.slashnet.org "
#endif

/*
 * You must always define this. If you can't compile glob.c, let us know.
 */
#define NEED_GLOB

/* On some channels mass modes can be confusing and in some case
 * spectacular so in the interest of keeping sanity, Jordy added this
 * mode compressor to the client. It reduces the duplicate modes that
 * might occur on a channel.. it's explained in names.c much better.
 */
#define COMPRESS_MODES

/*
 * Uncomment the following if the gecos field of your /etc/passwd has other
 * information in it that you don't want as the user name (such as office
 * locations, phone numbers, etc).  The default delimiter is a comma, change
 * it if you need to. If commented out, the entire gecos field is used. 
 */
#define GECOS_DELIMITER ','

/*
 * MAIL_DELIMITER specifies the unique text that separates one mail message
 * from another in the mail spool file when using UNIX_MAIL.
 */
#define MAIL_DELIMITER "From "

/*
 * Uncomment the following to make ircII read a list of irc servers from
 * the ircII.servers file in the ircII library. This file should be
 * whitespace separated hostname:portnum*password (with the portnum and
 * password being optional). This server list will supercede the
 * DEFAULT_SERVER. 
*/
#if defined(WINNT) || defined(__EMX__)
#define SERVERS_FILE "irc-serv"
#else
#define SERVERS_FILE "ircII.servers"
#endif

/*
 * Certain versions of Tcl lib have a PLUS version which preloads the scripts
 * into the binary so that the script directory is not required to run the 
 * tcllib. This offers some benefit at the expense of a slightly larger binary.
 */
#undef TCL_PLUS

/*
 * we define the default network type for server groups. Do not just
 * undefine this.
 */
 #define DEFAULT_NETWORK "efnet"

/*
 * Below are the IRCII variable defaults.  For boolean variables, use 1 for
 * ON and 0 for OFF.  You may set string variable to NULL if you wish them to
 * have no value.  None of these are optional.  You may *not* comment out or
 * remove them.  They are default values for variables and are required for
 * proper compilation.
 */

#if !defined(__EMX__) && !defined(WINNT)
/* if this file has something in it, then we'll use it instead. */
#include "../.config.h"
#endif

#if !defined(_USE_LOCAL_CONFIG)
/* NO _USE_LOCAL_CONFIG so use these instead */

/* On some channels mass modes can be confusing and in some case
 * spectacular so in the interest of keeping sanity, Jordy added this
 * mode compressor to the client. It reduces the duplicate modes that
 * might occur on a channel.. it's explained in names.c much better.
 */
 #define COMPRESS_MODES


/*
 * Define this if you want the $glob() function to be in your client.
 * There is a case for having this functino and a case against having
 * this function:
 *
 * Pro: makes it easier to write scripts like xdcc, since they can easily
 *      get at the filenames in your xdcc directory
 * ConS8 with $unlink(), $rmdir(), etc, it makes it that much easier for
 *      a backdoor to do damage to your account.
 *
 * You will have to weigh the evidence and decide if you want to include it.
 */
#define INCLUDE_GLOB_FUNCTION

/* crisk graciously allowed me to include his hebrew modification to ircII
 * in the client. defining this variable to 1 allows that happen. It also
 * adds a HEBREW_TOGGLE variable which can turn this feature on/off
 */
#undef WANT_HEBREW

/* if you use cidentd the filename is called .authlie instead of .noident.
 * as well some modifications to the format of the file were made. So we 
 * require some pre-knowledge of what to expect. WinNT identd servers will 
 * also require this.
 */
/* one or the other of these. not both */
#undef CIDENTD
#undef WDIDENT

/* 
 * Define this if your using a hacked ident and want to fake your username.
 * maybe we could also use this to specify what file to write this hack to. 
 * Some examples are ~/.noident and ~/.authlie
 */
#undef IDENT_FAKE

/*
 * I moved this here because it seemed to be the most appropriate
 * place for it.  Define this if you want support for ``/window create''
 * and its related features.  If you dont want it, youll save some code,
 * and you wont need 'wserv', and if you do want it, you can have it in
 * all of its broken glory (no, i dont have plans to fix it =)
 * window create doesn't make any sense on Windows 95/NT.
 */
#if !defined(GUI) && defined(WINNT)
#undef WINDOW_CREATE
#else
#define WINDOW_CREATE
#endif

/*
 * Define this if you want an mIRC compatable /dcc resume capability.
 * Note that this BREAKS THE IRC PROTOCOL, and if you use this feature,
 * the behavior is NON COMPLIANT.  If this warning doesnt bother you,
 * and you really want this feature, then go ahead and #define this.
 */
#define MIRC_BROKEN_DCC_RESUME ON

/*
 * Set the following to 1 if you wish for IRCII not to disturb the tty's flow
 * control characters as the default.  Normally, these are ^Q and ^S.  You
 * may have to rebind them in IRCII.  Set it to 0 for IRCII to take over the
 * tty's flow control.
 */
#define USE_FLOW_CONTROL ON

/* 
 * Make ^Z stop the irc process by default, if undefined, ^Z will self-insert
 * by default
 */
#define ALLOW_STOP_IRC

/* And here is the port number for default client connections.  */
#define IRC_PORT 6667

/* 
 * If you define UNAME_HACK, the uname information displayed in the
 * CTCP VERSION info will appear as "*IX" irregardless of any other
 * settings.  Useful for paranoid users who dont want others to know
 * that theyre running a buggy SunOS machine. >;-)
 */
#undef UNAME_HACK

/* 
 * If you define ONLY_STD_CHARS, only "normal" characters will displayed.
 * This is recommended when you want to start BitchX in an xterm without
 * the usage of the special "vga"-font. 
 */
#undef ONLY_STD_CHARS

/*
 * Normally BitchX uses only the IBMPC (cp437) charset.
 * Define LATIN1, if you want to see the standard Latin1 characters
 * (i.e. Ä Ö Ü ä ö ü ß <-> "A "O "U "a "o "u \qs ).
 *
 * You will still be able to see ansi graphics, but there will be some
 * smaller problems (i.e. after a PageUp).
 *
 * If you use xterm there is no easy way to use both fonts at the same
 * time.  You have to decide if you use the standard (latin1) fonts or
 * vga.pcf (cp437).
 *
 * Is here there any solution to use both fonts nethertheless ?
 */
#undef LATIN1

/*
 * If you use LINUX and non ISO8859-1 fonts with custom screen mapping,
 * and if your see some pseudographics instead of your national characters,
 * define this to solve the problem.
 *
 */
#undef CHARSET_CUSTOM

/* 
 * If you want the non-ansi BitchX logo only define this ASCII_LOGO
 *
 * Note: On the console ansi graphics can be displayed just fine, even 
 * when you've defined LATIN1. The problem is that ansi graphics will look
 * ugly if you use the scroll up feature (PageUp/PageDown)
 */
#undef ASCII_LOGO

/* If you define REVERSE_WHITE_BLACK, then the format codes for black and
 * white color are revepsed. (%W, %w is bold black and black, %K, %k is bold
 * white and white). This way the default format-strings are readable on
 * a display with white background and black foreground.
 */ 
#undef REVERSE_WHITE_BLACK

/*
 * Define this if you want support for ircII translation tables.
 */
#define TRANSLATE

/*
 * WinNT and EMX probably need an ident server, so define this unless you have
 * an external one, or don't want ident support (bad idea).
 */
#if defined(WINNT) || defined(__EMX__)
#define WANT_IDENTD
#endif

/*
 * Define the name of your ircrc file here.
 */
#if defined(WINNT) || defined(__EMX__)
#define IRCRC_NAME "/irc-rc"
#else
#define IRCRC_NAME "/.ircrc"
#endif

#define DEFAULT_PING_TYPE 1
#define DEFAULT_MSGLOG ON
#define DEFAULT_AUTO_NSLOOKUP OFF
#define DEFAULT_ALT_CHARSET ON
#define DEFAULT_FLOOD_KICK ON
#define DEFAULT_FLOOD_PROTECTION ON
#define DEFAULT_CTCP_FLOOD_PROTECTION ON
#define DEFAULT_MAX_AUTOGET_SIZE 2000000
#define DEFAULT_LLOOK_DELAY 120
#define DEFAULT_ALWAYS_SPLIT_BIGGEST ON
#define DEFAULT_AUTO_UNMARK_AWAY OFF
#define DEFAULT_AUTO_WHOWAS OFF
#define DEFAULT_BANTIME 600
#define DEFAULT_BEEP ON
#define DEFAULT_BEEP_MAX 3
#define DEFAULT_BEEP_WHEN_AWAY OFF
#define DEFAULT_BOLD_VIDEO ON
#define DEFAULT_BLINK_VIDEO ON
#define DEFAULT_CHANNEL_NAME_WIDTH 10
#define DEFAULT_CLOCK ON
#define DEFAULT_CLOCK_24HOUR OFF
#define DEFAULT_COMMAND_MODE OFF
#define DEFAULT_COMMENT_HACK ON
#define DEFAULT_DCC_BLOCK_SIZE 2048
#define DEFAULT_DISPLAY ON
#define DEFAULT_DO_NOTIFY_IMMEDIATELY ON
#define DEFAULT_EIGHT_BIT_CHARACTERS ON
#define DEFAULT_EXEC_PROTECTION ON
#define DEFAULT_FLOOD_AFTER 4
#define DEFAULT_FLOOD_RATE 5
#define DEFAULT_FLOOD_USERS 10
#define DEFAULT_FLOOD_WARNING OFF
#define DEFAULT_FULL_STATUS_LINE ON
#define DEFAULT_HELP_PAGER ON
#define DEFAULT_HELP_PROMPT ON
#define DEFAULT_HIGH_BIT_ESCAPE OFF
#define DEFAULT_HIDE_PRIVATE_CHANNELS OFF
#define DEFAULT_HISTORY 100
#define DEFAULT_HOLD_MODE OFF
#define DEFAULT_HOLD_MODE_MAX 0
#define DEFAULT_INDENT ON
#define DEFAULT_INPUT_ALIASES OFF
#define DEFAULT_INSERT_MODE ON
#define DEFAULT_INVERSE_VIDEO ON
#define DEFAULT_LASTLOG 1000
#define DEFAULT_LOG OFF
#define DEFAULT_MAIL 2
#define DEFAULT_NO_CTCP_FLOOD ON
#define DEFAULT_NOTIFY_HANDLER "QUIET"
#define DEFAULT_NOTIFY_INTERVAL 60
#define DEFAULT_NOTIFY_LEVEL "ALL DCC"
#define DEFAULT_NOTIFY_ON_TERMINATION OFF
#define DEFAULT_NOTIFY_USERHOST_AUTOMATIC ON
#define DEFAULT_SCROLL_LINES ON
#define DEFAULT_SEND_IGNORE_MSG OFF
#define DEFAULT_SEND_OP_MSG ON
#define DEFAULT_SHELL_LIMIT 0
#define DEFAULT_SHOW_AWAY_ONCE ON
#define DEFAULT_SHOW_CHANNEL_NAMES ON
#define DEFAULT_SHOW_END_OF_MSGS OFF
#define DEFAULT_SHOW_NUMERICS OFF
#define DEFAULT_SHOW_STATUS_ALL OFF
#define DEFAULT_SHOW_WHO_HOPCOUNT OFF
#define DEFAULT_META_STATES 5
#define DEFAULT_IGNORE_TIME 600
#define DEFAULT_MAX_DEOPS 2
#define DEFAULT_MAX_IDLEKICKS 2
#define DEFAULT_NUM_BANMODES 4
#define DEFAULT_NUM_KICKS 1
#define DEFAULT_NUM_OF_WHOWAS 4
#define DEFAULT_NUM_OPMODES 4
#define DEFAULT_SEND_AWAY_MSG OFF
#define DEFAULT_SEND_CTCP_MSG ON
#define DEFAULT_SOCKS_PORT 1080
#define DEFAULT_AUTO_AWAY_TIME 600
#define DEFAULT_AUTO_RECONNECT ON
#define DEFAULT_AUTO_UNBAN 600
#define DEFAULT_CDCC ON
#define DEFAULT_CDCC_FLOOD_AFTER 3
#define DEFAULT_CDCC_FLOOD_RATE 4
#define DEFAULT_CTCP_DELAY 3
#define DEFAULT_CTCP_FLOOD_BAN ON
#define DEFAULT_DCC_AUTORENAME ON
#define DEFAULT_DCC_AUTORESUME OFF
#define DEFAULT_DCC_AUTORENAME_ON_NICKNAME OFF
#define DEFAULT_DCC_BAR_TYPE 0 /* 0 or 1 */
#define DEFAULT_DOUBLE_STATUS_LINE ON
#define DEFAULT_FTP_GRAB OFF
#define DEFAULT_HTTP_GRAB OFF
#define DEFAULT_HELP_WINDOW OFF
#define DEFAULT_NICK_COMPLETION ON
#define DEFAULT_NICK_COMPLETION_CHAR ':'
#define DEFAULT_NICK_COMPLETION_LEN 2
#define DEFAULT_NICK_COMPLETION_TYPE 0  /* 0 1 2 */
#define DEFAULT_NOTIFY ON
#define DEFAULT_QUEUE_SENDS 0
#define DEFAULT_MAX_SERVER_RECONNECT 2
#define DEFAULT_SERVER_GROUPS OFF
#define DEFAULT_WINDOW_DESTROY_PART OFF
#define DEFAULT_WINDOW_DESTROY_QUERY OFF
#define DEFAULT_SUPPRESS_SERVER_MOTD ON
#define DEFAULT_TAB ON
#define DEFAULT_TAB_MAX 8
#define DEFAULT_TIMESTAMP OFF
#define DEFAULT_TIMESTAMP_AWAYLOG_HOURLY ON
#define DEFAULT_TIMESTAMP_STR "%I:%M%p "
#define DEFAULT_UNDERLINE_VIDEO ON
#define DEFAULT_VERBOSE_CTCP ON
#define DEFAULT_DISPLAY_ANSI ON
#define DEFAULT_DISPLAY_PC_CHARACTERS 4
#define DEFAULT_DCC_AUTOGET OFF
#define DEFAULT_DCC_GET_LIMIT 0
#define DEFAULT_DCC_SEND_LIMIT 5
#define DEFAULT_DCC_QUEUE_LIMIT 10
#define DEFAULT_DCC_LIMIT 10
#define DEFAULT_DCCTIMEOUT 600
#define DEFAULT_FLOATING_POINT_MATH OFF
#define DEFAULT_LLOOK OFF
#define DEFAULT_CLOAK OFF
#define DEFAULT_AINV 0
#define DEFAULT_ANNOY_KICK OFF
#define DEFAULT_AOP_VAR OFF
#define DEFAULT_AUTO_AWAY ON
#define DEFAULT_KICK_OPS ON
#define DEFAULT_AUTO_REJOIN ON
#define DEFAULT_DEOPFLOOD ON
#if defined(__EMXPM__) || defined(WIN32)
#define DEFAULT_CODEPAGE 437
#endif
#define DEFAULT_CTCP_FLOOD_AFTER 3
#define DEFAULT_CTCP_FLOOD_RATE 10
#define DEFAULT_DEOPFLOOD_TIME 30
#define DEFAULT_DEOP_ON_DEOPFLOOD 3
#define DEFAULT_DEOP_ON_KICKFLOOD 3
#define DEFAULT_KICK_IF_BANNED OFF
#define DEFAULT_HACKING 0  /* 0 1 2 */
#define DEFAULT_JOINFLOOD ON
#define DEFAULT_JOINFLOOD_TIME 50
#define DEFAULT_KICKFLOOD ON
#define DEFAULT_KICKFLOOD_TIME 30
#define DEFAULT_KICK_ON_DEOPFLOOD 3
#define DEFAULT_KICK_ON_JOINFLOOD 4
#define DEFAULT_KICK_ON_KICKFLOOD 4
#define DEFAULT_KICK_ON_NICKFLOOD 3
#define DEFAULT_KICK_ON_PUBFLOOD 30
#define DEFAULT_NICKFLOOD ON
#define DEFAULT_NICKFLOOD_TIME 30
#ifdef __EMXPM__
#define DEFAULT_NICKLIST 10
#else
#define DEFAULT_NICKLIST 100
#endif
#define DEFAULT_NICKLIST_SORT 0
#define DEFAULT_LAME_IDENT OFF
#define DEFAULT_LAMELIST ON
#define DEFAULT_SHITLIST ON
#define DEFAULT_USERLIST ON
#define DEFAULT_PUBFLOOD OFF
#define DEFAULT_PUBFLOOD_TIME 20
#define DEFAULT_CONNECT_DELAY 1
#define DEFAULT_CONNECT_TIMEOUT 30
#define DEFAULT_STATUS_NO_REPEAT ON
#define DEFAULT_STATUS_DOES_EXPANDOS OFF
#define DEFAULT_DISPATCH_UNKNOWN_COMMANDS OFF
#define DEFAULT_SCROLLBACK_LINES 512
#define DEFAULT_SCROLLBACK_RATIO 50
#define DEFAULT_SCROLLERBARS ON
#define DEFAULT_ND_SPACE_MAX 160
#define DEFAULT_CPU_SAVER_AFTER 0
#define DEFAULT_CPU_SAVER_EVERY 0
#define DEFAULT_NO_FAIL_DISCONNECT OFF
#define DEFAULT_MAX_URLS 20	/* this defines the MAX number of urls saved */
#undef BITCHX_LITE
#undef EMACS_KEYBINDS	       /* change this is you have problems with 
				* your keyboard
				*/
#define EXEC_COMMAND
#undef PUBLIC_ACCESS		/* 
				 * this define removes /load /exec commands
				 */
#define DEFAULT_OPERVIEW_HIDE 0 /* defines the operview window. if hidden or not */ 
#define WANT_OPERVIEW ON
#define WANT_EPICHELP 	ON	/* epic help command. /ehelp. */
#define WANT_LLOOK	ON	/* do we want built-in llooker. */
#define WANT_CDCC	ON	/* do we want the cdcc system */
#define WANT_FTP	ON	/* do we want the ftp dcc comamnd */
#if defined(HAVE_RESOLV) && defined(HAVE_ARPA_NAMESER_H) && defined(HAVE_RESOLV_H)
#define WANT_NSLOOKUP	ON
#else
#undef WANT_NSLOOKUP
#endif
#define WANT_TABKEY	ON
#define WANT_CHELP	ON
#define WANT_USERLIST	ON
#undef HUMBLE			/* define this for a hades look */

#define WANT_DETACH OFF	/* this is here for the detach/re-attach code
			   which is essentially a mini-screen */

#define ALLOW_DETACH ON
#define DEFAULT_DETACH_ON_HUP OFF

#undef OLD_RANDOM_BEHAVIOR   /* semi randomness for random() */

#ifdef WANT_OPERVIEW
#define DEFAULT_OPER_VIEW OFF
#endif

#if defined(NON_BLOCKING_CONNECTS)
#define DEFAULT_DCC_FAST ON
#else
#define DEFAULT_DCC_FAST OFF
#endif

#endif 
/* _USE_LOCAL_CONFIG */

#define DEFAULT_KICK_REASON "Bitch-X BaBy!"
#define DEFAULT_PROTECT_CHANNELS "*"
#define DEFAULT_SHITLIST_REASON "Surplus Lamerz must go!!!!"
#define DEFAULT_BEEP_ON_MSG "MSGS"
#define DEFAULT_CMDCHARS "/"
#define DEFAULT_CONTINUED_LINE "          "
#define DEFAULT_HIGHLIGHT_CHAR "INVERSE"
#define DEFAULT_LASTLOG_LEVEL "ALL"
#define DEFAULT_MSGLOG_LEVEL "MSGS NOTICES SEND_MSG"
#define DEFAULT_LOGFILE "IrcLog"
#define DEFAULT_SHELL "/bin/sh"
#define DEFAULT_SHELL_FLAGS "-c"
#define DEFAULT_USERINFO ""
#define DEFAULT_XTERM "rxvt"
#define DEFAULT_XTERM_OPTIONS "-bg black -fg white"
#define DEFAULT_DCC_DLDIR "~"

#define DEFAULT_PAD_CHAR ' '
#define DEFAULT_USERMODE "+iw"  /* change this to the default usermode */
#define DEFAULT_OPERMODE "swfck"
#define DEFAULT_CHANMODE "+nt" /* default channel mode */

#define DEFAULT_SWATCH "KILLS,CLIENTS,TRAFFIC,REHASH,KLINE,BOTS,OPER,SQUIT,SERVER,CONNECT,FLOOD,USER,STATS,NICK,ACTIVEK"

#define DEFAULT_WORD_BREAK " \t"

#define DEFAULT_JOIN_NEW_WINDOW 0
#define DEFAULT_QUERY_NEW_WINDOW 0
#ifdef GUI
#define DEFAULT_JOIN_NEW_WINDOW_TYPE "create hide swap last double on split on"
#define DEFAULT_QUERY_NEW_WINDOW_TYPE "create hide swap last double on split on"
#else
#define DEFAULT_JOIN_NEW_WINDOW_TYPE "new hide_others double on"
#define DEFAULT_QUERY_NEW_WINDOW_TYPE "new hide_others double on"
#endif

#define DEFAULT_MDI OFF

#ifdef __EMX__
#define DEFAULT_FONT "6x10"
#elif defined(GTK)
#define DEFAULT_FONT "vga"
#else
#define DEFAULT_FONT "-fn vga11x19"
#endif

/*#define CLOAKED  "emacs"*/	/*
				 * define this to the program you want to
				 * show up in "ps" and "top" to hide irc
				 * from evil sys-admins.
				 */

#if !defined(NON_BLOCKING_CONNECTS) && defined(DEFAULT_DCC_FAST)
#undef DEFAULT_DCC_FAST
#define DEFAULT_DCC_FAST OFF
#endif

#if defined LATIN1
/* Make sure the keyboard works */
#undef EMACS_KEYBINDS
/* No line chars available, so better define this: */
#define ONLY_STD_CHARS 1
#endif

/*
 * on certain systems we can define NON_BLOCKING to 1
 * connects are then done alot differantly. We can perform actual work
 * in the background, while connecting. This also protects us from certain
 * "bombs" that are available.  If you have trouble with this undef
 * the NON_BLOCKING_CONNECTS. DCC sends/gets are much improved with this.
 */

#if defined(HEBREW) && !defined(TRANSLATE)
#define TRANSLATE 1
#endif
#if defined(TRANSLATE) && !defined(HEBREW)
#define HEBREW 1
#endif

#if defined(PUBLIC_ACCESS)
#undef EXEC_COMMAND
#undef WANT_TCL
#undef WANT_FTP
#endif

#if defined(WINNT) || defined(__EMX__) || defined(GUI)
#undef WANT_DETACH
#endif

/*
 * This is the filename of the identd file to use
 */
#ifdef CIDENTD
#define DEFAULT_IDENT_HACK ".authlie"
#elif defined(WDIDENT)
#define DEFAULT_IDENT_HACK ".noident"
#else
#define DEFAULT_IDENT_HACK ".noident"
#endif

#if !defined(WANT_CHATNET)
#undef WANT_CHATNET      /* define just for codelogic */
#endif

#if defined(_USE_LOCAL_CONFIG) && !defined(SHOULD_NOTIFY_BITCHX_COM)
#undef WANT_NOTIFY_BITCHX_COM
#endif

#undef PARANOID		/* #define this if your paranoid about dcc hijacking */
#undef WANT_CHAN_NICK_SERV	/* do we want to include some chan/nick/oper server commands */

/* new epic stuff */
#define OLD_STATUS_S_EXPANDO_BEHAVIOR
#define DEFAULT_NEW_SERVER_LASTLOG_LEVEL "NONE"
#define DEFAULT_RANDOM_LOCAL_PORTS 0
#define DEFAULT_RANDOM_SOURCE 0
#define DEFAULT_TERM_DOES_BRIGHT_BLINK 0


#if defined(BITCHX_LITE) && defined(WANT_TCL)
#undef BITCHX_LITE
#endif

#if defined(BITCHX_LITE)
#undef WANT_DLL
#undef WANT_TRANSLATE
#undef WANT_DETACH
#undef WANT_NSLOOKUP
#undef HEBREW
#undef CLOAKED
#undef WANT_OPERVIEW
#undef WANT_EPICHELP
#undef WANT_LLOOK
#undef WANT_CDCC
#undef WANT_FTP
#undef WANT_TABKEY
#undef WANT_CHELP
#undef WANT_USERLIST
#undef COMPRESS_MODES
#undef ALLOW_DETACH
#endif

#define DEFAULT_TKLINE_TIME 10
#define DEFAULT_BOTCHAR '.'		/* default char to enter dcc chat. */
					/* oper serv's tend to send .'s */

#define WANT_CORE

#undef OFF
#undef ON

#include "color.h"		/* all color options here. */

#endif /* __config_h_ */
