/* Possum Mail copyright 1997 Patrick J. Edwards */

#include <stdio.h>
#include <stdlib.h>
#include "irc.h"
#include "struct.h"
#include "dcc.h"
#include "ircaux.h"
#include "ctcp.h"
#include "status.h"
#include "lastlog.h"
#include "server.h"
#include "screen.h"
#include "vars.h"
#include "misc.h"
#include "output.h"
#include "list.h"
#include "module.h"
#define INIT_MODULE
#include "modval.h"
#include "llist.h"
#include "head.h"

#define MAX_FNAME_LENGTH 2048
#define MAX_FBUFFER_SIZE 2048
#define MAX_HINFO_SIZE 127

/* structures */

struct msg_header_struct {
  char from[MAX_HINFO_SIZE+1], 
       to[MAX_HINFO_SIZE+1], 
       subject[MAX_HINFO_SIZE+1], 
       date[MAX_HINFO_SIZE+1];
  long body_offset;
};
typedef struct msg_header_struct msg_header;

struct mbox_struct {
  char filename[2048];
  int number, lastaccess, size;
  llist *headers;
} MBOX;
typedef struct mbox_struct mbox;

#define cparse convert_output_format
#define PM_PROMPT "%W<%GP%gosso%GM%W>%n" 
#define PM_VERSION "0.2"

/* prototypes */

char *strchop(char *s);
int parse_header(FILE *f, llist *l);
llist *read_mbox(char *fname);

BUILT_IN_DLL(pm_headers);
BUILT_IN_DLL(pm_count);
BUILT_IN_DLL(pm_list);
BUILT_IN_DLL(pm_read);
BUILT_IN_DLL(pm_help);
BUILT_IN_DLL(pm_mailbox);

/*
void pm_check
void pm_update
*/
char *Possum_Version(IrcCommandDll **intp) 
{
	return PM_VERSION;
}

int Possum_Init(IrcCommandDll **intp, Function_ptr *global_table) {
  char *tmp = getenv("MAIL");
  
  initialize_module("possum");
  
  MBOX.headers = NULL;
  
  add_module_proc(COMMAND_PROC, "possum", "pmheaders", NULL, 0, 0, pm_headers, NULL);
  add_module_proc(COMMAND_PROC, "possum", "pmcount", NULL, 0, 0, pm_count, NULL);
  add_module_proc(COMMAND_PROC, "possum", "pmlist", NULL, 0, 0, pm_list, NULL);
  add_module_proc(COMMAND_PROC, "possum", "pmread", NULL, 0, 0, pm_read, NULL);
  add_module_proc(COMMAND_PROC, "possum", "pmmailbox", NULL, 0, 0, pm_mailbox, NULL);
  add_module_proc(COMMAND_PROC, "possum", "pmhelp", NULL, 0, 0, pm_help, NULL);

  if (tmp) strncpy(MBOX.filename, tmp, MAX_FNAME_LENGTH);

  put_it("%s Possom Mail %s for BitchX has been excited.", cparse(PM_PROMPT, NULL, NULL), PM_VERSION);
  put_it("%s %s", cparse(PM_PROMPT, NULL, NULL), "Type /PMHELP for help.");
  if (tmp) {
    put_it("%s Using %s for default mail box.", cparse(PM_PROMPT, NULL, NULL), MBOX.filename);
    MBOX.headers = read_mbox(MBOX.filename);
  } else {
    put_it("%s Could not find MAIL in your environment.", cparse(PM_PROMPT, NULL, NULL));
    put_it("%s You will have to manually set it with /PMMAILBOX.", cparse(PM_PROMPT,NULL, NULL));
  }
  return 0;      
}
                                     
int parse_header(FILE *f, llist *l) {
  msg_header h;
  char s[MAX_FBUFFER_SIZE];

  if (!feof(f)) { 
    fgets((char *)s, MAX_FBUFFER_SIZE, f);
    strchop(s);
  }

  while ((strcmp(s,"")!=0) && (!feof(f))) {
    if (strstr(s, "From: ")) {
      char *tmp = strstr(s,": ")+2;
      if (tmp) strncpy(h.from, tmp, MAX_HINFO_SIZE);
    } else if (strstr(s, "Subject: ")) {
      char *tmp = strstr(s,": ")+2;
      if (tmp) strncpy(h.subject, tmp, MAX_HINFO_SIZE);
    } else if (strstr(s, "To: ")) {
      char *tmp = strstr(s,": ")+2;
      if (tmp) strncpy(h.to, tmp, MAX_HINFO_SIZE);
    } else if (strstr(s, "Date: ")) {
      char *tmp = strstr(s,": ")+2;
      if (tmp) strncpy(h.date, tmp, MAX_HINFO_SIZE);
    }
    fgets((char *)s, MAX_FBUFFER_SIZE, f);
    strchop(s);
  }
  h.body_offset = ftell(f);
  return(lpush(l, &h));
}

