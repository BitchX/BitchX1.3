/*
 * AOL Instant Messanger Module for BitchX 
 *
 * By Nadeem Riaz (nads@bleh.org)
 *
 * aim.c
 *
 * Window, Init, Cleanup, and Version Routines
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
#include <input.h>
#include <module.h>
#define INIT_MODULE
#include <modval.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "toc.h"
#include "aim.h"


char *name = "aim";
char *timer_id;

#ifdef BITCHX_PATCH
struct tab_key_struct tks;

int do_aim_tabkey_overwrite(int x, char *p, int *c, char **s) {
	if ( get_dllint_var("aim_window") && current_window == get_window_by_name("AIM") ) 
		return 1;
	else
		return 0;
}

char * get_next_buddy_complete() {
	char *bud = NULL;
	int a;
	LLE t;	
	/* 
	 * This is about as badly written as humany possible 
	 * ^- some would say its even worse than that
	 */
	while ( 1 ) {
		if ( tks.list == 1 ) {
			/* We traverse the buddy list forwards */
			LL mems;
			if ( tks.pos > groups->items ) {
				break;
			}
			t = groups->head;
			for (a=0; a < tks.pos; a++) 
				t = t->next;
			mems = ((struct group *)t->data)->members;			
			if ( tks.subpos == -1 )
				tks.subpos = 1;			
			if ( tks.subpos > mems->items ) {
				tks.pos++;
				continue;
			}
			t = mems->head;
			for (a=0; a<tks.subpos; a++) 
				t = t->next;
			tks.subpos++;
			bud = t->key;
			break;			
		} else {		
			/* Traverse the msd'd them list in reverse (last msg'd = first completed) */	
			t = msgdthem->head;
			tks.list = 0;			
			/* If we haven't msg'd anyone yet, go through the buddy list */
			if ( msgdthem == NULL || msgdthem->items == 0) {
				debug_printf("msgdthem == null or has no items");
				tks.list = 1;
				tks.pos = 1;
				tks.subpos = -1;
				continue;
			}
			/* Initilization */			
			if ( tks.pos == -1 ) {
				debug_printf("set tks.pos to %d",msgdthem->items);
				tks.pos = msgdthem->items;
			}
			for (a=0; a < tks.pos; a++) {
				t = t->next;
			}
			bud = t->key;
			debug_printf("tks.pos == %d name = %s",tks.pos,t->key);
			tks.pos--;
			/* No more msg'd them nicks, next call we switch over to buddy list */
			if ( tks.pos == 0 ) {
				tks.list = 1;
				tks.pos = 1;
				tks.subpos = -1;
			}
			break;
		}	
	}				
	return bud;
}

char * aim_tabkey_overwrite(int x, char *p, int *c, char **s) {
	char *z = NULL;	
	char *bud = NULL;
	char *t;
	(*c) = 0;	
	
	if ( state != STATE_ONLINE )
		return NULL;
	
	bud = get_next_buddy_complete();
		
	if ( bud == NULL )
		return NULL;
	debug_printf("bud = %s",bud);
	t = (char *) malloc(strlen(bud)+50);
	sprintf(t,"/amsg %s  ",bud);
	m_s3cat(&z,space,t);
	(*c) = 1;
	return z;
}

