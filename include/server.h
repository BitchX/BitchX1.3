/*
 * server.h: header for server.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: server.h 87 2010-06-26 08:18:34Z keaston $
 */

#ifndef __server_h_
#define __server_h_
  
/* for ChannelList */
#include "who.h"
#include "names.h"
#include "struct.h"
#include "ssl.h"

/*
 * type definition to distinguish different
 * server versions
 */
#define Server2_7	0
#define Server2_8	1
#define Server2_9	2
#define Server2_10	3
#define Server2_8ts4	4
#define Server2_8hybrid 5
#define Server2_8hybrid6 6
#define Server2_8comstud 7

#define Server_u2_8     10
#define Server_u2_9     11
#define Server_u2_10	12
#define Server_u3_0     13

struct notify_stru;

typedef struct _queued_send
{
	struct _queued_send *next;
	int	server;
	int	des;
	char	*buffer;
} QueueSend;

typedef struct _sping_ {
	struct _sping_ *next;
	char *sname;
#ifdef HAVE_GETTIMEOFDAY
	struct timeval in_sping;
#else
	time_t in_sping;
#endif
} Sping;


/* Server: a structure for the server_list */
typedef	struct
{
	char	*name;			/* the name of the server */
	char	*itsname;		/* the server's idea of its name */
	char	*password;		/* password for that server */
	char	*snetwork;
	char	*cookie;		/* TS4 op cookie */
	int	port;			/* port number on that server */

	char	*nickname;		/* nickname for this server */
	char    *s_nickname;            /* last NICK command sent */
	char    *d_nickname;            /* Default nickname to use */
	char	*userhost;
	int     fudge_factor;           /* How much s_nickname's fudged */
	int     nickname_pending;       /* Is a NICK command pending? */
	int     orignick_pending;       /* Is a ORIGNICK command pending? */
	int     server_change_pending;  /* We are waiting for a server change */

	char	*away;			/* away message for this server */
	time_t  awaytime;
	int	operator;		/* true if operator */
	int	server2_8;
	int	version;		/* the version of the server -
					 * defined above */
	char	*version_string;	/* what is says */
	int	whois;			/* true if server sends numeric 318 */
	long	flags;			/* Various flags */
	long	flags2;
	int	login_flags;		/* server login flags */
	char	*umodes;
	char    umode[80];		/* User Mode storage */ 
	int	connected;		/* true if connection is assured */
	int	write;			/* write descriptor */
	int	read;			/* read descriptior */
	int	eof;			/* eof flag for server */
	int	motd;			/* motd flag (used in notice.c) */
	int	sent;			/* set if something has been sent,
					 * used for redirect */
	int	lag;			/* indication of lag from server CDE*/
	time_t	lag_time;		/* time ping sent to server CDE */
	time_t	last_msg;		/* last mesg recieved from the server CDE */

	time_t		last_sent;	/* last mesg time sent */
	QueueSend 	*queue;		/* queue of lines to send to a server */	

	char	*buffer;		/* buffer of what dgets() doesn't get */


	WhoEntry	*who_queue;	/* Who queue */
	IsonEntry	*ison_queue;	/* Ison queue */
	UserhostEntry	*userhost_queue;/* Userhost queue */

	NotifyList	notify_list;	/* Notify list for this server */
	NotifyList	watch_list;	/* Watch list for this server */
	int		watch;		/* dalnet watch. available */
	int	copy_from;		/* server to copy the channels from
					 * when (re)connecting */
	int	ctcp_dropped;		/* */
	int	ctcp_not_warned;	/* */
	time_t	ctcp_last_reply_time;	/* used to limit flooding */

	struct sockaddr_foobar local_addr;      /* ip address of this connection */
	struct sockaddr_foobar uh_addr;		/* ip address of this connection */
	struct sockaddr_foobar local_sockname; /* sockname of this connection */

	ChannelList	*chan_list;	/* list of channels for this server */
	int	in_delay_notify;
	int	link_look;
	time_t	link_look_time;
	int	trace_flags;
	int	in_who_kill;
	int	in_trace_kill;
	int	stats_flags;
	int	in_timed_server;

	char	*redirect;

	irc_server *tmplink;	/* list of linked servers */
	irc_server *server_last;/* list of linked servers */
	irc_server *split_link; /* list of linked servers */

	void	(*parse_server) (char *);	/* pointer to parser for this server */
	unsigned long ircop_flags;
	Sping	*in_sping;
	int reconnects;
	int reconnecting;
	int reconnect;
	int closing;
	int retries;
	int try_once;
	int old_server;
	int req_server;
	int server_change_refnum;
#ifdef NON_BLOCKING_CONNECTS
	int connect_wait;
	int c_server;
	int from_server;
#endif
	char *orignick;
	time_t connect_time;
#if defined(HAVE_SSL) && !defined(IN_MODULE)   
	SSL_CTX* ctx;
	int enable_ssl;
	int ssl_error;
	SSL* ssl_fd;
#endif
	char *sasl_nick;
	char *sasl_pass;

/* recv_nick: the nickname of the last person to send you a privmsg */
	char *recv_nick;
/* sent_nick: the nickname of the last person to whom you sent a privmsg */
	char *sent_nick;
	char *sent_body;
}	Server;

