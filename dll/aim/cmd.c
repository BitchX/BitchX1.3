/*
 * AOL Instant Messanger Module for BitchX 
 *
 * By Nadeem Riaz (nads@bleh.org)
 *
 * cmd.c
 *
 * User commands (aliases) (client -> libtoc)
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
#include <sys/stat.h>
#include <module.h>
#include <modval.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "toc.h"
#include "aim.h"

char current_chat[512];
char away_message[2048];
LL msgdus;
LL msgdthem;

/* Commands */

void asignon(IrcCommandDll *intp, char *command, char *args, char *subargs,char *helparg) {
	char *user;
	char *pass;
	char *tochost,*authhost;
	int tocport, authport;
	int x;	
	
	if ( state == STATE_ONLINE ) {
		statusprintf("You are already online.");
		statusprintf("Please disconnect first (/asignoff), before trying to reoconnect.");
		return;
	}
	
	user = get_dllstring_var("aim_user");
	pass = get_dllstring_var("aim_pass");
	tochost = get_dllstring_var("aim_toc_host");
	authhost = get_dllstring_var("aim_auth_host");
	x = get_dllint_var("aim_permdeny");
	tocport = get_dllint_var("aim_toc_port");
	authport = get_dllint_var("aim_auth_port");
	

	if ( ! VALID_ARG(user) || ! VALID_ARG(pass) ) {
		statusprintf("Please set your password and user name, by doing");
		statusprintf("/set aim_user <user name>");
		statusprintf("/set aim_pass <password>");
		return;
	}

	/* This doent change anything-- should rm it */
	if ( x < 1 || x > 4) 
		permdeny = PERMIT_PERMITALL;
	else 
		permdeny = x;
	

	if ( VALID_ARG(tochost) )
		strncpy(aim_host,tochost,513);
	if ( tocport > 0 && tocport < (64*1024) ) 
		aim_port = tocport;
	if ( VALID_ARG(authhost) )
		strncpy(login_host,authhost,513);
	if ( authport > 0 && authport < (64*1024) )
		login_port = authport;
			
	if ( toc_login(user,pass) < 0) {
		statusprintf("Couldn't connect to instant messanger");
	}
	if ( get_dllint_var("aim_window") )
		build_aim_status(get_window_by_name("AIM"));		
	
	msgdthem = CreateLL();
	msgdus = CreateLL();
}



void asignoff(IrcCommandDll *intp, char *command, char *args, char *subargs, char *helparg) {
	if ( state != STATE_ONLINE ) {
		statusprintf("Please connect to aim first (/aconnect)");
		return;
	}	
	delete_timer("aimtime");
	toc_signoff();	
	if ( get_dllint_var("aim_window") )
		build_aim_status(get_window_by_name("AIM"));	
	FreeLL(msgdthem);
	FreeLL(msgdus);
}

void amsg(IrcCommandDll *intp, char *command, char *args, char *subargs, char *helparg) {
	char *nick,*nnick,*loc;
	
	CHECK_TOC_ONLINE();
	
	/* loc = msg, nick = username to send msg to */
	loc = LOCAL_COPY(args);
	nick = new_next_arg(loc, &loc);
	
	REQUIRED_ARG(nick,command,helparg);
	
	if ( nick[0] == '#' ) {
		struct buddy_chat *b;
		nick++;
		REQUIRED_ARG(nick,command,helparg);
		b = (struct buddy_chat *) find_buddy_chat(nick);
		if ( ! b ) {
			statusprintf("Error not on buddy chat %s", nick);
			return;
		}
		/* chatprintf("sent msg %s to buddy chat %s",loc,nick); */
		serv_chat_send(b->id,loc);
	} else {
		char *ruser,*rnick;
		nnick = (char *) malloc(strlen(nick)+10);
		rnick = rm_space(nick);
		ruser = rm_space(get_dllstring_var("aim_user"));
		sprintf(nnick,"%s@AIM",rnick);
		msgprintf("%s", cparse(fget_string_var(FORMAT_SEND_MSG_FSET), 
			"%s %s %s %s",update_clock(GET_TIME), 
			nnick, ruser, loc));
		serv_send_im(nick,loc);
		RemoveFromLLByKey(msgdthem,rnick);
		AddToLL(msgdthem,rnick,NULL);
#ifdef BITCHX_PATCH
		tks.list = 0;
		tks.pos = -1;
#endif		
		free(rnick); free(ruser);
	}	
	
	debug_printf("sending msg to %s '%s'",nick,loc);	
	return;
}

