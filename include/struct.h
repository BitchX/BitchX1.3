/*
 * struct.h: header file for structures needed for prototypes
 *
 * Written by Scott Reynolds, based on code by Michael Sandrof
 * Heavily modified by Colten Edwards for BitchX
 *
 * Copyright(c) 1997
 *
 */

#ifndef __struct_h_
#define	__struct_h_

#ifdef WINNT
#include <windows.h>
#endif

#include "alist.h"
#include "hash.h"
#include "config.h"
#include "ssl.h"
#include <netinet/in.h>

/*
 * struct sockaddr_storage isn't avaiable on all ipv6-ready-systems, bleh ;(
 * and i'm too lazy to #ifdef every sockaddr declaration. --wojtekka
 */

struct sockaddr_foobar
{
       union {
               struct sockaddr_in sin;
#ifdef IPV6
           struct sockaddr_in6 sin6;
#endif
       } sins;
};

#define sf_family sins.sin.sin_family
#define sf_port sins.sin.sin_port
#define sf_addr sins.sin.sin_addr
#ifdef IPV6
#      define sf_addr6 sins.sin6.sin6_addr
#endif


#ifdef GUI
typedef struct _menuref {
	int refnum, menuid;
#if defined(__EMXPM__) || defined(WIN32)
	HWND menuhandle;
#elif defined(GTK)
	GtkWidget *menuhandle;
	char *menutext;
	int checked;
#endif
	struct _menuref *next;
} MenuRef;

typedef struct _menu_item
{
	struct _menu_item	*next;
	char			*name;
	char			*alias;
	char			*submenu;
	int			menuid;
	int			menutype;
	int			refnum;
} MenuList;                                

typedef struct	_menu_struct
{
	struct _menu_struct	*next;
	char			*name;
	MenuList		*menuorigin;
#if defined(__EMXPM__) || defined(WIN32)
	HWND			sharedhandle;
#elif defined(GTK)
	GtkWidget		*sharedhandle;
#endif
	 MenuRef		*root;
} MenuStruct;
#endif


typedef struct 
{
	int is_read;
	int is_write;
	int port;
	char *server;
	unsigned long flags;
	time_t time;
	void (*func_read) (int);
	void (*func_write) (int);
	void (*cleanup) (int);
	void *info;
#if defined(HAVE_SSL) && !defined(IN_MODULE)
	SSL_CTX* ctx;
	int ssl_error;
	SSL* ssl_fd;
#endif
} SocketList;

typedef char *(bf) (char *, char *);
typedef struct 
{
	char    *name;
	bf      *func;
}       BuiltInFunctions;

typedef struct _BuiltInDllFunctions
{
	struct _BuiltInDllFunctions *next;
	char    *name;
	char	*module;
	bf      *func;
}       BuiltInDllFunctions;
                
typedef enum NoiseEnum {
	UNKNOWN = 0,
	SILENT,
	QUIET,
	NORMAL,
	NOISY
} Noise;
                                
/* Hook: The structure of the entries of the hook functions lists */
typedef struct	hook_stru
{
struct	hook_stru *next;

	char	*nick;			/* /on type NICK stuff */
	char	*stuff;			/* /on type nick STUFF */

	int	not;			/* /on type ^nick stuff */
	Noise	noisy;			/* /on [^-+]type nick stuff */

	int	sernum;			/* /on #type NUM nick stuff */
					/* Default sernum is 0. */

	int	global;			/* set if loaded from `global' */
	int	flexible;		/* on type 'NICK' stuff */
	int	debug;			/* turn debug on/off */
	char	*filename;		/* Where it was loaded */
	int	(*hook_func) (char *, char *, char **);
	int	(*num_func) (int, char *, char **);
}	Hook;

/* HookFunc: A little structure to keep track of the various hook functions */
typedef struct
{
	char	*name;			/* name of the function */
	Hook	*list;			/* pointer to head of the list for this
					 * function */
	int	params;			/* number of parameters expected */
	int	mark;
	unsigned flags;
}	HookFunc;

typedef struct _NumericFunction
{
	struct _NumericFunction *next;
	char	*name;
	char	*module;
	int	number;
	Hook	*list;
}       NumericFunction;
                
typedef struct _RawFunction
{
	struct _RawFunction *next;
	char	*name;
	char	*module;
	int	(*func) (char *, char *, char *, char **);
} RawDll;

/* UrlList: structure for the urls in your Url List */
typedef struct  urllist_stru
{
	struct  urllist_stru *next;     /* pointer to next url entry */
	char    *name;                  /* name */
} UrlList;

