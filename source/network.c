/*
 * network.c -- handles stuff dealing with connecting and name resolving
 *
 * Written by Jeremy Nelson in 1995
 * See the COPYRIGHT file or do /help ircii copyright
 */
#define SET_SOURCE_SOCKET

#include "irc.h"
static char cvsrevision[] = "$Id: network.c 203 2012-06-02 11:38:37Z keaston $";
CVS_REVISION(network_c)
#include "struct.h"
#include "ircterm.h"

#include "ircaux.h"
#include "output.h"
#include "vars.h"

#include "struct.h"

#define MAIN_SOURCE
#include "modval.h"

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif
#ifdef PARANOID
/* NaiL^d0d: no hijack please, we need random bytes, in stdlib.h */
#include <stdlib.h>
#endif

extern char hostname[NAME_LEN+1];
extern int  use_socks;
char *socks_user = NULL;

#if !defined(WTERM_C) && !defined(STERM_C)

/*
 * Stuff pertaining to bouncing through socks proxies.
 *
 * written by Joshua J. Drake
 * on Nov. 4, 1998
 * last modified, nov. 14
 */
#define SOCKS_PORT	1080
#define SOCKS4_VERSION	4
#define SOCKS5_VERSION	5

/* auth types */
#define AUTH_NONE       0x00
#define AUTH_GSSAPI     0x01
#define AUTH_PASSWD     0x02
#define AUTH_CHAP       0x03

/* auth errors */
#define AUTH_OK        0
#define AUTH_FAIL      -1

/* commands */
#define SOCKS_CONNECT   1
#define SOCKS_BIND      2
#define SOCKS_UDP       3
#define SOCKS_PING      0x80
#define SOCKS_TRACER    0x81
#define SOCKS_ANY       0xff

/* errors */
#define SOCKS5_NOERR            0x00
#define SOCKS5_RESULT           0x00
#define SOCKS5_FAIL             0x01
#define SOCKS5_AUTHORIZE        0x02
#define SOCKS5_NETUNREACH       0x03
#define SOCKS5_HOSTUNREACH      0x04
#define SOCKS5_CONNREF          0x05
#define SOCKS5_TTLEXP           0x06
#define SOCKS5_BADCMND          0x07
#define SOCKS5_BADADDR          0x08

/* flags */
#define SOCKS5_FLAG_NONAME      0x01
#define SOCKS5_FLAG_VERBOSE     0x02
#define SOCKS5_IPV4ADDR         0x01
#define SOCKS5_HOSTNAME         0x03
#define SOCKS5_IPV6ADDR         0x04

char *socks4_error(char cd)
{
	switch (cd)
	{
		case 91:
			return "rejected or failed";
			break;
		case 92:
			return "no identd";
			break;
		case 93:
			return "identd response != username";
			break;
		default:
			return "Unknown error";
	}
}

char *socks5_error(char cd)
{
	switch (cd)
	{
		case SOCKS5_FAIL:
			return "Rejected or failed";
			break;
		case SOCKS5_AUTHORIZE:
			return "Connection not allowed by ruleset";
			break;
		case SOCKS5_NETUNREACH:
			return "Network unreachable";
			break;
		case SOCKS5_HOSTUNREACH:
			return "Host unreachable";
			break;
		case SOCKS5_CONNREF:
			return "Connection refused";
			break;
		case SOCKS5_TTLEXP:
			return "Time to live expired";
			break;
		case SOCKS5_BADCMND:
			return "Bad command";
			break;
		case SOCKS5_BADADDR:
			return "Bad address";
			break;
		default:
			return "Unknown error";
	}
}

/*
 * try to negotiate a SOCKS4 connection.
 *
 */