char * amsg_complete_func(int x, char *p, int *c, char **s) {
	/* statusprintf("x = %d",x); */
	char *z = NULL;
	char *inp;
	int wc;
	LLE g,m;
	LL l;	
	(*c) = 0;
	if ( state != STATE_ONLINE )
		return NULL;
	
		
	l = CreateLL();
	
	debug_printf("possible = '%s' len = %d",p,strlen(p));
	inp = m_strdup( get_input() ? get_input() : empty_string);
	wc = word_count(inp);
	debug_printf("input = %s wc = %d",inp,wc);
	new_free(&inp);
	if ( wc > 2 ) 
		return NULL;	
	
	/* First go through people we've msg'd already */
	for ( TLL(msgdus,g) ) {
		if ( p && my_strnicmp(p, g->key, strlen(p)) ) 
			continue;
		AddToLL(l,g->key,NULL);		
	}
	
	/* Then people who msg'd us */	
	for ( TLL(msgdthem,g) ) {
		if ( p && my_strnicmp(p, g->key, strlen(p)) ) 
			continue;
		if ( ! FindInLL(l,g->key) )
			AddToLL(l,g->key,NULL);		
	}	
	
	/* And last, the Buddy */
	for ( TLL(groups,g) ) {
		struct group *grp = (struct group *)g->data;
		for ( TLL(grp->members,m) ) {
			struct buddy *bud = (struct buddy *)m->data;
			if ( p && my_strnicmp(p, bud->name, strlen(p)) ) 
				continue;
			if ( ! FindInLL(l,bud->name) )
				AddToLL(l,bud->name,NULL);
		}
	}
	
	if ( l->items == 1 && ! strcasecmp(l->head->next->key,p) ) {
		char *bud = NULL;
		(*c) = 1;
		bud = get_next_buddy_complete();
		debug_printf("We are going to go to get_next_buddy_comp!");
		if ( bud )
			m_s3cat(&z,space,bud);
		else 
			debug_printf("set z  to null because bud is null");	
	} else {
		for( TLL(l,g) ) {
			(*c)++;
			debug_printf("adding %s",g->key);
			m_s3cat(&z,space,g->key);	
		}
	}

	debug_printf("in test func!, p = %s",p);
	FreeLL(l);
	return (z);
}
#endif /* BITCHX_PATCH */

/* Window code, straight from nap module by panasync */

void update_aim_window(Window *tmp) {
	char statbuff[1024];
	char st[1024];
	char *t;
	char s[80];
	int numbuds_online, numbuds_total;
	
	if ( state == STATE_ONLINE ) {	
		t= ctime(&login_time);
		t[strlen(t)-6] = '\0'; /* remove \n, year, & space -- NOT Y10K READY !@! */
		sprintf(st,"Online since: %s", t);
	} else
		strcpy(st,"Offline");
	
	if ( is_idle ) {
		strcpy(s,"(Idle)");
	} else if ( is_away ) {
		strcpy(s,"(Away)");
	} else {
		strcpy(s,"");
	}
	
	/* Find the number of buddies online */
	numbuds_online = numbuds_total = 0;
	if ( groups != NULL ) {
		LLE g,b;
		struct group *grp;
		struct buddy *bud;		
		for ( TLL(groups,g) ) {
			grp = (struct  group *) g->data;
			numbuds_total +=  grp->members->items;
			for( TLL(grp->members,b) ) {
				bud = (struct buddy *) b->data;
				if ( bud->present)
					numbuds_online++;
			}
		}	
	} 
	sprintf(statbuff, "[1;45m Buddies: %d/%d Lag: %d Evil: %d  %s %%>%s ", numbuds_online, numbuds_total,(lag_ms / 1000000),my_evil,s,st);
	set_wset_string_var(tmp->wset, STATUS_FORMAT1_WSET, statbuff);
	
	sprintf(statbuff, "[1;45m %%>%s ", st);
	set_wset_string_var(tmp->wset, STATUS_FORMAT2_WSET, statbuff);

	update_window_status(tmp, 1);
}

int build_aim_status(Window *tmp)
{
	Window *tmp1;
	if (!(tmp1 = tmp))
		tmp1 = get_window_by_name("AIM");
	if (tmp1)
	{
		update_aim_window(tmp1);
		build_status(tmp1, NULL, 0);
		update_all_windows();
		return 1;
	}
	return 0;
}


void toggle_aimwin_hide (Window *win, char *unused, int onoff) {
	Window *tmp;
	if ((tmp = get_window_by_name("AIM")))
	{
		if (onoff)
		{
			if (tmp->screen)
				hide_window(tmp);
			build_aim_status(tmp);
			update_all_windows();
			cursor_to_input();
		}
		else
		{
			show_window(tmp);
			resize_window(2, tmp, 6);
			build_aim_status(tmp);
			update_all_windows();
			cursor_to_input();
		}
	}
}

