/*
 * Qbx by pod (pod@caro.net)
 * Some of this code is from Qir by Ben Turner (seti@detroit.org)
 */

#include "irc.h"
#include "struct.h"
#include "hook.h"
#include "module.h"
#define INIT_MODULE
#include "modval.h"

#include "qbx.h"

#define cparse convert_output_format

int qfd, querying = 0, qbx_on = 1, q_type = 0;
char q_server[256], q_chan[256];
struct timeval q_tv;

void privmsg(char *to, const char *format, ...) {
	va_list ap;
	char buffer[1024];
	va_start(ap, format);
	vsnprintf(buffer, sizeof(buffer), format, ap);
	va_end(ap);
	queue_send_to_server(from_server, "PRIVMSG %s :%s", to, buffer);
}

int time_delta(struct timeval *later, struct timeval *past) {
    if(later->tv_usec < past->tv_usec)  {
	later->tv_sec--;
	later->tv_usec+= 1000000;
    }
    return (later->tv_sec - past->tv_sec) * 1000 +
	(later->tv_usec - past->tv_usec) / 1000;
}


void q_timeout(int fd) {
	put_it("No response from %s", q_server);
	privmsg(q_chan, "No response from %s", q_server);
	close_socketread(qfd);
	querying = 0;
}

void q_timer(int fd) {
	int i = 0, j = 0, k = 0, type = q_type;
	int nextpart = 0, num_players = 0, cheats = 0;
	struct quake_variable variable[50];
	struct timeval tv;
	char status[65507], temp[1024];
	char server_hostname[1024], server_maxclients[1024], server_mapname[1024];
	char server_fraglimit[1024], server_timelimit[1024], server_game[1024];

	/* ick. */
	memset(&temp, 0, sizeof(temp));
	memset(&server_hostname, 0, sizeof(server_hostname));
	memset(&server_maxclients, 0, sizeof(server_maxclients));
	memset(&server_mapname, 0, sizeof(server_mapname));
	memset(&server_fraglimit, 0, sizeof(server_fraglimit));
	memset(&server_timelimit, 0, sizeof(server_timelimit));
	memset(&server_game, 0, sizeof(server_game));

	/* now we should be expecting something back */
	memset(&status, 0, sizeof(status));
	if(recv(fd, status, 65507, 0) < 0) {
		put_it("Error receiving from %s: %s", q_server, strerror(errno));
		privmsg(q_chan, "Error receiving from %s: %s", q_server, strerror(errno));
		close_socketread(fd);
		querying = 0;
		return;
	}
	gettimeofday(&tv, NULL);
	close_socketread(fd);

	memset(&variable, 0, sizeof(variable));

	/* now we have our info, but we still need to decode it! */
	if(type == 1)
		i = 7;
	else if(type == 2)
		i = 11;
	else if(type == 3)
		i = 20;

	/* the following while loop copies all the variables from the status message
		into an array together with their value */
	while (status[i] != '\n') {
		/* check whether it's time for a new variable */
		if (status[i] == '\\') {
			if (nextpart) {
				nextpart = 0;
				variable[j].value[k] = '\0';
				j++;
			} else {
				nextpart = 1;
				variable[j].name[k] = '\0';
			}
			k = 0;
		} else {
			/* put our character in the correct position */
			if (nextpart)
				variable[j].value[k] = status[i];
			else
				variable[j].name[k] = status[i];
			k++;
		}
		i++;
	}
	variable[j].value[k] = '\0';

	/* now we can count how many players there are on the server */
	i++;
	put_it(&status[i]);
	while (i < strlen(status)) {
		if (status[i] == '\n') {
			num_players++;
		}
		i++;
	}

	for (i = 0; i < 50; i++) {
		if(type != 3) {
			if(!strcmp("hostname", variable[i].name))
				strcpy(server_hostname, variable[i].value);
			if(!strcmp("maxclients", variable[i].name))
				strcpy(server_maxclients, variable[i].value);
		} else {
			if(!strcmp("sv_hostname", variable[i].name))
				strcpy(server_hostname, variable[i].value);
			if(!strcmp("sv_maxclients", variable[i].name))
				strcpy(server_maxclients, variable[i].value);
			if(!strcmp("g_gametype", variable[i].name)) {
				switch(atoi(variable[i].name)) {
					case 0:  strcpy(server_game, "FFA"); break;
					case 1:  strcpy(server_game, "DUEL"); break;
					case 3:  strcpy(server_game, "TEAM DM"); break;
					case 4:  strcpy(server_game, "CTF"); break;
					default: strcpy(server_game, "UNKNOWN"); break;
				}
			}
		}
		if(type == 1) {
			if(!strcmp("map", variable[i].name))
				strcpy(server_mapname, variable[i].value);
			if(!strcmp("*gamedir", variable[i].name))
				strcpy(server_game, variable[i].value);
			if(!strcmp("cheats", variable[i].name))
				cheats = 1;
		}
		else {
			if(!strcmp("mapname", variable[i].name))
				strcpy(server_mapname, variable[i].value);
		}
		if(type == 2) {
			if(!strcmp("gamename", variable[i].name))
				strcpy(server_game, variable[i].value);
		}
		if(!strcmp("timelimit", variable[i].name))
			strcpy(server_timelimit, variable[i].value);
		if(!strcmp("fraglimit", variable[i].name))
			strcpy(server_fraglimit, variable[i].value);
	}
	if(type == 1) {
		snprintf(temp, 1024, "%s : players: %d/%s, ping: %d, map: %s, timelimit: %s, fraglimit: %s", server_hostname, num_players, server_maxclients, time_delta(&tv, &q_tv),  server_mapname, server_timelimit, server_fraglimit);
		if(server_game[0] != '\0') {
			char temp2[1024];
			snprintf(temp2, 1024, ", game: %s", server_game);
			strcat(temp, temp2);
		}
		if(cheats)
			strcat(temp, ", cheats enabled");
	}
	if(type == 2)
		snprintf(temp, 1024, "%s : players: %d/%s, ping: %d, map: %s, timelimit: %s, fraglimit: %s, game: %s", server_hostname, num_players, server_maxclients, time_delta(&tv, &q_tv), server_mapname, server_timelimit, server_fraglimit, server_game);
	if(type == 3)
		snprintf(temp, 1024, "%s : players: %d/%s, ping: %d, map: %s, gametype: %s, timelimit: %s, fraglimit: %s", server_hostname, num_players, server_maxclients, time_delta(&tv, &q_tv), server_mapname, server_game, server_timelimit, server_fraglimit);
	put_it(temp);
	privmsg(q_chan, temp);
	querying = 0;
}