int socks4_connect(int s, int portnum, struct sockaddr_in *server)
{
struct _sock_connect {
	char version;
	char type;
	unsigned short port;
	unsigned long address;
	char username[NAME_LEN+1];
} sock4_connect;
char socksreq[10];
char *p;
int red;

	memset(&sock4_connect, 0, sizeof(sock4_connect));
	sock4_connect.version = SOCKS4_VERSION;
	sock4_connect.type = SOCKS_CONNECT;
	sock4_connect.port = server->sin_port;

	strncpy(sock4_connect.username, socks_user? socks_user: getenv("USER") ? getenv("USER") : username, NAME_LEN);
	p = inet_ntoa(server->sin_addr);
	sock4_connect.address = inet_addr(p);
	if ((red = write(s, &sock4_connect, 8 + strlen(sock4_connect.username) + 1)) == -1)
	{
		bitchsay("Cannot write to socks proxy: %s", strerror(errno));
		return 0;
	}
	alarm(get_int_var(CONNECT_TIMEOUT_VAR));
	if ((red = read(s, socksreq, 8)) == -1)
	{
		alarm(0);
		bitchsay("Cannot read from socks proxy: %s", strerror(errno));
		return 0;
	}
	alarm(0);
	if (socksreq[1] != 90)
	{
		bitchsay("Cannot connect to SOCKS4 proxy: %s", socks4_error(socksreq[1]));
		return 0;
	}
	return 1;
}

/*
 * try to negotiate a SOCKS5 connection. (with the socket/username, to the server)
 */
int socks5_connect(int s, int portnum, struct sockaddr_in *server)
{
struct _sock_connect {
	char version;
	char type;
	char authtype;
	char addrtype;
	unsigned long address;
	unsigned short port;
	char username[NAME_LEN+1];
} sock5_connect;
char tmpbuf[25], *p;
struct in_addr tmpAddr;
unsigned short tmpI;
int red;

	/* propose any authentication */
	memset(&sock5_connect, 0, sizeof(sock5_connect));
	sock5_connect.version = SOCKS5_VERSION;
	sock5_connect.type = SOCKS_CONNECT;
	sock5_connect.authtype = AUTH_NONE; /* AUTH_PASSWD, AUTH_GSSAPI, AUTH_CHAP */

	if ((red = write(s, &sock5_connect, 4)) == -1)
	{
		bitchsay("Cannot write to proxy: %s", strerror(errno));
		return 0;
	}

	memset(tmpbuf, 0, sizeof(tmpbuf));
	alarm(get_int_var(CONNECT_TIMEOUT_VAR));
	if ((red = read(s, tmpbuf, sizeof(tmpbuf)-1)) == -1)
	{
		alarm(0);
		bitchsay("Cannot use SOCKS5 proxy, read failed during auth: %s", strerror(errno));
		return 0;
	}
	alarm(0);
	/* report server desired authentication (if not none) */
	if (tmpbuf[1] != AUTH_NONE)
	{
		bitchsay("Cannot use SOCKS5 proxy, server wants type %x authentication.", tmpbuf[1]);
		return 0;
	}

	/* try to bounce to target */
	memset(&sock5_connect, 0, sizeof(sock5_connect));
	sock5_connect.version = SOCKS5_VERSION;
	sock5_connect.type = SOCKS_CONNECT;
	sock5_connect.addrtype = SOCKS5_IPV4ADDR;
	p = inet_ntoa(server->sin_addr);
	sock5_connect.address = inet_addr(p);
	sock5_connect.port = server->sin_port;

	if ((red = write(s, &sock5_connect, 10)) == -1)
	{
		bitchsay("Cannot write to the proxy: %s", strerror(errno));
		return 0;
	}
	memset(tmpbuf, 0, sizeof(tmpbuf)-1);
	alarm(get_int_var(CONNECT_TIMEOUT_VAR));
	if ((red = read(s, tmpbuf, sizeof(tmpbuf)-1)) == -1)
	{
		alarm(0);
		bitchsay("Cannot use SOCKS5 proxy, read failed during bounce: %s. Attempting SOCKS4", strerror(errno));
		return 0;
	}
	alarm(0);
	if (tmpbuf[0] != SOCKS5_VERSION)
	{
		bitchsay("This is not a SOCKS5 proxy.");
		return 0;
	}
	if (tmpbuf[1] != SOCKS5_NOERR)
	{
		bitchsay("Cannot use SOCKS5 proxy, server failed: %s", socks5_error(tmpbuf[1]));
		return 0;
	}
	/*
	 * read the rest of the response (depending on what type of address they use)
	 */
	alarm(get_int_var(CONNECT_TIMEOUT_VAR));
	switch (tmpbuf[3])
	{
		case 1:
			read(s, tmpbuf, 4);
			memcpy(&tmpAddr.s_addr, tmpbuf, 4);
			read(s, tmpbuf, 2);
			tmpbuf[3] = '\0';
			tmpI = atoi(tmpbuf);
			bitchsay("SOCKS5 bounce successful, your address will be: %s:%d", inet_ntoa(tmpAddr), ntohs(tmpI));
			break;
		case 3:
		{
			char buffer[256];
			read(s, tmpbuf, 1);
			tmpbuf[1] = '\0';
			tmpI = atoi(tmpbuf);
			read(s, tmpbuf, tmpI);
			tmpbuf[tmpI] = '\0';
			strncpy(buffer, tmpbuf, sizeof(buffer));
			read(s, tmpbuf, 2);
			tmpbuf[3] = '\0';
			tmpI = atoi(tmpbuf);
			bitchsay("SOCKS5 bounce successful, your address will be: %s:%d", buffer, ntohs(tmpI));
			break;
		}
		case 4:
			read(s, tmpbuf, 18);
			/* don't report address of ipv6 addresses. */
			bitchsay("SOCKS5 bounce successful. [ipv6]");
			break;
		default:
			bitchsay("error tmpbuf[3]: %x", tmpbuf[3]);
			alarm(0);
			return 0;
	}
	alarm(0);
	return 1;
}