/* IrcCommand: structure for each command in the command table */
typedef	struct
{
	char	*name;					/* what the user types */
	char	*server_func;				/* what gets sent to the server
							 * (if anything) */
	void	(*func) (char *, char *, char *, char *);	/* function that is the command */
	unsigned	flags;
	char	*help;
}	IrcCommand;


typedef	struct _IrcCommandDll
{
	struct  _IrcCommandDll *next;			/* pointer to next record. */
	char	*name;					/* what the user types */
	char	*module;
	char	*server_func;				/* what gets sent to the server
							 * (if anything) */
	void	(*func) (struct _IrcCommandDll *, char *, char *, char *, char *);	/* function that is the command */
	unsigned	flags;
	char	*result;
	char	*help;
}	IrcCommandDll;

typedef struct _WindowDll
{
	struct _WindowDll *next;
	char	*name;
	char	*module;
	struct WindowStru *(*func) (struct WindowStru *, char **, char *);
	char	*help;
} WindowDll;

typedef struct _last_msg_stru
{
	struct _last_msg_stru *next;
	char    *from;
	char	*uh;
	char	*to;
	char    *last_msg;
	char	*time;
	int     count;
} LastMsg;
                                
typedef struct  userlist_stru
{
	struct	userlist_stru	*next;	/* pointer to next user entry */
	char	*nick;			/* user's name in nick!user@host */
	char	*host;
	char	*comment;
	char	*channels;		/* channel for list to take effect */
	char	*password;		/* users password */
unsigned long	flags;			/* this users flags */
	time_t	time;			/* time when put on list */
}	UserList;

/* ShitList: structure proto for the shitlist */
typedef struct  shitlist_stru
{
	struct	shitlist_stru	*next;	/* pointer to next shit entry */
	char	*filter;		/* filter in nick!user@host */
	int	level;			/* level of shitted */
	char	*channels;		/* channel for list to take effect */
	char	*reason;		/* Reason */
	time_t	time;			/* time shit was put on */
}	ShitList;

/* WordKickList: structure for your wordkick list */
typedef struct  wordkicklist_stru
{
	struct	wordkicklist_stru *next;	/* pointer to next user entry */
	char	*string;			/* string */
	char	*channel;			/* channel */
}	WordKickList;

/* LameList: structure for the users on your LameNick Kick*/
typedef struct  lamelist_stru
{
	struct	lamelist_stru	*next;	/* pointer to next lame entry */
	char	*nick;			/* Lame Nick */
}	LameList;

/* invitetoList: structure for the invitetolist list */
typedef struct  invitetolist_stru
{
	struct	invitetolistlist_stru	*next;	/* pointer to next entry */
	char    *channel;			/* channel */
	int     times;				/* times I have been invited */
	time_t  time;				/* time of last invite */
}	InviteToList;

typedef struct server_split
{
	struct server_split *next;
	char *name;	/* name of this server. */
	char *link;	/* linked to what server */
	int status;	/* split or not */
	int count;	/* number of times we have not found this one */
	int hopcount; 	/* number of hops away */
	time_t time; 	/* time of split */
} irc_server;

/*
 * ctcp_entry: the format for each ctcp function.   note that the function
 * described takes 4 parameters, a pointer to the ctcp entry, who the message
 * was from, who the message was to (nickname, channel, etc), and the rest of
 * the ctcp message.  it can return null, or it can return a malloced string
 * that will be inserted into the oringal message at the point of the ctcp.
 * if null is returned, nothing is added to the original message

 */
struct _CtcpEntry;

typedef char *((*CTCP_Handler) (struct _CtcpEntry *, char *, char *, char *));

typedef	struct _CtcpEntry
{
	char		*name;  /* name of ctcp datatag */
	int		id;	/* index of this ctcp command */
	int		flag;	/* Action modifiers */
	char		*desc;  /* description returned by ctcp clientinfo */
	CTCP_Handler 	func;	/* function that does the dirty deed */
	CTCP_Handler 	repl;	/* Function that is called for reply */
}	CtcpEntry;

struct _CtcpEntryDll;

typedef char *((*CTCP_DllHandler) (struct _CtcpEntryDll *, char *, char *, char *));

typedef	struct _CtcpEntryDll
{
	struct		_CtcpEntryDll *next;
	char		*name;  /* name of ctcp datatag */
	char		*module; /* name of module associated with */
	int		id;	/* index of this ctcp command */
	int		flag;	/* Action modifiers */
	char		*desc;  /* description returned by ctcp clientinfo */
	CTCP_DllHandler	func;	/* function that does the dirty deed */
	CTCP_DllHandler	repl;	/* Function that is called for reply */
}	CtcpEntryDll;