void query_q_server(char *server, int port, int game) {
	char buffer[16];

	struct sockaddr_in qsock;
	struct hostent *he;

	querying = 1;

	/* look up our quakeserver address */
	if ((he = gethostbyname(server)) == NULL) {
		put_it("unknown host: %s", server);
		close(qfd);
		querying = 0;
		return;
	}

	qfd = connect_by_number(server, &port, SERVICE_CLIENT, PROTOCOL_UDP, 1);
	memset(buffer, 0, sizeof(buffer));
	memset(&qsock, 0, sizeof(struct sockaddr_in));
	if(game == 3)
		strcpy(buffer, "\xFF\xFF\xFF\xFFgetstatus");
	else
		strcpy(buffer, "\xFF\xFF\xFF\xFFstatus");

	qsock.sin_family = AF_INET;
	qsock.sin_port = htons(port);
	qsock.sin_addr = *((struct in_addr *)he->h_addr);

	put_it("Sending status request to %d.%d.%d.%d...",
			(unsigned char) he->h_addr_list[0][0],
			(unsigned char) he->h_addr_list[0][1],
			(unsigned char) he->h_addr_list[0][2],
			(unsigned char) he->h_addr_list[0][3]);

	/* send our information to the quake server */
	sendto(qfd, buffer, strlen(buffer), 0, (struct sockaddr *)&qsock, sizeof(struct sockaddr));
	gettimeofday(&q_tv, NULL);
	
	strncpy(q_server, server, 256);
	q_type = game;
	add_socketread(qfd, port, 0, server, q_timer, NULL);
	add_sockettimeout(qfd, 5, q_timeout);
}

int pub_proc(char *which, char *str, char **unused) {
	char *loc, *nick, *chan, *cmd, *serv;
	int port = 0;

	if(qbx_on == 0) return 1;
	loc = LOCAL_COPY(str);
	nick = next_arg(loc, &loc);
	chan = next_arg(loc, &loc);
	cmd = next_arg(loc, &loc);
	if(cmd && *cmd != '!') return 1;
	if(my_stricmp(cmd, Q3A_COMMAND) && my_stricmp(cmd, Q2_COMMAND) && my_stricmp(cmd, QW_COMMAND))
		return 1;
	serv = next_arg(loc, &loc);
	if(serv == NULL) {
		privmsg(chan, "%s: Give me a server to query", nick);
		return 1;
	}
	if(querying == 1) {
		privmsg(chan, "%s: A query is already in progress", nick);
		return 1;
	}
	if(strchr(serv, ':')) {
		serv = strtok(serv, ":");
		port = atoi(strtok(NULL, ""));
	}
	else
		port = 0;
	strncpy(q_chan, chan, 256);
	if(!my_stricmp(cmd, Q3A_COMMAND)) { /* quake3 server (my fav =) */
		if(port == 0) port = Q3A_GAME_PORT;
		query_q_server(serv, port, 3);
		return 1;
	}
	else if(!my_stricmp(cmd, Q2_COMMAND)) { /* quake2 server */
		if(port == 0) port = Q2_GAME_PORT;
		query_q_server(serv, port, 2);
		return 1;
	}
	else if(!my_stricmp(cmd, QW_COMMAND)) { /* quakeworld server */
		if(port == 0) port = QW_GAME_PORT;
		query_q_server(serv, port, 1);
		return 1;
	}
	return 1;
}

BUILT_IN_DLL(qbx_cmd)
{
	if(!my_stricmp(args, "on")) {
		qbx_on = 1;
		put_it("Qbx turned on");
		return;
	}
	if(!my_stricmp(args, "off")) {
		qbx_on = 0;
		put_it("Qbx turned off");
		return;
	}
	userage("QBX", helparg);
}

int Qbx_Init(IrcCommandDll **intp, Function_ptr *global_table) {
	initialize_module("qbx");
	add_module_proc(HOOK_PROC, "qbx", NULL, "* % !q*", PUBLIC_LIST, 1, NULL, pub_proc);
	add_module_proc(HOOK_PROC, "qbx", NULL, "* % !q*", PUBLIC_OTHER_LIST, 1, NULL, pub_proc);
	add_module_proc(COMMAND_PROC, "qbx", "qbx", NULL, 0, 0, qbx_cmd, "<on|off>\n- Turns Qbx on or off");
	put_it("Qbx %s loaded", QBX_VERSION);
	return 0;
}

char *Qbx_Version(IrcCommandDll **intp) {
	return QBX_VERSION;
}