int handle_socks(int fd, struct sockaddr_in addr, char *host, int portnum)
{
	struct sockaddr_in proxy;
	struct hostent *hp;
                
	memset(&proxy, 0, sizeof(proxy));
	if (!(hp = gethostbyname(host)))
	{
		bitchsay("Unable to resolve SOCKS proxy host address: %s", host);
		return -1;
	}
	bcopy(hp->h_addr, (char *)&proxy.sin_addr, hp->h_length);
	proxy.sin_family = AF_INET;
	proxy.sin_port = htons(portnum);
	alarm(get_int_var(CONNECT_TIMEOUT_VAR));
	if (connect(fd, (struct sockaddr *)&proxy, sizeof(proxy)) < 0)
	{
		alarm(0);
		bitchsay("Unable to connect to SOCKS5 proxy: %s", strerror(errno));
		close(fd);
		return -1;
	}
	alarm(0);
	if (!socks5_connect(fd, portnum, &addr))
	{
		close(fd);
		if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		{
			bitchsay("Unable to get socket: %s", strerror(errno));
			return -1;
		}
		alarm(get_int_var(CONNECT_TIMEOUT_VAR));
		if (connect(fd, (struct sockaddr *)&proxy, sizeof(proxy)) < 0)
		{
			alarm(0);
			bitchsay("Unable to connect to SOCKS4 proxy: %s", strerror(errno));
			return -1;
		}
		alarm(0);
		if (!socks4_connect(fd, portnum, &addr))
		{
			close(fd);
			return -1;
		}
	}
	return fd;
}
#endif


/*
 * connect_by_number:  Wheeeee. Yet another monster function i get to fix
 * for the sake of it being inadequate for extension.
 *
 * we now take four arguments:
 *
 *	- hostname - name of the host (pathname) to connect to (if applicable)
 *	- portnum - port number to connect to or listen on (0 if you dont care)
 *	- service -	0 - set up a listening socket
 *			1 - set up a connecting socket
 *	- protocol - 	0 - use the TCP protocol
 *			1 - use the UDP protocol
 *
 *
 * Returns:
 *	Non-negative number -- new file descriptor ready for use
 *	-1 -- could not open a new file descriptor or 
 *		an illegal value for the protocol was specified
 *	-2 -- call to bind() failed
 *	-3 -- call to listen() failed.
 *	-4 -- call to connect() failed
 *	-5 -- call to getsockname() failed
 *	-6 -- the name of the host could not be resolved
 *	-7 -- illegal or unsupported request
 *	-8 -- no socks access
 *
 *
 * Credit: I couldnt have put this together without the help of BSD4.4-lite
 * User SupplimenTary Document #20 (Inter-process Communications tutorial)
 */
