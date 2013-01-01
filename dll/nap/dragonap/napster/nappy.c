/*
napster code base by Drago (drago@0x00.org)
released: 11-30-99
*/

#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <signal.h>
#include <stdarg.h>
#include <assert.h>

//#define PACKET_FILE "10.0.0.115.1165-208.178.163.58.8888"

//#define PACKET_FILE "10.0.0.115.1146-208.178.163.58.7777"
//#define PACKET_FILE "208.178.163.58.6666-10.0.0.115.1117"

#define DEBUG

#ifdef DEBUG
#define D(fmt, args...) fprintf(stderr, fmt, ## args)
#else
#define D(fmt, args...) (void)0
#endif

#define I(fmt, args...) fprintf(stdout, fmt, ## args)

#define E(fmt, args...) fprintf(stderr, fmt, ## args)


typedef unsigned char COMMAND;

char *nslookup(char *addr);
int connect_to(char *to, int port);

void NAP_login(void); 
int NAP_send(char *, int);
void NAP_disconnect(void);
void NAP_connect(void);
void NAP_loop(void);
void NAP_getcommand(void);
void NAP_addfile(char *, char *, int, int, int, int);
int NAP_readcount(char *, int); 
void KB_getcommand(void);
void NAP_search(char *); 
int getargs(char *, char ***);
void freeargs(char **);
char *argstring(char **);
void NAP_requestfile(char *, char *); 
void NAP_getfile(char *);
char *basefile(char *);

char *username;
char *password;
int dataport=0;
int connectionspeed=6;

enum recv_Commands {
	CMDR_STATS=214,
	CMDR_EMAILADDR=3,
	CMDR_MOTD=109,
	CMDR_SEARCHRESULTS=201,
	CMDR_SEARCHRESULTSEND=202,
	CMDR_ERROR=0,
	CMDR_ANOTHERUSER=148,
	CMDR_SENDFILE=95,
	CMDR_USERSPEED=209,
	CMDR_REQUESTUSERSPEED=89,
	CMDR_FILEREADY=204,
	CMDR_GETERROR=206,
	CMDR_GETQUEUE=108
};


enum send_Commands {
	CMDS_LOGIN=2,
	CMDS_SEARCH=200,
	CMDS_REQUESTFILE=203,
	CMDS_ADDFILE=100,
	CMDS_ADDHOTLIST=208
};

int NAP_fd;


int main(int argc, char **argv) {
	if (argc<3) {
		I("nappy v0.1Beta1 By Drago (drago@0x00.org)\n");
		I("Usage: %s <username> <password> [line speed]\n", argv[0]);
		exit(1);
	}
	username=argv[1];
	password=argv[2];
	if (argc>3) connectionspeed=atoi(argv[3]);
	NAP_connect();
	NAP_login();
	NAP_loop();
	NAP_disconnect();
	exit(0);
}

void NAP_loop() {
	int sret;
	struct timeval tv, tmptv;
	fd_set rfd, tmprfd;
	FD_ZERO(&rfd);
	FD_SET(NAP_fd, &rfd);
	FD_SET(STDIN_FILENO, &rfd);
	tv.tv_sec=1;
	tv.tv_usec=0;
	while (1) {
		tmprfd=rfd;
		tmptv=tv;
		sret = select(NAP_fd+1, &tmprfd, NULL, NULL, &tmptv);
		if (sret>0) {
			if (FD_ISSET(NAP_fd, &tmprfd)) {
				NAP_getcommand();
			}
			if (FD_ISSET(STDIN_FILENO, &tmprfd)) {
				KB_getcommand();
			}
		}
	}
}

void NAP_sendcommand(COMMAND ncmd, char *fmt, ...) {
	char buff[2048];
	COMMAND cmd[4];

	va_list ap;
	va_start(ap, fmt);
	cmd[0]=vsnprintf(buff, sizeof(buff), fmt, ap);
	va_end(ap);

	cmd[1]='\0';
	cmd[2]=ncmd;
	cmd[3]='\0';

	D("Send Flags: %d %d %d %d\n", cmd[0], cmd[1], cmd[2], cmd[3]);
	D("Send Data: %s\n", buff);

	NAP_send(cmd, 4);
	NAP_send(buff, cmd[0]);
}

