/*
 * pkga.c --
 *
 *	This file contains a simple Tcl package "pkga" that is intended
 *	for testing the Tcl dynamic loading facilities.
 *
 *	CopyRight Colten Edwards aka panasync@efnet Jan 1997
 */
/* compile with
 * gcc -o -I../include -fPIC -o pkga.o pkga.c
 * gcc -shared -o pkga.so pkga.o
 */

#include "irc.h"
#include "struct.h"
#include "dcc.h"
#include "server.h"
#include "ircaux.h"
#include "alias.h"
#include "ctcp.h"
#include "list.h"
#include "struct.h"
#include "numbers.h"
#include "output.h"
#include "commands.h"
#include "vars.h"
#include "module.h"
#define INIT_MODULE
#include "modval.h"

/*
 * Prototypes for procedures defined later in this file:
 */
static void	Pkga_EqCmd (IrcCommandDll *, char *, char *, char *);
static char	*Pkga_newctcp (CtcpEntryDll *, char *, char *, char *);
static char	*Pkga_ctcppage (CtcpEntryDll *, char *, char *, char *);
static	char	*Pkga_alias (char *);

static	int	Pkga_numeric (char *, char *, char **);
/*
 *----------------------------------------------------------------------
 *
 * Pkga_EqCmd --
 *
 *	This procedure is invoked to process the "pkga_eq" Tcl command.
 *	It expects two arguments and returns 1 if they are the same,
 *	0 if they are different.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

void Pkga_EqCmd(intp, command, args, subargs)
    IrcCommandDll *intp;			/* Current interpreter. */
    char *command;
    char *args;				/* Number of arguments. */
    char *subargs;			/* Argument strings. */
{
char *arg1, *arg2;
	arg1 = next_arg(args, &args);
	arg2 = next_arg(args, &args);
	if (!arg1 || !arg2)
		return;
	put_it("arg1 %s arg2", !my_stricmp(arg1, arg2)?"eq":"!eq");
	return;
}

static char *Pkga_newctcp (CtcpEntryDll *dll, char *from, char *to, char *args)
{
char putbuf[500];
	sprintf(putbuf, "%c%s %s%c", CTCP_DELIM_CHAR, dll->name, my_ctime(time(NULL)), CTCP_DELIM_CHAR);
	send_text(from, putbuf, "NOTICE", 0, 0);
	return NULL;
}

static char *Pkga_ctcppage (CtcpEntryDll *dll, char *from, char *to, char *args)
{
char putbuf[500];
	sprintf(putbuf, "%c%s %s%c", CTCP_DELIM_CHAR, dll->name, my_ctime(time(NULL)), CTCP_DELIM_CHAR);
	send_text(from, putbuf, "NOTICE", 0, 0);
	put_it(" %s is paging you", from);
	return NULL;
}

static char *Pkga_alias (char *word)
{
	if (!word || !*word)
		return m_strdup("no string passed");
	/* caller free's this string */
	return m_strdup(word);
}

static int Pkga_numeric (char *from, char *user, char **args)
{
	put_it("Server numeric 1 being handled");
	return 1;
}

static int Pkga_raw (char *comm, char *from, char *userhost, char **args)
{
	if (*args[1] && !strncasecmp(args[1], "test", 4))
	{
		PasteArgs(args, 1);
		put_it("PRIVMSG from %s!%s [%s]", from, userhost, args[1]);
		send_to_server("%s %s :You sent me a privmsg [%s]", comm, from, args[1]);
		return 1;
	}
	return 0;
}

char *Pkga_Version(IrcCommandDll **intp)
{
	return "99.9";
}

int new_dcc_output(int type, int s, char *buf, int len)
{
	if (type == DCC_CHAT)
	{
		put_it("handler new dcc chat");
		write(s, buf, len);
	}
	return 0;
}

int Pkga_Init(IrcCommandDll **intp, Function_ptr *global_table)
{
int i;
Server *sptr;
	initialize_module("pkga");
	sptr = get_server_list();
	for (i = 0; i < server_list_size(); i++)
		put_it("server%d -> %s", i, sptr[i].name);
	
	add_module_proc(COMMAND_PROC, "pkga", "pkga_eq", NULL, 0, 0, Pkga_EqCmd, NULL);
	add_module_proc(CTCP_PROC, "pkga", "blah", "New ctcp Type", -1, CTCP_SPECIAL | CTCP_TELLUSER, Pkga_newctcp, NULL);
	add_module_proc(CTCP_PROC, "pkga", "page", "Page User", -1, CTCP_SPECIAL | CTCP_TELLUSER, Pkga_ctcppage, NULL);
	add_module_proc(ALIAS_PROC, "pkga", "blah", NULL, 0, 0, Pkga_alias, NULL);
	add_module_proc(HOOK_PROC, "pkga", NULL, NULL, 1, 0, Pkga_numeric, NULL);
	add_module_proc(VAR_PROC, "pkga", "new_variable", "TEST VALUE", STR_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(RAW_PROC, "pkga", "PRIVMSG", NULL, 0, 0, Pkga_raw, NULL);
	add_dcc_bind("CHAT", "pkga", NULL, NULL, NULL, new_dcc_output, NULL);
	return 0;
}
