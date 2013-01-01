#include <time.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include "toc.h"

static time_t lastsent = 0;
time_t login_time = 0;
int my_evil;
int is_idle = 0;
int lag_ms = 0;
int time_to_idle = 600;
int is_away = 0;
static struct timeval lag_tv;

void serv_add_buddy(char *name)
{
	char buf[1024];
	snprintf(buf, sizeof(buf), "toc_add_buddy %s", normalize(name));
	sflap_send(buf, -1, TYPE_DATA);
}

void serv_remove_buddy(char *name)
{
	char buf[1024];
	snprintf(buf, sizeof(buf), "toc_remove_buddy %s", normalize(name));
	sflap_send(buf, -1, TYPE_DATA);
}

int serv_got_im(char *name, char *message, int away)
{
	/*
	struct conversation *cnv;
	int is_idle = -1;
	*/
	char *nname;

	nname = strdup(normalize(name));

	if (!strcasecmp(normalize(name), nname)) {
		if (!strcmp(message, LAGOMETER_STR)) {
			struct timeval tv;
                        int ms;

			gettimeofday(&tv, NULL);

			ms = 1000000 * (tv.tv_sec - lag_tv.tv_sec);

			ms += tv.tv_usec - lag_tv.tv_usec;
			
			lag_ms = ms;
			use_handler(TOC_HANDLE,TOC_LAG_UPDATE,NULL);
			
                        return -1;
		}

	}

	/*
        cnv = find_conversation(name);

        if (cnv == NULL)
                cnv = new_conversation(name);

        if (away)
                write_to_conv(cnv, message, WFLAG_AUTO | WFLAG_RECV);
        else
                write_to_conv(cnv, message, WFLAG_RECV);


        if (cnv->makesound && extrasounds)
                play_sound(RECEIVE);


        if (awaymessage != NULL) {
                time_t t;

                time(&t);


                if ((cnv == NULL) || (t - cnv->sent_away) < 120)
                        return;

                cnv->sent_away = t;

                if (is_idle)
                        is_idle = -1;


                serv_send_im(name, awaymessage->message, 1);

                if (is_idle == -1)
                        is_idle = 1;

                write_to_conv(cnv, awaymessage->message, WFLAG_SEND | WFLAG_AUTO);
        }
	*/
	toc_debug_printf("Received im from %s : %s\n",name,message);
	return 1;
}

void serv_finish_login()
{
        char *buf;
        
        buf = strdup(user_info);
        escape_text(buf);
        serv_set_info(buf);
	free(buf);

	use_handler(TOC_HANDLE,TOC_REINSTALL_TIMER,NULL);

        time(&login_time);
	serv_touch_idle();

        serv_add_buddy(aim_username);
        
	if (!registered)
	{
		/* show_register_dialog(); */
		save_prefs();
	}
}

void serv_add_buddies(LL buddies)
{
	char buf[MSG_LEN];
	LLE e;
        int n, num = 0;
	
        n = snprintf(buf, sizeof(buf), "toc_add_buddy");
        for ( TLL(buddies,e) ) {
                if (num == 20) {
                        sflap_send(buf, -1, TYPE_DATA);
                        n = snprintf(buf, sizeof(buf), "toc_add_buddy");
                        num = 0;
                }
                ++num;
                n += snprintf(buf + n, sizeof(buf) - n, " %s", normalize((char *)e->key));
        }
	sflap_send(buf, -1, TYPE_DATA);
}




void serv_got_update(char *name, int loggedin, int evil, time_t signon, time_t idle, int type)
{
        struct buddy *b;
        char *nname,**args;
                     
        b = find_buddy(name);

        nname = strdup(normalize(name));
        if (!strcasecmp(nname, normalize(aim_username))) {
		/* 
                correction_time = (int)(signon - login_time);
                update_all_buddies();
		*/
		my_evil = evil;
                if (!b)
                        return;
        }

        
        if (!b) {
                toc_debug_printf("Error, no such person\n");
                return;
        }

        /* This code will 'align' the name from the TOC */
        /* server with what's in our record.  We want to */
        /* store things how THEY want it... */
	/*
        if (strcmp(name, b->name)) {
                GList  *cnv = conversations;
                struct conversation *cv;

                char *who = g_malloc(80);

                strcpy(who, normalize(name));

                while(cnv) {
                        cv = (struct conversation *)cnv->data;
                        if (!strcasecmp(who, normalize(cv->name))) {
                                g_snprintf(cv->name, sizeof(cv->name), "%s", name);
                                if (find_log_info(name) || log_all_conv)
                                        g_snprintf(who, 63, LOG_CONVERSATION_TITLE, name);
                                else
                                        g_snprintf(who, 63, CONVERSATION_TITLE, name);
                                gtk_window_set_title(GTK_WINDOW(cv->window), who);
				*/
                                /* no free 'who', set_title needs it.
                                 */
				 /*
                                break;
                        }
                        cnv = cnv->next;
                }

                g_snprintf(b->name, sizeof(b->name), "%s", name); */
                /*gtk_label_set_text(GTK_LABEL(b->label), b->name);*/

                /* okay lets save the new config... */

        /* } */

        b->idle = idle;
        b->evil = evil;
        b->uc = type;
        
        b->signon = signon;

        if (loggedin) {
                if (!b->present) {
                        b->present = 1;
			args = (char **) malloc(sizeof(char *)*1);
			args[0] = strdup(b->name);
			use_handler(TOC_HANDLE,TOC_BUDDY_LOGGED_ON,args);
			free(args[0]); free(args);
                        /* do_pounce(b->name); */
                }
        } else {
		if ( b->present ) {
			args = (char **) malloc(sizeof(char *)*1);
			args[0] = strdup(b->name);
			use_handler(TOC_HANDLE,TOC_BUDDY_LOGGED_OFF,args);
			free(args[0]); free(args);
		}
                b->present = 0;
	}
	/*
        set_buddy(b);
	*/
}


