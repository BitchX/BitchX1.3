/*
  Europa - Copyright (c) 1999, Ed Schlunder <zilym@asu.edu>

  This is free software distributable under the terms of the GNU GPL-- See
  the file COPYING for details.
 */

#define MOD_VERSION "0.01"
#define MOD_NAME "Europa"
#include "europa.h"

#include <mysql/mysql.h>

/* yay! mysql.h and irc.h both define NAME_LEN... */
#undef NAME_LEN
#include "irc.h"
#include "struct.h"
#include "server.h"
#include "ircaux.h"
#include "status.h"
#include "screen.h"
#include "vars.h"
#include "misc.h"
#include "output.h"
#include "hook.h"
#include "module.h"
#define INIT_MODULE
#include "modval.h"
#include "irc_std.h"

#define MAX_WORD 100
#define MAX_WORDS 1000
#define MAX_CHANNELS 10
#define MAX_QUERY 1000

int beQuiet = 0;
MYSQL mysql;

/* called when bitchx user enters IRC command "/europa DATA" */
BUILT_IN_DLL(europa)
{
  int i;

  put_it("** /europa %s baby!", args);
}

/* print some chat to both the IRC server (other channel members) and the local
   client window... */
void dualOut(const char *str, const char *chan) {
  put_it("<europa> %s", str);
  send_to_server("PRIVMSG %s :%s", chan, str);
}

/* a wrapper on dualOut() to do printf-like text formatting on outgoing text */
void sout(char *chan, const char *format, ...) {
  va_list args;
  char tmp[MAX_WORD * MAX_WORDS];

  if(beQuiet)
    return;

  va_start(args, format);
  vsnprintf(tmp, MAX_WORD*MAX_WORDS, format, args);
  va_end(args);

  dualOut(tmp, chan);
}

void sdunno(char *args[]) {
  sout(args[1], "%s: Hmm, I don't know that one...", args[0]);
}

void shello(char *chan, char *who) {
  if(who == NULL)
    sout(chan, "hey, how's it going?");
  else
    sout(chan, "%s: hey, how's it going?", who);
}

/* attempts to find an answer to a keyword that we've been asked about... */
char *dbLookup(char *keyword, char *table) {
  MYSQL_RES *res;
  MYSQL_ROW row;
  char query[MAX_QUERY];
  char *answer, *encodedKeyword = malloc(strlen(keyword) * 2 + 1);

  mysql_escape_string(encodedKeyword, keyword, strlen(keyword));
  if(snprintf(query, MAX_QUERY,
	      "select answer from %s where keyword like '%s'", 
	      table, encodedKeyword) == -1) {
    put_it("** Europa query overflow (increase MAX_QUERY)");

    free(encodedKeyword);
    return NULL;
  }
  free(encodedKeyword);

  if(mysql_query(&mysql, query))
    return NULL;

  if(!(res = mysql_store_result(&mysql))) {
    /* this shouldn't happen... */
    put_it("** Europa query failure: %s", query);
    return NULL;
  }
      
  row = mysql_fetch_row(res);
  if(row == NULL) {
      mysql_free_result(res);
      return NULL;
  }

  answer = strdup(row[0]);

  mysql_free_result(res);
  return answer;
}

/* args[0] - nick of sender
   args[1] - channel text was said on
  args[2] - actual text */
void processChat(int argc, char *args[], char *argl[]) {
  int i;

  if(argc < 3) return;
  if(strcmp(args[3], "shutup") == 0) {
    sout(args[1], "%s: okay, okay...", args[0]);
    beQuiet = -1;
    return;
  }

  if(strcmp(args[3], "hello") == 0 ||
     strcmp(args[3], "hello?") == 0) {
    if(beQuiet)
      beQuiet = 0;
    else
      shello(args[1], args[0]);
  }

  if(argc < 4) return;
  if(strcmp(args[3], "ex") == 0 ||
     strcmp(args[3], "explain") == 0) {
    int pengy = 0;
    char *answer;

    answer = dbLookup(args[4], "fact");
    if(answer == NULL) {
      answer = dbLookup(args[4], "facts");
      if(answer == NULL) {
	sdunno(args);
	return;
      }
      pengy = -1;
    }

    if(pengy)
      sout(args[1], "%s: %s (from Pengy)", args[0], answer);
    else
      sout(args[1], "%s: %s", args[0], answer);

    free(answer);
    return;
  }

  if(strcmp(args[3], "learn") == 0) {
    char query[MAX_QUERY];
    char *encodedKeyword = malloc(strlen(args[4]) * 2 + 1),
      *encodedAnswer = malloc(strlen(argl[5]) * 2 + 1);

    mysql_escape_string(encodedKeyword, args[4], strlen(args[4]));
    mysql_escape_string(encodedAnswer, argl[5], strlen(argl[5]));

    snprintf(query, MAX_QUERY, "insert into fact values('%s','%s')", 
	     encodedKeyword, encodedAnswer);

    free(encodedKeyword);
    free(encodedAnswer);

    if(mysql_query(&mysql, query))
      put_it("** Europa db query failed: %s", query);
    else
      sout(args[1], "%s: %s learned, thanks...", args[0], args[4]);

    return;
  }

  if(strcmp(args[3], "forget") == 0) {
    char query[MAX_QUERY];
    char *encodedKeyword = malloc(strlen(args[4]) * 2 + 1);

    mysql_escape_string(encodedKeyword, args[4], strlen(args[4]));
    snprintf(query, MAX_QUERY, 
	     "delete from fact where keyword='%s'", encodedKeyword);
    free(encodedKeyword);

    if(mysql_query(&mysql, query)) {
      snprintf(query, MAX_QUERY, 
	       "delete from facts where keyword='%s'", args[4]);
      if(mysql_query(&mysql, query)) {
	put_it("** Europa db query failed: %s", query);
	sout(args[1], "%s: I didn't know anything about %s anyway...", 
	     args[0], args[4]);
      }
      else
	sout(args[1], "%s: %s forgotten from Pengy db...", args[0], args[4]);
    }
    else
      sout(args[1], "%s: %s forgotten...", args[0], args[4]);

    return;
  }
}

