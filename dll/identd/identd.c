/*
 * experimental identd module for bx. 
 * adds a /set identd on/off toggle.
 * adds a /set identd_user <username>
 * and implements a identd server for bx. This obviously won't work unless
 * your running as root. And is mainly meant for use with os/2 and win95 
 * ports. 
 * Copyright 1998 Colten Edwards. 
 */ 

#include "irc.h"
#include "struct.h"
#include "ircaux.h"
#include "output.h"
#include "vars.h"
#include "module.h"
#define INIT_MODULE
#include "modval.h"

void identd_read(int s)
{
char buffer[100];
char *bufptr;
unsigned int lport = 0, rport = 0;
	*buffer = 0;
	bufptr = buffer;
	if (recv(s, buffer, sizeof(buffer)-1, 0) <=0)
	{
		bitchsay("ERROR in identd request");
		close_socketread(s);
		return;
	}
	if (sscanf(bufptr, "%d , %d", &lport, &rport) == 2)
	{
		if (lport < 1 || rport < 1 || lport > 32767 || rport > 32767)
		{
			close_socketread(s);
			bitchsay("ERROR port for identd bad [%d:%d]", lport, rport);
			return;
		}
		sprintf(buffer, "%hu , %hu : USERID : UNIX : %s", lport, rport, get_dllstring_var("identd_user"));
		dcc_printf(s, "%s\r\n", buffer);
		bitchsay("Sent IDENTD request %s", buffer);
		set_socketflags(identd, now);
	}
	close_socketread(s);
}

void identd_handler(int s)
{
struct  sockaddr_in     remaddr;
int sra = sizeof(struct sockaddr_in);
int sock = -1;
#if 0
	if (!(get_dllint_var("identd")) || !(get_dllstring_var("identd_user")))
	{
		int opt = 0;
		int optlen = sizeof(opt);
		getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&opt, optlen);
		return;
	}
#endif
	if ((sock = accept(s, (struct sockaddr *) &remaddr, &sra)) > -1)
	{
		if (!(get_dllint_var("identd")) || !(get_dllstring_var("identd_user")))
		{
			close(sock);
			return;
		}
		add_socketread(sock, s, 0, inet_ntoa(remaddr.sin_addr), identd_read, NULL);
		add_sockettimeout(sock, 20, NULL);
	}
}

int start_identd(void)
{
int sock = -1;
unsigned short port = 113;
	if (identd != -1)
		return -1;
	if ((sock = connect_by_number(NULL, &port, SERVICE_SERVER, PROTOCOL_TCP, 1)) > -1)
		add_socketread(sock, port, 0, NULL, identd_handler, NULL);
	identd = sock;
	return 0;
}

int Identd_Cleanup(IrcCommandDll **intp, Function_ptr *global_table)
{
	if (identd != -1)
	{
		close_socketread(identd);
		identd = -1;
	}
	remove_module_proc(VAR_PROC, MODULENAME, NULL, NULL);
	return 0;
}

int Identd_Init(IrcCommandDll **intp, Function_ptr *global_table)
{
#if !defined(__EMX__) && !defined(WINNT)
	if (getuid() && geteuid())
	{
		return -1;
	}
#endif
	initialize_module("Identd");
	add_module_proc(VAR_PROC, MODULENAME, "identd", NULL, BOOL_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(VAR_PROC, MODULENAME, "identd_user", NULL, STR_TYPE_VAR, 0, NULL, NULL);
	put_it("%s", convert_output_format("$G $0 v$1 by panasync", "%s %s", MODULENAME, "0.01"));
	return start_identd();
}