void NAP_getcommand(void) {
	char rbuff[1024];
	static COMMAND cmd[4];
	read(NAP_fd, cmd, sizeof(cmd));
	if (cmd[1]==0) {
		int r;
		assert(sizeof(rbuff) > cmd[0]);
		r=NAP_readcount(rbuff, cmd[0]);
	} else {
		int r=0;
		int cc=cmd[3]+1;
		while(cc>0) {
			assert(sizeof(rbuff) > r);
			if (read(NAP_fd, &rbuff[r], sizeof(char))==1) r++;
			if (rbuff[r-1]=='.') cc--;
		}
		rbuff[r]=0;
	}

	D("Recv Flags: %d %d %d %d\n", cmd[0], cmd[1], cmd[2], cmd[3]);
	D("Recv Data: %s\n", rbuff);

	switch(cmd[2]) {
		case CMDR_GETQUEUE:
			I("Added to get queue: %s\n", rbuff);
		break;
		case CMDR_REQUESTUSERSPEED:
			I("Requested user speed: %s\n", rbuff);
		break;
		case CMDR_GETERROR:
			I("Get error: %s\n", rbuff);
		break;
		case CMDR_FILEREADY:
			I("File is ready: %s\n", rbuff);
			NAP_getfile(rbuff);
		break;
		case CMDR_USERSPEED:
			I("User speed: %s\n", rbuff);
		break;
		case CMDR_SENDFILE:
			I("Someone trying to get file: %s\n", rbuff);
		break;

		case CMDR_ANOTHERUSER:
			I("Another user has logged in as you!\n");
			exit(1);
		break;

		case CMDR_ERROR:
			E("Error: %s\n", rbuff);
		break;

		case CMDR_SEARCHRESULTS:
			I("Search results: %s\n", rbuff);
		break;

		case CMDR_SEARCHRESULTSEND:
			I("End of search results\n");
		break;

		case CMDR_STATS:
			I("Got stats: %s\n", rbuff);
		break;

		case CMDR_EMAILADDR:
			I("Email address: %s\n", rbuff);
		break;

		case CMDR_MOTD:
			I("Message: %s\n", rbuff);
		break;

		default:
			E("Unknown command: %d\n", cmd[2]);
		break;
	}
}

int NAP_readcount(char *buff, int c) {
	int rc=0;
	while (c>rc) {
		if (read(NAP_fd, &buff[rc], sizeof(char))==1) rc++; 
	}
	buff[rc]=0;
}

void NAP_disconnect(void) {
	close(NAP_fd);
}

void NAP_connect(void) {
#ifdef PACKET_FILE
	NAP_fd=open(PACKET_FILE, O_RDONLY);
#else
	NAP_fd=connect_to("208.178.163.59", 8888);
#endif
	if (!NAP_fd>0) {
		E("Error making connection.\n");
		exit(1);	
	}
}

void NAP_login(void) {
	int c;
	char loginstr[1024];
	c=snprintf(loginstr, sizeof(loginstr), "%s %s %d \"v2.0 BETA 3\" %d", username, password, dataport, connectionspeed);
	NAP_sendcommand(CMDS_LOGIN, loginstr);
}

int NAP_send(char *data, int s) {
	return write(NAP_fd, data, s);
}

int connect_to(char *to, int port) {
   struct sockaddr_in socka;
   int fd;
   fd=socket(AF_INET, SOCK_STREAM, 0);
   socka.sin_addr.s_addr=inet_addr(nslookup(to));
   socka.sin_family=AF_INET;
   socka.sin_port=htons(port);
   if (connect(fd, (struct sockaddr *)&socka, sizeof(struct sockaddr))!=0) {
      return -1;
   }
   return fd;
}

char *nslookup(char *addr) {
	struct hostent *h;
	if ((h=gethostbyname(addr)) == NULL) {
		return addr;
	}
	return (char *)inet_ntoa(*((struct in_addr *)h->h_addr))        ;
}

void KB_getcommand(void) {
	char **argv=NULL;
	int argc;
	char buff[1024];
	fgets(buff, sizeof(buff), stdin);
	buff[strlen(buff)-1]=0;
	argc=getargs(buff, &argv);

	if (argc>0) {
		if (!strcasecmp(argv[0], "raw")) {
			if (argc==1) {
				I("raw <command id> <data>\n");
			} else {
				char *data;
				if (argc==2) NAP_sendcommand(atoi(argv[1]), "");
				else {
					data=argstring(&argv[2]);
					NAP_sendcommand(atoi(argv[1]), data);
					free(data);
				}
			}
		} else if (!strcasecmp(argv[0], "get")) {
			if (argc<3) {
				I("get <name> <file>\n");
			} else {
				char *f;
				f=argstring(&argv[2]);
				NAP_requestfile(argv[1], f);
				free(f);
			}
		} else if (!strcasecmp(argv[0], "search")) {
			if (argc==1) {
				I("search <file>");
			} else {
				char *s;
				s=argstring(&argv[1]);
				NAP_search(s);
				free(s);
			}
		}
	}

	freeargs(argv);

//	NAP_sendcommand(CMDS_GETFILE, "Avenger713 \"The A-Team Theme.mp3\"");

}

void NAP_search(char *string) {
/*
FILENAME CONTAINS \"filename\" MAX_RESULTS 100000
*/

	NAP_sendcommand(CMDS_SEARCH, "FILENAME CONTAINS \"%s\" MAX_RESULTS 10", string);
}

