/* 
 *      hint.c - HINT shared library for BitchX (by panasync)
 *      
 *      	Written by |MaRe| 
 *      	
 */

#define HINT_VERSION "0.01"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "irc.h"
#include "struct.h"
#include "ircaux.h"
#include "status.h"
#include "screen.h"
#include "vars.h"
#include "misc.h"
#include "output.h"
#include "module.h"
#define INIT_MODULE
#include "modval.h"

#define HINT_LOADED "*** %W<%GHINT shared libarary loaded%W>"
#define getrandom(min, max) ((rand() % (int)(((max)+1) - (min))) + (min))
#define HINT_MAX_LEN 1000

int max_hints = 0;

char hint_buf[HINT_MAX_LEN+1]; /* max length of one hint */

#if !defined(WINNT) || !defined(__EMX__)
#define HINT_FILENAME "BitchX.hints"
#else
#define HINT_FILENAME "BitchX.hnt"
#endif

char *cp(char *fmt) { return (convert_output_format(fmt, NULL, NULL)); }

char *Hint_Version(IrcCommandDll **intp)
{
	return HINT_VERSION;
}

char *show_hint(int num)
{
FILE *fp;
char *f = NULL;
int i;
char *converted = NULL;
	malloc_strcpy(&f, HINT_FILENAME);
	if ((fp = uzfopen(&f, get_string_var(LOAD_PATH_VAR), 0)))
	{
		for (i=0;i<=num;i++) 
			fgets(hint_buf, HINT_MAX_LEN, fp);
		hint_buf[strlen(hint_buf)-1]='\0';
		converted = cp(hint_buf);
		fclose(fp);
	}
	new_free(&f);
	return converted;
}

BUILT_IN_DLL(shint)
{
  int number;
  if (max_hints <= 0)
  {
    put_it("%s", cp("*** %W<%Ghint%W>%n No hints avaible."));
    return;
  }
  
  number = atoi(args);
  
  if (word_count(args)==0) number=-1;  /* hehe, this is malicious..:=) */
  if (number >= 0 && number <= max_hints) 
    put_it("%s", show_hint(number)); 
  else
    {
      put_it("%s Specify number from 0-%d", cp("*** %W<%Ghint%W>%n"), max_hints);
      return;
    }
}

BUILT_IN_DLL(hint)
{
  int hint_no;
  
  if (max_hints <= 0)
  {
    put_it("%s", cp("*** %W<%Ghint%W>%n No hints avaible."));
    return;
  }
  
  hint_no = getrandom(0, max_hints);
  put_it("%s", show_hint(hint_no));
}

BUILT_IN_DLL(hintsay)
{
	int hint_no;
	char *tmp;
	if (max_hints <= 0)
	{
		put_it("%s", cp("*** %W<%Ghint%W>%n No hints avaible."));
		return;
	}

	hint_no = getrandom(0, max_hints);
	if (!(tmp = next_arg(args, &args)))
		if (!(tmp = get_current_channel_by_refnum(0)))
			return;
	tmp = make_channel(tmp);
	send_to_server("PRIVMSG %s :%s", tmp, show_hint(hint_no));
}

BUILT_IN_DLL(rehint)
{
  FILE *fp;
  char *f = NULL;
  max_hints=0;
  malloc_strcpy(&f, HINT_FILENAME);
  if ((fp = uzfopen(&f, get_string_var(LOAD_PATH_VAR), 0)))
  {
    while (fgets(hint_buf, HINT_MAX_LEN, fp)!=NULL) max_hints++;
    fclose(fp);
    put_it("%s (using %d hints)", cp("*** %W<%Ghint%W> Reloaded%n"), max_hints);
    max_hints--;
  } else
    put_it("%s (%s)", cp("*** %W<%Ghint%W>%n: Hint file not found"), f);
  new_free(&f);
}

BUILT_IN_DLL(hhelp)
{
  put_it("%s", cp("*** %W<%Ghint%W> %GHINT%n (version %W0.1b%n by %Y|MaRe|%n) %GHELP%n"));
  if (word_count(args)==0)
  {
    put_it("%s", cp("*** %W<%Ghint%W>%n Commands avaible:"));
    put_it("%s", cp("*** %W<%Ghint%W>%n  HINT    SHINT    HINTSAY"));
    put_it("%s", cp("*** %W<%Ghint%W>%n  REHINT  HELP"));
    put_it("%s", cp("*** %W<%Ghint%W>%n More help with %W/HHELP %Y<command>%n"));
  } else
  {
    if (!strcmp(upper(args), "HINT"))
    {
     put_it("%s", cp("*** %W<%Ghint%W>%n Usage: %W/HINT%n"));
     put_it("%s", cp("*** %W<%Ghint%W>%n %YHINT%n: this will print you randomly")); 
     put_it("%s", cp("*** %W<%Ghint%W>%n %YHINT%n: chosen hint from the hint database."));      
    }
    
    if (!strcmp(upper(args), "SHINT"))
    {
     put_it("%s", cp("*** %W<%Ghint%W>%n Usage: %W/SHINT%n %Y<number>%n"));
     put_it("%s", cp("*** %W<%Ghint%W>%n %YSHINT%n: this will print the Xth")); 
     put_it("%s", cp("*** %W<%Ghint%W>%n %YSHINT%n: hint from the hint database."));      
    }
    
    if (!strcmp(upper(args), "REHINT"))
    {
     put_it("%s", cp("*** %W<%Ghint%W>%n Usage: %W/REHINT%n"));
     put_it("%s", cp("*** %W<%Ghint%W>%n %YREHINT%n: this will reload the")); 
     put_it("%s", cp("*** %W<%Ghint%W>%n %YREHINT%n: hint database."));      
    }
  }
}

int Hint_Init(IrcCommandDll **intp, Function_ptr *global_table)
{
FILE *fp;
char *f = NULL;
	initialize_module("hint");
  /* add /HINT */ 
	add_module_proc(COMMAND_PROC, "hint", "hint", NULL, 0, 0, hint, NULL);
  
  /* add /SHINT */
	add_module_proc(COMMAND_PROC, "hint", "shint", NULL, 0, 0, shint, NULL);

  /* add /REHINT */
	add_module_proc(COMMAND_PROC, "hint", "rehint", NULL, 0, 0, rehint, NULL);

  /* add /HINTSAY */
	add_module_proc(COMMAND_PROC, "hint", "hintsay", NULL, 0, 0, hintsay, NULL);
  
  /* add /HHELP */
	add_module_proc(COMMAND_PROC, "hint", "hhelp", NULL, 0, 0, hhelp, NULL);
  
	srand(time(NULL));
	put_it("%s", cp(HINT_LOADED));
  
	max_hints=0;
	malloc_strcpy(&f, HINT_FILENAME);
	if ((fp = uzfopen(&f, get_string_var(LOAD_PATH_VAR), 0)))
	{
		while (fgets(hint_buf, HINT_MAX_LEN, fp)!=NULL) 
			max_hints++;
		fclose(fp);
		put_it("%s (using %d hints)", cp("*** %W<%Ghint%W>%n"), max_hints);
		put_it("%s", cp("*** %W<%Ghint%W>%n try %W/HHELP%n."));
		max_hints--;
	} else
		put_it("%s", cp("*** %W<%Ghint%W>%n Hint file not found"));
	new_free(&f);
	return 0;
}
