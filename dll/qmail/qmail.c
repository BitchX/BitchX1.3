/*
 * Mail check routines. Based on EPIC's mail check
 */

#include "irc.h"
#include "struct.h"

#include "hook.h"
#include "ircaux.h"
#include "output.h"
#include "lastlog.h"
#include "status.h"
#include "vars.h"
#include "window.h"
#include <sys/stat.h>

#include "module.h"
#define INIT_MODULE
#include "modval.h"

#ifndef UNIX_MAIL
#define UNIX_MAIL "~/Maildir"
#endif

/*
 * check_mail_status: returns 0 if mail status has not changed, 1 if mail
 * status has changed 
 */

int check_qmail_status(void)
{
	int count = 0;
	DIR *dp;
	struct dirent *dir;
	static int c = 0;
	char *m;
	char *mail_path = NULL;
	
	if (!get_int_var(MAIL_VAR))
		return 0;
	
	if (!(mail_path = get_dllstring_var("qmaildir")))
                m = m_sprintf("%s/new", UNIX_MAIL);
	else
		m = m_sprintf("%s/new", mail_path);
	mail_path = expand_twiddle(m);
	new_free(&m);

	if (!mail_path)
		return 0;
	if ((dp = opendir(mail_path)))
	{
		while ((dir = readdir(dp)))
		{
			if (!dir->d_ino || (dir->d_name[0] == '.'))
				continue;
			count++;
		}
		closedir(dp);
	}
	if (count > c)
	{
		c = count;
		return c;
	}
	else if (count <= c)
	{
		c = count;
		return -count;
	}
	return 0;
}

/*
 * check_mail: This here thing counts up the number of pieces of mail and
 * returns it as static string.  If there are no mail messages, null is
 * returned. 
 */

char	*check_qmail (void)
{
static 	int old_count = 0;
static	char ret_str[12];
static  int	i = 0;

	switch (get_int_var(MAIL_VAR))
	{
		case 0:
			return NULL;
		case 1:
		{
			char this[] = "\\|/-";
			int count;
			if ((count = check_qmail_status()) > 0)
			{
				set_display_target(NULL, LOG_CRAP);
				if (do_hook(MAIL_LIST, "%s %s", "Mail", "Yes"))
					put_it("%s", convert_output_format(fget_string_var(FORMAT_MAIL_FSET), "%s %s %s", update_clock(GET_TIME), "Mail", "Yes"));
				reset_display_target();
				if (i == 4)
					i = 0;
				sprintf(ret_str, "%c", this[i++]);
			}
			else if (count == 0)
				i = 0;
			return *ret_str ? ret_str : NULL;
		}
		case 2:
		{
			register int count = 0;
			count = check_qmail_status();
			if (count == 0)
			{
				old_count = 0;
				return NULL;
			}
			if  (count > 0)
			{
				if (count > old_count)
				{
					set_display_target(NULL, LOG_CRAP);
					if (do_hook(MAIL_LIST, "%d %d", count - old_count, count))
						put_it("%s", convert_output_format(fget_string_var(FORMAT_MAIL_FSET), "%s %s %s", update_clock(GET_TIME), "Mail", "Yes"));
					reset_display_target();
				}

				old_count = count;
				sprintf(ret_str, "%d", old_count);
				return ret_str;
			}
			else
			{
				if (*ret_str)
					return ret_str;
			}
		}
	}
	return NULL;
}

char *name = "Qmail";

int Qmail_Cleanup(IrcCommandDll **interp, Function_ptr *global_table)
{
	remove_module_proc(VAR_PROC, name, NULL, NULL);
	remove_module_proc(CHECK_EXT_MAIL_STATUS|TABLE_PROC, name, NULL, NULL);
	remove_module_proc(CHECK_EXT_MAIL|TABLE_PROC, name, NULL, NULL);
	return 3;
}

int Qmail_Init(IrcCommandDll **interp, Function_ptr *global_table)
{
	initialize_module(name);
	add_module_proc(VAR_PROC, name, "qmaildir", "~/Maildir", STR_TYPE_VAR, 0, NULL, NULL);
	global[CHECK_EXT_MAIL_STATUS] = (Function_ptr) check_qmail_status;
	global[CHECK_EXT_MAIL] = (Function_ptr) check_qmail;
	return 0;
}
