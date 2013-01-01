#ifndef _PROXY_H
#define _PROXY_H

/* proxy types */
#define PROXY_NONE 0
#define PROXY_HTTP 1
#define PROXY_SOCKS 2		/* Not Implemented !! */


extern struct hostent * proxy_gethostbyname(char *host);
extern int proxy_connect(int  sockfd, struct sockaddr *serv_addr, int addrlen );

extern int proxy_type;
extern char proxy_host[256];
extern int proxy_port;
extern char *proxy_realhost;

#endif /* _PROXY_H */