struct transfer_struct {
	unsigned short packet_id;
	unsigned char byteorder;
	u_32int_t byteoffset;
}; 


typedef struct _File_Stat {
	struct _File_Stat *next;
	char *filename;
	long filesize;
} FileStat;

typedef struct _File_List {
	struct _File_List *next;
	char * description;
	char * notes;
	FileStat *filename;
	char * nick;
	int packnumber;
	int numberfiles;
	double filesize;
	double minspeed;
	int gets;
	time_t timequeue;
} FileList;

#if 0
typedef	struct	DCC_struct
{
	struct DCC_struct *next;
	char		*user;
	char		*userhost;
	unsigned int	flags;
	int		read;
	int		write;
	int		file;

	u_32int_t	filesize;

	int		dccnum;
	int		eof;
	char		*description;
	char		*othername;
	struct in_addr	remote;
	u_short		remport;
	u_short		local_port;
	u_32int_t	bytes_read;
	u_32int_t	bytes_sent;

	int				window_sent;
	int				window_max;

	int		in_dcc_chat;
	int		echo;
	int		in_ftp;
	int		dcc_fast;
	
	
	struct timeval	lasttime;
	struct timeval	starttime;
	char		*buffer;
	char		*cksum;
	char		*encrypt;
	char		*dccbuffer;
	
	u_32int_t	packets_total;
	u_32int_t	packets_transfer;
	struct transfer_struct transfer_orders;
	void (*dcc_handler) (struct DCC_struct *, char *);

}	DCC_list;
#endif

/* Hold: your general doubly-linked list type structure */

typedef struct HoldStru
{
	char	*str;
	struct	HoldStru	*next;
	struct	HoldStru	*prev;
	int	logged;
}	Hold;

typedef struct	lastlog_stru
{
	int	level;
	char	*msg;
	time_t	time;
	struct	lastlog_stru	*next;
	struct	lastlog_stru	*prev;
}	Lastlog;

struct	ScreenStru;	/* ooh! */

#define NICK_CHANOP 	0x0001
#define NICK_HALFOP	0x0002
#define NICK_AWAY	0x0004
#define NICK_VOICE	0x0008
#define NICK_IRCOP	0x0010

#define nick_isop(s) (s->flags & NICK_CHANOP)
#define nick_isvoice(s) (s->flags & NICK_VOICE)
#define nick_ishalfop(s) (s->flags & NICK_HALFOP)
#define nick_isaway(s) (s->flags & NICK_AWAY)
#define nick_isircop(s) (s->flags & NICK_IRCOP)

typedef unsigned int my_uint;

/* NickList: structure for the list of nicknames of people on a channel */
typedef struct nick_stru
{
	struct	nick_stru	*next;	/* pointer to next nickname entry */
	char	*nick;			/* nickname of person on channel */
	char	*host;
	char	*ip;
	char	*server;
	int	serverhops;
	my_uint	ip_count;
	UserList *userlist;
	ShitList *shitlist;

	my_uint flags;
#if 0
	int	chanop;			/* True if the given nick has chanop */
	int	halfop;
	int	away;
	int	voice;
	int	ircop;
#endif	
	time_t	idle_time;

	my_uint	floodcount;
	time_t	floodtime;

	my_uint	nickcount;
	time_t  nicktime;

	my_uint	kickcount;
	time_t	kicktime;

	my_uint	joincount;
	time_t	jointime;

	my_uint	dopcount;
	time_t	doptime;

	my_uint	bancount;
	time_t	bantime;


	time_t	created;

	my_uint	stat_kicks;		/* Total kicks done by user */
	my_uint	stat_dops;		/* Total deops done by user */
	my_uint	stat_ops;		/* Total ops done by user */
	my_uint	stat_hops;
	my_uint	stat_dhops;
	my_uint	stat_eban;
	my_uint	stat_uneban;
	my_uint	stat_bans;		/* Total bans done by user */
	my_uint	stat_unbans;		/* Total unbans done by user */
	my_uint	stat_nicks;		/* Total nicks done by user */
	my_uint	stat_pub;		/* Total publics sent by user */
	my_uint	stat_topics;		/* Total topics set by user */

	my_uint	sent_reop;
	time_t	sent_reop_time;
	my_uint	sent_voice;
	time_t	sent_voice_time;
	
	my_uint	sent_deop;
	time_t	sent_deop_time;
	my_uint	need_userhost;		/* on join we send a userhost for this nick */	
	my_uint	check_clone;		/* added for builtin clone detect */
}	NickList;