typedef struct ser_group_list
{
	struct ser_group_list	*next;
	char	*name;
	int	number;
}	SGroup;

typedef	unsigned	short	ServerType;

	int	find_server_group (char *, int);
	char *	find_server_group_name (int);

	void	BX_add_to_server_list (char *, int, char *, char *, char *, char *, char *, int, int);
	int	BX_build_server_list (char *);
	int	connect_to_server (char *, int, int);
	void	BX_get_connected (int, int);
	void	try_connect (int, int);
	int	BX_read_server_file (char *);
	void	BX_display_server_list (void);
	void	do_server (fd_set *, fd_set *);
	int	BX_connect_to_server_by_refnum (int, int);
	int	BX_find_server_refnum (char *, char **);
	
	void	BX_set_server_cookie (int, char *);
	char	*BX_get_server_cookie (int);
	
extern	int	attempting_to_connect;
/*extern	int	number_of_servers;*/
extern	int	connected_to_server;
extern	int	never_connected;
extern	int	primary_server;
extern	int	from_server;
extern 	int	last_server;
extern	int	parsing_server_index;

extern	SGroup	*server_group_list;

	void	servercmd (char *, char *, char *, char *);
	char	*BX_get_server_nickname (int);
	char	*BX_get_server_name (int);
	char	*BX_get_server_itsname (int);
	char	*get_server_pass (int);
	int	BX_find_in_server_list (char *, int);
	char	*BX_create_server_list (char *);
	void	BX_set_server_motd (int, int);
	int	BX_get_server_motd (int);
	int	BX_get_server_operator (int);
	int	BX_get_server_version (int);
	void	BX_close_server (int, char *);
	int	BX_is_server_connected (int);
	void	BX_flush_server (void);
	void	BX_set_server_operator (int, int);
	void	BX_server_is_connected (int, int);
	int	BX_parse_server_index (char *);
	void	BX_parse_server_info (char *, char **, char **, char **, char **, char **, char **);
	long	set_server_bits (fd_set *, fd_set *);
	void	BX_set_server_itsname (int, char *);
	void	BX_set_server_version (int, int);
	char	*BX_get_possible_umodes(int);
		
	int	BX_is_server_open (int);
	int	BX_get_server_port (int);
	
	int	BX_get_server_lag (int);
	void	BX_set_server_lag (int, int);
	time_t	get_server_lagtime (int);
	void	set_server_lagtime (int, time_t);
	
	char	*BX_set_server_password (int, char *);
	void	BX_set_server_nickname (int, char *);

	void	BX_set_server2_8 (int , int);
	int	BX_get_server2_8 (int);

	void	BX_close_all_server (void);
	void	disconnectcmd (char *, char *, char *, char *);
	char	*BX_get_umode (int);
	int	BX_server_list_size (void);

	void    BX_set_server_away                 (int, char *, int);
	char *  BX_get_server_away                 (int);
	time_t	get_server_awaytime		(int);
	void	set_server_awaytime		(int, time_t);

	void set_server_recv_nick(int server, const char *nick);
	char *get_server_recv_nick(int server);
	void set_server_sent_nick(int server, const char *nick);
	char *get_server_sent_nick(int server);
	void set_server_sent_body(int server, const char *msg_body);
	char *get_server_sent_body(int server);

	void    server_redirect                 (int, char *);
	int     BX_check_server_redirect           (char *);
	char *	BX_get_server_network		(int);
	void	BX_server_disconnect		(int, char *);
	void	send_from_server_queue		(void);
	void	clear_sent_to_server		(int);
	int	sent_to_server			(int);
	void	BX_set_server_flag			(int, int, int);
	int	BX_get_server_flag			(int, int);
	char *	get_server_userhost		(int);
	void	got_my_userhost			(UserhostItem *item, char *nick, char *stuff);
	void	BX_set_server_version		(int, int);
	int	BX_get_server_version		(int);
	void	set_server_version_string	(int, const char *);
	char *	get_server_version_string	(int);

	void	BX_set_server_redirect		(int, const char *);
	char *	BX_get_server_redirect		(int);
		
	void	change_server_nickname		(int, char *);
	void	register_server			(int, char *);
	void	BX_fudge_nickname			(int, int);
	char	*BX_get_pending_nickname		(int);
	void	accept_server_nickname		(int, char *);
	void	BX_reset_nickname			(int);
	void	nick_command_is_pending		(int, int);
	void	orignick_is_pending		(int, int);
	int	is_orignick_pending		(int);

	void	set_server_ircop_flags		(int, unsigned long);