int BX_connect_by_number(char *hostn, unsigned short *portnum, int service, int protocol, int nonblocking)
{
	int fd = -1;
	int is_unix = (hostn && *hostn == '/');
	int sock_type, proto_type;

#ifndef __OPENNT
	sock_type = (is_unix) ? AF_UNIX : AF_INET;
#else
	sock_type = AF_INET;
#endif

	proto_type = (protocol == PROTOCOL_TCP) ? SOCK_STREAM : SOCK_DGRAM;

	if ((fd = socket(sock_type, proto_type, 0)) < 0)
		return -1;

	set_socket_options (fd);

	/* Unix domain server */
#ifdef HAVE_SYS_UN_H
	if (is_unix)
	{
		struct sockaddr_un name;

		memset(&name, 0, sizeof(struct sockaddr_un));
		name.sun_family = AF_UNIX;
		strcpy(name.sun_path, hostn);
#ifdef HAVE_SUN_LEN
# ifdef SUN_LEN
		name.sun_len = SUN_LEN(&name);
# else
		name.sun_len = strlen(hostn) + 1;
# endif
#endif

		if (is_unix && (service == SERVICE_SERVER))
		{
			if (bind(fd, (struct sockaddr *)&name, strlen(name.sun_path) + sizeof(name.sun_family)))
				return close(fd), -2;
			if (protocol == PROTOCOL_TCP)
				if (listen(fd, 4) < 0)
					return close(fd), -3;
		}

		/* Unix domain client */
		else if (service == SERVICE_CLIENT)
		{
			alarm(get_int_var(CONNECT_TIMEOUT_VAR));
			if (connect (fd, (struct sockaddr *)&name, strlen(name.sun_path) + 2) < 0)
			{
				alarm(0);
				return close(fd), -4;
			}
			alarm(0);
		}
	}
	else
#endif

	/* Inet domain server */
	if (!is_unix && (service == SERVICE_SERVER))
	{
		int length;
#ifdef IP_PORTRANGE
		int ports;
#endif
		/* Even on an IPv6 client this opens up a IPv4 socket... for now.
		 * (Some OSes need two sockets to be able to accept both IPv4 and
		 * IPv6 connections). */
		struct sockaddr_in name;

		memset(&name, 0, sizeof name);
		name.sin_family = AF_INET;
		name.sin_addr.s_addr = htonl(INADDR_ANY);

		name.sin_port = htons(*portnum);
#ifdef PARANOID
		name.sin_port += (unsigned short)(rand() & 255);
#endif
		
#ifdef IP_PORTRANGE
		if (getenv("EPIC_USE_HIGHPORTS"))
		{
			ports = IP_PORTRANGE_HIGH;
			setsockopt(fd, IPPROTO_IP, IP_PORTRANGE, 
					(char *)&ports, sizeof(ports));
		}
#endif

		if (bind(fd, (struct sockaddr *)&name, sizeof(name)))
			return close(fd), -2;

		length = sizeof (name);
		if (getsockname(fd, (struct sockaddr *)&name, &length))
			return close(fd), -5;

		*portnum = ntohs(name.sin_port);

		if (protocol == PROTOCOL_TCP)
			if (listen(fd, 4) < 0)
				return close(fd), -3;
#ifdef NON_BLOCKING_CONNECTS
		if (nonblocking && set_non_blocking(fd) < 0)
			return close(fd), -4;
#endif
	}

	/* Inet domain client */
	else if (!is_unix && (service == SERVICE_CLIENT))
	{
		struct sockaddr_foobar server;
		int server_len;
		struct hostent *hp;
#ifdef WINNT
		char buf[BIG_BUFFER_SIZE+1];
#endif		
#ifdef IPV6
		struct addrinfo hints, *res;
#else
		struct sockaddr_in localaddr;
		if (LocalHostName)
		{
			memset(&localaddr, 0, sizeof(struct sockaddr_in));
			localaddr.sin_family = AF_INET;
			localaddr.sin_addr = LocalHostAddr.sf_addr;
			localaddr.sin_port = 0;
			if (bind(fd, (struct sockaddr *)&localaddr, sizeof(localaddr)))
				return close(fd), -2;
		}
#endif

		memset(&server, 0, sizeof server);
#ifndef WINNT

#ifdef IPV6
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_flags = AI_ADDRCONFIG;
		hints.ai_socktype = proto_type;

		if (!getaddrinfo(hostn, NULL, &hints, &res) && res)
		{
			close(fd);

			if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
				return -1;
			set_socket_options (fd);

			memcpy(&server, res->ai_addr, res->ai_addrlen);
			server_len = res->ai_addrlen;
			server.sf_port = htons(*portnum);

			memset(&hints, 0, sizeof hints);
			hints.ai_family = res->ai_family;
			freeaddrinfo(res);
 
			if (LocalHostName && !getaddrinfo(LocalHostName, NULL, &hints, &res) && res)
			{
				int retval = bind(fd, (struct sockaddr *) res->ai_addr, res->ai_addrlen);
				freeaddrinfo(res);

				if (retval)	
					return close(fd), -2;
			}
		}
		else
			return close(fd), -6;

#else
		if (isdigit((unsigned char)hostn[strlen(hostn)-1]))
			inet_aton(hostn, (struct in_addr *)&server.sf_addr);
		else
		{
			if (!(hp = gethostbyname(hostn)))
	  			return close(fd), -6;
			memcpy(&server.sf_addr, hp->h_addr, hp->h_length);
		}
		server.sf_family = AF_INET;
		server.sf_port = htons(*portnum);
		server_len = sizeof server.sins.sin;
#endif /* IPV6 */
		
#else
		/* for some odd reason resolv() fails on NT... */
/*		server = (*(struct sockaddr_in *) hostn);*/
		if (!hostn)
		{
			gethostname(buf, sizeof(buf));
			hostn = buf;
		}
		if ((server.sf_addr.s_addr = inet_addr(hostn)) == -1)
		{
			if ((hp = gethostbyname(hostn)) != NULL)
			{
				memset(&server, 0, sizeof(server));
				bcopy(hp->h_addr, (char *) &server.sf_addr,
					hp->h_length);
				server.sf_family = hp->h_addrtype;
			}
			else
				return (-2);
		}
		else
		server.sf_family = AF_INET;
		server.sf_port = (unsigned short) htons(*portnum);
		server_len = sizeof server.sins.sin;
#endif /* WINNT */

#ifdef NON_BLOCKING_CONNECTS
		if (!use_socks && nonblocking && set_non_blocking(fd) < 0)
			return close(fd), -4;
#endif

#if !defined(WTERM_C) && !defined(STERM_C) && !defined(IPV6)

		if (use_socks && get_string_var(SOCKS_HOST_VAR))
		{

			fd = handle_socks(fd, *((struct sockaddr_in*) &server), get_string_var(SOCKS_HOST_VAR), get_int_var(SOCKS_PORT_VAR));
			if (fd == -1)
				return -4;
			else
				return fd;
		}
#endif
		alarm(get_int_var(CONNECT_TIMEOUT_VAR));
		if (connect(fd, (struct sockaddr *)&server, server_len) < 0 && errno != EINPROGRESS)
		{
			alarm(0);
			return close(fd), -4;
		}
		alarm(0);
	}

	/* error */
	else
		return close(fd), -7;

	return fd;
}