typedef	struct	DisplayStru
{
	char	*line;
	int	linetype;
	struct	DisplayStru	*next;
	struct	DisplayStru	*prev;
}	Display;

typedef struct WinSetStru
{
/* These are copied over from /set's */
	char	*status_mode;
	char	*status_topic;
	char	*status_umode;
	char	*status_hold_lines;
	char	*status_hold;
	char	*status_voice;
	char	*status_channel;
	char	*status_notify;
	char	*status_oper_kills;
	char	*status_lag;
	char	*status_mail;
	char	*status_query;
	char	*status_server;
	char	*status_clock;
	char	*status_users;
	char	*status_away;
	char	*status_dcccount;
	char	*status_cdcccount;
	char	*status_chanop;
	char	*status_cpu_saver;
	char	*status_msgcount;
	char	*status_nick;	
	char	*status_flag;
	char	*status_halfop;
							
/* These are the various formats from a window make_status() creates these */ 
	char    *mode_format;
	char    *umode_format;
	char    *topic_format;
	char    *query_format;
	char    *clock_format;
	char    *hold_lines_format;
	char    *channel_format;
	char    *mail_format;
	char    *server_format;
	char    *notify_format;
	char    *kills_format;
	char    *status_users_format;
	char    *lag_format;
	char	*cpu_saver_format;
	char	*msgcount_format;
	char	*dcccount_format;
	char	*cdcc_format;
	char	*nick_format;
	char	*flag_format;
	char	*away_format;
	
#define MAX_FUNCTIONS		36
	char	*status_user_formats0;
	char	*status_user_formats1;
	char	*status_user_formats2;
	char	*status_user_formats3;
	char	*status_user_formats4;
	char	*status_user_formats5;
	char	*status_user_formats6;
	char	*status_user_formats7;
	char	*status_user_formats8;
	char	*status_user_formats9;
	char	*status_user_formats10;
	char	*status_user_formats11;
	char	*status_user_formats12;
	char	*status_user_formats13;
	char	*status_user_formats14;
	char	*status_user_formats15;
	char	*status_user_formats16;
	char	*status_user_formats17;
	char	*status_user_formats18;
	char	*status_user_formats19;
	char	*status_user_formats20;
	char	*status_user_formats21;
	char	*status_user_formats22;
	char	*status_user_formats23;
	char	*status_user_formats24;
	char	*status_user_formats25;
	char	*status_user_formats26;
	char	*status_user_formats27;
	char	*status_user_formats28;
	char	*status_user_formats29;
	char	*status_user_formats30;
	char	*status_user_formats31;
	char	*status_user_formats32;
	char	*status_user_formats33;
	char	*status_user_formats34;
	char	*status_user_formats35;
	char	*status_user_formats36;
	char	*status_user_formats37;
	char	*status_user_formats38;
	char	*status_user_formats39;
	char	*status_scrollback;
	char	*status_window;
		
	char	*status_line[3];	/* The status lines string current display */
	char	*status_format[4];	/* holds formated status info from build_status */
	char	*format_status[4];	/* holds raw format for window from /set */	

	char	*window_special_format;
}	 WSet;

