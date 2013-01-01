/*  ARCFour - a symmetric streaming cipher - Implimentation by Humble
 *
 *  This is a variable-key-length symmetric stream cipher developed in 1987
 *  by Ron Rivest (the R in RSA). It used to be proprietary but was reverse
 *  engineered and released publicly in September 1994. The cipher is now
 *  freely available but the name RC4 is a trademark of RSA Data Security
 *  Inc. This cipher is secure, proven, easy to impliment, and quite fast.
 */
#define IN_MODULE
#include "irc.h"
#include "struct.h"
#include "ircaux.h"
#include "ctcp.h"
#include "status.h"
#include "lastlog.h"
#include "server.h"
#include "screen.h"
#include "vars.h"
#include "misc.h"
#include "output.h"
#include "module.h"
#include "hash2.h"
#include "hook.h"
#include "dcc.h"
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include "arcfour.h"
#include "md5.h"
#define INIT_MODULE
#include "modval.h"

typedef struct {
	int sock;
	char ukey[16];
	arckey *outbox;
	arckey *inbox;
} arclist;

/* from dcc.c. Why isnt this in dcc.h? */

typedef struct _DCC_List
{
	struct _DCC_List *next;
	char *nick;   /* do NOT free this. it's a shared pointer. */
	SocketList sock;
} DCC_List;
      

static arclist *keyboxes[16];

static const double arc_ver = 1.0;
static unsigned int typenum = 0;

/*
 * Initialize our box, and return the key used. It better be enough to hold
 * 16 chars + 1 null!
 */

static char *init_box(char *ukey, arckey *key)
{
	MD5_CTX md5context;
	char buf[256];
	int fd;

	fd = open("/dev/urandom", O_RDONLY);
	if (fd >= 0) {
		read(fd, buf, sizeof(buf));
		close(fd);
	}
	else
	{
		buf[(int)buf[69]] ^= getpid();
		buf[(int)buf[226]] ^= getuid();
		buf[(int)buf[119]] ^= getpid();
	}

	MD5Init(&md5context);
	MD5Update(&md5context, buf, sizeof(buf));
	MD5Final(&md5context);

	memcpy(ukey, buf, 16);
	ukey[16] = '\0';

	arcfourInit(key, md5context.digest, 16);
	return ukey;
}

static inline void arcfourInit(arckey *arc, char *userkey, unsigned short len)
{
	register arcword *S = arc->state, x = 0, y = 0, pos = 0, tmp;

	/* Seed the S-box linearly, then mix in the key while stiring briskly */
	arc->i = arc->j = 0;				 /* Initialize i and j to 0 */
	while((S[x] = --x));				 /* Initialize S-box, backwards */

	/* Note: Some of these optimizations REQUIRE arcword to be 8-bit unsigned */
	do {						 /* Spread user key into real key */
		y += S[x] + userkey[pos];			 /* Keys, shaken, not stirred */
		tmp = S[x]; S[x] = S[y]; S[y] = tmp;  /* Swap S[i] and S[j] */
		if (++pos >= len)	pos = 0;		 /* Repeat user key to fill array */
	} while(++x);				 /* ++x is faster than x++ */
}

static inline char *arcfourCrypt(arckey *arc, char *data, int len)
{
	register arcword *S = arc->state, i = arc->i, j = arc->j, tmp;
	register int c = 0;

	do {
		j += S[++i];				/* Shake S-box, stir well */
		tmp = S[i]; S[i] = S[j]; S[j] = tmp; /* Swap S[i] and S[j] */
		data[c] ^= S[(S[i] + S[j])];		/* XOR the crypto into our data */
	} while (++c < len);				/* Neat, ++x is faster then x++ */

	arc->i = i;					/* Save modified i and j counters */
	arc->j = j;					/* Continue where we left off */
	return data;					/* Return pointer to ciphertext */
}

int Arcfour_Init(IrcCommandDll **intp, Function_ptr *global_table)
{
	initialize_module("arcfour");
	memset(keyboxes, 0, sizeof(keyboxes));
/*
	dcc_output_func = send_dcc_encrypt;
	dcc_input_func = get_dcc_encrypt;
	dcc_open_func = start_dcc_crypt;
	dcc_close_func = end_dcc_crypt;
*/
	typenum = add_dcc_bind("SCHAT", "schat", init_schat, start_dcc_crypt, get_dcc_encrypt, send_dcc_encrypt, end_dcc_crypt);
	add_module_proc(DCC_PROC, "schat", "schat", "Secure DCC Chat", 0, 0, dcc_sdcc, NULL);
	return 0;
}

int Arcfour_Cleanup(IrcCommandDll **intp)
{
/*	remove_dcc_bind("SCHAT", typenum); */
	return 1;
}

static arclist *find_box(int sock)
{
	int i, tmp;
	tmp = sizeof(keyboxes)/sizeof(arclist *);
	for (i = 0; i < tmp; i++)
		if (keyboxes[i] && (keyboxes[i]->sock == sock))
			return keyboxes[i];
	return (arclist *)NULL;
}


static char *dcc_crypt (int sock, char *buf, int len)
{
	arclist *box;
	if ((box = find_box(sock))) {
		arcfourCrypt(box->outbox, buf, len-2);
		return buf;
	}
	return NULL;
}