void abl(IrcCommandDll *intp, char *command, char *args, char *subargs,char *helparg) {
	char *cmd,*loc;
	
	CHECK_TOC_ONLINE();
	
	/* loc = msg, nick = username to send msg to */
	loc = LOCAL_COPY(args);
	cmd = new_next_arg(loc, &loc);
	
	REQUIRED_ARG(cmd,command,helparg);
	
	if  ( ! strcasecmp(cmd,"show" ) ) {
			struct buddy *b;
			LLE tg,tb;
			LL mems;
			for ( TLL(groups,tg) ) {
				mems = ((struct group *) tg->data)->members;
				statusprintf("Group: %s", tg->key);
				for ( TLL(mems,tb) ) {
					b = (struct buddy *)tb->data;
					statusprintf("\t\t%s %d",b->name,b->present);
				}
			}
	} else if ( ! strcasecmp(cmd,"add") ) {
		char *buddy,*group;
		group = new_next_arg(loc, &loc);
		REQUIRED_ARG(group,command,helparg);

		if ( ! VALID_ARG(loc) ) {
	        	buddy = group;
	        	group = (char *) malloc(strlen("Buddies")+2);
        		strcpy(group,"Buddies");
		} else {
		        buddy = new_next_arg(loc,&loc);
		}

		if ( user_add_buddy(group,buddy) > 0 ) {
			statusprintf("Added buddy %s to group %s",buddy,group);
		} else {
        		statusprintf("%s is already in your buddy list",buddy);
		}	       
	} else if ( ! strcasecmp(cmd,"del") ) {
		char *buddy;
		buddy = new_next_arg(loc,&loc);
		REQUIRED_ARG(buddy,command,helparg);

		if ( user_remove_buddy(buddy) > 0 ) {
			statusprintf("Removed buddy %s",buddy);
		} else {
			statusprintf("%s is not in your buddy list",buddy);
		}
	} else if ( ! strcasecmp(cmd,"addg") ) {
		char *group;
		struct group *g;
		group = new_next_arg(loc,&loc);	
		REQUIRED_ARG(group,command,helparg);	

		g = find_group(group);
		if ( g  ) {
			statusprintf("Group %s already exists",args);
			return;
		}

		add_group(group);
		statusprintf("Created group %s",group);
	} else if ( ! strcasecmp(cmd,"delg") ) {
		char *group,*newgroup;
		int ret;

		group = new_next_arg(loc, &loc);
		newgroup = new_next_arg(loc,&loc);
		REQUIRED_ARG(group,command,helparg);
		
		if ( ! VALID_ARG(newgroup) ) {
			statusprintf("Usage: /abl delg <old group> 1 (delete group and all buddies in it)");
			statusprintf("       /abl delg <old group>  <new group> (delete group and move all buddies in it to new group)");
			return;
		} 	

		if ( ! strcasecmp(newgroup,"1") ) 
			ret = remove_group(group,NULL,2);
		else 
			ret = remove_group(group,newgroup,1);
		if ( ret > 0 ) 
			statusprintf("Removed group %s",group);	
		else 
			statusprintf("Group %s doesn't exist",group);		
	} else 
		statusprintf("Error unknown buddy list management command: %s", cmd);
}	

void awarn(IrcCommandDll *intp, char *command, char *args, char *subargs, char *helparg) {
	char *buddy,*mode,*loc;
	mode = NULL;
	
	CHECK_TOC_ONLINE();
	
	loc = LOCAL_COPY(args);
	buddy = new_next_arg(loc, &loc);
	mode = new_next_arg(loc,&loc);
	
	REQUIRED_ARG(buddy,command,helparg);	
	
	if ( VALID_ARG(mode) && ! strcasecmp(mode,"anon") ) {
		serv_warn(buddy,1);
	} else {
		serv_warn(buddy,0);
	}
	statusprintf("Warned: %s",buddy);
}

