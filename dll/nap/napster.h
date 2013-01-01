
#ifndef _napster_h
#define _napster_h

#include "modval.h"

typedef struct _Stats {
	int libraries;
	int gigs;
	int songs;
	unsigned long total_files;
	double total_filesize;
	unsigned long files_served;
	double filesize_served;
	unsigned long files_received;
	double filesize_received;
	double max_downloadspeed;
	double max_uploadspeed;
	time_t starttime;
	unsigned long shared_files;
	double shared_filesize;
} Stats;

extern Stats statistics;

typedef struct {
	char *username;
	char *password;
	int connection_speed;
} _N_AUTH;

typedef unsigned char _N_CMD;
                
typedef struct {
	int libraries;
	int gigs;
	int songs;
} _N_STATS;

#define cparse convert_output_format

typedef struct _nick_struct {
	struct _nick_struct *next;
	char	*nick;
  	int	speed;
	unsigned long shared;
} NickStruct;

typedef struct _AUDIO_HEADER {
	unsigned long filesize;
	int mpeg25;
	int ID;
	int layer;
	int error_protection;
	int bitrate_index;
	int sampling_frequency;
	int padding;
	int extension;
	int mode; /* 0 = STEREO 1 = Joint 2 = DUAL 3 = Mono */
	int mode_ext;
	int copyright;
	int original;
	int emphasis;
	int stereo;
	int jsbound;
	int sblimit;
	int true_layer;
	int framesize;
	int freq;
	unsigned long totalframes;
	unsigned long bitrate;
} AUDIO_HEADER;

typedef struct _files {
	struct _files	*next;
	char		*filename;
	char		*checksum;
	unsigned long	filesize;
	time_t		time;
	int		bitrate;
	int		freq;
	int		stereo;
	int		type;
} Files;

typedef struct _ignore_nick_struct {
	struct _ignore_nick_struct *next;
	char *nick;
	unsigned long start;
	unsigned long end;
} IgnoreStruct;

typedef struct _channel_struct {
	struct _channel_struct *next;
	char *channel;
	char *topic;
	int injoin;
	NickStruct *nicks;
} ChannelStruct;

typedef struct _file_struct {
	struct _file_struct *next;
	char *name;
	char *checksum;
	unsigned long filesize;
	unsigned int bitrate;
	unsigned int freq;
	unsigned int seconds;
	char *nick;
	unsigned long ip;
	int port;
	unsigned short speed;	
} FileStruct;

typedef struct _getfile_ {
	struct _getfile_ *next;	

	char	*nick;
	char	*ip;
	char	*checksum;
	char	*filename;
	char	*realfile;

	int	socket;
	int	port;
	int	write;
	int	count;
	unsigned long filesize;
	unsigned long received;
	unsigned long resume;
	time_t	starttime;
	time_t	addtime;
	int	speed;
	int	up;
} GetFile;

typedef struct _resume_file_ {
	struct _resume_file_ *next;
	char *checksum;
	unsigned long filesize;
	char *filename;
	FileStruct *results;
} ResumeFile;

enum nap_Commands {
	CMDR_ERROR		= 0,
	CMDS_UNKNOWN		= 1,
	CMDS_LOGIN		= 2, /* user pass dataport "version" speed */
	CMDR_EMAILADDR		= 3, /* email address */
	CMDR_BASTARD		= 4, /* unknown */
	CMDS_REGISTERINFO	= 6, /* userinfo */
	CMDS_CREATEUSER		= 7, /* create user account */
	CMDR_CREATED		= 8, /* account created */
	CMDR_CREATEERROR	= 9, /* username taken */
	CMDR_ILLEGALNICK	= 10, /* illegal nickname specified */
	
	CMDR_LOGINERROR		= 13, 

	CMDS_OPTIONS		= 14, /* NAME:%s ADDRESS:%s CITY:%s STATE:%s PHONE:%s AGE:%s INCOME:%s EDUCATION:%s   *login options */

	CMDR_MSTAT		= 15, 
	CMDR_REQUESTUSERSPEED	= 89,
	CMDR_SENDFILE		= 95,
	CMDS_ADDFILE		= 100,

	CMDS_REMOVEFILE		= 102, /* "\path\to\filename\" for removal */

	CMDR_GETQUEUE		= 108,
	CMDR_MOTD		= 109,
	CMDR_ANOTHERUSER	= 148,
	CMDS_SEARCH		= 200,
	CMDR_SEARCHRESULTS	= 201,
	CMDR_SEARCHRESULTSEND	= 202,

	/* if dataport is 0 we use 500 to request a transfer. 0 is a firewalled host */
	CMDS_REQUESTFILE	= 203,
	CMDR_FILEREADY		= 204,

	CMDS_SENDMSG		= 205,
	CMDR_GETERROR		= 206,

	CMDS_ADDHOTLIST		= 207,

