 /* $Id: cdns.h 3 2008-02-25 09:49:14Z keaston $ */
 
/*
 * cdns.h: header for cdns.c
 */

#ifndef _CDNS_H_
#define _CDNS_H_

/* We gotta know about the fd_set type, so we gonna inclue this  */
#include "irc.h"

#include "struct.h"
#include "newio.h"
#endif

#define MAXALIASES	35
#define MAXADDRS	35

typedef struct	_hent {
	char	*h_name;	/* official name of host */
	char	*h_aliases[MAXALIASES];	/* alias list */
	int	h_addrtype;	/* host address type */
	int	h_length;	/* length of address */
	/* list of addresses from name server */
	struct	in_addr	h_addr_list[MAXADDRS];
#define	h_addr	h_addr_list[0]	/* address, for backward compatiblity */
} my_hostent;

typedef struct dns_struct {
	/* 'in': This is the string we want to resolve. It can be
	 * either a host or ip. The string WILL be malloc'ed by
	 * the "add" function. Do NOT malloc or free this variable!
	 */
	char *in;

	/* 'out': This is our "resolved" string. It can be
	 * either a host or ip. The string WILL be malloc'ed by
	 * the internal resolver. Do NOT malloc or free this variable!
	 */
	char *out;

	/* alias: this is passed to the alias parser which then interpret's 
	 * the output.
	 */
	char *alias;
	
	/* 'callback': This is our callback function. When our 'in' gets
	 * resolved, we will call this function, with this structure
	 * as our parameter. Do NOT malloc or free this variable.
	 */
	void (*callback) (struct dns_struct *); /* Our callback function */

	/* 'callinfo': This allows us to store information that you might
	 * need for later use when in your 'callback' function. This is
	 * just a void pointer, so you MUST malloc any of the data you want to
	 * store here. Once you are done with this variable, you must
	 * free it!!! BEWARE BEWARE, YOU MUST FREE THIS VARIABLE YOURSELF!
	 */
	void *callinfo;

	/* 'ip': Internally used, to find out what the 'out' variable is.
	 * ie, is it a host, or is it ip
	 */
	int ip;

	/* hostentr: A copy of the hostent structure returned from the
	 * name server.  I am adding it here to extend the ability to
	 * read multiple IPs.  Although this is not generally necessary.
	 * - Brian Smith
	 */
	my_hostent *hostentr;

	/* 'next': Internally used. our "next" pointer. */
        struct dns_struct *next;
} DNS_QUEUE;

void start_dns(void);
void stop_dns(void);
void kill_dns(void);
void set_dns_output_fd(fd_set *);
void dns_check(fd_set *);
void check_dns_queue(void);
void add_to_dns_queue(char *, void (*) (DNS_QUEUE *), char *, void *, int);
my_hostent *duphostent(struct hostent *orig);
void freemyhostent(my_hostent *freeme);

#define DNS_URGENT 1
#define DNS_NORMAL 2

#define Q_NEXT(tmp) ((tmp)->next)
#define Q_OPEN(headp, tailp) (*(headp) = *(tailp) = NULL)