void apd(IrcCommandDll *intp, char *command, char *args, char *subargs, char *helparg) {
	char *cmd,*loc;
	loc = LOCAL_COPY(args);
	cmd = new_next_arg(loc,&loc);

	CHECK_TOC_ONLINE();
	
	REQUIRED_ARG(cmd,command,helparg);

	if ( ! strcasecmp(cmd,"show") ) { 
		LLE t;
		statusprintf("User Mode: %s",((permdeny >= 1 && permdeny <= 4) ? PERMIT_MODES[permdeny] : "ERROR: Unknown"));
		statusprintf("Permit:");
		ResetLLPosition(permit);
		while ( (t=GetNextLLE(permit)) ) {
			statusprintf("\t\t%s",t->key);
		}
		
		ResetLLPosition(deny);
		statusprintf("Deny:");
		while ( (t=GetNextLLE(deny)) ) {
			statusprintf("\t\t%s",t->key);
		}		
	} else if ( ! strcasecmp(cmd,"mode") )  {
		char *mode;
		int newmode;
		mode = new_next_arg(loc,&loc);		
		REQUIRED_ARG(mode,command,helparg);

		if ( ! strcasecmp(mode,"permitall") )  {
			newmode = PERMIT_PERMITALL;
		} else if ( ! strcasecmp(mode,"denyall") ) {
			newmode = PERMIT_DENYALL;
		} else if ( ! strcasecmp(mode,"denysome") ) {
			newmode = PERMIT_DENYSOME;
		} else if ( ! strcasecmp(mode,"permitsome") ) {
			newmode = PERMIT_PERMITSOME;
		} else {
			userage(command,helparg);
			return;
		}
	
		if ( newmode == permdeny ) {
			statusprintf("We are already in %s mode",mode);
			return;
		} else {
			permdeny = newmode;
			set_dllint_var("aim_permdeny_mode",permdeny);
			serv_set_permit_deny();
			serv_save_config();
		}

		statusprintf("Switch to %s mode",mode);				
	} else if ( !strcasecmp(cmd,"addp") ) {
		char *buddy;
		buddy = new_next_arg(loc,&loc);
		REQUIRED_ARG(buddy,command,helparg);
		
		if ( add_permit(buddy) < 0 ) {
			statusprintf("%s is already in your permit list!");
			return;
		}	
		if ( permdeny != PERMIT_PERMITSOME ) 
			statusprintf("Note: although %s will be added to your permit list, no tangible change will occur because you are in the improper mode (see help on apermdeny)",buddy);
		statusprintf("Added %s to your permit list",buddy);		
	} else if ( !strcasecmp(cmd,"delp") ) {
		char *buddy;
		buddy = new_next_arg(loc,&loc);
		REQUIRED_ARG(buddy,command,helparg);
		
		if ( remove_permit(buddy) < 0 )
			statusprintf("%s is not in your permit list!",buddy);
		else 
			statusprintf("Remvoed %s from your permit list",buddy);					
	} else if ( !strcasecmp(cmd,"addd") ) {
		char *buddy;
		buddy = new_next_arg(loc,&loc);
		REQUIRED_ARG(buddy,command,helparg);	
		
		if ( add_deny(buddy) < 0 ) {
			statusprintf("%s is already in your deny list!");
			return;
		}
		if ( permdeny != PERMIT_DENYSOME ) 
			statusprintf("Note: although %s will be added to your deny list, no tangible change will occur because you are in the improper mode (see help on apermdeny)",buddy);
		statusprintf("Added %s to your deny list",buddy);		
	} else if ( !strcasecmp(cmd,"deld") ) {	
		char *buddy;
		buddy = new_next_arg(loc,&loc);
		REQUIRED_ARG(buddy,command,helparg);
		
		if ( remove_deny(buddy) < 0 )
			statusprintf("%s is not in your deny list!",buddy);
		else 
			statusprintf("Remvoed %s from your deny list",buddy);				
	} else 
		statusprintf("Error unknown permit/deny cmd %s",cmd);
}

