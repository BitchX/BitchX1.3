/* 
 * Copyright Colten Edwards 1997.
 * various miscellaneous routines needed for irc functions
 */
 
#ifndef _misc_h
#define _misc_h

#define KICKLIST		0x01
#define LEAVELIST		0x02
#define JOINLIST		0x03
#define CHANNELSIGNOFFLIST	0x04
#define PUBLICLIST		0x05
#define PUBLICOTHERLIST		0x06
#define PUBLICNOTICELIST	0x07
#define NOTICELIST		0x08
#define TOPICLIST		0x09
#define MODEOPLIST		0x0a
#define MODEDEOPLIST		0x0b
#define MODEBANLIST		0x0c
#define MODEUNBANLIST		0x0d
#define NICKLIST		0x0e
#define MODEHOPLIST		0x0f
#define MODEDEHOPLIST		0x10
#define MODEEBANLIST		0x11
#define MODEUNEBANLIST		0x12

enum color_attributes {	
	BLACK = 0, BLUE, GREEN, CYAN, RED, MAGENTA, YELLOW, WHITE, 
	BLACKB, BLUEB, GREENB, CYANB, REDB, MAGENTAB, YELLOWB, WHITEB,NO_COLOR, 
	BACK_BLACK, BACK_RED, BACK_GREEN, BACK_YELLOW,
	BACK_BLUE, BACK_MAGENTA, BACK_CYAN, BACK_WHITE, 
	BACK_BBLACK, BACK_BRED, BACK_BGREEN, BACK_BYELLOW,
	BACK_BBLUE, BACK_BMAGENTA, BACK_BCYAN, BACK_BWHITE, 
	REVERSE_COLOR, BOLD_COLOR, BLINK_COLOR, UNDERLINE_COLOR
};

/* prepare_command() flags */
#define PC_SILENT 3	
#define PC_TOPIC 2
#define NEED_OP 1
#define NO_OP 0
	
extern char *color_str[];
extern	int	split_watch;
void	clear_link (irc_server **);
extern  irc_server *tmplink, *server_last;

#ifndef BITCHX_LITE
#define MAX_LAST_MSG 10
#else
#define MAX_LAST_MSG 2
#endif

extern LastMsg last_msg[MAX_LAST_MSG+1];
extern LastMsg last_dcc[MAX_LAST_MSG+1];
extern LastMsg last_sent_dcc[MAX_LAST_MSG+1];
extern LastMsg last_notice[MAX_LAST_MSG+1];
extern LastMsg last_servermsg[MAX_LAST_MSG+1];
extern LastMsg last_sent_msg[MAX_LAST_MSG+1];
extern LastMsg last_sent_notice[MAX_LAST_MSG+1];
extern LastMsg last_sent_topic[2];
extern LastMsg last_sent_wall[2];
extern LastMsg last_topic[2];
extern LastMsg last_wall[MAX_LAST_MSG+1];
extern LastMsg last_invite_channel[2];
extern LastMsg last_ctcp[2];
extern LastMsg last_ctcp_reply[2];
extern LastMsg last_sent_ctcp[2];


	void	put_user (const NickList *, const char *);
	void	update_stats	(int, NickList *, ChannelList *, int);
	int	check_split	(char *, char *);
	void	BX_userage		(char *, char *);
	void	stats_k_grep_end (void);
	char	*stripansicodes (const char *);
	char	*stripansi	(unsigned char *);
	NickTab	*BX_gettabkey (int, int, char *);
	void	BX_addtabkey (char *, char *, int);
	void	clear_array (NickTab **, char *);
	char	*BX_random_str (int, int);
	int	check_serverlag (void);
	void	check_auto_away (time_t);