	CMDS_ADDHOTLISTSEQ	= 208,
	CMDR_HOTLISTONLINE	= 209,
	CMDR_USEROFFLINE	= 210, 

	CMDS_BROWSE		= 211,
	CMDR_BROWSERESULT	= 212,
	CMDR_BROWSEENDRESULT	= 213,
	CMDR_STATS		= 214,

	CMDS_REQUESTRESUME	= 215, /* checksum filesize */
	CMDR_RESUMESUCCESS	= 216, /* nick ip port filename checksum size connection */
	CMDR_RESUMEEND		= 217, /* end resume for checksum filesize */

	CMDS_UPDATE_GET1	= 218, /* add 1 to download */
	CMDS_UPDATE_GET		= 219, /* sub 1 from download */
	CMDS_UPDATE_SEND1	= 220, /* add 1 for send */
    	CMDS_UPDATE_SEND	= 221, /* sub 1 from send */

	CMDR_HOTLISTSUCCESS	= 301,
	CMDR_HOTLISTERROR	= 302, /* not on hotlist */
	CMDS_HOTLISTREMOVE	= 303, /* nick */
	
	CMDS_JOIN		= 400,
	CMDS_PART		= 401,
	CMDS_SEND		= 402,
	CMDR_PUBLIC		= 403,
	CMDR_ERRORMSG		= 404,
	CMDR_JOIN		= 405,
	CMDR_JOINNEW		= 406,
	CMDR_PARTED		= 407,
	CMDR_NAMES		= 408,
	CMDR_ENDNAMES		= 409,
	CMDS_TOPIC		= 410, /* got/change topic */

	CMDS_REQUESTFILEFIRE	= 500,
	CMDR_FILEINFOFIRE	= 501, /* if firewalled then expect a 501 request send */

	CMDS_REQUESTINFO	= 600,
	CMDS_FILESIZE		= 601,
	CMDS_REQUESTSIZE	= 602,
	CMDS_WHOIS		= 603,
	CMDR_WHOIS		= 604,
	CMDR_WHOWAS		= 605,
	CMDS_SETUSERLEVEL	= 606, /* moderators/administrators/elite */
	CMDR_FILEREQUEST	= 607, /* nick \"filename\" */
	CMDS_FILEINFO		= 608, /* nick \"filename\" */
	CMDR_ACCEPTERROR	= 609, /* accept failed on request */

	CMDS_KILLUSER		= 610, /* return 404 permission denied */
	CMDS_NUKEUSER		= 611, /* return 404 */
	CMDS_BANUSER		= 612,
	CMDS_SETDATAPORT	= 613, 
	CMDS_UNBANUSER		= 614,
	CMDS_BANLIST		= 615,
	CMDR_BANLIST_IP		= 616,
	CMDS_LISTCHANNELS	= 617,
	CMDR_LISTCHANNELS	= 618,

	CMDS_SENDLIMIT		= 619, /* nick "filename" queuelimit */
	CMDR_SENDLIMIT		= 620, /* nick "filename" filesize queuelimit */

	CMDR_MOTDS		= 621, 
	CMDS_MUZZLE		= 622,
	CMDS_UNMUZZLE		= 623,
	CMDS_UNNUKEUSER		= 624, /* return 404 */
	CMDS_SETLINESPEED	= 625,
	CMDR_DATAPORTERROR	= 626,
	CMDS_OPSAY		= 627,
	CMDS_ANNOUNCE		= 628,
	CMDR_BANLIST_NICK	= 629,
	
	CMDS_CHANGESPEED	= 700,
	CMDS_CHANGEPASS		= 701,
	CMDS_CHANGEEMAIL	= 702,
	CMDS_CHANGEDATA		= 703,

	CMDS_PING		= 751, /* user */
	CMDS_PONG		= 752, /* <user> recieved from a ping*/
			       /* <user> can also be used to send a pong */
	/* 753 */
	
	CMDS_RELOADCONFIG	= 800, /* <config variable> */
	CMDS_SERVERVERSION	= 801, /* none */
	/* 802 missing */
	CMDS_SETCONFIG		= 810, /* <config string */
	/* 811 */
	CMDS_CLEARCHANNEL	= 820,  /* channelname */
	/* 821 822 823 826 827 */
	CMDS_SENDME		= 824,
	CMDR_NICK		= 825,
	CMDS_NAME		= 830, /* <channel> returns 825 with nick info, 830 is recieved on end of list */
	/* 831 */


	/* the following are open-nap specific */
	CMDS_SERVERLINK		= 10100, /* link server  <server> <port> [<remote server>] */
	CMDS_SERVERUNLINK	= 10101, /* unlink server <server> <reason> */
	CMDS_SERVERKILL		= 10110, /* kill server <server> <reason> */
	CMDS_SERVERREMOVE	= 10111,  /* remove it <server> <reason> */
	CMDS_ADDMIMEFILE	= 10300	 /* add a mime file type */
#if 0
%s %s "%s"
Proper Syntax: /setpassword <user> <password> [reason]
SETPASSWORD
#endif
};