void awhois(IrcCommandDll *intp, char *command, char *args, char *subargs, char *helparg) {
	char *buddy,*loc;
	struct buddy *b;
	loc = LOCAL_COPY(args);
	buddy = new_next_arg(loc,&loc);

	CHECK_TOC_ONLINE();
	
	REQUIRED_ARG(buddy,command,helparg);
	
	b = find_buddy(buddy);
	if ( ! b ) {
		statusprintf("%s is not in your buddy list and thus I have no info stored on him/her",buddy);
		return;
	}

	statusprintf("%s", cparse("ÚÄÄÄÄÄ---Ä--ÄÄ-ÄÄÄÄÄÄ---Ä--ÄÄ-ÄÄÄÄÄÄÄÄÄ--- --  -", NULL));
        statusprintf("%s", cparse("| User       : $0-", "%s", b->name));
        statusprintf("%s", cparse("³ Class      : $0-", "%s", ((b->uc <= 5 && b->uc >= 0) ? USER_CLASSES[b->uc] : "Unknown")));
        statusprintf("%s", cparse("³ Evil       : $0-", "%d", b->evil));
	statusprintf("%s", cparse("³ SignOn     : $0-", "%s", my_ctime(b->signon)));
        statusprintf("%s", cparse(": Idle       : $0-", "%d", b->idle));
}

void asave (IrcCommandDll *intp, char *command, char *args, char *subargs, char *helparg) {
	IrcVariableDll *newv = NULL;
	FILE *outf = NULL;
	char *expanded = NULL;
	char buffer[BIG_BUFFER_SIZE+1];
	if (get_string_var(CTOOLZ_DIR_VAR))
		snprintf(buffer, BIG_BUFFER_SIZE, "%s/AIM.sav", get_string_var(CTOOLZ_DIR_VAR));
	else
		sprintf(buffer, "~/AIM.sav");
	expanded = expand_twiddle(buffer);
	if (!expanded || !(outf = fopen(expanded, "w")))
	{
		statusprintf("error opening %s", expanded ? expanded : buffer);
		new_free(&expanded);
		return;
	}
	for (newv = dll_variable; newv; newv = newv->next)
	{
		if (!my_strnicmp(newv->name, name, 3))
		{
			if (newv->type == STR_TYPE_VAR)
			{
				if (newv->string)
					fprintf(outf, "SET %s %s\n", newv->name, newv->string);
			}
			else if (newv->type == BOOL_TYPE_VAR)
				fprintf(outf, "SET %s %s\n", newv->name, on_off(newv->integer));
			else
				fprintf(outf, "SET %s %d\n", newv->name, newv->integer);
		}
	}

	/* Buddy list, perm/deny list, etc. stored on AIM server */
	
	/*
	 * Not sure what that does?
	if (do_hook(MODULE_LIST, "NAP SAVE %s", buffer))
		nap_say("Finished saving Napster variables to %s", buffer);
	 */
	statusprintf("Finished saving AIM variables to %s",buffer);
	fclose(outf);	
	new_free(&expanded);
	return;
}