void toggle_aimwin (Window *win, char *unused, int onoff){
	Window *tmp;
	if (onoff)
	{
		if ((tmp = get_window_by_name("AIM")))
			return;
		if ((tmp = new_window(win->screen)))
		{
			resize_window(2, tmp, 6);
			tmp->name = m_strdup("AIM");
#undef query_cmd
			tmp->query_cmd = m_strdup("asay");  
			tmp->double_status = 0;
			tmp->absolute_size = 1;
			tmp->update_status = update_aim_window;
			tmp->server = -2;
                	set_wset_string_var(tmp->wset, STATUS_FORMAT1_WSET, NULL);
                	set_wset_string_var(tmp->wset, STATUS_FORMAT2_WSET, NULL);
                	set_wset_string_var(tmp->wset, STATUS_FORMAT3_WSET, NULL);
                	set_wset_string_var(tmp->wset, STATUS_FORMAT_WSET, NULL);

			if (get_dllint_var("aim_window_hidden"))
				hide_window(tmp);
			else
				set_screens_current_window(tmp->screen, tmp);
			build_aim_status(tmp);
			update_all_windows();
			cursor_to_input();
		}
	}
	else
	{
		if ((tmp = get_window_by_name("AIM")))
		{
			if (tmp == target_window)
				target_window = NULL;
			delete_window(tmp);
			update_all_windows();
			cursor_to_input();
			                        
		}
	}
}


char *Aim_Version(IrcCommandDll *intp) {
	return AIM_VERSION;
}

int Aim_Cleanup(IrcCommandDll **interp, Function_ptr *global_table) {
	if ( state == STATE_ONLINE )
		toc_signoff();
#ifdef BITCHX_PATCH	
	overwrite_tabkey_comp(NULL,NULL);
	debug_printf("Didn't remove completions, thats probably gonna cause problems");
#endif		
	remove_module_proc(VAR_PROC, name, NULL, NULL);
	remove_module_proc(COMMAND_PROC,name,NULL,NULL);			
	remove_module_proc(ALIAS_PROC,name,NULL,NULL);		
	return 3;
}