unsigned long	get_server_ircop_flags		(int);	
extern		void	start_identd		(void);

	void	set_server_in_timed		(int, int);
	int	get_server_in_timed		(int);
	time_t	get_server_lastmsg		(int);
	
	int	close_all_servers		(char *);
	void close_unattached_servers (void);
	void close_unattached_server(int);

	void	set_server_orignick		(int, char *);
	char	*get_server_orignick		(int);

ChannelList	*BX_get_server_channels		(int);
	void	BX_set_server_channels		(int, ChannelList *);		
	void	BX_add_server_channels		(int, ChannelList *);
	void	set_server_channels_server	(int);
	int	get_server_channels_server	(int);
	
	int	BX_get_server_trace_flag		(int);
	void	BX_set_server_trace_flag		(int, int);
	int	BX_get_server_stat_flag		(int);
	void	BX_set_server_stat_flag		(int, int);

	void	set_server_reconnect		(int, int);
	void	set_server_reconnecting		(int, int);
	void set_server_old_server		(int, int);
	void set_server_req_server		(int, int);
	void set_server_retries		(int, int);
	void set_server_try_once	(int, int);
	void set_server_change_refnum	(int, int);
	int	get_server_reconnect		(int);
	int	get_server_reconnecting		(int);
	int get_server_change_pending   (int);
#ifdef HAVE_SSL
	void set_server_ssl(int, int);
	int get_server_ssl(int);
#endif
    int is_server_valid(char *name, int server);

#if 0
#ifdef HAVE_GETTIMEOFDAY
struct	timeval	get_server_sping		(int);
	void	set_server_sping		(int, struct timeval);
#else
	time_t	get_server_sping		(int);
	void	set_server_sping		(int, time_t);