int	lame_resolv (const char *hostname, struct sockaddr_foobar *buffer)
{
#ifdef IPV6
	struct addrinfo *res;
	if (getaddrinfo(hostname, NULL, NULL, &res) || !res || 
		res->ai_addrlen > sizeof *buffer)
		return -1;

	memmove(buffer, res->ai_addr, res->ai_addrlen);
	freeaddrinfo(res);
	return 0;
#else
	struct hostent 	*hp;

	if (!(hp = gethostbyname(hostname)))
		return -1;

	buffer->sf_family = AF_INET;
	memmove(&buffer->sf_addr, hp->h_addr, hp->h_length);
	return 0;
#endif	
}

#ifdef IPV6

extern struct sockaddr_foobar *BX_lookup_host (const char *host)
{
	static struct sockaddr_foobar sf;
	struct addrinfo *res;
	
	if (!getaddrinfo(host, NULL, NULL, &res) && res)
	{
		memcpy(&sf, res->ai_addr, sizeof(struct sockaddr_foobar));
		freeaddrinfo(res);

		return &sf;
	}

	return NULL;
}

extern char *BX_host_to_ip (const char *host)
{
	struct addrinfo *res;
	struct sockaddr_foobar *sf;
	static char ip[128];
	
	if (!getaddrinfo(host, NULL, NULL, &res) && res)
	{
		sf = (struct sockaddr_foobar*) res->ai_addr;
		inet_ntop(sf->sf_family, (sf->sf_family == AF_INET) ?
		  (void*) &sf->sf_addr : (void*) &sf->sf_addr6, ip, 128);
		freeaddrinfo(res);

		return ip;
	}
	else
		return empty_string;
}

