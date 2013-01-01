#include "irc.h"
#include "struct.h"
#include "ircaux.h"
#include "misc.h"
#include "output.h"
#include "module.h"
#include "list.h"
#include "vars.h"
#include "modval.h"
#include "napster.h"

static IgnoreStruct *ignores = NULL;

int check_nignore(char *nick)
{
	if (ignores && find_in_list((List **)&ignores, nick, 0))
		return 1;
	return 0;
}

void ignore_user(IrcCommandDll *intp, char *command, char *args, char *subargs, char *help)
{
IgnoreStruct *new;
char *nick;
	if (command && !my_stricmp(command, "nignore"))
	{
		if (!args || !*args)
		{
			char buffer[NAP_BUFFER_SIZE+1];
			int cols =	get_dllint_var("napster_names_columns") ? 
					get_dllint_var("napster_names_columns") : 
					get_int_var(NAMES_COLUMNS_VAR);
			int count = 0;
			if (!cols)
				cols = 1;
			*buffer = 0;
			nap_say("%s", cparse("Ignore List:", NULL));
			for (new = ignores; new; new = new->next)
			{
				strcat(buffer, cparse(get_dllstring_var("napster_names_nickcolor"), "%s %d %d", new->nick, 0, 0));
				strcat(buffer, space);
				if (count++ >= (cols - 1))
				{
					nap_put("%s", buffer);
					*buffer = 0;
					count = 0;
				}
			}
			if (*buffer)
				nap_put("%s", buffer);
			return;
		}
		while ((nick = next_arg(args, &args)))
		{
			if (*nick == '-')
			{
				nick++;
				if (!*nick)
					continue;
				if ((new = (IgnoreStruct *)remove_from_list((List **)&ignores, nick)))
				{
					new_free(&new->nick);
					new_free(&new);
					nap_say("Removed %s from ignore list", nick);
				}
			}
			else
			{
				new = new_malloc(sizeof(IgnoreStruct));
				new->nick = m_strdup(nick);
				new->start = time(NULL);
				new->next = ignores;
				ignores = new;
				nap_say("Added %s to ignore list", new->nick);
			}
		}
		return;
	}
}


void nap_echo(IrcCommandDll *intp, char *command, char *args, char *subargs, char *help)
{
int (*func)(char *, ...);
	if (!args || !*args)
		return;

	func = nap_say;
	while (args && (*args == '-'))
	{
		args++;
		if (!*args)
			break;
		if (tolower(*args) == 'x')
		{
			func = nap_put;
			next_arg(args, &args);
		}
		else
		{
			args--;
			break;
		}
	}
	if (args)
		(func)("%s", args);
}

extern GetFile *napster_sendqueue;

int count_download(char *nick)
{
int count = 0;
GetFile *gf;
	for (gf = napster_sendqueue; gf; gf = gf->next)
		if (!my_stricmp(gf->nick, nick))
			count++;
	return count;
}

void compute_soundex (char *d, int dsize, const char *s)
{
    int n = 0;

    /* if it's not big enough to hold one soundex word, quit without
       doing anything */
    if (dsize < 4)
    {
	if (dsize > 0)
	    *d = 0;
	return;
    }
    dsize--; /* save room for the terminatin nul (\0) */

    while (*s && !isalpha(*s))
	s++;
    if (!*s)
    {
	*d = 0;
	return;
    }

    *d++ = toupper (*s);
    dsize--;
    s++;

    while (*s && dsize > 0)
    {
	switch (tolower (*s))
	{
	    case 'b':
	    case 'p':
	    case 'f':
	    case 'v':
		if (n < 3)
		{
		    *d++ = '1';
		    dsize--;
		    n++;
		}
		break;
	    case 'c':
	    case 's':
	    case 'k':
	    case 'g':
	    case 'j':
	    case 'q':
	    case 'x':
	    case 'z':
		if (n < 3)
		{
		    *d++ = '2';
		    dsize--;
		    n++;
		}
		break;
	    case 'd':
	    case 't':
		if (n < 3)
		{
		    *d++ = '3';
		    dsize--;
		    n++;
		}
		break;
	    case 'l':
		if (n < 3)
		{
		    *d++ = '4';
		    dsize--;
		    n++;
		}
		break;
	    case 'm':
	    case 'n':
		if (n < 3)
		{
		    *d++ = '5';
		    dsize--;
		    n++;
		}
		break;
	    case 'r':
		if (n < 3)
		{
		    *d++ = '6';
		    dsize--;
		    n++;
		}
		break;
	    default:
		if (!isalpha (*s))
		{
		    /* pad short words with 0's */
		    while (n < 3 && dsize > 0)
		    {
			*d++ = '0';
			dsize--;
			n++;
		    }
		    n = 0; /* reset */
		    /* skip forward until we find the next word */
		    s++;
		    while (*s && !isalpha (*s))
			s++;
		    if (!*s)
		    {
			*d = 0;
			return;
		    }
		    if (dsize > 0)
		    {
			*d++ = ',';
			dsize--;
			if (dsize > 0)
			{
			    *d++ = toupper (*s);
			    dsize--;
			}
		    }
		}
		/* else it's a vowel and we ignore it */
		break;
	}
	/* skip over duplicate letters */
	while (*(s+1) == *s)
	    s++;

	/* next letter */
	s++;
    }
    /* pad short words with 0's */
    while (n < 3 && dsize > 0)
    {
	*d++ = '0';
	dsize--;
	n++;
    }
    *d = 0;
}