#endif
#endif
	Sping	*get_server_sping		(int, char *);
	void	clear_server_sping		(int, char *);
	void	set_server_sping		(int, Sping *);

	int	BX_get_server_trace_kill		(int);
	void	BX_set_server_trace_kill		(int, int);

	void	BX_set_server_last_ctcp_time	(int, time_t);
	time_t	BX_get_server_last_ctcp_time	(int);

	int	BX_get_server_linklook		(int);
	void	BX_set_server_linklook		(int, int);
	time_t	BX_get_server_linklook_time	(int);
	void	BX_set_server_linklook_time	(int, time_t);
	int	BX_get_server_read			(int);
	int	get_server_watch		(int);
	void	set_server_watch		(int, int);

	void	set_userhost_queue_top		(int, UserhostEntry *);
	UserhostEntry *userhost_queue_top	(int);

	void	set_ison_queue_top		(int, IsonEntry *);
	IsonEntry *ison_queue_top		(int);

	void	set_who_queue_top		(int, WhoEntry *);
	WhoEntry *who_queue_top			(int);

	void reconnect_server(int *, int *, time_t *);
	int finalize_server_connect(int, int, int);
	int next_server(int);
	void	do_idle_server (void);

/* XXXXX ick, gross, bad.  XXXXX */
void password_sendline (char *data, char *line);

	Server	*BX_get_server_list		(void);

	int	get_server_local_port		(int);
struct sockaddr_foobar	get_server_local_addr		(int);
struct sockaddr_foobar	get_server_uh_addr		(int);
NotifyItem	*get_server_notify_list		(int);
	void	BX_send_msg_to_nicks		(ChannelList *, int, char *);        
	void	BX_send_msg_to_channels		(ChannelList *, int, char *);        
	int	BX_is_server_queue			(void);
	int	save_servers			(FILE *);	        
	void	add_split_server		(char *, char *, int);
	irc_server *check_split_server		(char *);
	void	remove_split_server		(int, char *);
	void	clean_split_server_list		(int, time_t);
	void write_server_list(char *);
	void write_server_file (char *);
//	void set_server_sasl_nick(int, const char *);
	char *get_server_sasl_nick(int);
//	void set_server_sasl_pass(int, const char *);
	char *get_server_sasl_pass(int);
				
#define USER_MODE	0x0001
#define USER_MODE_A	USER_MODE << 0
#define USER_MODE_B	USER_MODE << 1
#define USER_MODE_C	USER_MODE << 2
#define USER_MODE_D	USER_MODE << 3
#define USER_MODE_E	USER_MODE << 4
#define USER_MODE_F	USER_MODE << 5
#define USER_MODE_G	USER_MODE << 6
#define USER_MODE_H	USER_MODE << 7
#define USER_MODE_I	USER_MODE << 8
#define USER_MODE_J	USER_MODE << 9
#define USER_MODE_K	USER_MODE << 10
#define USER_MODE_L	USER_MODE << 11
#define USER_MODE_M	USER_MODE << 12
#define USER_MODE_N	USER_MODE << 13
#define USER_MODE_O	USER_MODE << 14
#define USER_MODE_P	USER_MODE << 15
#define USER_MODE_Q	USER_MODE << 16
#define USER_MODE_R	USER_MODE << 17
#define USER_MODE_S	USER_MODE << 18
#define USER_MODE_T	USER_MODE << 19
#define USER_MODE_U	USER_MODE << 20
#define USER_MODE_V	USER_MODE << 21
#define USER_MODE_W	USER_MODE << 22
#define USER_MODE_X	USER_MODE << 23
#define USER_MODE_Y	USER_MODE << 24
#define USER_MODE_Z	USER_MODE << 25

#define LOGGED_IN	USER_MODE << 29
#define CLOSE_PENDING	USER_MODE << 30
#define CLOSING_SERVER  USER_MODE << 31
extern const char *umodes;

#define IMMED_SEND	0
#define QUEUE_SEND	1

#define LLOOK_SPLIT 0
#define CHAN_SPLIT 1

#endif /* __server_h_ */