typedef	struct	WindowStru
{
	char			*name;
	unsigned int	refnum;		/* the unique reference number,
					 * assigned by IRCII */
	int	server;			/* server index */
	int	last_server;		/* previous server index */
	int	top;			/* The top line of the window, screen
					 * coordinates */
	int	bottom;			/* The botton line of the window, screen
					 * coordinates */
	int	cursor;			/* The cursor position in the window, window
					 * relative coordinates */
	int	line_cnt;		/* counter of number of lines displayed in
					 * window */
	int	absolute_size;
	int	noscroll;		/* true, window scrolls... false window wraps */
	int	scratch_line;		/* True if a scratch window */
	int	old_size;		/* if new_size != display_size, resize_display */
	int	visible;		/* true, window ise, window is drawn... false window is hidden */
	int	update;			/* window needs updating flag */
	int	repaint_start;
	int	repaint_end;
	unsigned miscflags;		/* Miscellaneous flags. */
	int	beep_always;		/* should this window beep when hidden */
	unsigned long notify_level;
	unsigned long window_level;		/* The LEVEL of the window, determines what
						 * messages go to it */
	int	skip;
	int	columns;	
	char	*prompt;		/* A prompt string, usually set by EXEC'd process */
	int	double_status;		/* number of status lines */
	int	status_split;		/* split status to top and bottom */
	int	status_lines;		/* replacement for menu struct */
	
	char    *(*status_func[4][MAX_FUNCTIONS]) (struct WindowStru *);
	int	func_cnt[4];
	WSet	*wset;
			

	Display *top_of_scrollback,	/* Start of the scrollback buffer */
		*top_of_display,	/* Where the viewport starts */
		*ceiling_of_display,	/* the furthest top of display */
		*display_ip,		/* Where next line goes in rite() */
		*scrollback_point,
		*screen_hold;	/* Where t_o_d was at start of sb */
	int	display_buffer_size;	/* How big the scrollback buffer is */
	int	display_buffer_max;	/* How big its supposed to be */
	int	display_size;		/* How big the window is - status */

	int	lines_scrolled_back;	/* Where window is relatively */

	int	hold_mode;		/* True if we want to hold stuff */
	int	holding_something;	/* True if we ARE holding something */
	int	held_displayed;		/* lines displayed since last hold */
	int	lines_displayed;	/* Lines held since last unhold */
	int	lines_held;		/* Lines currently being held */
	int	last_lines_held;	/* Last time we updated "lines held" */
	int	distance_from_display;

	char	*current_channel;	/* Window's current channel */
	char	*waiting_channel;
	char	*bind_channel;
	char	*query_nick;		/* User being QUERY'ied in this window */
	char	*query_host;
	char	*query_cmd;
		
	NickList *nicks;		/* List of nicks that will go to window */

	/* lastlog stuff */
	Lastlog	*lastlog_head;		/* pointer to top of lastlog list */
	Lastlog	*lastlog_tail;		/* pointer to bottom of lastlog list */
	unsigned long lastlog_level;	/* The LASTLOG_LEVEL, determines what
					 * messages go to lastlog */
	int	lastlog_size;		/* number of messages in lastlog */
	int	lastlog_max;		/* Max number of msgs in lastlog */
	
	char	*logfile;		/* window's logfile name */
	/* window log stuff */
	int	log;			/* true, file logging for window is on */
	FILE	*log_fp;		/* file pointer for the log file */

	int	window_display;		/* should we display to this window */

	void	(*output_func)	(struct WindowStru *, const unsigned char *);
	void	(*status_output_func)	(struct WindowStru *);
	
	struct	ScreenStru	*screen;
	struct	WindowStru	*next;	/* pointer to next entry in window list (null
					 * is end) */
	struct	WindowStru	*prev;	/* pointer to previous entry in window list
					 * (null is end) */
	int	deceased;		/* set when window is killed */
	int	in_more;
	int	save_hold_mode;
	int	mangler;
	void	(*update_status)	(struct WindowStru *);
	void	(*update_input) (struct WindowStru *);
}	Window;

/*
 * WindowStack: The structure for the POP, PUSH, and STACK functions. A
 * simple linked list with window refnums as the data 
 */
typedef	struct	window_stack_stru
{
	unsigned int	refnum;
	struct	window_stack_stru	*next;
}	WindowStack;

typedef	struct
{
	int	top;
	int	bottom;
	int	position;
}	ShrinkInfo;

typedef struct PromptStru
{
	char	*prompt;
	char	*data;
	int	type;
	int	echo;
	void	(*func) (char *, char *);
	struct	PromptStru	*next;
}	WaitPrompt;


typedef	struct	ScreenStru
{
	int	screennum;
	Window	*current_window;
	unsigned int	last_window_refnum;	/* reference number of the
						 * window that was last
						 * the current_window */
	Window	*window_list;			/* List of all visible
						 * windows */
	Window	*window_list_end;		/* end of visible window
						 * list */
	Window	*cursor_window;			/* Last window to have
						 * something written to it */
	int	visible_windows;		/* total number of windows */
	WindowStack	*window_stack;		/* the windows here */

	struct	ScreenStru *prev;		/* These are the Screen list */
	struct	ScreenStru *next;		/* pointers */


	FILE	*fpin;				/* These are the file pointers */
	int	fdin;				/* and descriptions for the */
	FILE	*fpout;				/* screen's input/output */
	int	fdout;

	char	input_buffer[INPUT_BUFFER_SIZE+2];	/* the input buffer */
	int	buffer_pos;			/* and the positions for the */
	int	buffer_min_pos;			/* screen */

	int	input_cursor;
	char	*input_prompt;

	int     input_visible;
	int     input_zone_len;
	int     input_start_zone;
	int     input_end_zone;
	int     input_prompt_len;
	int     input_prompt_malloc;
	int     input_line;
	Lastlog *lastlog_hold;
		
	char	saved_input_buffer[INPUT_BUFFER_SIZE+2];
	int	saved_buffer_pos;
	int	saved_min_buffer_pos;

	WaitPrompt	*promptlist;



	int	meta_hit;
	int	quote_hit;			/* true if a key bound to
						 * QUOTE_CHARACTER has been
						 * hit. */
	int	digraph_hit;			/* A digraph key has been hit */
	unsigned char	digraph_first;


	char	*redirect_name;
	char	*redirect_token;
	int	redirect_server;

	char	*tty_name;
	int	co;
	int	li;
	int	old_co;
	int	old_li;
#ifdef WINNT
	HANDLE  hStdin;
	HANDLE  hStdout;
#endif
#ifdef GUI
	int     nicklist;
#endif
#if defined(__EMXPM__) || defined(WIN32)
#ifndef WIN32
	HVPS	hvps,
			hvpsnick;
#endif
	HWND	hwndFrame,
			hwndClient,
			hwndMenu,
			hwndLeft,
			hwndRight,
			hwndnickscroll,
			hwndscroll;
	int		VIO_font_width,
			VIO_font_height,
			spos, mpos, codepage;
	char	aviokbdbuffer[256];
#elif defined(GTK)
	GtkWidget	*window,
			*viewport,
			*menubar,
			*scroller,
			*clist,
			*scrolledwindow,
			*box;
	GdkFont			*font;
	char            *fontname;
	GtkAdjustment	*adjust;
	GtkNotebookPage	*page;
	gint		gtkio;
	int 		pipe[2];
	int 		maxfontwidth,
				maxfontheight;
#endif

	char	*menu;		/* windows menu struct */

	int	alive;
}	Screen;

