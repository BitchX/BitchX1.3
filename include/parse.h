/*
 * parse.h
 *
 * written by matthew green
 * copyright (c) 1993
 *
 * @(#)$Id: parse.h 3 2008-02-25 09:49:14Z keaston $
 */

#ifndef __parse_h_
#define __parse_h_

typedef struct {
	const char	*command;
	void 		(*inbound_handler) (char *, char **);
	void		(*outbound_handler) (char *);
	int		flags;
	unsigned long	bytes;
	unsigned long	count;
} protocol_command;
extern 	protocol_command rfc1459[];
extern	int		 num_protocol_cmds;

#define PROTO_NOQUOTE 	1 << 0
#define PROTO_DEPREC	1 << 1



	char	*BX_PasteArgs (char **, int);
	int	BX_BreakArgs (char *, char **, char **, int);
	void	parse_server (char *);
	void	irc2_parse_server (char *);
	int	annoy_kicks (int, char *, char *, char *, NickList *);
	int	ctcp_flood_check (char *, char *, char *);
	void	load_scripts (void);
	int	check_auto_reply (char *);
					
extern	char	*FromUserHost;
extern	char	*last_split_server;
extern	char	*last_split_from;
extern	int	in_server_ping;
extern	int	doing_privmsg;

#define WAIT_WHO 0
#define WAIT_BANS 1
#define WAIT_MODE 2

#define MAXPARA 15


#endif /* __parse_h_ */
