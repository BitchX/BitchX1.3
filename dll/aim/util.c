/*
 * AOL Instant Messanger Module for BitchX 
 *
 * By Nadeem Riaz (nads@bleh.org)
 *
 * util.c
 *
 * utility/misc functions
 */

#include <irc.h>
#include <struct.h>
#include <hook.h>
#include <ircaux.h>
#include <output.h>
#include <lastlog.h>
#include <status.h>
#include <vars.h>
#include <window.h>
#include <module.h>
#include <modval.h>

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "toc.h"
#include "aim.h"


void statusprintf(char *fmt, ...)
{
        char data[MAX_STATUS_MSG_LEN];
	char *sfmt, *prompt;
	va_list ptr;
	
	/* Tack on prompt */
	prompt = get_dllstring_var("aim_prompt");
	sfmt = (char *) malloc(strlen(prompt) + strlen(fmt)+5);	
	strcpy(sfmt,prompt);
	strcat(sfmt," ");
	strcat(sfmt,fmt);
	
        va_start(ptr, fmt);
        vsnprintf(data, MAX_STATUS_MSG_LEN - 1 , sfmt, ptr);
        va_end(ptr);
	free(sfmt);
        statusput(LOG_CRAP,data);
        return;  
}

void statusput(int log_type, char *buf) {	
	int lastlog_level;
	lastlog_level = set_lastlog_msg_level(log_type);

	if (get_dllint_var("aim_window") > 0)
		if (!(target_window = get_window_by_name("AIM")))
			target_window = current_window;
	
	if (window_display && buf)
	{
		add_to_log(irclog_fp, 0, buf, 0);
		add_to_screen(buf);
	}

	target_window = NULL;
	set_lastlog_msg_level(lastlog_level);
	return;
}

void msgprintf(char *fmt, ...)
{
        char data[MAX_STATUS_MSG_LEN];
	va_list ptr;
	
        va_start(ptr, fmt);
        vsnprintf(data, MAX_STATUS_MSG_LEN - 1 , fmt, ptr);
        va_end(ptr);
        statusput(LOG_CRAP,data);
        return;  
}

void debug_printf(char *fmt, ...)
{
#ifdef DEBUG_AIM
	FILE *fp;
        char data[MAX_OUTPUT_MSG_LEN];
        va_list ptr;	
	if ( ! (fp=fopen(AIM_DEBUG_LOG,"a")) ) {
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

char *rm_space(char *s) {
	char *rs;
	int x,y;
	rs = (char *) malloc(strlen(s) + 1);	
	x = y = 0;
	
	for (x =0; x<strlen(s);x++) {
		if ( s[x] != ' ' ) {  
			rs[y] = s[x];
			y++;
		}
	}
	rs[y] = '\0';
	return rs;
}

/* 
 * Still dont like split args routines in bitchx (new_next_arg)
 * It doesnt allow for args with "'s in them, next_arg doesnt allow
 * for args with spaces !@$. Need a *real* split routne that can handle
 * both (with escaping)
 */

/*
char **split_args(char *args,int max;) {
	int x;
	char *list[32];
	int arg = 0;
	int escape  = 0;
	int open_quote = 0;
	for (x=0;x<strlen(args);x++) {
		if ( args[x] == '\' ) {
			if ( ! escape ) {
				escape = 1;
				continue;
			}
		} else ( ! escape && args[x] == '"' ) {
			if ( open_quote ) {
				push(@args,$warg);
				$warg ="";
				arg++;
				$argbegin = $x+1;
				open_quote = 0;
			} else {
				open_quote = 1;
			}
			continue;
		} elsif ( ! $escape && ! $open_quote && substr($text,$x,1) eq " " ) {
			if ( ! $space ) {
				push(@args,$warg);
				$warg ="";
				$arg++;				
				$space = 1;
			}
			$argbegin = $x+1;
			next;
		} 
		if ( $space ) {
			$argbegin = $x;
			$space = 0;			
		} elsif ( $escape ) {
			$escape = 0;
		}
		$warg .= substr($text,$x,1);
	}		
	}
}


*/