char *strchop(char *s) {
  if (s) s[strlen(s)-1] = '\0';
  return(s);
}

llist *read_mbox(char *fname) 
{
  FILE *f;
  llist *headers;
  char *buf = (char *) malloc(MAX_FBUFFER_SIZE);
  
  f = fopen(fname, "r");
  if (!f) return(NULL);
  

  headers = lmake(sizeof(msg_header));
  if (!headers) return(NULL);
  
  while (!feof(f)) {
    fgets(buf, MAX_FBUFFER_SIZE, f);
    strchop(buf);
    if (ishead(buf))  {
      parse_header(f, headers);
/*      put_it("found header %d", headers->length);*/
    }
  }
  
  fclose(f);
  return(headers);
}

BUILT_IN_DLL(pm_mailbox)
{
  char *tmp;
  tmp = next_arg(args, &args);
  if (tmp) {
    strncpy(MBOX.filename, tmp, MAX_FNAME_LENGTH);
    put_it("%s Set mail box to: %s ", cparse(PM_PROMPT, NULL, NULL), MBOX.filename);
  } else
    put_it("%s You have to enter your mail box.", cparse(PM_PROMPT, NULL, NULL));
}

BUILT_IN_DLL(pm_help)
{
  put_it("%s Possum Mail %s", cparse(PM_PROMPT, NULL, NULL), PM_VERSION);
  put_it("%s /PMHELP     - This help.", cparse(PM_PROMPT, NULL, NULL));
  put_it("%s /PMMAILBOX  - Set your mailbox.", cparse(PM_PROMPT, NULL,NULL));
  put_it("%s /PMREAD [x] - Read the xth email.", cparse(PM_PROMPT, NULL, NULL));
  put_it("%s /PMLIST     - Lists all email From: lines.", cparse(PM_PROMPT, NULL, NULL));
  put_it("%s /PMCOUNT    - Returns the number of emails in the mail box.", cparse(PM_PROMPT, NULL, NULL));
}

/* this is a rather ugly and inefficent piece of code. blech. */
BUILT_IN_DLL(pm_read)
{
  char *number, *s;
  int num,  have_head=0;
  FILE *f;
  msg_header *h = NULL;

  number = next_arg(args, &args);
  if (number) num = atoi(number);
  else {
    put_it("%s You have to provide an arguement.", cparse(PM_PROMPT, NULL, NULL));
    return;
  }
  if (MBOX.headers && (num-1 > MBOX.headers->length)) return;
  
  s = (char *) malloc(MAX_FBUFFER_SIZE);
  if (!s) return;

  f = fopen(MBOX.filename, "r");
  if (!f) return;
  
    h = lindex(MBOX.headers, num-1);
    
    if (h && !have_head) {
      put_it("%s", cparse(PM_PROMPT"  %W<%YFrom%W>%n $0-", "%s", h->from));
      put_it("%s", cparse(PM_PROMPT"  %W<%YDate%W>%n $0-", "%s", h->date));
      put_it("%s", cparse(PM_PROMPT"  %W<%YSubject%W>%n $0-", "%s", h->subject));
      fseek(f, h->body_offset, SEEK_SET);
      do {
        strchop(fgets(s, 2560, f));
        if (ishead(s)) {
          have_head = 1;
          break;
        }
        put_it("%s%s", cparse("%G|%n", NULL, NULL), s);
      } while (!feof(f));
    }

  free(s); 
  fclose(f);
}

BUILT_IN_DLL(pm_list)
{
  unsigned long counter = 0;
  msg_header *h;

  do {
    h = lindex(MBOX.headers, counter);
    if (h)
      put_it("%s", cparse(PM_PROMPT"  %W<%Y$0%W>%n $1-", "%d %s", ++counter, h->from));
  } while (h);
}


BUILT_IN_DLL(pm_headers)
{
}

BUILT_IN_DLL(pm_count)
{
  if (MBOX.headers)
  put_it("%s Email Count: %d", cparse(PM_PROMPT,NULL, NULL), MBOX.headers->length);
}
