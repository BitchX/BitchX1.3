/*
 * irc.h: header file for all of ircII! 
 *
 * Written By Michael Sandrof
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: irc.h 206 2012-06-13 12:34:32Z keaston $
 */

#ifndef __irc_h
#define __irc_h
#define IRCII_COMMENT   "\002 Keep it to yourself!\002"
#define BUG_EMAIL "<bitchx-devel@lists.sourceforge.net>"

#define FSET 1

#ifndef __irc_c
extern const char irc_version[];
extern const char internal_version[];
#endif
extern char	*thing_ansi;
extern char	thing_star[4];

/*
 * Here you can set the in-line quote character, normally backslash, to
 * whatever you want.  Note that we use two backslashes since a backslash is
 * also C's quote character.  You do not need two of any other character.
 */
#define QUOTE_CHAR '\\'

#include "defs.h"
#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>

#include <signal.h>
#include <sys/param.h>

#ifdef __EMX__
# ifdef __EMXPM__
#   define AVIO_BUFFER 2048
#   define INCL_GPI
#   define INCL_AVIO
#   define INCL_DOS
# endif
#define INCL_WIN       /* Window Manager Functions */
#define INCL_BASE
#define INCL_VIO
#include <os2.h>
#elif defined(WINNT)
#  include <windows.h>
#  ifdef SOUND
#    include <mmsystem.h>
#  endif
#elif defined(GTK)
#  include <gtk/gtk.h>
#  include <gtk/gtkmenu.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif /* HAVE_SYS_TIME_H */
#endif /* TIME_WITH_SYS_TIME */

#ifdef HAVE_SYS_FCNTL_H
# include <sys/fcntl.h>
#else
  #ifdef HAVE_FCNTL_H
  #include <fcntl.h> 
  #endif /* HAVE_FCNTL_H */
#endif

#include <stdarg.h>
#include <unistd.h>
#ifdef __EMX__
#include <sys/select.h>
#endif

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#ifndef AI_ADDRCONFIG
#define AI_ADDRCONFIG 0
#endif

#include "bsdglob.h"

#include "irc_std.h"
#include "debug.h"
#include "newio.h"

/* these define what characters do, inverse, underline, bold and all off */
#define REV_TOG		'\026'		/* ^V */
#define REV_TOG_STR	"\026"
#define UND_TOG		'\037'		/* ^_ */
#define UND_TOG_STR	"\037"
#define BOLD_TOG	'\002'		/* ^B */
#define BOLD_TOG_STR	"\002"
#define ALL_OFF		'\017'		/* ^O */
#define ALL_OFF_STR	"\017"
#define BLINK_TOG	'\006'		/* ^F (think flash) */
#define BLINK_TOG_STR	"\006"
#define ROM_CHAR        '\022'          /* ^R */
#define ROM_CHAR_STR    "\022"
#define ALT_TOG		'\005'		/* ^E (think Extended) */
#define ALT_TOG_STR	"\005"
#define ND_SPACE	'\023'		/* ^S */
#define ND_SPACE_STR	"\023"

#define IRCD_BUFFER_SIZE	512
#define BIG_BUFFER_SIZE		(4 * IRCD_BUFFER_SIZE)
#define MAX_PROTOCOL_SIZE	(IRCD_BUFFER_SIZE - 2)

#ifndef INPUT_BUFFER_SIZE
#define INPUT_BUFFER_SIZE	(IRCD_BUFFER_SIZE - 20)
#endif


#define REFNUM_MAX 10

#ifndef RAND_MAX
#define RAND_MAX 2147483647
#endif

#define NICKNAME_LEN 30
#define NAME_LEN 80
#define REALNAME_LEN 50
#define PATH_LEN 1024

#ifndef MIN
#define MIN(a,b) ((a < b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) ((a > b) ? (a) : (b))
#endif

/* This section is for keeping track internally
 * the CVS revision info of the running client.
 * Since so many people are using CVS versions
 * for debugging purposes it's good to know what
 * file revisions they are running.
 */
#define CVS_REVISION(id) \
void id (char *buf)                 \
{                       \
    strcpy(buf, cvsrevision);       \
}