void achat (IrcCommandDll *intp, char *command, char *args, char *subargs, char *helparg) {
	char *arg1, *arg2, *arg3, *loc;

	loc = LOCAL_COPY(args);
	
	debug_printf("in achat!");
	
	CHECK_TOC_ONLINE();
	
	if ( ! strcasecmp(command,"asay") ) {
		if ( VALID_ARG(current_chat) ) {
			struct buddy_chat *b;
			b = find_buddy_chat(current_chat);
			if ( ! b ) {
				statusprintf("Not on a buddy chat");
				return;
			}
			serv_chat_send(b->id,loc);
		} else 
			statusprintf("Not on a buddy chat");
	} else if ( ! strcasecmp(command,"ajoin") ) {
		arg1 = new_next_arg(loc,&loc);
		REQUIRED_ARG(arg1,command,helparg);
		if ( arg1[0] == '#' )
			arg1++;		
		REQUIRED_ARG(arg1,command,helparg);
		if ( find_buddy_chat(arg1) ) {
			strncpy(current_chat,arg1,511);
			return;
		} 
		buddy_chat_join(arg1);
	} else if ( ! strcasecmp(command,"apart") ) {
		arg1 = new_next_arg(loc,&loc);
		if ( VALID_ARG(arg1) && arg1[0] == '#' )
			arg1++;
		if ( VALID_ARG(arg1) ) {		
			if ( buddy_chat_leave(arg1) ) {
				if ( ! strcasecmp(arg1,current_chat) ) {
					/* Replace Current Chat */
					strcpy(current_chat,"");
				}
			} else 
				statusprintf("Not on buddy chat %s",arg1);
		} else {
			if ( VALID_ARG(current_chat) ) {
				buddy_chat_leave(current_chat);
				/* Repalce Current Chat */
				strcpy(current_chat,"");
			} else 
				statusprintf("Not on a buddy chat");			
		}		
	} else if ( ! strcasecmp(command,"ainvite") ) {
		arg1 = new_next_arg(loc,&loc);
		arg2 = new_next_arg(loc,&loc);
		arg3 = new_next_arg(loc,&loc);
		REQUIRED_ARG(arg1,command,helparg);
		if ( arg1[0] == '#' ) 
			arg1++;
		REQUIRED_ARG(arg1,command,helparg);
		REQUIRED_ARG(arg2,command,helparg);
		REQUIRED_ARG(arg3,command,helparg);
		
		if ( buddy_chat_invite(arg1,arg2,arg3) < 0 ) {
			statusprintf("Not on buddy chat %s",arg1);
		}
	} else if ( !strcasecmp(command,"achats") ) {
		LLE t;
		statusprintf("Currently on: ");
		ResetLLPosition(buddy_chats);
		while ( (t=GetNextLLE(buddy_chats)) ) {
			statusprintf("\t\t%s",t->key);
		}
	} else if ( ! strcasecmp(command,"anames") ) {
		char *chat;
		arg1 = new_next_arg(loc,&loc);
		if ( VALID_ARG(arg1) ) 
			chat = arg1;
		else
			chat  = current_chat;
		if  ( VALID_ARG(chat) ) {
			struct buddy_chat *b;
			LLE t;
			b = find_buddy_chat(chat);
			if ( ! b ) {
				statusprintf("Not on buddy chat %s",chat);
				return;
			}
			statusprintf("Names on %s",b->name);
			ResetLLPosition(b->in_room);
			while ( (t=GetNextLLE(b->in_room)) ) {
				statusprintf("%s",t->key);
			}		
		} else 
			statusprintf("Not on a buddy chat");
	} else if ( ! strcasecmp(command,"acwarn") ) {
		int anon = 0;
		char *chat = NULL, *user = NULL, *mode = NULL;
		arg1 = new_next_arg(loc,  &loc);
		arg2 = new_next_arg(loc,&loc);
		arg3 = new_next_arg(loc,&loc);
		if ( VALID_ARG(arg1) && VALID_ARG(arg2) && VALID_ARG(arg3) ) {
			chat = arg1;
			user = arg2;
			mode = arg3;
		} else if ( VALID_ARG(arg1) && VALID_ARG(arg2) ) {
			chat = current_chat;
			user = arg1;
			mode = arg2;
		} else if ( VALID_ARG(arg1) ) {
			chat = current_chat;
			user = arg2;
			mode = NULL;
		}
		if ( VALID_ARG(mode) && ! strcasecmp(mode,"anon") )
			anon = 1;
		if ( chat[0] == '#' ) {
			chat++; 
			REQUIRED_ARG(chat,command,helparg);
		}
		if ( buddy_chat_warn(chat,user,1) < 0 )
			statusprintf("Not on buddy chat %s",chat);
		else
			statusprintf("Buddy Chat Warned %s",user);
	} else
		debug_printf("Unknown command in achat %s",command);
}