int Aim_Init(IrcCommandDll **interp, Function_ptr *global_table) {
	char buffer[BIG_BUFFER_SIZE+1];
	char *p;
	initialize_module(name);
	
	add_module_proc(VAR_PROC, name, "aim_user", NULL, STR_TYPE_VAR, 0, NULL, NULL);	
	add_module_proc(VAR_PROC, name, "aim_pass", NULL, STR_TYPE_VAR, 0, NULL, NULL);	
	add_module_proc(VAR_PROC, name, "aim_prompt", (char *)convert_output_format("%K[%YAIM%K]%n ", NULL, NULL), STR_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(VAR_PROC, name, "aim_permdeny_mode", NULL, INT_TYPE_VAR, 1, NULL, NULL);
	add_module_proc(VAR_PROC, name, "aim_toc_host", TOC_HOST, STR_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(VAR_PROC, name, "aim_toc_port", NULL, INT_TYPE_VAR, TOC_PORT, NULL, NULL);
	add_module_proc(VAR_PROC, name, "aim_auth_host", AUTH_HOST, STR_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(VAR_PROC, name, "aim_auth_port", NULL, INT_TYPE_VAR, AUTH_PORT, NULL, NULL);		
	add_module_proc(VAR_PROC, name, "aim_permdeny_mode", NULL, INT_TYPE_VAR, 1, NULL, NULL);
	add_module_proc(VAR_PROC, name, "aim_minutes_to_idle", NULL, INT_TYPE_VAR, time_to_idle/60, achange_idle, NULL);
	add_module_proc(VAR_PROC, name, "aim_window", NULL, BOOL_TYPE_VAR, 0, toggle_aimwin, NULL);
	add_module_proc(VAR_PROC, name, "aim_window_hidden", NULL, BOOL_TYPE_VAR, 0, toggle_aimwin_hide, NULL);
	

	add_module_proc(COMMAND_PROC, name, "amsg", "amsg", 0, 0, amsg, "<screen name|buddy chat> <message> instant messages");
	add_module_proc(COMMAND_PROC, name, "asignon", "asignon", 0, 0, asignon, "logs into aol instant messanger");
	add_module_proc(COMMAND_PROC, name, "asignoff", "asignoff", 0, 0, asignoff, "logs off of aol instant messanger");	
	add_module_proc(COMMAND_PROC, name, "abl", "abl", 0, 0, abl, "<command> <args...> Modify your buddy list\n/abl show -- Shows buddy list\n/abl add [group] <buddy> -- Adds buddy to group in buddy list\n/abl del <buddy> Removes buddy from buddy llist\n/abl addg <group> Create group group\n/abl delg <group> <newgroup|1> delete group group");	
	add_module_proc(COMMAND_PROC, name, "apd", "apd", 0, 0, apd, "<command> <args...> Modify your permit/deny lists\n/apd show -- Shows your permit & deny list\n/apd mode <permitall|denyall|permitsome|denysome> -- change your mode\n/apd addp <sn> -- Adds sn to your permit list\n/apd delp <sn> -- Removes sn from your permit list\n/apd addd <sn> -- Adds <sn> to your deny list\n/apd deld <sn> -- Removes sn from your deny list");
	add_module_proc(COMMAND_PROC, name, "adir", "adir", 0, 0, adir, "<command> <args...> Use the user directory\n/adir get <sn> Get sn's dir info\n/adir search -- Not implemented yet\n/adir set <first name> <middle name> <last name> <maiden name> <city> <state> <country> <email> <allow web searches? 1|0>");
	add_module_proc(COMMAND_PROC, name, "awarn", "awarn", 0, 0, awarn, "<aim screen name> [anon] warns user");	
	add_module_proc(COMMAND_PROC, name, "awhois", "awhois", 0, 0, awhois, "<screen name> displays info on sn (sn has to be in buddy list)");	
	add_module_proc(COMMAND_PROC, name, "asave", "asave", 0, 0, asave, "Saves AIM settings");	
	add_module_proc(COMMAND_PROC, name, "asay", "asay", 0, 0, achat, "<message> send a message to the current buddy chat");	
	add_module_proc(COMMAND_PROC, name, "apart", "apart", 0, 0, achat, "<buddy chat> leave buddy chat");	
	add_module_proc(COMMAND_PROC, name, "ajoin", "ajoin", 0, 0, achat, "<buddy chat> join buddy chat (first searches invite list, if its in it then joins that one, otherwise creats anew)");
	add_module_proc(COMMAND_PROC, name, "achats", "achats", 0, 0, achat, "display buddy chats you are on"); 
	add_module_proc(COMMAND_PROC, name, "ainvite", "ainvite", 0, 0, achat, "<screen name> <buddy chat> <msg> invite user to buddy chat with msg");		
	add_module_proc(COMMAND_PROC, name, "anames", "anames", 0, 0, achat, "<buddy chat>");			
	add_module_proc(COMMAND_PROC, name, "acwarn", "acwarn", 0, 0, achat, "<buddy chat> <screen name> <anon>");			
	add_module_proc(COMMAND_PROC, name, "aaway", "aaway", 0, 0, aaway, "<away msg> Go away or come back if away");						
	add_module_proc(COMMAND_PROC, name, "aquery", "aquery", 0, 0, aquery, "query user");						
	add_module_proc(COMMAND_PROC, name, "ainfo", "ainfo", 0, 0, ainfo, "<command> <args>\n/ainfo set <your info...> Sets your info\n/ainfo get <screen name> Retreives sn's info");						

#ifdef BITCHX_PATCH	
	add_completion_type("amsg", 2, CUSTOM_COMPLETION, &amsg_complete_func);
	overwrite_tabkey_comp(&do_aim_tabkey_overwrite,&aim_tabkey_overwrite);
#endif
	
	statusprintf("Aol Instant Messanger Module Loaded");
	sprintf(buffer, "$0+AIM %s by panasync - $2 $3", AIM_VERSION);
	fset_string_var(FORMAT_VERSION_FSET, buffer);
	snprintf(buffer, BIG_BUFFER_SIZE, "%s/AIM.sav", get_string_var(CTOOLZ_DIR_VAR));
	p  = expand_twiddle(buffer);
	load("LOAD", p, empty_string, NULL);
	new_free(&p);	
	bx_init_toc();
	return 0;	
}