/* called by BitchX whenever someone says something in the channel directed to
   this user... */
int public_ar_proc(char *which, char *str, char **unused) {
  char *local, *args[MAX_WORDS], *argl[MAX_WORDS];
  int i = 0, total, w = 0;

  /*
    we want to parse out the text into separate words. args[WORD_N] is
    a pointer to a single word while argl[WORD_N] is a pointer to the
    rest of the sentence beginning at word WORD_N.
  */
  argl[0] = str;
  while(i < strlen(str)) {
    if(str[i] != ' ') break;
    i++;
  }
  local = strdup(str + i);
  args[0] = local;

  total = strlen(local);
  while(i < total && w < MAX_WORDS) {
    if(local[i] == ' ') { 
      local[i] = 0;
      w++;
      while(++i < total)
	if(local[i] != ' ') break;

      args[w] = local + i;
      argl[w] = str + i;
    }

    i++;
  }

  processChat(w, args, argl);

  free(local);
  return 0;
}

int public_proc(char *which, char *str, char **unused) {
  char *local, *args[MAX_WORDS], *argl[MAX_WORDS];
  int i = 0, total, w = 0;

  /*
    we want to parse out the text into separate words. args[WORD_N] is
    a pointer to a single word while argl[WORD_N] is a pointer to the
    rest of the sentence beginning at word WORD_N.
  */
  argl[0] = str;
  while(i < strlen(str)) {
    if(str[i] != ' ') break;
    i++;
  }
  local = strdup(str + i);
  args[0] = local;

  total = strlen(local);
  while(i < total && w < MAX_WORDS) {
    if(local[i] == ' ') { 
      local[i] = 0;
      w++;
      while(++i < total)
	if(local[i] != ' ') break;

      args[w] = local + i;
      argl[w] = str + i;
    }

    i++;
  }

  if(w > 1) {
    if(strstr(argl[2], "hello") != NULL)
      shello(args[1], args[0]);
  }

  free(local);
  return 0;  
}

/* called when user enters irc command "/explain USER/CHANNEL KEYWORD" */
BUILT_IN_DLL(cmdExplain)
{
  char *local, *arg[MAX_WORDS], *argl[MAX_WORDS];
  int i = 0, total, w = 0;

  argl[0] = args;
  while(i < strlen(args)) {
    if(args[i] != ' ') break;
    i++;
  }
  local = strdup(args + i);
  arg[0] = local;

  total = strlen(local);
  while(i < total && w < MAX_WORDS) {
    if(local[i] == ' ') { 
      local[i] = 0;
      w++;
      while(++i < total)
	if(local[i] != ' ') break;

      arg[w] = local + i;
      argl[w] = args + i;
    }

    i++;
  }

  if(w) {
    int pengy = 0;
    char *answer = dbLookup(arg[1], "fact");

    if(answer == NULL) {
      answer = dbLookup(arg[1], "facts");
      if(answer == NULL) {
	put_it("** Europa doesn't know about %s", arg[1]);
	free(local);
	return;
      }

      pengy = -1;
    }

    if(pengy)
      sout(arg[0], "%s (from Pengy)", answer);
    else
      sout(arg[0], answer);
  }

  free(local);
  return;
}

int Europa_Init(IrcCommandDll **intp, Function_ptr *global_table) {
  initialize_module(MOD_NAME);

  add_module_proc(COMMAND_PROC, MOD_NAME, "europa", NULL, 0, 0, europa, NULL);
  add_module_proc(COMMAND_PROC, MOD_NAME, "explain", NULL, 0, 0, cmdExplain, NULL);
  add_module_proc(HOOK_PROC, MOD_NAME, NULL, "*", PUBLIC_AR_LIST, 1, NULL, public_ar_proc);
  add_module_proc(HOOK_PROC, MOD_NAME, NULL, "*", PUBLIC_LIST, 1, NULL, public_proc);

  put_it("** Europa v%s connecting to database backend...", MOD_VERSION);

  /* connect to the database server */
  if(!(mysql_connect(&mysql, DBHOST, DBUSER, DBPASSWD))) {
    put_it("** Server refused login/password.");
    return 0;
  }
  if(mysql_select_db(&mysql, DBNAME)) {
    put_it("** Server refused connection to '%s' database.", DBNAME);
    return 0;
  }

  put_it("** Europa loaded!");
  return 0;
}
