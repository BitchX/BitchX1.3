/*
 * Mail check routines. Based on EPIC's mail check
 */


#include "irc.h"
static char cvsrevision[] = "$Id: mail.c 3 2008-02-25 09:49:14Z keaston $";
CVS_REVISION(mail_c)
#include "struct.h"

#include "mail.h"
#include "lastlog.h"
#include "hook.h"
#include "vars.h"
#include "ircaux.h"
#include "output.h"
#include "window.h"
#include "status.h"
#include "misc.h"
#include "module.h"
#define MAIN_SOURCE
#include "modval.h"

#include <sys/stat.h>

#if	!defined(HAVE_QMAIL)
static	char	*mail_path = NULL;
#endif

#if	defined(HAVE_QMAIL)
/*
 * count_maildir_mail: counts total # of mails (in cur and new)
 */
int count_maildir_mail(void)
{
	int count = 0;
	DIR *dp;
	struct dirent *dir;
	char *mail_path, *m;
	
	m = m_sprintf("%s/cur", UNIX_MAIL);
	mail_path = expand_twiddle(m);
	new_free(&m);
        
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
        
        new_free(&mail_path);
        m = m_sprintf("%s/new", UNIX_MAIL);
	mail_path = expand_twiddle(m);           
	new_free(&m);
                                
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
        
        new_free(&mail_path);

        return count;
}
#endif

#ifndef UNIX_MAIL
#define UNIX_MAIL "/var/spool/mail"
#endif

#ifndef MAIL_DELIMITER
#define MAIL_DELIMITER "From "
#endif


#ifdef WANT_DLL
#define check_ext_mail global_table[CHECK_EXT_MAIL]
#define check_ext_mail_status global_table[CHECK_EXT_MAIL_STATUS]
#endif

/*
 * check_mail_status: returns 0 if mail status has not changed, 1 if mail
 * status has changed 
 */
#ifdef PUBLIC_ACCESS
int check_mail_status(void)
{
	return 0;
}
#else
int check_mail_status(void)
{
#if defined(HAVE_QMAIL)
	int count = 0;
	static int c = 0;
#else
static time_t old_stat = 0;
struct stat stat_buf;
#endif        
#ifdef WANT_DLL
	if (check_ext_mail)
		return (*check_ext_mail_status)();
#endif
	if (!get_int_var(MAIL_VAR))
	{
#if defined(HAVE_QMAIL)
		c = 0;
#else
		old_stat = 0;
#endif
		return (0);
	}
	
#if defined(HAVE_QMAIL)
	count = count_maildir_mail();
	if (count > c)
	{
		c = count;
		return c;
	}
	if (count < c)
	{
		int diff;
		diff = count - c;
		c = count;
		return diff;
	}
#else
	if (!mail_path)
	{
		char *tmp_mail_path;
		if ((tmp_mail_path = getenv("MAIL")) != NULL)
			mail_path = m_strdup(tmp_mail_path);
                else
#ifdef __EMX__
                        mail_path = m_sprintf("%s/mqueue", getenv("ETC"));
#else
                        mail_path = m_3dup(UNIX_MAIL, "/", username);
#endif
	}

	if (stat(mail_path, &stat_buf) == -1)
		return 0;

	if (stat_buf.st_ctime > old_stat)
	{
		old_stat = stat_buf.st_ctime;
		if (stat_buf.st_size)
			return 2;
	}
	if (stat_buf.st_size)
		return 1;
#endif		
	return 0;
}
#endif


/*
 * check_mail: This here thing counts up the number of pieces of mail and
 * returns it as static string.  If there are no mail messages, null is
 * returned. 
 */

char	*check_mail (void)
{
static 	int old_count = 0;
static	char ret_str[12];
static  int	i = 0;
#ifdef WANT_DLL
	if (check_ext_mail)
		return (char *)(*check_ext_mail)();
#endif
	switch (get_int_var(MAIL_VAR))
	{
		case 0:
			return NULL;
		case 1:
		{
			char this[] = "\\|/-";
#if defined(HAVE_QMAIL)
			int count = check_mail_status();
			if (count > 0)
			{
				set_display_target(NULL, LOG_CRAP);
				if (do_hook(MAIL_LIST, "%s %s", "Mail", "Yes"))
					put_it("%s", convert_output_format(fget_string_var(FORMAT_MAIL_FSET), "%s %s %s", update_clock(GET_TIME), "Mail", "Yes"));
				++i;
				reset_display_target();
				if (i == 4)
					i = 0;
			}
			sprintf(ret_str, "%c", this[i]);

			return ret_str;
#else
			switch(check_mail_status())
			{
				case 2:
					set_display_target(NULL, LOG_CRAP);
					if (do_hook(MAIL_LIST, "%s %s", "Mail", "Yes"))
						put_it("%s", convert_output_format(fget_string_var(FORMAT_MAIL_FSET), "%s %s %s", update_clock(GET_TIME), "Mail", "Yes"));
					reset_display_target();
					if (i == 4)
						i = 0;
					sprintf(ret_str, "%c", this[i++]);
				case 1:
					if (!*ret_str)
						return NULL;
					return ret_str;
				case 0:
					i = 0;
					return NULL;
			}
#endif
		}
		case 2:
		{
			register int count = 0;
#if defined(HAVE_QMAIL)
			count = count_maildir_mail();
			if (count == 0)
			{
				old_count = 0;
				return NULL;
			}
			if  (count > old_count)
			{
				set_display_target(NULL, LOG_CRAP);
				if (do_hook(MAIL_LIST, "%d %d", count - old_count, count))
					say("You have new email.");
				reset_display_target();
			}

			old_count = count;
			sprintf(ret_str, "%d", old_count);
			return ret_str;
#else
			FILE *mail;
			char buffer[255];
			switch(check_mail_status())
			{

				case 0:
					old_count = 0;
					return NULL;
				case 2:
				{
					if (!(mail = fopen(mail_path, "r")))
						return NULL;

					while (fgets(buffer, 254, mail))
						if (!strncmp(MAIL_DELIMITER, buffer, 5))
							count++;

					fclose(mail);
	
					if (count > old_count)
					{
						set_display_target(NULL, LOG_CRAP);
						if (do_hook(MAIL_LIST, "%d %d", count - old_count, count))
							say("You have new email.");
						reset_display_target();
					}
	
					old_count = count;
					sprintf(ret_str, "%d", old_count);
				}
				case 1:
					if (*ret_str)
						return ret_str;
			}
#endif
		}
	}
	return NULL;
}
