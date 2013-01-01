/*
napster code base by Drago (drago@0x00.org)
released: 11-30-99
*/

#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>

#include "napi.h"


_N_SERVER *n_GetServer(void) {
	char serverline[1024], *server;
	int fd, r, port;
	struct sockaddr_in socka;
	static _N_SERVER theserver;

	fd=socket(AF_INET, SOCK_STREAM, 0);
	socka.sin_addr.s_addr=inet_addr(n_nslookup(N_DISTRO_SERVER));
	socka.sin_family=AF_INET;
	socka.sin_port=htons(N_DISTRO_SERVER_PORT);

	if (connect(fd, (struct sockaddr *)&socka, sizeof(struct sockaddr))!=0) {
		n_Error("connect()");
		close(fd);
		return NULL;
	}

	r=read(fd, serverline, sizeof(serverline));
	if (r==-1) {
		n_Error("read()");
		close(fd);
		return NULL;
	}
	server=strstr(serverline, ":");
	if (!server) {
		n_Error("No port token?");
		close(fd);
		return NULL;
	}

	*server='\0';
	server++;
	port=atoi(server);
	server=serverline;

	strncpy(theserver.address, server, sizeof(theserver.address));
	theserver.port=port;

	n_Debug("Server: %s Port: %d", theserver.address, theserver.port);
	close(fd);
	return &theserver;
}

char *n_nslookup(char *addr) {
	struct hostent *h;
	if ((h=gethostbyname(addr)) == NULL) {
		return addr;
	}
	return (char *)inet_ntoa(*((struct in_addr *)h->h_addr));
}

int n_Connect(_N_SERVER *s, _N_AUTH *a) {
	int r, port;
	struct sockaddr_in socka;

	n_serverfd=socket(AF_INET, SOCK_STREAM, 0);
	socka.sin_addr.s_addr=inet_addr(n_nslookup(s->address));
	socka.sin_family=AF_INET;
	socka.sin_port=htons(s->port);

	if (connect(n_serverfd, (struct sockaddr *)&socka, sizeof(struct sockaddr))!=0) {
		n_Error("connect()");
		close(n_serverfd);
		return 0;
	}

	n_Debug("Connected");

	n_SendCommand(CMDS_LOGIN, "%s %s %d \"v2.0 BETA 3\" %d", a->username, a->password, n_dataport, n_connectionspeed);
	{
		_N_COMMAND *cmd;
		cmd=n_GetCommand();
		if (cmd->cmd[2]==CMDR_ERROR) {
			n_Error("%s", cmd->data);
			return 0;
		} else {
			n_HandleCommand(cmd);
		}
	}
	return 1;
}

void n_HandleCommand(_N_COMMAND *cmd) {
	switch (cmd->cmd[2]) {
		case CMDR_MOTD:
			if (n_HookMotd) {
				n_HookMotd(cmd->data);
			} else {
				n_Debug("No motd hook installed");
			}
		break;
		case CMDR_STATS:
/*
			D:napi.c:n_GetCommand():171:Data: 2104 197246 798
			2104==Libraries
			197246==songs
			798==gigs
*/
			if (n_HookStats) {
				_N_STATS s;
				if (sscanf(cmd->data, "%d %d %d",
					&s.libraries, &s.songs, &s.gigs)!=3) {
					n_Error("Too few args");
				}
				else n_HookStats(&s);
			} else {
				n_Debug("No stats hook installed");
			}
		break;
	}
}

void n_SendCommand(_N_CMD ncmd, char *fmt, ...) {
	char buff[2048];
	_N_COMMAND command;
	va_list ap;

	va_start(ap, fmt);

	command.cmd[0]=vsnprintf(buff, sizeof(buff), fmt, ap);
	va_end(ap);

	command.cmd[1]='\0';
	command.cmd[2]=ncmd;
	command.cmd[3]='\0';

	n_Debug("Flags: %d %d %d %d", command.cmd[0], command.cmd[1], command.cmd[2], command.cmd[3]);
	n_Debug("Data: %s", buff);

	n_Send(command.cmd, sizeof(command.cmd));
	n_Send(buff, command.cmd[0]);
}

int n_Send(char *data, int s) {
	return write(n_serverfd, data, s);
}

int n_Loop(void) {
	int sret;
	struct timeval tv;
	fd_set rfd;
	FD_ZERO(&rfd);
	FD_SET(n_serverfd, &rfd);
	tv.tv_sec=0;
	tv.tv_usec=0;

	sret = select(n_serverfd+1, &rfd, NULL, NULL, &tv);
	if (sret>0) {
		if (FD_ISSET(n_serverfd, &rfd)) {
			n_DoCommand();
			return 1;
		}
	}
}

_N_COMMAND *n_GetCommand(void) {
	static char rbuff[1024];
	static _N_COMMAND command;
	read(n_serverfd, command.cmd, sizeof(command.cmd));
	if (command.cmd[1]==0) {
		int r;
		assert(sizeof(rbuff) > command.cmd[0]);
		r=n_ReadCount(rbuff, command.cmd[0]);
	} else {
		int r=0;
		int cc=command.cmd[3]+1;
		while(cc>0) {
			assert(sizeof(rbuff) > r);
			if (read(n_serverfd, &rbuff[r], sizeof(char))==1) r++;
			if (rbuff[r-1]=='.') cc--;
		}
		rbuff[r]=0;
	}
	command.data=rbuff;
        n_Debug("Flags: %d %d %d %d", command.cmd[0], command.cmd[1], command.cmd[2], command.cmd[3]);
        n_Debug("Data: %s", command.data);
	return &command;
}

void n_DoCommand(void) {
	_N_COMMAND *cmd;
	cmd=n_GetCommand();
	n_HandleCommand(cmd);
}

int n_ReadCount(char *buff, int c) {
	int rc=0;
	while (c>rc) {
		if (read(n_serverfd, &buff[rc], sizeof(char))==1) rc++;
	}
	buff[rc]=0;
}

void n_SetMotdHook(void (*f)(char *)) {
	n_HookMotd=f;
	n_Debug("Installed motd hook");
}

void n_SetStatsHook(void (*f)(_N_STATS *)) {
	n_HookStats=f;
	n_Debug("Installed stats hook");
}