/* BanList: structure for the list of bans on a channel */
typedef struct ban_stru
{
	struct	ban_stru	*next;  /* pointer to next ban entry */
	char	*ban;			/* the ban */
	char	*setby;			/* person who set the ban */
	int	sent_unban;		/* flag if sent unban or not */
	time_t	sent_unban_time;	/* sent unban's time */
	time_t	time;			/* time ban was set */
	int	count;
}	BanList;

typedef struct _cset_stru
{
	struct _cset_stru *next;
	char	*channel;
	int	set_aop;		/* channel specific /set */
	int	set_annoy_kick;		/* channel specific /set */
	int	set_ainv;		/* channel specific /set */
	int	set_auto_join_on_invite;
	int	set_auto_rejoin;	/* channel specific /set */
	int	set_ctcp_flood_ban;
	int	set_deop_on_deopflood;	/* channel specific /set */
	int	set_deop_on_kickflood;	/* channel specific /set */
	int	set_deopflood;		/* channel specific /set */
	int	set_deopflood_time;	/* channel specific /set */
	int	set_hacking;		/* channel specific /set */
	int	set_kick_on_deopflood;	/* channel specific /set */
	int	set_kick_on_joinflood;
	int	set_kick_on_kickflood;	/* channel specific /set */
	int	set_kick_on_nickflood;	/* channel specific /set */
	int	set_kick_on_pubflood;	/* channel specific /set */
	int	set_kickflood;		/* channel specific /set */
	int	set_kickflood_time;	/* channel specific /set */
	int	set_nickflood;		/* channel specific /set */
	int	set_nickflood_time;	/* channel specific /set */
	int	set_joinflood;		/* channel specific /set */
	int	set_joinflood_time;	/* channel specific /set */
	int	set_pubflood;		/* channel specific /set */
	int	set_pubflood_ignore;	/* channel ignore time val */
	int	set_pubflood_time;	/* channel specific /set */
	int	set_userlist;		/* channel specific /set */
	int	set_shitlist;		/* channel specific /set */
	int	set_lame_ident;		/* channel specific /set */
	int	set_lamelist;		/* channel specific /set */
	int	set_kick_if_banned;     /* channel specific /set */
	int	bitch_mode;		/* channel specific /set */
	int	compress_modes;		/* channel specific /set */
	int	set_kick_ops;
	int	set_auto_limit;
	int	channel_log;
	int	set_bantime;
	char	*channel_log_file;
	char	*chanmode;
	unsigned long channel_log_level;
	char	*log_level;
} CSetList;

typedef struct chan_flags_stru {

	unsigned int got_modes : 1;
	unsigned int got_who : 1;
	unsigned int got_bans : 1;
	unsigned int got_exempt : 1;
} chan_flags;

