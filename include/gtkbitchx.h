#ifndef _GTK_BITCHX_
#define _GTK_BITCHX_

#if defined(GUI)
#ifdef __linux__
/* For glibc linux systems. */ 
#ifndef _REENTRANT
#define _REENTRANT
#endif
#ifndef _POSIX_SOURCE             
#define _POSIX_SOURCE
#endif
#define _P __P                 
#endif                         

/* So X11/X.h won't get included */
#define X_H

void menuitemhandler(gpointer *data);
GtkWidget *newsubmenu(MenuStruct *menutoadd);

int gtk_getx(void);
int gtk_gety(void);
void newmenubar(Screen *menuscreen, MenuStruct *menutoadd);
GtkWidget *newsubmenu(MenuStruct *menutoadd);
void gtk_new_window(Screen *new, Window *win);
void gtk_paste (GtkWidget *widget, GtkSelectionData *selection_data,
                           gpointer data);
void selection_handle (GtkWidget *widget,
					   GtkSelectionData *selection_data,
					   guint info, guint seltime);
int gtkprintf(char *format, ...);
void size_allocate (GtkWidget *widget, GtkWindow *window);

void gtk_about_box(char *about_text);
void gtk_label_set_color(GtkWidget *label, int color);
void gtk_tab_move(int page, int pos);
void gtk_main_paste(int refnum);

int notebook_page_by_refnum(int refnum);
void make_new_notebook(void);
void tab_to_window(Screen *tmp);
void window_to_tab(Screen *tmp);
char *gtk_windowlabel(Screen *tmp);
void gtk_windowicon(GtkWidget *window);

void make_tip(GtkWidget *widget, char *tip);

#endif

void gui_mutex_lock(void);
void gui_mutex_unlock(void);

#endif