/* 
 500 nick filename  -> "501 nick ip port filename md5 speed"
*/


typedef struct {
	unsigned short len;
	unsigned short command;
} _N_DATA;

typedef struct {
	int	cmd;
	int	(*func)(int, char *);
} _NAP_COMMANDS;

#define NAP_COMM(name) \
	int name (int cmd, char *args)

NAP_COMM(cmd_error);
NAP_COMM(cmd_unknown);
NAP_COMM(cmd_login);
NAP_COMM(cmd_email);
NAP_COMM(cmd_stats);
NAP_COMM(cmd_whois);
NAP_COMM(cmd_whowas);
NAP_COMM(cmd_joined);
NAP_COMM(cmd_topic);
NAP_COMM(cmd_names);
NAP_COMM(cmd_endnames);
NAP_COMM(cmd_parted);
NAP_COMM(cmd_public);
NAP_COMM(cmd_msg);
NAP_COMM(cmd_search);
NAP_COMM(cmd_endsearch);
NAP_COMM(cmd_browse);
NAP_COMM(cmd_endbrowse);
NAP_COMM(cmd_request);
NAP_COMM(cmd_getfile);
NAP_COMM(cmd_getfileinfo);
NAP_COMM(cmd_alreadyregistered);
NAP_COMM(cmd_registerinfo);
NAP_COMM(cmd_banlist);
NAP_COMM(cmd_offline);
NAP_COMM(cmd_filerequest);
NAP_COMM(cmd_dataport);
NAP_COMM(cmd_hotlist);
NAP_COMM(cmd_fileinfo);
NAP_COMM(cmd_hotlistsuccess);
NAP_COMM(cmd_hotlisterror);
NAP_COMM(cmd_channellist);
NAP_COMM(cmd_resumerequest);
NAP_COMM(cmd_resumerequestend);
NAP_COMM(cmd_send_limit_msg);
NAP_COMM(cmd_accepterror);

NAP_COMM(cmd_recname);
NAP_COMM(cmd_endname);
NAP_COMM(cmd_firewall_request);
NAP_COMM(cmd_ping);
NAP_COMM(cmd_bastard);
NAP_COMM(cmd_sendme);

BUILT_IN_DLL(load_napserv);
BUILT_IN_DLL(print_napster);
BUILT_IN_DLL(share_napster);
BUILT_IN_DLL(nclose);
BUILT_IN_DLL(nap_del);
BUILT_IN_DLL(nap_glist);
BUILT_IN_DLL(nap_request);
BUILT_IN_DLL(ignore_user);
BUILT_IN_DLL(nap_echo);

#define NAP_send(s, i) write(nap_socket, s, i);
SocketList *naplink_connect(char *, unsigned short);
void naplink_getserver(char *, unsigned short, int);

int nap_say(char *, ...);
int nap_put(char *, ...);
int send_ncommand(unsigned int, char *, ...);


void print_file(FileStruct *, int);
void naplink_handlelink(int);
int make_listen(int);
void set_napster_socket(int);
char *napster_status(void);
int build_napster_status(Window *);
int nap_finished_file(int, GetFile *);
int clean_queue(GetFile **, int);
void clear_filelist(FileStruct **);
void clear_files(Files **);
void nap_firewall_get(int);
char *mp3_time(unsigned long);
char *calc_md5(int, unsigned long);
int check_nignore(char *);

#if 0
#undef BUILT_IN_FUNCTION
#define BUILT_IN_FUNCTION(x, y) static char * x (char *fn, char * y)
#endif

BUILT_IN_FUNCTION(func_mp3_time);
BUILT_IN_FUNCTION(func_topic);
BUILT_IN_FUNCTION(func_onchan);
BUILT_IN_FUNCTION(func_onchannel);
BUILT_IN_FUNCTION(func_connected);
BUILT_IN_FUNCTION(func_hotlist);
BUILT_IN_FUNCTION(func_napchannel);
BUILT_IN_FUNCTION(func_raw);
BUILT_IN_FUNCTION(func_md5);

GetFile *find_in_getfile(GetFile **, int, char *, char *, char *, int, int);
int connectbynumber(char *, unsigned short *, int, int, int);

int count_download(char *);
void compute_soundex (char *, int, const char *);

char *base_name(char *);

extern FileStruct *file_search;
extern FileStruct *file_browse;
extern GetFile *napster_sendqueue;
extern char *nap_current_channel;
extern int nap_socket;
extern ChannelStruct *nchannels;
extern NickStruct *nap_hotlist;


#define NAP_DOWNLOAD	0x00
#define NAP_UPLOAD	0x01
#define NAP_QUEUED	0xf0
#define NAP_BUFFER_SIZE 2048

#endif