void alias_c(char *);
void alist_c(char *);
void array_c(char *);
void banlist_c(char *);
void botlink_c(char *);
void cdcc_c(char *);
void chelp_c(char *);
void commands_c(char *);
void commands2_c(char *);
void cset_c(char *);
void ctcp_c(char *);
void dcc_c(char *);
void debug_c(char *);
void encrypt_c(char *);
void exec_c(char *);
void files_c(char *);
void flood_c(char *);
void fset_c(char *);
void functions_c(char *);
void funny_c(char *);
void hash_c(char *);
void help_c(char *);
void history_c(char *);
void hook_c(char *);
void if_c(char *);
void ignore_c(char *);
void input_c(char *);
void irc_c(char *);
void ircaux_c(char *);
void keys_c(char *);
void lastlog_c(char *);
void list_c(char *);
void log_c(char *);
void mail_c(char *);
void misc_c(char *);
void modules_c(char *);
void names_c(char *);
void network_c(char *);
void newio_c(char *);
void notice_c(char *);
void notify_c(char *);
void numbers_c(char *);
void output_c(char *);
void parse_c(char *);
void queue_c(char *);
void readlog_c(char *);
void reg_c(char *);
void screen_c(char *);
void server_c(char *);
void stack_c(char *);
void status_c(char *);
void struct_c(char *);
void tcl_public_c(char *);
void term_c(char *);
void timer_c(char *);
void translat_c(char *);
void user_c(char *);
void userlist_c(char *);
void vars_c(char *);
void who_c(char *);
void whowas_c(char *);
void window_c(char *);
void words_c(char *);

/*
 * declared in irc.c 
 */
extern	int	current_numeric;
extern	char	*cut_buffer;
extern	char	oper_command;
extern	int	irc_port;
extern	int	current_on_hook;
extern	int	use_flow_control;
extern	char	*joined_nick;
extern	char	*public_nick;
extern	char	empty_string[];
extern	char	zero[];
extern	char	one[];
extern	char	on[];
extern	char	off[];
extern	char	space[];
extern	char	space_plus[];
extern	char	space_minus[];
extern	char	dot[];
extern	char	star[];
extern	char	comma[];
extern	char	nickname[];
extern	char	*ircrc_file;
extern	char	*bircrc_file;
extern	char	*LocalHostName;
extern	char	hostname[];
extern	char	userhost[];
extern	char	realname[];
extern	char	username[];
extern	char	*send_umode;
extern	char	*last_notify_nick;
extern	int	away_set;
extern	int	background;
extern	char	*my_path;
extern	char	*irc_path;
extern	char	*irc_lib;
extern	char	*args_str;
extern	char	*invite_channel;
extern	int	who_mask;
extern	char	*who_name;
extern	char	*who_host;
extern	char	*who_server;
extern	char	*who_file;
extern	char	*who_nick;
extern	char	*who_real;
extern	int	dumb_mode;
extern	int	use_input;
extern	time_t	idle_time;
extern	time_t	now;
extern  time_t  start_time;
extern	int	waiting_out;
extern	int	waiting_in;
extern	char	wait_nick[];
extern	char	whois_nick[];
extern	char	lame_wait_nick[];
extern	char	**environ;
extern	int	cuprent_numeric;
extern	int	quick_startup;
extern	char	version[];
extern 	fd_set	readables, writables;
extern	int	strip_ansi_in_echo;
extern	int	loading_global;
extern	const unsigned long bitchx_numver;
extern	const	char *unknown_userhost;
extern	char	*forwardnick;
extern	int	inhibit_logging;

extern	char	MyHostName[];
extern	struct	sockaddr_foobar MyHostAddr;
extern	struct	sockaddr_foobar LocalHostAddr;
extern	int	cpu_saver;
extern	struct	sockaddr_foobar	local_ip_address;


int	BX_is_channel (char *);
void	BX_irc_exit (int, char *, char *, ...);
void	BX_beep_em (int);
void	got_initial_version (char *);
void	parse_notice (char *, char **);
void	irc_quit (char, char *);
char	get_a_char (void);
void	load_scripts (void);
void	clear_whowas (void);
void	clear_variables (void);
void	clear_fset (void);
void	start_memdebug (void);

void	dump_load_stack		(int);	/* XXX command.c */
const char *  current_filename	(void);	/* XXX command.c */
int	current_line		(void);	/* XXX command.c */
	
char	*getenv (const char *);
void	get_line_return (char, char *);
void	get_line (char *, int, void (*)(char, char *));
void	BX_io (const char *);

#ifdef NEED_OSPEED
/* We need this for broken linux systems. */
extern short ospeed;
#endif

void reattach_tty(char *, char *);
void init_socketpath(void);
void kill_attached_if_needed(int);
void setup_pid();

#ifdef CLOAKED
void initsetproctitle(int, char **, char **);
void setproctitle(const char *, ...);
#endif

#endif /* __irc_h */
