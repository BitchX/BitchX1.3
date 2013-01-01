#ifndef _AIM_H
#define _AIM_H

#define AIM_VERSION "0.02"
#define AIM_DEBUG_LOG "/tmp/aim-bx.log"

/* Twice the actual length, we should never have problems */
#define MAX_STATUS_MSG_LEN 4096
#define cparse convert_output_format

/* Macro Fun */
#define CHECK_TOC_ONLINE() if ( state != STATE_ONLINE ) { statusprintf("Please connect to aim first (/asignon)"); return; }
#define VALID_ARG(x) !(!x || ! *x || ! strcasecmp(x,""))
#define REQUIRED_ARG(x,y,z) if ( ! VALID_ARG(x) ) { userage(y,z); return; }


/* cmd.c */

void asignon(IrcCommandDll *intp, char *command, char *args, char *subargs,char *helparg);
void asignoff(IrcCommandDll *intp, char *command, char *args, char *subargs, char *helparg);
void amsg(IrcCommandDll *intp, char *command, char *args, char *subargs, char *helparg);
void abl(IrcCommandDll *intp, char *command, char *args, char *subargs,char *helparg);
void apd(IrcCommandDll *intp, char *command, char *args, char *subargs,char *helparg);
void awarn(IrcCommandDll *intp, char *command, char *args, char *subargs, char *helparg);
void apermdeny(IrcCommandDll *intp, char *command, char *args, char *subargs, char *helparg);
void aspermdeny(IrcCommandDll *intp, char *command, char *args, char *subargs, char *helparg);
void aarpermitdeny(IrcCommandDll *intp, char *command, char *args, char *subargs, char *helparg);
void awhois(IrcCommandDll *intp, char *command, char *args, char *subargs, char *helparg);
void asave (IrcCommandDll *intp, char *command, char *args, char *subargs, char *helparg);
void achat (IrcCommandDll *intp, char *command, char *args, char *subargs, char *helparg);
void adir (IrcCommandDll *intp, char *command, char *args, char *subargs, char *helparg);
void aaway (IrcCommandDll *intp, char *command, char *args, char *subargs, char *helparg);
void aquery(IrcCommandDll *intp, char *command, char *args, char *subargs,char *helparg);
void ainfo(IrcCommandDll *intp, char *command, char *args, char *subargs,char *helparg);
void achange_idle(Window *w, char *s, int i);

extern char current_chat[512];
extern char away_message[2048];
extern LL msgdus;
extern LL msgdthem;


/* toc.c */
void bx_init_toc();
int toc_add_input_stream(int fd,int (*func)(int));
int toc_remove_input_stream(int fd) ;
int toc_main_interface(int type, char **args);
int toc_timer(int type, char **args);

extern void (*chatprintf)(char *, ...);


/* aim.c */
#ifdef BITCHX_PATCH
char * amsg_complete_func(int, char *, int *, char **);
char * get_next_buddy_complete();
int do_aim_tabkey_overwrite(int x, char *p, int *c, char **s);
char * aim_tabkey_overwrite(int x, char *p, int *c, char **s);
#endif
void update_aim_window(Window *tmp);
int build_aim_status(Window *tmp);
void toggle_aimwin_hide (Window *win, char *unused, int onoff);
void toggle_aimwin (Window *win, char *unused, int onoff);
int Aim_Cleanup(IrcCommandDll **interp, Function_ptr *global_table);
int Aim_Init(IrcCommandDll **interp, Function_ptr *global_table);
char *Aim_Version(IrcCommandDll *intp);

extern char *name;
extern char *timer_id;
#ifdef BITCHX_PATCH
struct tab_key_struct {
	int list;
	int pos;
	int subpos;
};
extern struct tab_key_struct tks;
#endif


/* util.c */

void statusprintf(char *fmt, ...);
void statusput(int log_type, char *buf);
void msgprintf(char *fmt, ...);
void debug_printf(char *fmt, ...);
char *rm_space(char *s);

#endif /* _AIM_H */