extern char *BX_ip_to_host (const char *ip)
{
	static char host[128];
	struct addrinfo hints = { 0 };
	struct addrinfo *res;

	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_NUMERICHOST;

	if (getaddrinfo(ip, NULL, &hints, &res) == 0)
	{
		if (!res || getnameinfo(res->ai_addr, res->ai_addrlen, host, 128, NULL, 0, 0))
			strlcpy(host, ip, sizeof host);
		freeaddrinfo(res);
	}
	else
		strlcpy(host, ip, sizeof host);

	return host;
}

extern char *BX_one_to_another (const char *what)
{
	if (isdigit(what[strlen(what)-1]) || strchr(what, ':'))
		return ip_to_host (what);
	else
		return host_to_ip (what);
}

#else

extern struct sockaddr_foobar *BX_lookup_host (const char *host)
{
	struct hostent *he;
	static struct sockaddr_foobar sf;

	alarm(1);
	he = gethostbyname(host);
	alarm(0);
	if (he)
	{
		sf.sf_family = AF_INET;
		memcpy(&sf.sf_addr, he->h_addr, sizeof(struct in_addr));
		return &sf;
	}
	else
		return NULL;
}

extern char *BX_host_to_ip (const char *host)
{
	struct hostent *hep = gethostbyname(host);
	static char ip[30];

	return (hep ? sprintf(ip,"%u.%u.%u.%u",	hep->h_addr[0] & 0xff,
						hep->h_addr[1] & 0xff,
						hep->h_addr[2] & 0xff,
						hep->h_addr[3] & 0xff),
						ip : empty_string);
}

extern char *BX_ip_to_host (const char *ip)
{
	struct in_addr ia;
	struct hostent *he;
	static char host[101];
	
	ia.s_addr = inet_addr(ip);
	he = gethostbyaddr((char*) &ia, sizeof(struct in_addr), AF_INET);

	return (he ? strncpy(host, he->h_name, 100): empty_string);
}

extern char *BX_one_to_another (const char *what)
{

	if (!isdigit((unsigned char)what[strlen(what)-1]))
		return host_to_ip (what);
	else
		return ip_to_host (what);
}

#endif

/*
 * It is possible for a race condition to exist; such that select()
 * indicates that a listen()ing socket is able to recieve a new connection
 * and that a later accept() call will still block because the connection
 * has been closed in the interim.  This wrapper for accept() attempts to
 * defeat this by making the accept() call nonblocking.
 */
int	my_accept (int s, struct sockaddr *addr, int *addrlen)
{
	int	retval;
	set_non_blocking(s);
	retval = accept(s, addr, addrlen);
	set_blocking(s);
	return retval;
}



int BX_set_non_blocking(int fd)
{
#ifdef NON_BLOCKING_CONNECTS
	int	res;

#if defined(NBLOCK_POSIX)
	int nonb = 0;
	nonb |= O_NONBLOCK;
#elif defined(NBLOCK_BSD)
	int nonb = 0;
	nonb |= O_NDELAY;
#elif defined(NBLOCK_SYSV)
	res = 1;

	if (ioctl (fd, FIONBIO, &res) < 0)
		return -1;
#else
#error no idea how to set an fd to non-blocking
#endif
#if (defined(NBLOCK_POSIX) || defined(NBLOCK_BSD)) && !defined(NBLOCK_SYSV)
	if ((res = fcntl(fd, F_GETFL, 0)) == -1)
		return -1;
	else if (fcntl(fd, F_SETFL, res | nonb) == -1)
		return -1;
#endif
#endif
	return 0;
}

int BX_set_blocking(int fd)
{
#ifdef NON_BLOCKING_CONNECTS
	int	res;

#if defined(NBLOCK_POSIX)
	int nonb = 0;
	nonb |= O_NONBLOCK;
#elif defined(NBLOCK_BSD)
	int nonb = 0;
	nonb |= O_NDELAY;
#elif defined(NBLOCK_SYSV)
	res = 0;

	if (ioctl (fd, FIONBIO, &res) < 0)
		return -1;
#else
#error no idea how to return an fd blocking
#endif
#if (defined(NBLOCK_POSIX) || defined(NBLOCK_BSD)) && !defined(NBLOCK_SYSV)
	if ((res = fcntl(fd, F_GETFL, 0)) == -1)
		return -1;
	else if (fcntl(fd, F_SETFL, res &~ nonb) == -1)
		return -1;
#endif
#endif
	return 0;
}


