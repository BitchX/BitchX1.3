
#ifndef _GUI_H_
#define _GUI_H_
#ifdef GUI

#define         GUISUBMENU             1
#define         GUIMENUITEM            (1 << 1)
#define         GUIIAMENUITEM          (1 << 2)
#define         GUIDEFMENUITEM         (1 << 3)
#define         GUIBRKMENUITEM         (1 << 4)
#define         GUISEPARATOR           (1 << 5)
#define         GUISHARED              (1 << 6)
#define         GUIBRKSUBMENU          (1 << 7)
#define         GUICHECKEDMENUITEM     (1 << 8)
#define         GUINDMENUITEM          (1 << 9)
#define         GUICHECKMENUITEM       (1 << 10)

#ifdef __EMXPM__
#include "PMbitchx.h"
#elif defined(GTK)
#include "gtkbitchx.h"
#elif defined(WIN32)
#include "winbitchx.h"
#endif

#define EVNONE    	0
#define EVMENU    	1
#define EVFOCUS   	2
#define EVREFRESH 	3
#define EVSTRACK  	4
#define EVSUP     	5
#define EVSDOWN   	6
#define EVSUPPG   	7
#define EVSDOWNPG 	8
#define EVKEY     	9
#define EVFILE    	10
#define EVTITLE		11
#define EVPASTE		12
#define EVDELETE	13

#define COLOR_INACTIVE	0
#define COLOR_ACTIVE	1
#define COLOR_HIGHLIGHT	2

#include "struct.h"
/* These are all the functions that each GUI module must provide for 
   a clean port to other GUI OSes. 

   gui_init() - main initialization routine.
   */
void gui_init(void);
void gui_clreol(void);
void gui_gotoxy(int col, int row);
void gui_clrscr(void);
void gui_left(int num);
void gui_right(int num);
void gui_scroll(int top, int bot, int n);
void gui_flush(void);
void gui_puts(unsigned char *buffer);
void gui_new_window(Screen *new, Window *win);
void gui_kill_window(Screen *killscreen);
void gui_settitle(char *titletext, Screen *screen);
void gui_font_dialog(Screen *screen);
void gui_file_dialog(char *type, char *path, char *title, char *ok, char *apply, char *code, char *szButton);
void gui_properties_notebook(void);
void gui_msgbox(void);
void gui_popupmenu(char *menuname);
void gui_paste(char *args);
void gui_setfocus(Screen *screen);
void gui_scrollerchanged(Screen *screen, int position);
void gui_query_window_info(Screen *screen, char *fontinfo, int *x, int *y, int *cx, int *cy);
void gui_play_sound(char *filename);
void gui_get_sound_error(int errnum, char *errstring);
void gui_menu(Screen *screen, char *addmenu);
void gui_exit(void);
void gui_screen(Screen *new);
void gui_resize(Screen *new);
int gui_send_mci_string(char *mcistring, char *retstring);
int gui_isset(Screen *screen, fd_set *rd, int what);
int gui_putc(int c);
int gui_read(Screen *screen, char *buffer, int maxbufsize);
int gui_screen_width(void);
int gui_screen_height(void);
void gui_setwindowpos(Screen *screen, int x, int y, int cx, int cy, int top, int bottom, int min, int max, int restore, int activate, int size, int position);
void gui_font_init(void);
void gui_font_set(char *font, Screen *screen);
void BX_gui_mutex_lock(void);
void BX_gui_mutex_unlock(void);
int gui_setmenuitem(char *menuname, int refnum, char *what, char *param);
void gui_remove_menurefs(char *menu);
void gui_mdi(Window *, char *, int);
void gui_update_nicklist(char *);
void gui_nicklist_width(int, Screen *);
void gui_startup(int argc, char *argv[]);
void gui_about_box(char *about_text);
void gui_activity(int color);
void gui_setfileinfo(char *filename, char *nick, int server);
void gui_setfd(fd_set *rd);

/* These are just miscellaneous GUI function declarations */
MenuStruct *findmenu(char *menuname);
void sendevent(unsigned int event, unsigned int window);

/* A fix for different systems declaring tputs differently */
#ifdef GTK
#ifdef __sun__
int      gtkputs (const char *str, int n, int (*blah)(int));
#else
int      gtkputs __P((const char *str, int n, int (*blah)(int)));
#endif
#define tputs gtkputs
#endif

#define current_screen      last_input_screen
#define INPUT_BUFFER        current_screen->input_buffer
#define ADD_TO_INPUT(x)     strmcat(INPUT_BUFFER, (x), INPUT_BUFFER_SIZE);

#endif
#endif