void serv_send_im(char *name, char *message)
{
	char buf[MSG_LEN - 7];
        snprintf(buf, MSG_LEN - 8, "toc_send_im %s \"%s\"%s", normalize(name),
                   message, ((is_away) ? " auto" : ""));
	sflap_send(buf, strlen(buf), TYPE_DATA);

        if (!is_away && strcasecmp(message,LAGOMETER_STR) != 0)
                serv_touch_idle();
	
}

void serv_close()
{
	toc_close();
}

void serv_save_config()
{
	char *buf = malloc(BUF_LONG);
	char *buf2 = malloc(MSG_LEN);
	toc_build_config(buf, BUF_LONG / 2); 
	snprintf(buf2, MSG_LEN, "toc_set_config {%s}", buf);
        sflap_send(buf2, -1, TYPE_DATA);
	free(buf2);
	free(buf);	
}

void serv_warn(char *name, int anon)
{
	char *send = malloc(256);
	snprintf(send, 255, "toc_evil %s %s", name,
		   ((anon) ? "anon" : "norm"));
	sflap_send(send, -1, TYPE_DATA);
	free(send);
}

void serv_add_permit(char *name)
{
	char buf[1024];
	snprintf(buf, sizeof(buf), "toc_add_permit %s", normalize(name));
	sflap_send(buf, -1, TYPE_DATA);
}



void serv_add_deny(char *name)
{
	char buf[1024];
	snprintf(buf, sizeof(buf), "toc_add_deny %s", normalize(name));
	sflap_send(buf, -1, TYPE_DATA);
}



void serv_set_permit_deny()
{
	char buf[MSG_LEN];
	char type[30];
	int at;
	LLE t;
	LL l;
        /* FIXME!  We flash here. */
        if (permdeny == PERMIT_PERMITALL || permdeny == PERMIT_PERMITSOME) {
		strcpy(type,"toc_add_permit");
		l = permit;		
        } else {
		strcpy(type,"toc_add_deny");                
		l = deny;
		
        }
	sflap_send(type, -1, TYPE_DATA);

	if ( permdeny == PERMIT_DENYALL || permdeny == PERMIT_PERMITALL ) {
		if ( permdeny == PERMIT_DENYALL ) 
			strcpy(type,"toc_add_permit");
		else 
			strcpy(type,"toc_add_deny");
		sflap_send(type, -1, TYPE_DATA);
		return;
	}
		
	at = snprintf(buf, sizeof(buf), "%s",type);
	for ( TLL(l,t) ) {
                at += snprintf(&buf[at], sizeof(buf) - at, " %s", normalize(t->key));
	}
	buf[at] = 0;
	sflap_send(buf, -1, TYPE_DATA);

}

void serv_got_joined_chat(int id, char *name)
{
        struct buddy_chat *b;

        b = (struct buddy_chat *) malloc(sizeof(struct buddy_chat));
        b->ignored = CreateLL();
        b->in_room = CreateLL();
        b->id = id;
	b->init_chat = 0;
        snprintf(b->name, 80, "%s", name);
	
	AddToLL(buddy_chats,name,b);
}

void serv_got_chat_left(int id)
{
        LLE t;
        struct buddy_chat *b = NULL;

        for ( TLL(buddy_chats,t) ) {
                b = (struct buddy_chat *)t->data;
                if (id == b->id) {
                        break;
                        }
                b = NULL;
        }

        if (!b)
                return;
	
	RemoveFromLLByKey(buddy_chats,b->name);
	
	toc_debug_printf("leaking memory in serv_got_chat_left");
}

void serv_accept_chat(int i)
{
        char *buf = malloc(256);
        snprintf(buf, 255, "toc_chat_accept %d",  i);
        sflap_send(buf, -1, TYPE_DATA);
        free(buf);
}

void serv_join_chat(int exchange, char *name)
{
        char buf[BUF_LONG];
        snprintf(buf, sizeof(buf)/2, "toc_chat_join %d \"%s\"", exchange, name);
        sflap_send(buf, -1, TYPE_DATA);
}