/* ChannelList: structure for the list of channels you are current on */
typedef	struct	channel_stru
{
	struct	channel_stru	*next;	/* pointer to next channel entry */
	char	*channel;		/* channel name */
	Window	*window;		/* the window that the channel is "on" */
	int	refnum;			/* window refnum */
	int	server;			/* server index for this channel */
	u_long	mode;			/* Current mode settings for channel */
	u_long	i_mode;			/* channel mode for cached string */
	char	*s_mode;		/* cached string version of modes */
	char	*topic;
	int	topic_lock;
		
	char 	*modelock_key;
	long	modelock_val;
	
	int	limit;			/* max users for the channel */
	time_t	limit_time;		/* time of last limit set */
	char	*key;			/* key for this channel */
	char	have_op;		/* true if you are a channel op */
	char	hop;			/* true if you are a half op */
	char	voice;			/* true if you are voice */
	char	bound;			/* true if channel is bound */
	char	*chanpass;		/* if TS4 then this has the channel pass */
	char	connected;		/* true if this channel is actually connected */

	
	HashEntry	NickListTable[NICKLIST_HASHSIZE];

	chan_flags	flags;


	time_t	max_idle;		/* max idle time for this channel */
	int	tog_limit;
	int	check_idle;		/* toggle idle check */
	int	do_scan;		/* flag for checking auto stuff */
	struct timeval	channel_create;		/* time for channel creation */
	struct timeval	join_time;		/* time of last join */


	int	stats_ops;		/* total ops I have seen in channel */
	int	stats_dops;		/* total dops I have seen in channel */
	int	stats_bans;		/* total bans I have seen in channel */
	int	stats_unbans;		/* total unbans I have seen in channel */

	int	stats_sops;		/* total server ops I have seen in channel */
	int	stats_sdops;		/* total server dops I have seen in channel */
	int	stats_shops;
	int	stats_sdehops;
	int	stats_sebans;
	int	stats_sunebans;
	int	stats_sbans;		/* total server bans I have seen in channel */
	int	stats_sunbans;		/* total server unbans I have seen in channel */

	int	stats_topics;		/* total topics I have seen in channel */
	int	stats_kicks;		/* total kicks I have seen in channel */
	int	stats_pubs;		/* total pubs I have seen in channel */
	int	stats_parts;		/* total parts I have seen in channel */
	int	stats_signoffs;		/* total signoffs I have seen in channel */
	int	stats_joins;		/* total joins I have seen in channel */
	int	stats_ebans;
	int	stats_unebans;
	int	stats_chanpass;
	int	stats_hops;
	int	stats_dhops;
		
	CSetList *csets;		/* All Channel sets */

	
	
	int	msglog_on;
	FILE	*msglog_fp;
	char	*logfile;
	unsigned long log_level;
		
	int	totalnicks;		/* total number of users in channel */
	int	maxnicks;		/* max number of users I have seen */
	time_t	maxnickstime;		/* time of max users */

	int	totalbans;		/* total numbers of bans on channel */


	BanList	*bans;			/* pointer to list of bans on channel */
	BanList	*exemptbans;		/* pointer to list of bans on channel */
	int	maxbans;		/* max number of bans I have seen */
	time_t	maxbanstime;		/* time of max bans */
	struct {
		char 	*op;
		int	type;
	} cmode[4];

	char	*mode_buf;
	int	mode_len;	

}	ChannelList;

typedef	struct	list_stru
{
	struct	list_stru	*next;
	char	*name;
}	List;

typedef struct	flood_stru
{
	struct flood_stru	*next;
	char	*name;
	char	*host;
	char	*channel;
	int	type;
	char	flood;
	unsigned long	cnt;
	time_t	start;
}	Flooding;


typedef	struct	_ajoin_list
{
	struct	_ajoin_list *next;
	char	*name;
	char	*key;
	char	*group;
	int	server;
	int	window;
	int	ajoin_list;
}	AJoinList;

/* a structure for the timer list */
typedef struct	timerlist_stru
{
	struct	timerlist_stru *next;
	char	ref[REFNUM_MAX + 1];
	unsigned long refno;
struct	timeval	time;
	int	(*callback) (void *, char *);
	char	*command;
	char	*subargs;
	int	events;
	time_t	interval;
	int	server;
	int	window;
	char	*whom;
	int	delete;
}	TimerList;

typedef struct nicktab_stru
{
	struct nicktab_stru *next;
	char *nick;
	char *type;
} NickTab;

typedef struct clonelist_stru
{
	struct clonelist_stru *next;
	char *number;
	char *server;
	int  port;
	int  socket_num;
	int  warn;
} CloneList;

typedef struct IgnoreStru
{
	struct IgnoreStru *next;
	char *nick;
	long type;
	long dont;
	long high;
	long cgrep;
	int num;
	char *pre_msg_high;
	char *pre_nick_high;
	char *post_high;
	struct IgnoreStru *looking;
	struct IgnoreStru *except;
} Ignore;