void NAP_addfile(char *filen, char *checksum, int size, int bitrate, int freq, int length) {
/* 
Recv Flags: 121 0 100 0
Recv Data: "C:\napster2\Music\Korn - Follow The Leader - 15 - Got The Life.mp3"
fd40cd10955270701dd169d4e6739858 749568 160 44100 37 
*/
	NAP_sendcommand(CMDS_ADDFILE, "\"%s\" %s %d %d %d %d", filen, checksum, size, bitrate, freq, length);
}

void NAP_requestfile(char *from, char *file) {
	NAP_sendcommand(CMDS_REQUESTFILE, "%s \"%s\"", from, file);
}

int getargs(char *s, char ***sargs) {
	char *orig;
	int escape=0;
	char *beginword;
	char *endword;
	char **args=NULL;
	int argc=0;
	int myidx=0;

	beginword=s;
	endword=NULL;

	while (1) {
		switch(*s) {
			case '"':
				escape^=1;
			break;

			case ' ':
				if (!escape)
					endword=s;
			break;
		}
		s++;
		if (*s=='\0') endword=s;
		if (endword) {
			int len=endword-beginword;
			char tmp[len+1];
			strncpy(tmp, beginword, len);
			tmp[len]=0;
			endword=NULL;
			beginword=s;
			myidx=argc;
			argc++;
			args=(char **) realloc(args, sizeof(char *) * argc);
			assert(args!=NULL);
			args[myidx]=strdup(tmp);
		}
		if (!*s) break;
	}
	myidx=argc;
	argc++;
	args=(char **) realloc(args, sizeof(char *) * argc);
	assert(args!=NULL);
	args[myidx]=NULL;
	*sargs=args;
	return argc-1;
}

void freeargs(char **args) {
	char **save=args;
	while (*args) {
		free(*args);
		args++;
	}
	free(save);
}

char *argstring(char **start) {
	char *buildit=NULL;
	int len=0;
	int addl=0;

	while (*start) {
		addl=strlen(*start);
		buildit=(char *) realloc(buildit, addl+len+2);
		assert(buildit!=NULL);
		strncpy(&buildit[len], *start, addl);
		len+=addl;
		buildit[len]=' ';
		buildit[len+1]='\0';
		len++;
		start++;
	}
	if (buildit!=NULL) {
		buildit[len-1]=0;
	}
	return buildit;
}

void NAP_getfile(char *data) {
/*
gato242 3068149784 6699 "d:\mp3\Hackers_-_07_-_Orbital_-_Halcyon_&_On_&_On.mp3"
8b451240c17fec98ea4f63e26bd42c60 7
*/

	int getfd, argc, port, linespeed, outfd, rc;
	unsigned long int longip;
	char getcmd[1024], cip[20], **argv, *nick, *ip, *file, *checksum;
	char indata[1024];
	char *fixedfile, c;
	char *savefile;
	struct sockaddr_in socka;
	argc=getargs(data, &argv);
	nick=argv[0];
	ip=argv[1];
	port=atoi(argv[2]);
	file=argv[3];
	checksum=argv[4];
	linespeed=atoi(argv[5]);
	getfd=socket(AF_INET, SOCK_STREAM, 0);
	socka.sin_addr.s_addr=atol(ip);
	socka.sin_family=AF_INET;
	socka.sin_port=htons(port);
	if (connect(getfd, (struct sockaddr *)&socka, sizeof(struct sockaddr))!=0) {
		E("error connecting to file host (%s)\n", strerror(errno));
		return;
	}

	read(getfd, &c, sizeof(c));
	snprintf(getcmd, sizeof(getcmd), "GET");
	write(getfd, getcmd, strlen(getcmd));
	sleep(1);
        snprintf(getcmd, sizeof(getcmd), "%s %s 0", username, file);
	write(getfd, getcmd, strlen(getcmd));

	fixedfile=file;
	fixedfile++;
	fixedfile[strlen(fixedfile)-1]=0;

	savefile=basefile(fixedfile);

	outfd=open(savefile, O_CREAT|O_EXCL|O_WRONLY);
	if (!outfd) {
		E("Error opening output file %s: %s\n", savefile, strerror(errno));
		return;
	}
	if (fork()) return;
	I("New child has been spawned with pid %d for download of %s from %s\n", getpid(), savefile, nick);
	while ((rc=read(getfd, indata, sizeof(indata)))>0) {
		if (rc==0) continue;
		write(outfd, indata, rc);
	}	
	I("File %s from %s done!\n", savefile, nick);
	close(getfd);
	close(outfd);
}

char *basefile(char *s) {
	char *lastslash;
	lastslash=strrchr(s, '\\');
	if (!lastslash) return basename(s);
	lastslash++;
	return basename(lastslash);
}