void adir (IrcCommandDll *intp, char *command, char *args, char *subargs, char *helparg) {
	char *cmd,*loc;

	loc = LOCAL_COPY(args);
	cmd = new_next_arg(loc,&loc);
	
	CHECK_TOC_ONLINE();
	REQUIRED_ARG(cmd,command,helparg);
	if ( !strcasecmp(cmd,"get") ) {
		char *sn;
		sn = new_next_arg(loc,&loc);
		REQUIRED_ARG(sn,command,helparg);
		
		serv_get_dir(sn);
	} else if ( ! strcasecmp(cmd,"search") ) {
		int fields = 0;
		char *field,*data;
		char *first,*middle,*last,*maiden;
		char *city,*state,*country,*email;
		first = middle = last = maiden = NULL;
		city = state = country = email = NULL;
				
		field = new_next_arg(loc,&loc);
		while ( VALID_ARG(field) ) {
			data = new_next_arg(loc,&loc);
			if ( VALID_ARG(data) ) {
				fields++;
				if ( ! strcasecmp(field,"first") || ! strcasecmp(field,"-first") )
					first = data;
				else if ( ! strcasecmp(field,"middle") || ! strcasecmp(field,"-middle") )
					middle = data;
				else if ( ! strcasecmp(field,"last") || ! strcasecmp(field,"-last") )
					last = data;
				else if ( ! strcasecmp(field,"maiden") || ! strcasecmp(field,"-maiden") )
					maiden = data;
				else if ( ! strcasecmp(field,"city") || ! strcasecmp(field,"-city") )
					city = data;
				else if ( ! strcasecmp(field,"state") || ! strcasecmp(field,"-state") )
					state = data;
				else if ( ! strcasecmp(field,"country") || ! strcasecmp(field,"-country") )
					country = data;
				else if ( ! strcasecmp(field,"email") || ! strcasecmp(field,"-email") )	
					email = data;
				else
					statusprintf("Illegal field: %s",field);
			} else {
				statusprintf("No search item for field %s",field);
			}
			serv_dir_search(first,middle,last,maiden,city,state,country,email);
		}
	} else if ( ! strcasecmp(cmd,"set") ) {
		char *first = new_next_arg(loc,&loc);
		char *middle = new_next_arg(loc,&loc);
		char *last = new_next_arg(loc,&loc);
		char *maiden = new_next_arg(loc,&loc);
		char *city = new_next_arg(loc,&loc);
		char *state =new_next_arg(loc,&loc);
		char *country = new_next_arg(loc,&loc);
		char *email = new_next_arg(loc,&loc);
		char *allow = new_next_arg(loc,&loc);
		int x; 
		
		REQUIRED_ARG(allow,command,helparg);
		if ( atoi(allow) ) {
			x = 1;
		} else {
			x = 0;
		}
		/* apparently sending email messes this up? */
		serv_set_dir(first,middle,last,maiden,city,state,country,email,x);
	} else
		debug_printf("Unknown command in adir %s",command);
}

void achange_idle(Window *w, char *s, int i) {
	time_to_idle = i * 60;
	debug_printf("time to idle = %d",time_to_idle);
}

void aaway (IrcCommandDll *intp, char *command, char *args, char *subargs, char *helparg) {
	char *loc;

	loc = LOCAL_COPY(args);
	CHECK_TOC_ONLINE();
		
	serv_set_away(args);
	
	if ( is_away ) {
		strncpy(away_message,args,2047);
		statusprintf("You are now marked as away");
	} else 
		statusprintf("You are now back.");
		
	if ( get_dllint_var("aim_window") )
		build_aim_status(get_window_by_name("AIM"));			
}

void aquery(IrcCommandDll *intp, char *command, char *args, char *subargs,char *helparg) {
	Window *tmp = NULL;
	char *loc,*n,*msg;
	char say[10] = "say";
	
	CHECK_TOC_ONLINE();
	loc = LOCAL_COPY(args);
	n = new_next_arg(loc,&loc);
	
	if ( get_dllint_var("aim_window") ) {
		strcpy(say,"asay");
		tmp = get_window_by_name("AIM");
	}
	if ( ! tmp ) 
		tmp = current_window;
	
	if ( VALID_ARG(n) ) {
#ifdef BITCHX_PATCH
		msg = (char *) malloc(strlen(n)+50);
		sprintf(msg,"-cmd amsg %s",n);
		debug_printf("Querying: %s",msg);
		window_query(tmp,&msg,NULL);
#else
	
		msg = (char *) malloc(strlen(n)+10);
		sprintf(msg,"amsg %s",n);
		debug_printf("nick = '%s' msg = '%s'",n,msg);
#undef query_cmd 
		tmp->query_cmd = m_strdup("amsg");
#undef query_nick
		tmp->query_nick = m_strdup(n);
		update_input(tmp);
#endif
	} else {
#undef query_cmd	
		tmp->query_cmd = m_strdup(say);
	}	
	debug_printf("Leaking memory in aquery");
}

void ainfo(IrcCommandDll *intp, char *command, char *args, char *subargs,char *helparg) {
	char *cmd,*loc;

	loc = LOCAL_COPY(args);
	cmd = new_next_arg(loc,&loc);
	
	CHECK_TOC_ONLINE();
	REQUIRED_ARG(cmd,command,helparg);
	
	if ( ! strcasecmp(cmd,"get") ) {
		char *nick = new_next_arg(loc,&loc);
		REQUIRED_ARG(nick,command,helparg);
		
		serv_get_info(nick);
	} else if ( ! strcasecmp(cmd,"set") ) {
		REQUIRED_ARG(loc,command,helparg);
		serv_set_info(loc);
	} else 
		statusprintf("Unknown command sent to ainfo: '%s'", cmd);
}