static int send_dcc_encrypt (int type, int sock, char *buf, int len)
{
	if (type == DCC_CHAT) {
		if (dcc_crypt(sock, buf, len)) {
			write(sock, buf, len);
			return 0;
		}
	}
	return -1;
}

static int get_dcc_encrypt (int type, int sock, char *buf, int parm, int len)
{
	if (type == DCC_CHAT) {
		if ((len = dgets(buf, sock, parm, BIG_BUFFER_SIZE, NULL)) > 0) {
			buf[len-1] = '\0';
			dcc_crypt(sock, buf, len);
			if (buf[len])
				buf[len] = '\0';
		}
	}
	return len;
}

/* Here we initialize the cryptography. Send the other end our key, and read
 * in theirs. The socket "s" won't have a crypt box unless it is supposed to
 * an encrypted connection.
 */


static int start_dcc_crypt (int s, int type, unsigned long d_addr, int d_port)
{
	arclist *tmpbox;
	put_it("start_dcc_crypt");
	if ((tmpbox = find_box(s))) {
		int len;
		char randkey[17];
		char buf[BIG_BUFFER_SIZE+1];
		memset(randkey, 0, sizeof(randkey));
		memset(buf, 0, sizeof(buf));
		tmpbox->outbox = (arckey *)new_malloc(sizeof(arckey));
		init_box(randkey, tmpbox->outbox);
		snprintf(buf, BIG_BUFFER_SIZE, "SecureDCC %s\r\n%n", randkey, &len);
		write(s, buf, len);
		memset(buf, 0, sizeof(buf));
		if ((len = dgets(buf, s, 1, BIG_BUFFER_SIZE, NULL)) > 0) {
			if (!my_strnicmp("SecureDCC", buf, 9)) {
				tmpbox->inbox = (arckey *)new_malloc(sizeof(arckey));
       	arcfourInit(tmpbox->inbox, next_arg(buf, &buf), 16);
			}
		}
		return 0;
	}
	return -1;
}

static int end_dcc_crypt (int s, unsigned long d_addr, int d_port)
{
	int i;
	for(i = 0; i < 16; i++) {	
		if (keyboxes[i] && (keyboxes[i]->sock == s)) {
			new_free(&(keyboxes[i]->inbox));
			new_free(&(keyboxes[i]->outbox));
			new_free(&keyboxes[i]);
			return 0;
		}
	}
	return -1;
}


static void start_dcc_chat(int s)
{
struct	sockaddr_in	remaddr;
int	sra;
int	type;
int	new_s = -1;
char	*nick = NULL;	
unsigned long flags;
DCC_int *n = NULL;
SocketList *sa;

	sa = get_socket(s);
	flags = sa->flags;
	nick = sa->server;
	sra = sizeof(struct sockaddr_in);
	new_s = accept(s, (struct sockaddr *) &remaddr, &sra);
	type = flags & DCC_TYPES;
	n = get_socketinfo(s);
	if ((add_socketread(new_s, ntohs(remaddr.sin_port), flags, nick, get_dcc_encrypt, NULL)) < 0)
	{
		erase_dcc_info(s, 0, "%s", convert_output_format("$G %RDCC error: accept() failed. punt!!", NULL, NULL));
		close_socketread(s);
		return;
	}
	flags &= ~DCC_WAIT;
	flags |= DCC_ACTIVE;
	set_socketflags(new_s, flags);
	set_socketinfo(new_s, n);

/*
	put_it("%s", convert_output_format(fget_string_var(FORMAT_DCC_CONNECT_FSET), 
		"%s %s %s %s %s %d", update_clock(GET_TIME),
		dcc_types[type],
		nick, n->userhost?n->userhost:"u@h",
		inet_ntoa(remaddr.sin_addr),ntohs(remaddr.sin_port)));
*/
	get_time(&n->starttime);
	close_socketread(s);
	start_dcc_crypt(new_s, type, n->remote.s_addr, ntohs(remaddr.sin_port));
}

/* set up the dcc hooks */

void dcc_sdcc (char *name, char *args)
{
	char *p;
	int tmp, i;
	DCC_int *new_sdcc;
	if (!my_stricmp(name, "schat") && (strlen(args) > 0)) {
		if (*args == ' ')
			new_next_arg(args, &args);
		else {
			p = strchr(args, ' ');
			if (p && *p)
				*p = 0;
		}
		new_sdcc = dcc_create(args, "SCHAT", NULL, 0, 0, typenum, DCC_TWOCLIENTS, start_dcc_chat);
/*		find_dcc_pending(new_sdcc->user, new_sdcc->filename, NULL, typenum, 1, -1); */
/*		new_i = find_dcc_pending(nick, filename, NULL, type, 1, -1); */
		tmp = sizeof(keyboxes)/sizeof(arclist *);
		for (i = 0; i < tmp; i++)
			if (!keyboxes[i])
/*				keyboxes[i]->sock = new_i->sock */;
	}
}

/* thanks to the new add_dcc_bind, we dont have to worry about any hooking... */

static int init_schat(char *type, char *nick, char *userhost, char *description, char *size, char *extra, unsigned long ip, unsigned int port)
{
/*	new_sdcc = dcc_create(args, "SCHAT", NULL, 0, 0, typenum, DCC_TWOCLIENTS, start_dcc_chat); */
	return 0;
}