ChannelList *	BX_prepare_command (int *, char *, int);
	int	rename_file (char *, char **);
	void	putlog (int, ...);

	void	add_mode_buffer ( char *, int);
	void	flush_mode (ChannelList *);
	void	flush_mode_all (ChannelList *);
	void	add_mode (ChannelList *, char *, int, char *, char *, int);
	int	delay_flush_all (void *, char *);
	char	*clear_server_flags (char *);
	char	*ban_it (char *, char *, char *, char *);

	void	log_toggle (int, ChannelList *);

	char	*cluster (char *);
	int	caps_fucknut (register unsigned char *);

	void    do_reconnect (char *);

	int	are_you_opped (char *);
	void	error_not_opped (const char *);

	char	*get_reason (char *, char *);
	char	*get_realname(char *);
	char	*get_signoffreason (char *);
	int	isme (char *);

	char 		*BX_convert_output_format (const char *, const char *, ...);
	char 		*convert_output_format2 (const char *);
	void		add_last_type (LastMsg *, int, char *, char *, char *, char *);
	int		check_last_type (LastMsg *, int, char *, char *);
	int		matchmcommand (char *, int);
	char		*convert_time (time_t);
	char		*BX_make_channel(char *);	


	int		timer_unban (void *, char *);
	void		check_server_connect (int);
	const char	*country(const char *);
	int		do_newuser (char *, char *, char *);
	int		char_fucknut (register unsigned char *, char, int);
	BanList		*ban_is_on_channel(register char *, register ChannelList *);
	BanList		*eban_is_on_channel(register char *, register ChannelList *);
	void		check_orig_nick(char *);

	char		*do_nslookup (char *, char *, char *, char *, int, void (*func)(), char *);
	void		set_nslookupfd(fd_set *);
	long		print_nslookup(fd_set *);
	void		auto_nslookup();
	int		freadln(FILE *, char *);


	void		BX_close_socketread(int);
	int		BX_add_socketread(int, int, unsigned long, char *, void (*func_read)(int), void (*func_write)(int));
	int		BX_check_socket(int);
	void		set_socket_read (fd_set *, fd_set *);
	void		scan_sockets (fd_set *, fd_set *);
	void		read_clonelist(int);
	void		read_clonenotify(int);
	void		read_netfinger(int);
	int		BX_write_sockets(int, unsigned char *, int, int);
	int		BX_read_sockets(int, unsigned char *, int);
	unsigned long	BX_set_socketflags(int, unsigned long);
	void		*BX_get_socketinfo(int);
	void		BX_set_socketinfo(int, void *);
	unsigned long	BX_get_socketflags(int);
	char		*get_socketserver(int);
	SocketList	*BX_get_socket(int);
	void		BX_add_sockettimeout(int, time_t, void *);
	int		BX_get_max_fd(void);
	int		BX_set_socketwrite(int);
	
#ifdef GUI
	void		scan_gui(fd_set *);
#endif		

#ifdef WANT_NSLOOKUP
/*
 * alib.h (C)opyright 1992 Darren Reed.
 */
#define	ARES_INITLIST	1
#define	ARES_CALLINIT	2
#define ARES_INITSOCK	4
#define ARES_INITDEBG	8
#define ARES_INITCACH    16

#define MAXPACKET	1024
#define MAXALIASES	35
#define MAXADDRS	35

#define	RES_CHECKPTR	0x0400

struct	hent {
	char	*h_name;	/* official name of host */
	char	*h_aliases[MAXALIASES];	/* alias list */
	int	h_addrtype;	/* host address type */
	int	h_length;	/* length of address */
	/* list of addresses from name server */
	struct	in_addr	h_addr_list[MAXADDRS];
#define	h_addr	h_addr_list[0]	/* address, for backward compatiblity */
};

struct	resinfo {
	char	*ri_ptr;
	int	ri_size;
};

struct	reslist {
	int	re_id;
	char	re_type;
	char	re_retries;
	char	re_resend;	/* send flag. 0 == dont resend */
	char	re_sends;
	char	re_srch;
	int	re_sent;
	u_long	re_sentat;
	u_long	re_timeout;
	struct	in_addr	re_addr;
	struct	resinfo	re_rinfo;
	struct	hent re_he;
	struct	reslist	*re_next, *re_prev;
	char	re_name[65];
	char	*nick;
	char	*host;
	char	*user;
	char	*channel;
	char	*command;
	int	server;
	void	(*func)();
};

struct	hostent	*ar_answer(char *, int, void (*func)(struct reslist *) );
void    ar_close(void);
int     ar_delete(char *, int);
int     ar_gethostbyname(char *, char *, int, char *, char *, char *, char *, int, void (*func)(), char *);
int     ar_gethostbyaddr(char *, char *, int, char *, char *, char *, char *, int, void (*func)(), char *);
int     ar_init(int);
int     ar_open(void);
long    ar_timeout(time_t, char *, int, void (*func)(struct reslist *) );
void	ar_rename_nick(char *, char *, int);
#endif

#ifndef	MIN
#define	MIN(a,b)	((a) > (b) ? (b) : (a))
#endif

#define getrandom(min, max) ((rand() % (int)(((max)+1) - (min))) + (min))


extern char *auto_str;
	
#endif
