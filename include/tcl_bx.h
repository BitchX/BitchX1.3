/*
 *
 * CopyRight Colten Edwards Oct 96
 *
 */

#ifndef _IRCTCL_H
#define _IRCTCL_H

int handle_dcc_bot (int, char *);
int handle_tcl_chan (int, char *, char *, char *);

typedef struct {
	char *name;
	int  (*func) ();
	int access;
	char *help;
} cmd_t;

extern cmd_t C_msg[];
extern cmd_t C_dcc[];
int check_tcl_dcc (char *, char *, char *, int);

void tcl_command (char *, char *, char *, char *);
void tcl_load (char *, char *, char *, char *);

#ifdef WANT_TCL


#include <tcl.h>
extern Tcl_Interp *tcl_interp;
void check_tcl_tand (char *, char *, char *);
void check_tcl_msgm (char *, char *, char *, char *, char *);
void check_tcl_pubm (char *, char *, char *, char *);
int check_tcl_pub (char *, char *, char *, char *);
int check_tcl_msg (char *, char *, char *, char *, char *);
int check_tcl_ctcp (char *, char *, char *, char *, char *, char *);


void check_tcl_join (char *,char *, char *, char *);
int check_tcl_raw (char *, char *);
void check_tcl_rejoin (char *,char *,char *,char *);
void check_tcl_split (char *,char *,char *,char *);
void check_tcl_chat (char *,int, char *);
int check_tcl_ctcr (char *,char *,char *,char *,char *,char *);
void check_tcl_mode (char *,char *,char *,char *,char *);
void check_tcl_kick (char *,char *,char *,char *,char *,char *);
void check_tcl_nick (char *,char *,char *,char *, char *);
void check_tcl_topc (char *,char *,char *,char *,char *);
void check_tcl_sign (char *, char *,char *, char *,char *);
void check_tcl_part (char *, char *, char *, char *);
int check_help_bind (char *);
int check_tcl_input (char *);
int check_on_hook (int, char *);
void check_timers (void);
void check_utimers (void);
void tcl_list_timer (Tcl_Interp *, TimerList *);
int check_on_hook (int, char *);
void add_tcl_vars(void);

void tcl_init (void);
void add_to_tcl(Window *, char *);
void init_public_tcl(Tcl_Interp *);
void init_public_var(Tcl_Interp *);
int Tcl_Invoke(Tcl_Interp *, char *, char *);


#define STDVAR (ClientData cd, Tcl_Interp *irp, int argc, char *argv[])

#define BADARGS(nl,nh,example) \
  if ((argc<(nl)) || (argc>(nh))) { \
    Tcl_AppendResult(irp,"wrong # args: should be \"",argv[0], \
		     (example),"\"",NULL); \
    return TCL_ERROR; \
  }


#endif

#endif
