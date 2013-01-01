/*
 * gaim
 *
 * Copyright (C) 1998-1999, Mark Spencer <markster@marko.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 * 
 */


/* 
 * Modified by Nadeem Riaz (nads@bleh.org)
 * 
 * Slight changes to better incorporate into libtoc
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/wait.h>
#include "toc.h"

void set_state(int i)
{
        state = i;
}

int escape_text(char *msg)
{
	char *c, *cpy;
	int cnt=0;
	/* Assumes you have a buffer able to cary at least BUF_LEN * 2 bytes */
	if (strlen(msg) > BUF_LEN) {
		fprintf(stderr, "Warning:  truncating message to 2048 bytes\n");
		msg[2047]='\0';
	}
	
	cpy = strdup(msg);
	c = cpy;
	while(*c) {
		switch(*c) {
		case '{':
		case '}':
		case '\\':
		case '"':
			msg[cnt++]='\\';
			/* Fall through */
		default:
			msg[cnt++]=*c;
		}
		c++;
	}
	msg[cnt]='\0';
	free(cpy);
	return cnt;
}


int escape_message(char *msg)
{
	char *c, *cpy;
	int cnt=0;
	/* Assumes you have a buffer able to cary at least BUF_LEN * 2 bytes */
	if (strlen(msg) > BUF_LEN) {
		toc_debug_printf("Warning:  truncating message to 2048 bytes\n");
		msg[2047]='\0';
	}
	
	cpy = strdup(msg);
	c = cpy;
	while(*c) {
		switch(*c) {
		case '$':
		case '[':
		case ']':
		case '(':
		case ')':
		case '#':
			msg[cnt++]='\\';
			/* Fall through */
		default:
			msg[cnt++]=*c;
		}
		c++;
	}
	msg[cnt]='\0';
	free(cpy);
	return cnt;
}

char *normalize(char *s)
{
	static char buf[BUF_LEN];
        char *t, *u;
        int x=0;

        u = t = malloc(strlen(s) + 1);

        strcpy(t, s);
        strdown(t);
        
	while(*t) {
		if (*t != ' ') {
			buf[x] = *t;
			x++;
		}
		t++;
	}
        buf[x]='\0';
        free(u);
	return buf;
}

void strdown(char *s) { 
	while ( *s ) {
		if ( *s >= 65 && *s <= 90) 
			*s += 32;
		s++;
	}
}

void toc_debug_printf(char *fmt, ...)
{
#ifdef DEBUG_LIB_TOC
	FILE *fp;
        char data[MAX_OUTPUT_MSG_LEN];
        va_list ptr;	
	if ( ! (fp=fopen(TOC_DEBUG_LOG,"a")) ) {
		perror("ERROR couldn't open debug log file!@$\n");
	}	
        va_start(ptr, fmt);
        vsnprintf(data, MAX_OUTPUT_MSG_LEN - 1 , fmt, ptr);
        va_end(ptr);
	fprintf(fp,"%s\n",data);
	fflush(fp);
        fclose(fp);
        return;  
#endif
}

void toc_msg_printf(int type, char *fmt, ...) {
        char data[MAX_OUTPUT_MSG_LEN];
	char *args[1];
        va_list ptr;	
        va_start(ptr, fmt);
        vsnprintf(data, MAX_OUTPUT_MSG_LEN - 1 , fmt, ptr);
        va_end(ptr);
	args[0] = data;
	use_handler(TOC_HANDLE,type,args);
        return;  	
}

char * strip_html(char * text)
{
	int i, j;
	int visible = 1;
	char *text2 = malloc(strlen(text) + 1);
	
	strcpy(text2, text);
	for (i = 0, j = 0;text2[i]; i++)
	{
		if(text2[i]=='<')
		{	
			visible = 0;
			continue;
		}
		else if(text2[i]=='>')
		{
			visible = 1;
			continue;
		}
		if(visible)
		{
			text2[j++] = text2[i];
		}
	}
	text2[j] = '\0';
	return text2;
}