void serv_chat_invite(int id, char *message, char *name)
{
        char buf[BUF_LONG];
        snprintf(buf, sizeof(buf)/2, "toc_chat_invite %d \"%s\" %s", id, message, normalize(name));
        sflap_send(buf, -1, TYPE_DATA);
}

void serv_chat_leave(int id)
{
        char *buf = malloc(256);
        snprintf(buf, 255, "toc_chat_leave %d",  id);
        sflap_send(buf, -1, TYPE_DATA);
        free(buf);
}

void serv_chat_whisper(int id, char *who, char *message)
{
        char buf2[MSG_LEN];
        snprintf(buf2, sizeof(buf2), "toc_chat_whisper %d %s \"%s\"", id, who, message);
        sflap_send(buf2, -1, TYPE_DATA);
}

void serv_chat_send(int id, char *message)
{
        char buf[MSG_LEN];
        snprintf(buf, sizeof(buf), "toc_chat_send %d \"%s\"",id, message);
        sflap_send(buf, -1, TYPE_DATA);
	serv_touch_idle();
}

void serv_chat_warn(int  id, char *user, int anon) {
	char send[256];
	snprintf(send, 255, "toc_chat_evil %d %s %s", id, user,  ((anon) ? "anon" : "norm"));
	sflap_send(send, -1, TYPE_DATA);
}


void serv_get_dir(char *name) {
        char buf[MSG_LEN];
        snprintf(buf, MSG_LEN, "toc_get_dir %s", normalize(name));
        sflap_send(buf, -1, TYPE_DATA);
}

void serv_set_dir(char *first, char *middle, char *last, char *maiden,
		  char *city, char *state, char *country, char *email, int web)
{
	char buf2[BUF_LEN], buf[BUF_LEN];
	/* sending email seems to mess this up? */
	snprintf(buf2, sizeof(buf2), "%s:%s:%s:%s:%s:%s:%s:%s", first,
		   middle, last, maiden, city, state, country,
		   (web == 1) ? "Y" : "");
	escape_text(buf2);
	snprintf(buf, sizeof(buf), "toc_set_dir %s", buf2);
	sflap_send(buf, -1, TYPE_DATA);
}

void serv_dir_search(char *first, char *middle, char *last, char *maiden,
		     char *city, char *state, char *country, char *email)
{
	char buf[BUF_LONG];
	snprintf(buf, sizeof(buf)/2, "toc_dir_search %s:%s:%s:%s:%s:%s:%s:%s", first, middle, last, maiden, city, state, country, email);
	toc_debug_printf("Searching for: %s,%s,%s,%s,%s,%s,%s\n", first, middle, last, maiden, city, state, country);
	sflap_send(buf, -1, TYPE_DATA);
}


void serv_get_info(char *name)
{
        char buf[MSG_LEN];
        snprintf(buf, MSG_LEN, "toc_get_info %s", normalize(name));
        sflap_send(buf, -1, TYPE_DATA);
}

void serv_set_info(char *info)
{
	char buf[MSG_LEN];
	snprintf(buf, sizeof(buf), "toc_set_info \"%s\n\"", info);
	sflap_send(buf, -1, TYPE_DATA);
}

void serv_touch_idle() {
	/* Are we idle?  If so, not anymore */
	if (is_idle > 0) {
		is_idle = 0;
                serv_set_idle(0);
		use_handler(TOC_HANDLE,TOC_WENT_IDLE,NULL);
        }
        time(&lastsent);
}

void serv_set_idle(int time)
{
	char buf[256];
	snprintf(buf, sizeof(buf), "toc_set_idle %d", time);
	sflap_send(buf, -1, TYPE_DATA);
}

int check_idle() {
	time_t t;

	time(&t);

	use_handler(TOC_HANDLE,TOC_REINSTALL_TIMER,NULL);	
        gettimeofday(&lag_tv, NULL);
	serv_send_im(aim_username, LAGOMETER_STR);

	/*
	if (report_idle != IDLE_GAIM)
                return TRUE;
	*/

	
	if (is_idle || is_away)
		return 1;

	toc_debug_printf("time_to_idle = %d, current idle = %d, t = %d, last_sent = %d",time_to_idle,(t - lastsent),t,lastsent);
	if ((t - lastsent) > time_to_idle) { /* 10 minutes! */
		serv_set_idle((int)t - lastsent);
		toc_debug_printf("went idle wieth time_to_idle = %d",time_to_idle);
		use_handler(TOC_HANDLE,TOC_WENT_IDLE,NULL);
		is_idle = 1;
        }

        
	return 1;

}

void serv_set_away(char *message) {
        char buf[MSG_LEN];
        if ( ! is_away && message ) {
		is_away = 1;
                snprintf(buf, MSG_LEN, "toc_set_away \"%s\"", message);
        } else {
		is_away = 0;
                snprintf(buf, MSG_LEN, "toc_set_away");
	}
	sflap_send(buf, -1, TYPE_DATA);
}