/* IrcVariable: structure for each variable in the variable table */
typedef struct
{
	char	*name;			/* what the user types */
	u_32int_t hash;
	int	type;			/* variable types, see below */
	int	integer;		/* int value of variable */
	char	*string;		/* string value of variable */
	void	(*func)(Window *, char *, int);		/* function to do every time variable is set */
	char	int_flags;		/* internal flags to the variable */
	unsigned short	flags;		/* flags for this variable */
}	IrcVariable;

/* IrcVariableDll: structure for each variable in the dll variable table */
typedef struct _ircvariable
{
	struct _ircvariable *next;
	char	*name;			/* what the user types */
	char	*module;
	int	type;			/* variable types, see below */
	int	integer;		/* int value of variable */
	char	*string;		/* string value of variable */
	void	(*func)(Window *, char *, int);	/* function to do every time variable is set */
	char	int_flags;		/* internal flags to the variable */
	unsigned short	flags;		/* flags for this variable */
}	IrcVariableDll;

typedef struct _virtuals_struc
{
	struct _virtuals_struc *next;
	char *address;      /* IPv4 or IPv6 address */
	char *hostname;
} Virtuals;


#define ALIAS_MAXARGS 32
	
struct ArgListT {
	char *	vars[ALIAS_MAXARGS];
	char *	defaults[ALIAS_MAXARGS];
	int	void_flag;
	int	dot_flag;
};

typedef struct ArgListT ArgList;

typedef struct  AliasItemStru
{
	char    *name;                  /* name of alias */
	u_32int_t hash;
	char    *stuff;                 /* what the alias is */
	char    *stub;                  /* the file its stubbed to */
	int     global;                 /* set if loaded from global' */
	int	cache_revoked;		/* Cache revocation index. */
	int	debug;			/* debug invoke? */
	ArgList *arglist;
}	Alias;

typedef	struct	notify_stru
{
	char	*nick;			/* Who are we watching? */
	u_32int_t hash;
	char 	*host;
	char	*looking;
	int	times;
	time_t	lastseen;
	time_t	period;
	time_t	added;
	int	flag;			/* Is the person on irc? */
} NotifyItem;


typedef struct notify_alist
{
	struct notify_stru	**list;
	int			max;
	int			max_alloc;
	alist_func 		func;
	hash_type		hash;
	char *			ison;
} NotifyList;

typedef Window *(*window_func) (Window *, char **args, char *usage);

typedef struct window_ops_T {
	char		*command;
	window_func	func;
	char		*usage;
} window_ops;
                        

typedef struct command_struct
{
	IrcCommand		*command_list;
	window_ops		*window_commands;
	BuiltInFunctions	*functions;
	IrcVariable		*variables;
	IrcVariable		*fsets;
/*	CSetArray		*csets;
	WSetArray		*wsets;*/
	HashEntry		*parse;
} CommandStruct;

typedef void (*dcc_function) (char *, char *);

typedef struct _DCC_dllcommands
{
	struct	_DCC_dllcommands *next;
	char		*name;
	dcc_function	function;
	char		*help;
	char		*module;
} DCC_dllcommands;

typedef struct _dcc_internal {
	u_32int_t	struct_type;		/* type of socket */
	char		*user;			/* user being dcc'd */ 
	char		*userhost;		/* possible userhost */
	int		server;			/* server from which this user came from */
	char		*encrypt;		/* password used */
	char		*filename;		/* filename without path or type*/
	char		*othername;		/* possible other info */
	u_32int_t	bytes_read;		/* number of bytes read */
	u_32int_t	bytes_sent;		/* number of bytes sent */
	struct timeval	starttime;		/* when did this dcc start */
	struct timeval	lasttime;		/* last time of activity */
	struct transfer_struct transfer_orders;	/* structure for resending files */
	int		file;			/* file handle open file */
	u_32int_t	filesize;		/* the filesize to get */
	u_32int_t	packets;		/* number of blocksize packets recieved */
	int		eof;			/* in EOF condition. */
	int		blocksize;		/* this dcc's blocksize */
	int		dcc_fast;		/* set if non-blocking used */
	short		readwaiting; 		/* expect a data on the port */
	unsigned short	remport;		/* the remport we are connected to */
	unsigned short	localport;		/* the localport we are on */
	struct	in_addr	remote;			/* this dcc's remote address */
	unsigned int	dccnum;			/* dcc number we are at */
	UserList	*ul;			/* is this person on the userlist */
} DCC_int;

typedef struct _dcc_struct_type {
	u_32int_t 	struct_type;
} dcc_struct_type;
                       
#endif /* __struct_h_ */
