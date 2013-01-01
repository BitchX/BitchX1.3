/*
 * Program source is Copyright 1993 Colten Edwards.
 * Central Data Services is granted the right to use this source
 * in thier product known as Quote Menu. They are also granted the right
 * to modify this source in the advent of problems and or new features
 * which they may want to add to the program.
 */

#include <stdlib.h>
#include <dirent.h>
#include <ncurses.h>
#define MAXPATH MAXNAMLEN 
/* Defines for shadows. Used in the Box rountine. */

#define NO_SHADOW           0
#define SINGLER_SHADOW      1
#define DOUBLER_SHADOW      2
#define SINGLEL_SHADOW      3
#define DOUBLEL_SHADOW      4

#define NO_LINE             0
#define SINGLE_LINE         1
#define DOUBLE_LINE         2
#define SINGLE_TOP          3
#define SINGLE_SIDE         4
#define SOLID_LINES         5
#define TOP_BOT_SLINE       6
#define TOP_BOT_DLINE       7
#define TOP_BOT_SOLID       8

#define NUM_KEYS 80
/* Number of color table entries */
#define MAXC_TABLE   6

/* Number of User Defined Functions */
#define MAXU_FUNCS   10

#define CURCOL        24   /* Starting edit cursor column */


#define WRITE_FILE   0
#define READ_FILE    1



#define CURSOROFF 0x2000

/* A define used for the CONFIG entries on the max. path allowed.*/

#define WILDCARD  0x01
#define EXTENSION 0x02
#define FILENAME  0x04
#define DIRECTORY 0x08
#define DRIVE     0x10


typedef struct  {
             char letter;
             int return_answer;
          } _default;


#define LOC_YN_Y 18
#define LOC_YN_X 24


extern struct _VIDEO {
    unsigned char winleft;
    unsigned char wintop;
    unsigned char winright;
    unsigned char winbottom;
    unsigned char attribute;
    unsigned char normattr;
    unsigned char currmode;
    unsigned char screenheight;
    unsigned char screenwidth;
    unsigned char graphics;
    unsigned char needcgasync;
    char *videobuffer;
} _video;

typedef struct dlistmem *dlistptr;

struct datainforec {
       char *option;
	char *help;
      
	int other;
	char *save;
	int (*func)();
	int integer;
	int type;
#if 0
       long act_size;
       int volume;
       long pos_in_archive;
       dev_t dev;
       uid_t uid;
       gid_t gid;
       umode_t mode;
       umode_t org_mode;
       long size;
       time_t atime;
       time_t mtime;
       time_t ctime;
       time_t backup_time;
       char name_len;
       char compressed;
       long checksum;
#endif
       int mark;
};

struct dlistmem {
        struct datainforec datainfo;
        dlistptr nextlistptr;
        dlistptr prevlistptr;
};

typedef struct cell
{
   /*
    * Possible Startup and Exit Procedures to use
    */
   int (*ListEntryProc)(struct cell *cell );
   int (*ListExitProc)(struct cell *cell);
   /*
    * What to do on a key  and the screen paint routine
    * What to do on a special key
    */
   int (*ListEventProc)(struct cell *cell);
   int (*ListPaintProc)(struct cell *cell);
   int (*OtherGetKeyProc)(struct cell *cell);

   /* Possible actions to take with a keystroke. */
   /*
    * The current line we are at.
    */
   int (*current_event)(struct cell *cell);

   int (*event1)(struct cell *cell);
   int (*event2)(struct cell *cell);

   int (*UpdateStatusProc)(struct cell *cell);

   struct KeyTable *keytable;
   struct FuncTable *func_table;

   /* These are what start everything off. */
   /* start is the start of the list.*/
   /* end is the end of the list. */
   /* current is the current highlighted line on the screen*/

   dlistptr start;
   dlistptr current;
   dlistptr end;
   /*
    * Begin and end of the current display list. current should be somewhere
    * between these.
    */
   dlistptr list_start;
   dlistptr list_end;

   WINDOW *window;	   /* WINDOW from ncurses */

   int redraw;             /* screen needs attention */
   int srow;               /* window start row cordinate */
   int scol;               /* Window start Column */
   int erow;               /* Window Ending Row */
   int ecol;               /* Window ending column */
   int max_cols;           /* Max Columns on the screen */
   int max_rows;           /* Max Rows on the screen */
   int save_row;           /* save the cursor pos row */
   int save_col;           /* save the cursor pos col */
   int cur_pos;            /* current cursor column when using edit*/
   int cur_row;            /* current cursor row when using edit */
   int desc_start;         /* Used when editing large fields. */

   unsigned int termkey;            /* this is cleared on enter, setting it to
                                     * any value other than zero causes the
                                     * function to return. Usually set to the
                                     * key we terminate with.
                                     */
   unsigned int special;            /* Holds value for the current item function
                                      * number
                                      */


   unsigned int key;                /* Current Keystroke */
   unsigned int other_getkey;       /* set this to use other get_char routine */
                                     /* if supplied */

   unsigned int normcolor;          /* Normal Text Screen Color*/
   unsigned int barcolor;           /* Bar text Screen Color */

   int insert_mode;                 /* Insert/OverWrite Cursor Mode */

   int menu_bar;                   /* Set this if a menu at the bottom
                                     * of the screen is needed
                                     */
   int dest_src;
   char *filename;
   char **argv;
   int argc;
}  CELL;


typedef struct KeyTable
{
   int key;
   int func_num;
   char desc[40];
} KEYTABLE;

typedef struct UKeyTable
{
   int key;
   char desc[40];
   char program[MAXPATH+1];
   char params[40];
} USERKEYTABLE;

typedef struct FuncTable
{
   int key;
   int (*func)(struct cell *c);
} FUNC_TABLE ;

struct COLOR {
    unsigned char dosborder ;
    unsigned char dostext   ;
    unsigned char background;
    unsigned char back_text;
    unsigned char menuborder ;
    unsigned char menuback   ;
    unsigned char menutext   ;
    unsigned char menucursor ;
    unsigned char menushadow ;
    unsigned char fileborder;
    unsigned char filebar;
    unsigned char filetext;
};


#define LS_END                0
#define LS_HOME               1
#define LS_PGUP               2
#define LS_PGDN               3
#define CURSOR_UP             4
#define CURSOR_DN             5
#define LS_QUIT               6
#define WRAP_CURSOR_DN        7
#define WRAP_CURSOR_UP        8
#define DO_SHELL              9
#define LS_ENTER              10
#define UPFUNC		      11
#define TOGGLE                12
#define PLUS_IT               13
#define MINUS_IT              14
#define LS_CLEAR              15
#define LS_TOGGLE             16
#define RENAME_FILE           17
#define CHANGE_SRC            18
#define CHANGE_DEST           19
#define LS_PLAY		      20
