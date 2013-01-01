#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include "toc.h"

extern int state;
int sfd;
int (*cb)(int);

int toc_got_im(char **args) {
	printf("msg: %s %s",args[0],args[1]);
	return 1;
}

int toc_remove_input_stream(int fd) {
        return 1;
}

int toc_add_input_stream(int fd,int (*func)(int)) {
	sfd = fd;
	cb = func;
	printf("got input stream!\n");
	return 1;
}

void statusput(char *buf) {
	printf("%s\n",buf);
}

void statusprintf(char *fmt, ...)
{
        char data[MAX_OUTPUT_MSG_LEN];
        va_list ptr;
        va_start(ptr, fmt);
        vsnprintf(data, MAX_OUTPUT_MSG_LEN - 1 , fmt, ptr);
        va_end(ptr);
        statusput(data);
        return;  
}

int main(int argc, char **argv) {
	fd_set set;
	init_toc();
	printf("state: %d\n",state);
	toc_login(argv[1],argv[2]);
	install_handler(TOC_IM_IN,&toc_got_im);
	printf("back from toc login call!\n");
	while ( 1 )  {
		FD_SET(sfd,&set);
		if ( select(sfd+1,&set,NULL,NULL,NULL) ) {
			if ( FD_ISSET(sfd,&set) ) {
				printf("data on sock!\n");
				cb(sfd);
			}
		}
		FD_ZERO(&set);
	}
	return 1;
}
