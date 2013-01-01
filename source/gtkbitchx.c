/* gtkbitchx.c: Written by Brian Smith (NuKe) for BitchX */

/* So X11/X.h won't get included */
#define X_H

#include <zvt/zvtterm.h>
#include <gdk/gdkkeysyms.h>
#include <pthread.h>
#ifdef SOUND
#include <esd.h>
#endif
#include "ircaux.h"
#include "misc.h"
#include "window.h"
#include "server.h"
#include "hook.h"
#include "input.h"
#include "commands.h"
#include "list.h"
#include "hash2.h"
#include "gui.h"
#include "newio.h"
#include "../doc/BitchX.xpm"

char BitchXbg[] = "BitchX.xpm";

GtkWidget	*mainwindow,
		    *mainviewport,
		    *mainmenubar,
		    *mainscroller,
		    *mainscrolledwindow,
		    *mainclist,
		    *mainbox;
GdkFont		*mainfont;
char            *mainfontname;
GtkAdjustment	*mainadjust;
int 		mainpipe[2];

/* For MDI */
GtkWidget       *MDIWindow,
                *notebook;
int current_mdi_mode = 0;

/* This is a pointer to the clipboard data (cut & paste) */
char 		*clipboarddata = NULL, 
		*pasteargs = NULL, *selectdata;

/* Theses variables are used for interthread communication and 
   synchronization */
gint            gtkio;

int		gtkipcin[2], 
		guiipc[2], 
		gtkcommandpipe[2];
pthread_t	gtkthread;
pthread_mutex_t evmutex;
pthread_cond_t 	evcond, 
		gtkcond;
int predicate;;
extern fd_set 	readables;

/* These are for $lastclickline() function */
char		*lastclicklinedata = NULL;

int		lastclickcol,
		lastclickrow,
		contextx,
		contexty;

/* Used by the scroller bars */
int		newscrollerpos,
		lastscrollerpos,
		lastscrollerwindow;
unsigned long	menucmd;
int		justscrolled = 0;

/* needed for rclick */
#include "keys.h"

unsigned long	menucmd;   

/* This structure is used for passing commands to the gtk thread from the main thread */
int gtkincommand = FALSE;
typedef struct _gtkcommand {
	int		command;
	gpointer	data[20];
} GtkCommand;

GtkCommand gtkcommand;

/* This is used in File Dialog to pass code to the main thread */
char		*codeptr,
		*paramptr;

/* Used for sound support */
#ifdef SOUND
char		*gtk_hostname = NULL;
int		esd_connection = -1;
#endif

/* Function Prototypes */
MenuStruct *findmenu(char *menuname);
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

/* Function prototypes needed for the properties notebook */
IrcVariable *return_irc_var(int nummer);
IrcVariable *return_fset_var(int nummer);
CSetArray *return_cset_var(int nummer);
WSetArray *return_wset_var(int nummer);
int in_properties = 0;

#define WIDTH	10

gushort bx_red[] = { 	0x0000, 0xbbbb, 0x0000, 0xaaaa, 0x0000, 0xbbbb, 0x0000, 0xaaaa, 0x7777, 
			0xffff, 0x0000, 0xeeee, 0x0000, 0xffff, 0x0000, 0xffff, 0xaaaa, 0x0000 };
gushort bx_green[] = { 	0x0000, 0x0000, 0xbbbb, 0xaaaa, 0x0000, 0x0000, 0xbbbb, 0xaaaa, 0x7777, 
			0x0000, 0xffff, 0xeeee, 0x0000, 0x0000, 0xeeee, 0xffff, 0xaaaa, 0x0000 };
gushort bx_blue[] = { 	0x0000, 0x0000, 0x0000, 0x0000, 0xcccc, 0xbbbb, 0xbbbb, 0xaaaa, 0x7777, 
			0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 0xeeee, 0xffff, 0xaaaa, 0x0000};

GdkColor colors[] =
{
	{ 0, 0x0000, 0x0000, 0x0000 },	/* 0  black */
	{ 0, 0xbbbb, 0x0000, 0x0000 },	/* 1  red */
	{ 0, 0x0000, 0xbbbb, 0x0000 },	/* 2  green */
	{ 0, 0xaaaa, 0xaaaa, 0x0000 },	/* 3  yellow */
	{ 0, 0x0000, 0x0000, 0xcccc },	/* 4  blue */
	{ 0, 0xbbbb, 0x0000, 0xbbbb },	/* 5  magenta */
	{ 0, 0x0000, 0xbbbb, 0xbbbb },	/* 6  cyan */
	{ 0, 0xaaaa, 0xaaaa, 0xaaaa }, 	/* 7  white */
	{ 0, 0x7777, 0x7777, 0x7777 }, 	/* 8  grey */
	{ 0, 0xffff, 0x0000, 0x0000 },	/* 9  bright red */
	{ 0, 0x0000, 0xffff, 0x0000 },	/* 10 bright green */
	{ 0, 0xeeee, 0xeeee, 0x0000 },	/* 11 bright yellow */
	{ 0, 0x0000, 0x0000, 0xffff },	/* 12 bright blue */
	{ 0, 0xffff, 0x0000, 0xffff },	/* 13 bright magenta */
	{ 0, 0x0000, 0xeeee, 0xeeee },	/* 14 bright cyan */
	{ 0, 0xffff, 0xffff, 0xffff },	/* 15 bright white */
};

typedef enum _gtkcommandlist 
{
	GTKNONE,
	GTKFILEDIALOG,
	GTKMENUBAR,
	GTKEXIT,
	GTKSETFOCUS,
	GTKMENUPOPUP,
	GTKNEWWIN,
	GTKTITLE,
	GTKPROP,
	GTKFONT,
	GTKWINKILL,
	GTKSCROLL,
	GTKMDI,
	GTKMSGBOX,
	GTKPASTE,
	GTKSETFONT,
	GTKREDRAWMENUS,
	GTKNICKLIST,
	GTKSETWINDOWPOS,
	GTKABOUTBOX,
	GTKACTIVITY,
	GTKNICKLISTWIDTH
} gtkcommandlist;

#ifdef __sun__
int      gtkputs (const char *str, int n, int (*blah)(int))
#else
int      gtkputs __P((const char *str, int n, int (*blah)(int)))
#endif
{
	if (str && *str)
	{
		/*write((output_screen && output_screen->pipe[1]) ? output_screen->pipe[1] : mainpipe[1], str, strlen(str));*/
		if (output_screen && output_screen->pipe[1])
			write(output_screen->pipe[1], str, strlen(str));
		return strlen(str);
	}
	return 0;
}

/* Find the screen of the Window that generated an event */
Screen *findscreen(GtkWidget *widget)
{
Screen *tmp;

	for (tmp = screen_list; tmp; tmp = tmp->next)
	{
		if(tmp->alive && widget == tmp->window)
			return tmp;
	}
	return NULL;
}

Screen *findoutputscreen(int fd)
{
Screen *tmp;
	
	for (tmp = screen_list; tmp; tmp = tmp->next)
	{
		if(tmp->alive && fd == tmp->pipe[0])
			return tmp;
	}
	return NULL;
}

int notebook_page_by_refnum(int refnum)
{
	Window *thiswindow = get_window_by_refnum(refnum);

	if(thiswindow)
	{
		Screen *thisscreen = thiswindow->screen;

		if(thisscreen && thisscreen->page)
			return gtk_notebook_page_num(GTK_NOTEBOOK(notebook),thisscreen->page->child);
	}
	return -1;
}

void sendevent(unsigned int event, unsigned int window)
{
	unsigned int evbuf[2];

	if(guiipc[1])
	{
		evbuf[0] = event;
		evbuf[1] = window;
		write(guiipc[1], (void *)evbuf, sizeof(unsigned int) * 2);
	}
	return;
}

int new_menuref(MenuRef **root, int refnum, char *menutext, int checked)
{
MenuRef *new = new_malloc(sizeof(MenuRef));

	if (new)
	{
		new->refnum = refnum;
		if (menutext)
			new->menutext = m_strdup(menutext);
		else
			new->menutext = NULL;
		new->checked = checked;
		new->next = NULL;
		new->menuhandle = NULL;
        
		if (!root || !*root)
			*root = new;
		else
		{
			MenuRef *prev = NULL, *tmp = *root;
			while (tmp)
			{
				prev = tmp;
				tmp = tmp->next;
			}
			if (prev)
				prev->next = new;
			else
				*root = new;
		}
		return TRUE;
	}
	return FALSE;
}

int remove_menuref(MenuRef **root, int refnum)
{
MenuRef *prev = NULL, *tmp = *root;

	while (tmp)
	{
		if (tmp->refnum == refnum)
		{
			if (!prev)
			{
				new_free(&tmp->menutext);
				new_free(&tmp);
				*root = NULL;
				return 0;
			}
			else
			{
				prev->next = tmp->next;
				new_free(&tmp->menutext);
				new_free(&tmp);
				return 0;
			}
		}
		tmp = tmp->next;
	}
	return 0;
}

MenuRef *find_menuref(MenuRef *root, int refnum)
{
MenuRef *tmp = root;

	while (tmp)
	{
		if (tmp->refnum == refnum)
			return tmp;
		tmp = tmp->next;
	}
	return NULL;
}

/* This gets called when a window close event occurs, if it is the last
   active window, kill_screen will exit the application if WINDOW_CREATE
   is defined.  Otherwise it exits the application immediately.  */
gint window_destroy(GtkWidget *widget, GdkEvent *event, gpointer data)
{
#ifdef WINDOW_CREATE
Screen *closescreen;

	if((closescreen = findscreen(widget)) && closescreen->current_window)
		sendevent(EVDELETE, closescreen->current_window->refnum);
	return TRUE;
#else
	irc_exit(1, NULL, "%s rocks your world!", VERSION);
	return FALSE;
#endif
}

/* Wait for a command to be processed */
void gtk_wait_command(void)
{
#if 1
	/* should use a mutex here to be clean */
	while (gtkincommand==TRUE)
		usleep(10);
#else
	pthread_cond_wait(&gtkcond, &evmutex);
#endif
}

/* Write a formatted string to the output_screen */
int gtkprintf(char *format, ...) 
{
	va_list args;
	char    putbuf[400];

	va_start(args, format);
	vsprintf(putbuf, format, args);
	va_end(args);

	if(output_screen && output_screen->pipe[1])
		write(output_screen->pipe[1], putbuf, strlen(putbuf));
	return strlen(putbuf);
}            
                                                                                                                                                                          
/* Write a character to the current output_screen */
int gtkputc(unsigned char c)
{
	unsigned char tmpbuf[2];

	tmpbuf[0] = (unsigned char)c; tmpbuf[1]=0;
	if (output_screen && output_screen->pipe[1])
		write(output_screen->pipe[1], tmpbuf, 1);
	return 1;
}

void drawnicklist(Screen *this_screen)
{
	NickList *n, *list = NULL;
	ChannelList *cptr;
	char minibuffer[NICKNAME_LEN+1];
	int nickcount=0;
	char *tmptext[1];

	if(!this_screen)
		return;

	gtk_clist_clear((GtkCList *)this_screen->clist);

	if(this_screen->current_window && this_screen->current_window->current_channel)
	{
		cptr = lookup_channel(this_screen->current_window->current_channel, this_screen->current_window->server, 0);
		list = sorted_nicklist(cptr, get_int_var(NICKLIST_SORT_VAR));

		for(n = list; n; n = n->next)
		{
				minibuffer[0]='\0';
				if(nick_isvoice(n))
						strcpy(minibuffer, "+");
				if(nick_isop(n))
						strcpy(minibuffer, "@");
				if(nick_isircop(n))
						strcat(minibuffer, "*");
				strcat(minibuffer, n->nick);
				tmptext[0] = &minibuffer[0];
				gtk_clist_append((GtkCList *)this_screen->clist, tmptext);
				gtk_clist_set_background((GtkCList *)this_screen->clist, nickcount, &colors[0]);
				gtk_clist_set_foreground((GtkCList *)this_screen->clist, nickcount, &colors[7]);
				nickcount++;
		}
		clear_sorted_nicklist(&list);
	}
}

/* This gets called on a keypress event and writes it to the input queue */
gint gtk_keypress(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
char extkeycode[4];

	if (gtkipcin[1])
	{
		extkeycode[2] = 0;
		switch(event->keyval)
		{
			case GDK_Home:
				extkeycode[2] = '7';
				break;
			case GDK_End:
				extkeycode[2] = '8';
				break;
			case GDK_Page_Up:
				extkeycode[2] = '5';
				break;
			case GDK_Page_Down:
				extkeycode[2] = '6';
				break;
			case GDK_Insert:
				extkeycode[2] = '2';
				break;
			case GDK_Delete:
				extkeycode[2] = '3';
				break;
			case GDK_Up:
				extkeycode[2] = 'A';
				break;
			case GDK_Down:
				extkeycode[2] = 'B';
				break;
			case GDK_Left:
				extkeycode[2] = 'D';
				break;
			case GDK_Right:
				extkeycode[2] = 'C';
				break; 
		}
		if (extkeycode[2] != 0)
		{
			extkeycode[0] = 27;
			extkeycode[1] = '[';
			extkeycode[3] = '~';
			if(event->keyval >= GDK_Left && event->keyval <= GDK_Down)
				write(gtkipcin[1], extkeycode, 3);
			else
				write(gtkipcin[1], extkeycode, 4);
			return TRUE;
		}
		write(gtkipcin[1], event->string, strlen(event->string));
        }
	return TRUE;
}

/* This is the main output function, code will need to be added here to make
   sure text goes to the correct window as well as ANSI handling etc, this 
   will probably be the most complex part of gtkBitchX */

void output_info(gpointer data, int pipefd)
{
	unsigned char buffer[201] = "";
	int t;
	Screen *os;

	gdk_threads_enter();
	t = read(pipefd, buffer, 200);
	buffer[t] = 0;

	os = findoutputscreen(pipefd);
	if (os && os->viewport)
		zvt_term_feed((ZvtTerm *)os->viewport, buffer, t);
	gdk_threads_leave();
}

/* This function sets the window size to the current columns and rows */
void gtk_sizewindow(Screen *thisscreen)
{
	zvt_term_set_size((ZvtTerm *)thisscreen->viewport, thisscreen->co, thisscreen->li);
	if(!get_int_var(MDI_VAR))
		size_allocate(thisscreen->viewport, (GtkWindow *)thisscreen->window);
}

void gtk_label_set_color(GtkWidget *label, int color)
{
	GtkStyle *style;
	GtkStyle *defstyle;

	style = gtk_style_new();
	/*
	 * This is kind of a hack.
	 */
	defstyle = gtk_widget_get_default_style();

	switch(color)
	{
	case COLOR_INACTIVE:
		gtk_widget_set_style(label, defstyle);
		break;
	case COLOR_HIGHLIGHT:
		style->fg[0] = colors[12];
		gtk_widget_set_style(label, style);
		break;
	case COLOR_ACTIVE:
		if ((label->style->fg[0].red == defstyle->fg[0].red) &&
			(label->style->fg[0].blue == defstyle->fg[0].blue) &&
			(label->style->fg[0].green == defstyle->fg[0].green))
		{
			style->fg[0] = colors[9];
			gtk_widget_set_style(label, style);
		}
		break;
	}
}

/* This may be going away, it replaces code which is currently elsewhere in BitchX */
void gtk_resize(Screen *this_screen)
{
	co = this_screen->co; li = this_screen->li;

	/* Recalculate some stuff that was done in input.c previously */
	this_screen->input_line = this_screen->li-1;

	this_screen->input_zone_len = this_screen->co - (WIDTH * 2);
	if (this_screen->input_zone_len < 10)
		this_screen->input_zone_len = 10;             /* Take that! */

	this_screen->input_start_zone = WIDTH;
	this_screen->input_end_zone = this_screen->co - WIDTH;
}                  

/* This gets called when the window size event occurs */
void gtk_windowsize(GtkWidget *widget, gpointer data)
{
	Screen *sizescreen;

	if(get_int_var(MDI_VAR))
		sizescreen = screen_list;
	else
		sizescreen = findscreen(widget);

	while(sizescreen)
	{

        /* In MDI mode we check that viewport exists... otherwise the tab is being removed */
		if (sizescreen->alive && sizescreen->maxfontwidth > 0 && sizescreen->maxfontwidth > 0 && sizescreen->viewport)
		{
			/* Not sure why it is x3 and x2.... klass shows as 2x2 and the variation is 6x4 */
			sizescreen->co = (int) ((sizescreen->viewport->allocation.width-(GTK_WIDGET(sizescreen->viewport)->style->klass->xthickness*3)) / sizescreen->maxfontwidth);
                        sizescreen->li = (int) ((sizescreen->viewport->allocation.height-(GTK_WIDGET(sizescreen->viewport)->style->klass->ythickness*2)) / sizescreen->maxfontheight);
			gtk_sizewindow(sizescreen);
			/* Set the nicklist width */
			if(sizescreen->nicklist)
			{
				gtk_widget_set_usize(sizescreen->clist, sizescreen->nicklist, 0);
				size_allocate(sizescreen->viewport, (GtkWindow *)sizescreen->window);
			}
			 /*size_allocate((GtkWidget *)sizescreen->viewport, (GtkWindow *)sizescreen->window);*/
			gtk_resize(sizescreen);
			recalculate_windows(sizescreen);

			/* Send the resize message to the main thread */
			sendevent(EVREFRESH, sizescreen->current_window->refnum);
		}
		if(get_int_var(MDI_VAR))
			sizescreen = sizescreen->next;
		else
			sizescreen = NULL;
	}
}

/* This gets called when a window gets the focus */
void gtk_windowfocus(GtkWidget *widget, gpointer *data)
{
	Screen *focusscreen;

	if ((focusscreen = findscreen(widget))!=NULL && focusscreen->current_window)
		sendevent(EVFOCUS, focusscreen->current_window->refnum);
}

/* This gets called when a notebook page gets the focus */
void gtk_notebookpage(GtkWidget *widget, GtkNotebookPage *page, gint page_num, gpointer *data)
{
	Screen *focusscreen = screen_list;
	GtkWidget *child = page->child;

	while(focusscreen)
	{
		if(focusscreen->alive && focusscreen->window == child)
		{
			focusscreen->page = page;
			if (focusscreen->current_window)
			{
				gtk_label_set_color(focusscreen->page->tab_label, COLOR_INACTIVE);
				sendevent(EVFOCUS, focusscreen->current_window->refnum);
			}
			return;
		}
		focusscreen = focusscreen->next;
	}
}

/* This gets called when the scroller bar gets moved on a window */
void gtk_scrollerchanged(GtkWidget *widget, Screen *thisscreen)
{
	if (justscrolled && thisscreen->current_window && thisscreen->current_window->refnum == lastscrollerwindow)
	{
		justscrolled = 0;
		return;
	}
	newscrollerpos = thisscreen->adjust->value - 1;
	if (newscrollerpos < 0)
		newscrollerpos=0;
	if (newscrollerpos > get_int_var(SCROLLBACK_VAR))
		newscrollerpos=get_int_var(SCROLLBACK_VAR);
	sendevent(EVSTRACK, thisscreen->current_window->refnum);
}

/* This handles right click events on the viewport */
int gtk_contextmenu(GtkWidget *widget, GdkEventButton *event, Screen *this_screen)
{
	int statusstart, doubleclick = FALSE;
	char *zvt_buffer;

	if(event->type == GDK_2BUTTON_PRESS)
		doubleclick = TRUE;

	/* This will only give decent results with fixed width fonts at the moment */

	if (this_screen->maxfontheight > 0 && this_screen->maxfontwidth > 0)
	{
		lastclickrow = (int)(event->y/this_screen->maxfontheight);
		lastclickcol = (int)(event->x/this_screen->maxfontwidth);
	}
	else
		lastclickrow = lastclickcol = 1;

	last_input_screen=output_screen=this_screen;
	make_window_current(this_screen->current_window);
	statusstart=this_screen->li - 2;

	zvt_buffer = zvt_term_get_buffer((ZvtTerm *)this_screen->viewport, NULL, VT_SELTYPE_LINE, 0, lastclickrow+1, 0, lastclickrow);
	if (zvt_buffer && *zvt_buffer)
	{
		int z;
		malloc_strcpy(&lastclicklinedata, zvt_buffer);
		for (z = 0;z < this_screen->co && lastclicklinedata[z]; z++)
			if(lastclicklinedata[z] == '\r' || lastclicklinedata[z] == '\n')
				lastclicklinedata[z] = 0;
		free(zvt_buffer);
	}
	else
		new_free(&lastclicklinedata);

	if(this_screen->window_list_end->status_lines == 0 && this_screen->window_list_end->double_status == 1 && this_screen->window_list_end->status_split == 1)
		statusstart=statusstart-1;
	if(this_screen->window_list_end->status_lines == 1 && this_screen->window_list_end->double_status == 0 && this_screen->window_list_end->status_split == 0)
		statusstart=statusstart-1;
	if(this_screen->window_list_end->status_lines == 1 && this_screen->window_list_end->double_status == 1 && this_screen->window_list_end->status_split == 1)
		statusstart=statusstart-1;
	if(this_screen->window_list_end->status_lines == 1 && this_screen->window_list_end->double_status == 1 && this_screen->window_list_end->status_split == 0)
		statusstart=statusstart-2;
	if(lastclickrow <= (current_window->screen->li - 2) && lastclickrow >= statusstart)
	{
		switch(event->button)
		{
		case 3:
			if(doubleclick)
				sendevent(EVKEY, STATUSRDBLCLICK);
			else
				sendevent(EVKEY, STATUSRCLICK);
			break;
		case 2:
			if(doubleclick)
				sendevent(EVKEY, STATUSMDBLCLICK);
			else
				sendevent(EVKEY, STATUSMCLICK);
			break;
		case 1:
			if(doubleclick)
				sendevent(EVKEY, STATUSLDBLCLICK);
			else
				sendevent(EVKEY, STATUSLCLICK);
			break;
		}
	}
	else
	{
		switch(event->button)
		{
		case 3:
			if(doubleclick)
				sendevent(EVKEY, RDBLCLICK);
			else
				sendevent(EVKEY, RCLICK);
			break;
		case 2:
			if(doubleclick)
				sendevent(EVKEY, MDBLCLICK);
			else
				sendevent(EVKEY, MCLICK);
			break;
		case 1:
			if(doubleclick)
				sendevent(EVKEY, LDBLCLICK);
			else
				sendevent(EVKEY, LCLICK);
			break;
		}
	}

	if (event->button == 2) {
		int len;
		char *tmpptr;

		if(clipboarddata)
			free(clipboarddata);
		tmpptr = vt_get_selection(((ZvtTerm *)(this_screen->viewport))->vx,
#ifdef HAVE_NEW_ZVT
								  this_screen->co*this_screen->li,
#endif
								  &len);

		clipboarddata = malloc(len+1);
		strncpy(clipboarddata, tmpptr, len);
		clipboarddata[len] = 0;
	}
	gtk_widget_grab_focus(widget);
	return FALSE;
}

/* These are support routines for gui_file_dialog() */

/* Ripped from functions.c because I couldn't get it to link from there */
char * fencode (unsigned char * input)
{
	char	*result;
	int	i = 0;

	result = (char *)new_malloc(strlen((char *)input) * 2 + 1);
	while (*input)
	{
		result[i++] = (*input >> 4) + 0x41;
		result[i++] = (*input & 0x0f) + 0x41;
		input++;
	}
	result[i] = '\0';

	return result;		/* DONT USE RETURN_STR HERE! */
}

typedef struct _gtkparam {
	GtkWidget	*window;
	Screen		*screen;
	gpointer	data[10];
} GtkParam;
	
void gtk_file_ok(GtkWidget *widget, GtkParam *param)
{
	char *tmp;

	codeptr = (char *)param->data[0];
	paramptr = m_strdup("OK ");
	tmp = fencode(gtk_file_selection_get_filename(GTK_FILE_SELECTION(param->window)));
	gtk_widget_destroy(GTK_WIDGET(param->window));
	malloc_strcat(&paramptr, tmp);
	new_free(&tmp);
	sendevent(EVFILE, param->screen->current_window->refnum);
	free(param);
}

void gtk_file_cancel(GtkWidget *widget, GtkParam *param)
{
	gtk_widget_destroy(GTK_WIDGET(param->window));
	codeptr = (char *)param->data[0];
	paramptr = m_strdup("CANCEL ");
	sendevent(EVFILE, param->screen->current_window->refnum);
	free(param);
}

void delete(GtkWidget *widget, GtkWidget *event, GtkParam *param)
{
	gtk_widget_destroy(GTK_WIDGET(param->window));
	free(param);
}

void delete2(GtkWidget *widget, GtkParam *param)
{
	gtk_widget_destroy(GTK_WIDGET(param->window));
	free(param);
}

void zvt_load_font(char *fontname, Screen *screen)
{
	GdkFont *font = NULL;

	font = gdk_font_load(fontname);
	if(!font)
	{
		bitchsay("Unable to load font \"%s\".", fontname);
		/* Fall back to fixed */
		font = gdk_font_load("fixed");
		fontname = "fixed";
		if(!font)
		{
			printf("Fatal %s error! Could not load \"fixed\" font!\r\n", VERSION);
			exit(100);
		}
	}

	gtk_widget_hide(screen->viewport);
	zvt_term_set_fonts((ZvtTerm *)screen->viewport, font, font);
	screen->maxfontwidth = ((ZvtTerm *)screen->viewport)->charwidth;
	screen->maxfontheight = ((ZvtTerm *)screen->viewport)->charheight;
	screen->font = font;

	if(screen->fontname)
		new_free(&screen->fontname);
	screen->fontname = m_strdup(fontname);
	gtk_sizewindow(screen);
	gtk_widget_show(screen->viewport);
	sendevent(EVREFRESH, screen->current_window->refnum);
}

void gtk_windowicon(GtkWidget *window)
{
#ifndef USE_IMLIB
	GtkStyle *iconstyle;
#endif
	GdkBitmap *bitmap;
	GdkPixmap *icon_pixmap = NULL;

#ifndef USE_IMLIB
	iconstyle = gtk_widget_get_style(window);
	if (!icon_pixmap)
		icon_pixmap = gdk_pixmap_create_from_xpm_d(window->window, &bitmap, &iconstyle->bg[GTK_STATE_NORMAL], BitchX);
#else
	gdk_imlib_data_to_pixmap(BitchX, &icon_pixmap, &bitmap);
#endif

	gdk_window_set_icon(window->window, NULL, icon_pixmap, bitmap);
}

void font_ok_handler(GtkWidget *widget, GtkParam *param)
{
	gchar *font;

	font = gtk_entry_get_text((GtkEntry *)param->data[0]);
	if(!font)
	{
		bitchsay("Unable to load font.");
		free(param);
		return;
	}
	/* All Windows is checked */
	if(GTK_TOGGLE_BUTTON(param->data[2])->active)
	{
		Screen *tmp;
		for (tmp = screen_list; tmp; tmp = tmp->next)
		{
			if(tmp->alive && tmp->viewport)
				zvt_load_font(font, tmp);
		}
	}
	else
		zvt_load_font(font, param->screen);

	/* Default font is checked */
	if(GTK_TOGGLE_BUTTON(param->data[1])->active)
		set_string_var(DEFAULT_FONT_VAR, font);
	gtk_widget_destroy(GTK_WIDGET(param->window));
	free(param);
}

void font_ok_handler2(GtkWidget *widget, GtkParam *param)
{
	gchar *font;

	font = gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(param->window));
	if(font)
		gtk_entry_set_text((GtkEntry *)param->data[0], font);
	gtk_widget_destroy(GTK_WIDGET(param->window));
	free(param);
}

typedef struct _propstruct {
	GtkWidget *nbwindow,
	*clist,
	*entryfield,
	*check;
	int which, last;
	Window *nb_window;
} GtkProp;

GtkWidget *notebook;

/* So we can free it later */
GtkProp *PropPages[4] = { NULL, NULL, NULL, NULL };

void updatevars(GtkProp *gtkprop)
{
	int i;
	char *szBuffer;
	ChannelList *chan;

	if(gtkprop->last > -1)
	{
		switch(gtkprop->which)
		{
		case 1:
			switch(return_irc_var(gtkprop->last)->type)
			{
			case BOOL_TYPE_VAR:
				i = GTK_TOGGLE_BUTTON(gtkprop->check)->active;
				set_int_var(gtkprop->last, i);
				break;
			case CHAR_TYPE_VAR:
				szBuffer = gtk_entry_get_text((GtkEntry *)gtkprop->entryfield);
				set_string_var(gtkprop->last, szBuffer);
				break;
			case STR_TYPE_VAR:
				szBuffer = gtk_entry_get_text((GtkEntry *)gtkprop->entryfield);
				set_string_var(gtkprop->last, szBuffer);
				break;
			case INT_TYPE_VAR:
				szBuffer = gtk_entry_get_text((GtkEntry *)gtkprop->entryfield);
				set_int_var(gtkprop->last, my_atol(szBuffer));
				break;
			}
			break;
		case 2:
			if(gtkprop->nb_window && gtkprop->nb_window->current_channel)
				chan = (ChannelList *) find_in_list((List **)get_server_channels(from_server), gtkprop->nb_window->current_channel, 0);
			else
				chan = NULL;

			if(chan)
			{
				switch(return_cset_var(gtkprop->last)->type)
				{
				case BOOL_TYPE_VAR:
					i = GTK_TOGGLE_BUTTON(gtkprop->check)->active;
					set_cset_int_var(chan->csets, gtkprop->last, i);
					break;
				case CHAR_TYPE_VAR:
					szBuffer = gtk_entry_get_text((GtkEntry *)gtkprop->entryfield);
					set_cset_str_var(chan->csets, gtkprop->last, szBuffer);
					break;
				case STR_TYPE_VAR:
					szBuffer = gtk_entry_get_text((GtkEntry *)gtkprop->entryfield);
					set_cset_str_var(chan->csets, gtkprop->last, szBuffer);
					break;
				case INT_TYPE_VAR:
					szBuffer = gtk_entry_get_text((GtkEntry *)gtkprop->entryfield);
					set_cset_int_var(chan->csets, gtkprop->last, my_atol(szBuffer));
					break;
				}
			}
			break;
		case 3:
			szBuffer = gtk_entry_get_text((GtkEntry *)gtkprop->entryfield);
			fset_string_var(gtkprop->last, szBuffer);
			break;
		case 4:
			szBuffer = gtk_entry_get_text((GtkEntry *)gtkprop->entryfield);
			set_wset_string_var(gtkprop->nb_window->wset, gtkprop->last, szBuffer);
			break;
		}
	}
}

void nb_delete(GtkWidget *widget, GtkWidget *event, GtkProp *gtkprop)
{
	int z;

	for(z=0;z<4;z++)
		updatevars(PropPages[z]);
	gtk_widget_destroy(GTK_WIDGET(gtkprop->nbwindow));
	/* Free the memory used by the pages */
	for(z=0;z<4;z++)
	{
		if(PropPages[z])
			free(PropPages[z]);
		PropPages[z] = NULL;
	}

	in_properties = 0;
}

int gtk_click_nicklist(GtkWidget *widget, GdkEventButton *event, GtkWidget *window)
{
	gchar *nicktext = NULL;
	int row, col, doubleclick = FALSE;

	if(event->type == GDK_2BUTTON_PRESS)
		doubleclick = TRUE;

	gtk_clist_get_selection_info((GtkCList *)widget,
								 event->x,
								 event->y,
								 &row,
								 &col);
	gtk_clist_get_text((GtkCList *)widget, row, col, &nicktext);

	malloc_strcpy(&lastclicklinedata, nicktext);
	lastclickcol = lastclickrow = 0;
	switch(event->button)
	{
	case 3:
		if(doubleclick)
			sendevent(EVKEY, NICKLISTRDBLCLICK);
		else
			sendevent(EVKEY, NICKLISTRCLICK);
		break;
	case 2:
		if(doubleclick)
			sendevent(EVKEY, NICKLISTMDBLCLICK);
		else
			sendevent(EVKEY, NICKLISTMCLICK);
		break;
	case 1:
		if(doubleclick)
			sendevent(EVKEY, NICKLISTLDBLCLICK);
		else
			sendevent(EVKEY, NICKLISTLCLICK);
		break;
	}
	return FALSE;
}

void select_row(GtkWidget *widget,  gint row, gint column, GdkEventButton *event, GtkProp *gtkprop)
{
	char szBuffer[100], *tmptext;
	ChannelList *chan;

	updatevars(gtkprop);
	switch(gtkprop->which)
	{
	case 1:
		switch(return_irc_var(row)->type)
		{
		case BOOL_TYPE_VAR:
			gtk_widget_set_sensitive(gtkprop->entryfield, FALSE);
			gtk_widget_set_sensitive(gtkprop->check, TRUE);

			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtkprop->check), get_int_var(row));
			break;
		case CHAR_TYPE_VAR:
			gtk_widget_set_sensitive(gtkprop->entryfield, TRUE);
			gtk_widget_set_sensitive(gtkprop->check, FALSE);

			tmptext = get_string_var(row);
			if(tmptext)
				gtk_entry_set_text((GtkEntry *)gtkprop->entryfield, tmptext);
			else
				gtk_entry_set_text((GtkEntry *)gtkprop->entryfield, "");
			break;
		case STR_TYPE_VAR:
			gtk_widget_set_sensitive(gtkprop->entryfield, TRUE);
			gtk_widget_set_sensitive(gtkprop->check, FALSE);

			tmptext = get_string_var(row);
			if(tmptext)
				gtk_entry_set_text((GtkEntry *)gtkprop->entryfield, tmptext);
			else
				gtk_entry_set_text((GtkEntry *)gtkprop->entryfield, "");
			break;
		case INT_TYPE_VAR:
			gtk_widget_set_sensitive(gtkprop->entryfield, TRUE);
			gtk_widget_set_sensitive(gtkprop->check, FALSE);

			sprintf(szBuffer, "%d", get_int_var(row));
			gtk_entry_set_text((GtkEntry *)gtkprop->entryfield, szBuffer);
			break;
		}
		break;
	case 2:
		if(gtkprop->nb_window && gtkprop->nb_window->current_channel)
			chan = (ChannelList *) find_in_list((List **)get_server_channels(from_server), gtkprop->nb_window->current_channel, 0);
		else
			chan = NULL;

		if(chan)
		{
			switch(return_cset_var(row)->type)
			{
			case BOOL_TYPE_VAR:
				gtk_widget_set_sensitive(gtkprop->entryfield, FALSE);
				gtk_widget_set_sensitive(gtkprop->check, TRUE);

				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtkprop->check), get_cset_int_var(chan->csets, row));
				break;
			case CHAR_TYPE_VAR:
				gtk_widget_set_sensitive(gtkprop->entryfield, TRUE);
				gtk_widget_set_sensitive(gtkprop->check, FALSE);

				tmptext = get_cset_str_var(chan->csets, row);
				if(tmptext)
					gtk_entry_set_text((GtkEntry *)gtkprop->entryfield, tmptext);
				else
					gtk_entry_set_text((GtkEntry *)gtkprop->entryfield, "");
				break;
			case STR_TYPE_VAR:
				gtk_widget_set_sensitive(gtkprop->entryfield, TRUE);
				gtk_widget_set_sensitive(gtkprop->check, FALSE);

				tmptext = get_cset_str_var(chan->csets, row);
				if(tmptext)
					gtk_entry_set_text((GtkEntry *)gtkprop->entryfield, tmptext);
				else
					gtk_entry_set_text((GtkEntry *)gtkprop->entryfield, "");
				break;
			case INT_TYPE_VAR:
				gtk_widget_set_sensitive(gtkprop->entryfield, TRUE);
				gtk_widget_set_sensitive(gtkprop->check, FALSE);

				sprintf(szBuffer, "%d", get_cset_int_var(chan->csets, row));
				gtk_entry_set_text((GtkEntry *)gtkprop->entryfield, szBuffer);
				break;
			}
			break;
		}
		break;
	case 3:
		gtk_widget_set_sensitive(gtkprop->entryfield, TRUE);
		gtk_widget_set_sensitive(gtkprop->check, FALSE);
		tmptext = fget_string_var(row);
		if(tmptext)
			gtk_entry_set_text((GtkEntry *)gtkprop->entryfield, tmptext);
		else
			gtk_entry_set_text((GtkEntry *)gtkprop->entryfield, "");
		break;
	case 4:
		gtk_widget_set_sensitive(gtkprop->entryfield, TRUE);
		gtk_widget_set_sensitive(gtkprop->check, FALSE);
		tmptext = get_wset_string_var(gtkprop->nb_window->wset, row);
		if(tmptext)
			gtk_entry_set_text((GtkEntry *)gtkprop->entryfield, tmptext);
		else
			gtk_entry_set_text((GtkEntry *)gtkprop->entryfield, "");
		break;
	}

	gtkprop->last = row;
}

void font_browse(GtkWidget *widget, GtkParam *firstparam)
{
	GtkWidget 	*fontdialog;
	GtkParam 	*param;

	fontdialog = gtk_font_selection_dialog_new("Font Browser");

	param = malloc(sizeof(GtkParam));

	param->window = fontdialog;
	param->screen = (Screen *)gtkcommand.data[0];
	param->data[0] = firstparam->data[0];

	gtk_signal_connect(GTK_OBJECT(GTK_FONT_SELECTION_DIALOG(fontdialog)->ok_button), "clicked", GTK_SIGNAL_FUNC(font_ok_handler2), param);
	gtk_signal_connect(GTK_OBJECT(GTK_FONT_SELECTION_DIALOG(fontdialog)->cancel_button), "clicked", GTK_SIGNAL_FUNC(delete2), param);
	gtk_widget_show(fontdialog);
}

void gtk_setfocus(Screen *thisscreen)
{
	if(get_int_var(MDI_VAR))
	{
		Screen *tmp = screen_list;
		GtkWidget *page;
		int z = 0;

		while(tmp)
		{

			if(tmp->alive)
			{
				page = gtk_notebook_get_nth_page((GtkNotebook *)notebook,z);
				if(page == thisscreen->window)
					gtk_notebook_set_page((GtkNotebook *)notebook, z);
				z++;
			}
			tmp = tmp->next;
		}
	}
	else
	{
		gdk_window_raise(GTK_WIDGET(thisscreen->window)->window);
		/*gtk_window_activate_focus(GTK_WINDOW(thisscreen->window));*/
	}
}

void gtk_command_handler(void)
{
	char buf[2];

	gdk_threads_enter();

	/* Clear the command */
	read(gtkcommandpipe[0], buf, 1);
        
	switch(gtkcommand.command)
	{
		/* Paste command */
	case GTKPASTE:
		{
			static GdkAtom targets_atom = GDK_NONE;

			if (targets_atom == GDK_NONE)
				targets_atom = gdk_atom_intern ("STRING", FALSE);

			/* Save the parameters passed */
			if(pasteargs)
				free(pasteargs);
			pasteargs = (char *)gtkcommand.data[0];

			gtk_selection_convert (current_window->screen->window, GDK_SELECTION_PRIMARY, targets_atom,
					       GDK_CURRENT_TIME);
		}
		break;
		/* File Dialog */
	case GTKFILEDIALOG:
		{
			GtkWidget *filew;
			GtkParam  *param;
			int t;

			filew = gtk_file_selection_new((char *)gtkcommand.data[2]);

			param = malloc(sizeof(GtkParam));

			param->window = filew;
			param->screen = (Screen *)gtkcommand.data[7];
			param->data[0] = gtkcommand.data[6];

			gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filew)->ok_button), "clicked", (GtkSignalFunc) gtk_file_ok, param);
			gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filew)->cancel_button), "clicked", (GtkSignalFunc) gtk_file_cancel, param);

			gtk_file_selection_set_filename(GTK_FILE_SELECTION(filew), (char *)gtkcommand.data[1]);

			gtk_widget_show(filew);
			for(t=0;t<6;t++)
				new_free(&gtkcommand.data[t]);
		}
		break;
		/* Creates a menubar on a window */
	case GTKMENUBAR:
		{
			if(!my_stricmp((char *)gtkcommand.data[1], "-delete"))
			{
				if(((Screen *)gtkcommand.data[0])->menubar)
				{
					gtk_widget_destroy(((Screen *)gtkcommand.data[0])->menubar);
					((Screen *)gtkcommand.data[0])->menubar = NULL;
				}
			}
			else
				newmenubar((Screen *)gtkcommand.data[0], (MenuStruct *)gtkcommand.data[2]);
			new_free(&gtkcommand.data[1]);
		}
		break;
		/* Exit procedure for gtk thread */
	case GTKEXIT:
		gtk_exit(0);
		break;
		/* Focuses the window passed to it */
	case GTKSETFOCUS:
		gtk_setfocus((Screen *)gtkcommand.data[0]);
		break;
		/* Creates a popup context menu */
	case GTKMENUPOPUP:
		{
		MenuStruct *menupopup;

		if((menupopup=findmenu((char *)gtkcommand.data[0]))!=NULL)
			gtk_menu_popup(GTK_MENU(newsubmenu(menupopup)), NULL, NULL, NULL, NULL, 0, 0);
		new_free(&gtkcommand.data[0]);
		}
		break;
		/* Updates the nicklist window */
	case GTKNICKLIST:
		{
			char *channel = (char *)gtkcommand.data[0];

			if(channel)
			{
				ChannelList *cptr = lookup_channel(channel, from_server, 0);
				Window *this_window;

				if(cptr)
				{
					this_window = get_window_by_refnum(cptr->refnum);
					if(this_window && this_window->screen)
						drawnicklist(this_window->screen);
				}

			}
			else
			{
				Screen *tmp = screen_list;

				while(tmp)
				{
					if(tmp->alive)
						drawnicklist(tmp);
					tmp = tmp->next;
				}
			}
		}
		break;
		/* Creates an nice looking about box for the /about command */
	case GTKABOUTBOX:
		gtk_about_box((char *)gtkcommand.data[0]);
		break;
		/* Creates a new IRC window */
	case GTKNEWWIN:
		gtk_new_window((Screen *)gtkcommand.data[0], (Window *)gtkcommand.data[1]);
		gdk_threads_leave();
		return;
		break;
		/* Sets the title bar of the window */
	case GTKTITLE:
		if(((Screen *)gtkcommand.data[0])->window)
		{
			if(get_int_var(MDI_VAR))
			{
				Screen *tmpscreen = (Screen *)gtkcommand.data[0];
				char *labeltext = "<none>";

				if(tmpscreen->current_window)
				{
					if(tmpscreen->current_window->current_channel)
						labeltext = tmpscreen->current_window->current_channel;
					else if(tmpscreen->current_window->query_nick)
						labeltext = tmpscreen->current_window->query_nick;
				}

				if(tmpscreen->page)
					gtk_label_set_text(GTK_LABEL(tmpscreen->page->tab_label),labeltext);
				if(current_window && current_window->screen == (Screen *)gtkcommand.data[0])
					gtk_window_set_title(GTK_WINDOW(MDIWindow), (char *)gtkcommand.data[1]);
			}
			else
				gtk_window_set_title(GTK_WINDOW(((Screen *)gtkcommand.data[0])->window), (char *)gtkcommand.data[1]);
		}
		new_free(&gtkcommand.data[1]);
		break;
		/* Opens a properties notebook for the current window */
	case GTKPROP:
		{
			GtkWidget 	*nbwindow,
			*frame,
			*label,
			*tmphbox,
			*tmpvbox,
			*scrollwindow;
			GtkProp *gtkprop;
			char titletext[150], *entryname[1];
			int t = 0;

			in_properties = 1;

			nbwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);

			PropPages[0] = gtkprop = malloc(sizeof(GtkProp));

			gtkprop->nbwindow = nbwindow;
			
			gtk_signal_connect(GTK_OBJECT(nbwindow), "delete_event", GTK_SIGNAL_FUNC (nb_delete), gtkprop);

			gtk_container_border_width(GTK_CONTAINER(nbwindow), 10);

			notebook = gtk_notebook_new();
			gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
			gtk_container_add(GTK_CONTAINER(nbwindow), notebook);
			gtk_widget_show(notebook);

			/* Sets */
			gtkprop = malloc(sizeof(GtkProp));

			gtkprop->nbwindow = nbwindow;
			frame = gtk_frame_new("Page 1 of 5");
			gtk_container_border_width(GTK_CONTAINER(frame), 10);
			gtk_widget_set_usize(frame, 500, 250);

			tmphbox = gtk_hbox_new(TRUE, 10);
			gtk_container_add(GTK_CONTAINER(frame), tmphbox);
			gtk_container_border_width(GTK_CONTAINER(tmphbox), 10);

			scrollwindow = gtk_scrolled_window_new (NULL, NULL);
			gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwindow),
							GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
			gtk_box_pack_start(GTK_BOX(tmphbox), scrollwindow, TRUE, TRUE, 0);
			gtk_widget_show(scrollwindow);

			gtkprop->clist = gtk_clist_new(1);
			gtk_clist_set_column_auto_resize(GTK_CLIST(gtkprop->clist), 0, TRUE);
			gtk_container_add(GTK_CONTAINER(scrollwindow), gtkprop->clist);
			gtk_widget_show(gtkprop->clist);
                        
			for(t=0; t<NUMBER_OF_VARIABLES; t++)
			{
				entryname[0] = return_irc_var(t)->name;
				gtk_clist_append((GtkCList *)gtkprop->clist, entryname);
			}

			tmpvbox = gtk_vbox_new(FALSE, 10);
			gtk_box_pack_start(GTK_BOX(tmphbox), tmpvbox, TRUE, TRUE, 0);

			gtkprop->entryfield = gtk_entry_new();
			gtk_box_pack_start(GTK_BOX(tmpvbox), gtkprop->entryfield, TRUE, TRUE, 0);
			gtk_widget_show(gtkprop->entryfield);

			gtkprop->check = gtk_check_button_new_with_label("On/Off");
			gtk_box_pack_start(GTK_BOX(tmpvbox), gtkprop->check, TRUE, FALSE, 0);

			gtk_widget_set_sensitive(gtkprop->entryfield, FALSE);
			gtk_widget_set_sensitive(gtkprop->check, FALSE);
                        
			gtkprop->nb_window = current_window;
			gtkprop->which = 1;
			gtkprop->last = -1;
			gtk_signal_connect(GTK_OBJECT(gtkprop->clist), "select_row", GTK_SIGNAL_FUNC(select_row), gtkprop);

			gtk_widget_show(gtkprop->check);
			gtk_widget_show(tmphbox);
                        gtk_widget_show(tmpvbox);
			gtk_widget_show(frame);

			label = gtk_label_new("Sets");
			gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, label);

			/* Csets */
			PropPages[1] = gtkprop = malloc(sizeof(GtkProp));

			gtkprop->nbwindow = nbwindow;
			frame = gtk_frame_new("Page 2 of 5");
			gtk_container_border_width(GTK_CONTAINER(frame), 10);
			gtk_widget_set_usize(frame, 500, 250);

			tmphbox = gtk_hbox_new(TRUE, 10);
			gtk_container_add(GTK_CONTAINER(frame), tmphbox);
			gtk_container_border_width(GTK_CONTAINER(tmphbox), 10);

			scrollwindow = gtk_scrolled_window_new (NULL, NULL);
			gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwindow),
                                                        GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
			gtk_box_pack_start(GTK_BOX(tmphbox), scrollwindow, TRUE, TRUE, 0);
			gtk_widget_show(scrollwindow);

			gtkprop->clist = gtk_clist_new(1);
			gtk_clist_set_column_auto_resize(GTK_CLIST(gtkprop->clist), 0, TRUE);
			gtk_container_add(GTK_CONTAINER(scrollwindow), gtkprop->clist);
			gtk_widget_show(gtkprop->clist);

			for(t=0; t<NUMBER_OF_CSETS; t++)
			{
				entryname[0] = return_cset_var(t)->name;
				gtk_clist_append((GtkCList *)gtkprop->clist, entryname);
			}

			tmpvbox = gtk_vbox_new(FALSE, 10);
			gtk_box_pack_start(GTK_BOX(tmphbox), tmpvbox, TRUE, TRUE, 0);

			gtkprop->entryfield = gtk_entry_new();
			gtk_box_pack_start(GTK_BOX(tmpvbox), gtkprop->entryfield, TRUE, TRUE, 0);
			gtk_widget_show(gtkprop->entryfield);

			gtkprop->check = gtk_check_button_new_with_label("On/Off");
			gtk_box_pack_start(GTK_BOX(tmpvbox), gtkprop->check, TRUE, FALSE, 0);

			gtk_widget_set_sensitive(gtkprop->entryfield, FALSE);
			gtk_widget_set_sensitive(gtkprop->check, FALSE);

			gtkprop->nb_window = current_window;
			gtkprop->which = 2;
			gtkprop->last = -1;
			gtk_signal_connect(GTK_OBJECT(gtkprop->clist), "select_row", GTK_SIGNAL_FUNC(select_row), gtkprop);
                        
			gtk_widget_show(gtkprop->check);
			gtk_widget_show(tmphbox);
			gtk_widget_show(tmpvbox);
			gtk_widget_show(frame);

			label = gtk_label_new("Csets");
			gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, label);

			/* Fsets */
			PropPages[2] = gtkprop = malloc(sizeof(GtkProp));

			gtkprop->nbwindow = nbwindow;
			frame = gtk_frame_new("Page 3 of 5");
			gtk_container_border_width(GTK_CONTAINER(frame), 10);
			gtk_widget_set_usize(frame, 500, 250);

			tmphbox = gtk_hbox_new(TRUE, 10);
			gtk_container_add(GTK_CONTAINER(frame), tmphbox);
			gtk_container_border_width(GTK_CONTAINER(tmphbox), 10);

			scrollwindow = gtk_scrolled_window_new (NULL, NULL);
			gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwindow),
							GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
			gtk_box_pack_start(GTK_BOX(tmphbox), scrollwindow, TRUE, TRUE, 0);
			gtk_widget_show(scrollwindow);

			gtkprop->clist = gtk_clist_new(1);
			gtk_clist_set_column_auto_resize(GTK_CLIST(gtkprop->clist), 0, TRUE);
			gtk_container_add(GTK_CONTAINER(scrollwindow), gtkprop->clist);
			gtk_widget_show(gtkprop->clist);

			for(t=0; t<NUMBER_OF_FSET; t++)
			{
				entryname[0] = return_fset_var(t)->name;
				gtk_clist_append((GtkCList *)gtkprop->clist, entryname);
			}
                        
			tmpvbox = gtk_vbox_new(FALSE, 10);
			gtk_box_pack_start(GTK_BOX(tmphbox), tmpvbox, TRUE, TRUE, 0);

			gtkprop->entryfield = gtk_entry_new();
			gtk_box_pack_start(GTK_BOX(tmpvbox), gtkprop->entryfield, TRUE, TRUE, 0);
			gtk_widget_show(gtkprop->entryfield);

			gtkprop->check = gtk_check_button_new_with_label("On/Off");
			gtk_box_pack_start(GTK_BOX(tmpvbox), gtkprop->check, TRUE, FALSE, 0);

			gtk_widget_set_sensitive(gtkprop->entryfield, FALSE);
			gtk_widget_set_sensitive(gtkprop->check, FALSE);

			gtkprop->nb_window = current_window;
			gtkprop->which = 3;
			gtkprop->last = -1;
			gtk_signal_connect(GTK_OBJECT(gtkprop->clist), "select_row", GTK_SIGNAL_FUNC(select_row), gtkprop);

			gtk_widget_show(gtkprop->check);
			gtk_widget_show(tmphbox);
			gtk_widget_show(tmpvbox);
			gtk_widget_show(frame);

			label = gtk_label_new("Fsets");
			gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, label);

			/* Wsets */
			PropPages[3] = gtkprop = malloc(sizeof(GtkProp));

			gtkprop->nbwindow = nbwindow;
			frame = gtk_frame_new("Page 4 of 5");
			gtk_container_border_width(GTK_CONTAINER(frame), 10);
			gtk_widget_set_usize(frame, 500, 250);

			tmphbox = gtk_hbox_new(TRUE, 10);
			gtk_container_add(GTK_CONTAINER(frame), tmphbox);
			gtk_container_border_width(GTK_CONTAINER(tmphbox), 10);

			scrollwindow = gtk_scrolled_window_new (NULL, NULL);
			gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwindow),
							GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
			gtk_box_pack_start(GTK_BOX(tmphbox), scrollwindow, TRUE, TRUE, 0);
			gtk_widget_show(scrollwindow);

			gtkprop->clist = gtk_clist_new(1);
			gtk_clist_set_column_auto_resize(GTK_CLIST(gtkprop->clist), 0, TRUE);
			gtk_container_add(GTK_CONTAINER(scrollwindow), gtkprop->clist);
			gtk_widget_show(gtkprop->clist);

			for(t=0; t<NUMBER_OF_WSETS; t++)
			{
				entryname[0] = return_wset_var(t)->name;
				gtk_clist_append((GtkCList *)gtkprop->clist, entryname);
			}

			tmpvbox = gtk_vbox_new(FALSE, 10);
			gtk_box_pack_start(GTK_BOX(tmphbox), tmpvbox, TRUE, TRUE, 0);

			gtkprop->entryfield = gtk_entry_new();
			gtk_box_pack_start(GTK_BOX(tmpvbox), gtkprop->entryfield, TRUE, TRUE, 0);
			gtk_widget_show(gtkprop->entryfield);

			gtkprop->check = gtk_check_button_new_with_label("On/Off");
			gtk_box_pack_start(GTK_BOX(tmpvbox), gtkprop->check, TRUE, FALSE, 0);

			gtk_widget_set_sensitive(gtkprop->entryfield, FALSE);
			gtk_widget_set_sensitive(gtkprop->check, FALSE);

			gtkprop->nb_window = current_window;
			gtkprop->which = 4;
			gtkprop->last = -1;
			gtk_signal_connect(GTK_OBJECT(gtkprop->clist), "select_row", GTK_SIGNAL_FUNC(select_row), gtkprop);

			gtk_widget_show(gtkprop->check);
			gtk_widget_show(tmphbox);
			gtk_widget_show(tmpvbox);
			gtk_widget_show(frame);

			label = gtk_label_new("Wsets");
			gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, label);

			/* GTK Settings */
			frame = gtk_frame_new("Page 5 of 5");
			gtk_container_border_width(GTK_CONTAINER(frame), 10);
			gtk_widget_show(frame);

			label = gtk_label_new("GTK Settings");
			gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, label);

			strcpy(titletext, VERSION);
			strcat(titletext, " Properties");

			if(current_window->current_channel)
			{
				strcat(titletext, " for ");
				strcat(titletext, current_window->current_channel);
			}

			gtk_window_set_title(GTK_WINDOW(nbwindow), titletext);
			gtk_widget_show(nbwindow);
		}
		break;
		/* The Font Dialog */
	case GTKFONT:
		{
			GtkWidget *fontwindow,
			*fontentry,
			*browsebutton,
			*defaultcheck,
			*allwindowscheck,
			*okbutton,
			*cancelbutton,
			*hbox,
			*vbox;
			GtkParam  *param = malloc(sizeof(GtkParam));
			Screen *fontscreen = (Screen *)gtkcommand.data[0];

			param->window = fontwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
			param->screen = fontscreen;

			gtk_window_set_title(GTK_WINDOW(fontwindow), "Font Selection");
			gtk_container_border_width(GTK_CONTAINER(fontwindow), 10);

			vbox = gtk_vbox_new(TRUE, 10);
			gtk_container_add(GTK_CONTAINER(fontwindow), vbox);
			gtk_container_border_width(GTK_CONTAINER(vbox), 10);

			fontentry = gtk_entry_new();
			gtk_box_pack_start(GTK_BOX(vbox), fontentry, TRUE, TRUE, 0);
			if(fontscreen && fontscreen->fontname)
				gtk_entry_set_text((GtkEntry *)fontentry, fontscreen->fontname);

			browsebutton = gtk_button_new_with_label("Browse");
			gtk_box_pack_start(GTK_BOX(vbox), browsebutton, TRUE, TRUE, 0);

			defaultcheck = gtk_check_button_new_with_label("Default Font");
			allwindowscheck = gtk_check_button_new_with_label("Change for All Windows");

			gtk_box_pack_start(GTK_BOX(vbox), defaultcheck, TRUE, TRUE, 0);
			gtk_box_pack_start(GTK_BOX(vbox), allwindowscheck, TRUE, TRUE, 0);

			hbox = gtk_hbox_new(TRUE, 10);
			gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

			okbutton = gtk_button_new_with_label("Ok");
			cancelbutton = gtk_button_new_with_label("Cancel");

			gtk_box_pack_start(GTK_BOX(hbox), okbutton, TRUE, TRUE, 0);
			gtk_box_pack_start(GTK_BOX(hbox), cancelbutton, TRUE, TRUE, 0);

			param->data[0] = fontentry;
			param->data[1] = defaultcheck;
			param->data[2] = allwindowscheck;

			gtk_signal_connect(GTK_OBJECT(okbutton), "clicked", GTK_SIGNAL_FUNC(font_ok_handler), param);
			gtk_signal_connect(GTK_OBJECT(cancelbutton), "clicked", GTK_SIGNAL_FUNC(delete2), param);
			gtk_signal_connect(GTK_OBJECT(browsebutton), "clicked", GTK_SIGNAL_FUNC(font_browse), param);

			gtk_widget_show(hbox);
			gtk_widget_show(okbutton);
			gtk_widget_show(cancelbutton);
			gtk_widget_show(allwindowscheck);
			gtk_widget_show(defaultcheck);
			gtk_widget_show(browsebutton);
			gtk_widget_show(fontentry);
			gtk_widget_show(vbox);
			gtk_widget_show(fontwindow);
		}
		break;
	case GTKREDRAWMENUS:
		{
			Screen *tmpscreen;
			/* Recreate the menus after a change */
			for(tmpscreen = screen_list; tmpscreen; tmpscreen = tmpscreen->next)
			{
				MenuStruct *tmpms;
				if(tmpscreen->alive && tmpscreen->menu && (tmpms = findmenu(tmpscreen->menu)))
					newmenubar(tmpscreen, tmpms);
			}
		}
		break;
	case GTKSETWINDOWPOS:
		if(gtkcommand.data[1])
			gdk_window_raise(((GtkWidget *)gtkcommand.data[0])->window);
		if(gtkcommand.data[2])
			gdk_window_lower(((GtkWidget *)gtkcommand.data[0])->window);
		if(gtkcommand.data[3])
			gdk_window_resize(((GtkWidget *)gtkcommand.data[0])->window, (int)gtkcommand.data[7], (int)gtkcommand.data[8]);
		if(gtkcommand.data[4])
			gdk_window_move(((GtkWidget *)gtkcommand.data[0])->window, (int)gtkcommand.data[9], (int)gtkcommand.data[10]);
		if(gtkcommand.data[5])
			gdk_window_hide(((GtkWidget *)gtkcommand.data[0])->window);
		if(gtkcommand.data[6])
			gdk_window_show(((GtkWidget *)gtkcommand.data[0])->window);
		break;
	case GTKSETFONT:
		{
			Screen *this_screen = (Screen *)gtkcommand.data[1];
			zvt_load_font((char *)gtkcommand.data[0], (Screen *)gtkcommand.data[1]);
			if((this_screen->nicklist = get_int_var(NICKLIST_VAR))==0)
				gtk_widget_hide(this_screen->scrolledwindow);
			else
			{
				gtk_widget_show(this_screen->scrolledwindow);
				gtk_widget_set_usize(this_screen->scrolledwindow, this_screen->nicklist, 0);
				size_allocate(this_screen->viewport, (GtkWindow *)this_screen->window);
			}
			free(gtkcommand.data[0]);
		}
		break;
	case GTKWINKILL:
		{
			Screen *thisscreen = (Screen *)gtkcommand.data[0];

			gtk_signal_disconnect_by_data (GTK_OBJECT(thisscreen->window), thisscreen);

			gdk_input_remove(thisscreen->gtkio);
			new_close(thisscreen->pipe[0]);
			new_close(thisscreen->pipe[1]);

			thisscreen->page = NULL;

			if(get_int_var(MDI_VAR))
			{
				Screen *tmp = screen_list;
				GtkWidget *page;
				int z = 0;

				/* Make sure gtk_windowsize() doesn't try to use this screen */
				thisscreen->viewport = (GtkWidget *)NULL;
				while(tmp)
				{

					if(tmp->alive)
					{
						page = gtk_notebook_get_nth_page((GtkNotebook *)notebook,z);
						if(page == thisscreen->window)
							gtk_notebook_remove_page((GtkNotebook *)notebook, z);
						z++;
					}
					tmp = tmp->next;
				}
			}
			if(thisscreen->window)
				gtk_widget_destroy(thisscreen->window);
			thisscreen->window = 0;
		}
		break;
	case GTKMDI:
		if(gtkcommand.data[0])
		{
			Screen *tmp = screen_list;

			current_mdi_mode = 1;

			MDIWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);

			gtk_window_set_title(GTK_WINDOW(MDIWindow), VERSION);
			gtk_window_set_wmclass(GTK_WINDOW(MDIWindow), VERSION, VERSION);
			gtk_window_set_policy(GTK_WINDOW (MDIWindow), TRUE, TRUE, TRUE);
			gtk_widget_realize(MDIWindow);

			/*gtk_signal_connect(GTK_OBJECT(nbwindow), "delete_event", GTK_SIGNAL_FUNC (nb_delete), gtkprop);*/

			gtk_container_border_width(GTK_CONTAINER(MDIWindow), 0);

			notebook = gtk_notebook_new();
			gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
			gtk_container_add(GTK_CONTAINER(MDIWindow), notebook);
			gtk_widget_show(notebook);

			while(tmp)
			{
				if(tmp->alive)
				{
					GtkWidget *tmpwidget = tmp->window, *label;

					gtk_signal_disconnect_by_data (GTK_OBJECT(tmp->window), tmp);
					gtk_signal_disconnect_by_data (GTK_OBJECT(tmp->clist), tmp);
					gtk_signal_disconnect_by_data (GTK_OBJECT(tmp->viewport), tmp);
					gtk_signal_disconnect_by_data (GTK_OBJECT(tmp->adjust), tmp);
					tmp->window = gtk_hbox_new(0,0);

					gtk_widget_reparent(tmp->box, tmp->window);
					gtk_widget_destroy(tmpwidget);

					if(!tmp->current_window)
						label = gtk_label_new("<none>");
					else if(tmp->current_window->current_channel)
						label = gtk_label_new(tmp->current_window->current_channel);
					else if(tmp->current_window->query_nick)
						label = gtk_label_new(tmp->current_window->query_nick);
					else
						label = gtk_label_new("<none>");

					gtk_notebook_append_page (GTK_NOTEBOOK(notebook), tmp->window, label);
					zvt_term_set_color_scheme((ZvtTerm *)tmp->viewport, bx_red, bx_green, bx_blue);
					gtk_widget_show_all (tmp->window);

					/* Re-attach handlers to the events */
					gtk_signal_connect (GTK_OBJECT(tmp->window), "selection_get",
										GTK_SIGNAL_FUNC (selection_handle), tmp);
					gtk_signal_connect (GTK_OBJECT(tmp->window), "selection_received",
										GTK_SIGNAL_FUNC (gtk_paste), tmp);
					gtk_signal_connect(GTK_OBJECT(tmp->window), "delete_event", GTK_SIGNAL_FUNC(window_destroy), tmp);
					gtk_signal_connect(GTK_OBJECT(tmp->viewport), "button_press_event", GTK_SIGNAL_FUNC(gtk_contextmenu), (gpointer)tmp);
					gtk_signal_connect(GTK_OBJECT(tmp->adjust), "value_changed", GTK_SIGNAL_FUNC(gtk_scrollerchanged), tmp);
					gtk_signal_connect(GTK_OBJECT(tmp->clist), "button_press_event", GTK_SIGNAL_FUNC(gtk_click_nicklist), (gpointer)tmp);
					gtk_signal_connect(GTK_OBJECT(tmp->viewport), "key_press_event", GTK_SIGNAL_FUNC(gtk_keypress), tmp);
				}

				tmp = tmp->next;
			}

			gtk_signal_connect(GTK_OBJECT(notebook), "switch-page", GTK_SIGNAL_FUNC(gtk_notebookpage), MDIWindow);
			gtk_signal_connect(GTK_OBJECT(MDIWindow), "size_allocate", GTK_SIGNAL_FUNC(gtk_windowsize), MDIWindow);
			gtk_widget_show(MDIWindow);

			gtk_windowicon(MDIWindow);
		}
		else
		{
			Screen *tmp = screen_list;

			current_mdi_mode = 0;

			while(tmp)
			{
				if(tmp->alive)
				{
					GtkWidget *tmpwidget = tmp->window;

					gtk_signal_disconnect_by_data (GTK_OBJECT(tmp->window), tmp);
					gtk_signal_disconnect_by_data (GTK_OBJECT(tmp->clist), tmp);
					gtk_signal_disconnect_by_data (GTK_OBJECT(tmp->viewport), tmp);
					gtk_signal_disconnect_by_data (GTK_OBJECT(tmp->adjust), tmp);
					tmp->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
					gtk_window_set_title(GTK_WINDOW(tmp->window), VERSION);
					gtk_window_set_wmclass(GTK_WINDOW(tmp->window), VERSION, VERSION);
					gtk_window_set_policy(GTK_WINDOW (tmp->window), TRUE, TRUE, TRUE);
					gtk_widget_realize(tmp->window);

					gtk_widget_reparent(tmp->box, tmp->window);
					gtk_widget_destroy(tmpwidget);

					zvt_term_set_color_scheme((ZvtTerm *)tmp->viewport, bx_red, bx_green, bx_blue);
					gtk_widget_show_all(tmp->window);

					tmp->page = NULL;

					/* Re-attach handlers to the events */
					gtk_signal_connect(GTK_OBJECT(tmp->window), "delete_event", GTK_SIGNAL_FUNC(window_destroy), tmp);
					gtk_signal_connect(GTK_OBJECT(tmp->viewport), "key_press_event", GTK_SIGNAL_FUNC(gtk_keypress), tmp);
					gtk_signal_connect(GTK_OBJECT(tmp->window), "focus_in_event", GTK_SIGNAL_FUNC(gtk_windowfocus), tmp);
					gtk_signal_connect (GTK_OBJECT(tmp->window), "selection_get",
										GTK_SIGNAL_FUNC (selection_handle), tmp);
					gtk_signal_connect (GTK_OBJECT(tmp->window), "selection_received",
										GTK_SIGNAL_FUNC (gtk_paste), tmp);
					gtk_signal_connect(GTK_OBJECT(tmp->window), "size_allocate", GTK_SIGNAL_FUNC(gtk_windowsize), tmp);
					gtk_signal_connect(GTK_OBJECT(tmp->viewport), "button_press_event", GTK_SIGNAL_FUNC(gtk_contextmenu), (gpointer)tmp);
					gtk_signal_connect(GTK_OBJECT(tmp->adjust), "value_changed", GTK_SIGNAL_FUNC(gtk_scrollerchanged), tmp);
					gtk_signal_connect(GTK_OBJECT(tmp->clist), "button_press_event", GTK_SIGNAL_FUNC(gtk_click_nicklist), (gpointer)tmp);

					gtk_windowicon(tmp->window);
					gtk_sizewindow(tmp);

				}

				tmp = tmp->next;
			}

			gtk_widget_destroy(MDIWindow);
			sendevent(EVTITLE,0);
		}
		break;
	case GTKSCROLL:
		justscrolled = 1;
		gtk_adjustment_set_value(((Screen *)gtkcommand.data[0])->adjust, *((int *)gtkcommand.data[1]));
		free(gtkcommand.data[1]);
		break;
	case GTKACTIVITY:
		{
			int color = (int)gtkcommand.data[0];
			Window *this_window = (Window *)gtkcommand.data[1];

			if (this_window->screen->page && notebook_page_by_refnum(this_window->refnum) != gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)))
				gtk_label_set_color(this_window->screen->page->tab_label, color);
		}
		break;
	case GTKMSGBOX:
		{
		GtkWidget 	*dialog,
		*button,
		*label;
		GtkParam 	*param;

		dialog = gtk_dialog_new();

		param = malloc(sizeof(GtkParam));

		param->window = dialog;

		gtk_window_set_title(GTK_WINDOW(dialog), VERSION);

		button = gtk_button_new_with_label("Ok");
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
		gtk_widget_show(button);

		gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(delete2), param);
		gtk_signal_connect(GTK_OBJECT(dialog), "delete_event", GTK_SIGNAL_FUNC(delete), param);

		label = gtk_label_new((char *)gtkcommand.data[0]);
		gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), 20);
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, TRUE, TRUE, 0);
		gtk_widget_show(label);

		gtk_widget_show(dialog);

		new_free(&gtkcommand.data[0]);
		}
		break;
	case GTKNICKLISTWIDTH:
		{
			Screen *this_screen = (Screen *)gtkcommand.data[0];
			int width = (int)gtkcommand.data[1];
			int old = this_screen->nicklist;

			this_screen->nicklist = width;

			if(old && !width)
				gtk_widget_hide(this_screen->scrolledwindow);
			else if(!old && width)
			{
				gtk_widget_show(this_screen->scrolledwindow);
				gtk_widget_set_usize(this_screen->scrolledwindow, width, 0);
				size_allocate(this_screen->viewport, (GtkWindow *)this_screen->window);
			}
			else
				gtk_sizewindow(this_screen);
		}
		break;
	}
	gtkcommand.command = GTKNONE;
	gtkincommand = FALSE;
	pthread_cond_signal(&gtkcond);
	gdk_threads_leave();
}


/* Supplies the last selected text as the selection. */
void
selection_handle (GtkWidget *widget,
		  GtkSelectionData *selection_data,
		  guint info, guint seltime)
{
	if(clipboarddata)
		gtk_selection_data_set (selection_data, GDK_SELECTION_TYPE_STRING,
					8, clipboarddata, strlen(clipboarddata));
}

/* Used to tell the main thread to continue after the window has
   been created. */
void viewport_map(GtkWidget *widget, gpointer data)
{
	pthread_mutex_lock(&evmutex);
	predicate = 1;
	pthread_cond_signal(&evcond);
	pthread_mutex_unlock(&evmutex);
}

/* Add menubar after the screen has been mapped */
void window_map(GtkWidget *widget, gpointer data)
{
	Screen *mapscreen;

	if ((mapscreen = findscreen(widget))!=NULL && mapscreen->window)
	{
		zvt_term_set_size((ZvtTerm *)mapscreen->viewport, mapscreen->co, mapscreen->li);
		if(mapscreen->nicklist)
		{
			gtk_widget_set_usize(mapscreen->scrolledwindow, mapscreen->nicklist, 0);
			size_allocate(mapscreen->viewport, (GtkWindow *)mapscreen->window);
		}
	}
}

void gtk_sendcommand(int wait)
{
	if(gtkincommand && wait)
		gtk_wait_command();

	gtkincommand = TRUE;
	write(gtkcommandpipe[1], "A", 1);

	if(wait)
		gtk_wait_command();
}

/* Initialize all needed structures, create the main window,
   start the gtk_thread and gtk handler */

void gtkbx_init(void)
{
	int z;
	GtkStyle *style;
	GdkColormap *cmap;
	GtkWidget *tmpbox;
	char *bg = NULL;

	close(0);
	close(1);
	close(2);

	/* glib 1.2.10 seems to poll on fd 0,
	 * so we can't leave it in a closed state.
	 */
	z = open("/dev/null", O_RDONLY);
	if(z>0)
	{
		dup2(z, 0);
		close(z);
	}

	/* Load the default font */
	mainfont = gdk_font_load("vga");

	/* Absolute fallback fixed */
	if(!mainfont)
	{
		mainfont = gdk_font_load("fixed");
		if(!mainfont)
		{
		    printf("Fatal %s error! Could not load \"fixed\" font!\r\n", VERSION);
		    exit(100);
		}
		mainfontname = m_strdup("fixed");
	}
	else
		mainfontname = m_strdup("vga");

	/* Create the main window */
	mainwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(mainwindow), VERSION);
	gtk_window_set_wmclass(GTK_WINDOW(mainwindow), VERSION, VERSION);
	gtk_window_set_policy(GTK_WINDOW (mainwindow), TRUE, TRUE, TRUE);
	gtk_widget_realize(mainwindow);

	/* Attach handlers to the events */
	gtk_signal_connect(GTK_OBJECT(mainwindow), "delete_event", GTK_SIGNAL_FUNC(window_destroy), mainwindow);
	gtk_signal_connect(GTK_OBJECT(mainwindow), "size_allocate", GTK_SIGNAL_FUNC(gtk_windowsize), mainwindow);
	gtk_signal_connect(GTK_OBJECT(mainwindow), "focus_in_event", GTK_SIGNAL_FUNC(gtk_windowfocus), mainwindow);

	gtk_container_set_border_width(GTK_CONTAINER(mainwindow), 0);

	tmpbox = gtk_hbox_new(FALSE, 0); mainbox = gtk_vbox_new(FALSE, 0);

	mainviewport = (GtkWidget *)zvt_term_new();
	zvt_term_set_fonts((ZvtTerm *)mainviewport, mainfont, mainfont);
	zvt_term_set_scrollback((ZvtTerm *)mainviewport, 0);
	zvt_term_set_blink((ZvtTerm *)mainviewport, TRUE);
	zvt_term_set_bell((ZvtTerm *)mainviewport, TRUE);

	gtk_signal_connect(GTK_OBJECT(mainviewport), "key_press_event", GTK_SIGNAL_FUNC(gtk_keypress), mainwindow);

	/* Add BitchX colors to the system colormap */
	cmap = gdk_colormap_get_system();
	for(z=0;z<16;z++)
		gdk_color_alloc(cmap, &colors[z]);

	gtk_signal_connect(GTK_OBJECT(mainviewport), "map", GTK_SIGNAL_FUNC(viewport_map), mainwindow);

	/* Create the vertical scroll bar */
	mainadjust = (GtkAdjustment *)gtk_adjustment_new(get_int_var(SCROLLBACK_VAR), 0, get_int_var(SCROLLBACK_VAR)+22, 1, 22, 22);
	mainscroller = gtk_vscrollbar_new(mainadjust);
	GTK_WIDGET_UNSET_FLAGS(mainscroller, GTK_CAN_FOCUS);

	/* Create the nicklist */
	mainscrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (mainscrolledwindow),
									GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	mainclist = gtk_clist_new(1);
	gtk_clist_set_column_auto_resize(GTK_CLIST(mainclist), 0, TRUE);
	/* Set the color to black */
	style = gtk_style_copy(gtk_widget_get_style(mainclist));
	style->base[0] = style->base[1] = style->bg[0] = style->bg[1] = colors[0];
	gtk_widget_set_style(mainclist, style);
	gtk_container_add(GTK_CONTAINER(mainscrolledwindow), mainclist);

	gtk_signal_connect(GTK_OBJECT(mainclist), "button_press_event", GTK_SIGNAL_FUNC(gtk_click_nicklist), (gpointer)mainwindow);

	/* Add to the layout */
	gtk_container_add(GTK_CONTAINER(mainwindow), mainbox);

	gtk_box_pack_start(GTK_BOX(tmpbox), GTK_WIDGET(mainviewport), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(tmpbox), mainscroller, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(tmpbox), mainscrolledwindow, FALSE, TRUE, 0);

	gtk_box_pack_end(GTK_BOX(mainbox), tmpbox, TRUE, TRUE, 0);

	/* Add to the gtk IO event list */
	gtkio = gdk_input_add(mainpipe[0], GDK_INPUT_READ, (GdkInputFunction)output_info, (gpointer)NULL);
	gdk_input_add(gtkcommandpipe[0], GDK_INPUT_READ, (GdkInputFunction)gtk_command_handler, (gpointer)NULL);

	gtk_windowicon(mainwindow);

	/* Show the widgets */
	gtk_widget_show(mainscrolledwindow);
	gtk_widget_show(mainclist);
	gtk_widget_show(mainviewport);
	gtk_widget_show(mainscroller);
	gtk_widget_show(mainbox);
	gtk_widget_show(tmpbox);

	gtk_signal_connect (GTK_OBJECT(mainwindow), "selection_get",
						GTK_SIGNAL_FUNC (selection_handle), mainwindow);

	gtk_signal_connect (GTK_OBJECT(mainwindow), "selection_received",
						GTK_SIGNAL_FUNC (gtk_paste), mainwindow);

	gtk_selection_add_target (mainwindow, GDK_SELECTION_PRIMARY,
							  GDK_SELECTION_TYPE_STRING, 0);

	zvt_term_set_color_scheme((ZvtTerm *)mainviewport, bx_red, bx_green, bx_blue);

	/* Use a background XPM if it exists */
	if ((bg = path_search(BitchXbg, get_string_var(LOAD_PATH_VAR))))
		zvt_term_set_background((ZvtTerm *)mainviewport, bg, FALSE, FALSE);

	zvt_term_set_size((ZvtTerm *)mainviewport, 80, 25);

#ifdef SOUND
	gtk_hostname = m_strdup(getenv("DISPLAY"));
	if(gtk_hostname)
		esd_connection = esd_open_sound((char *)gtk_hostname);
#endif

	gtk_widget_show(mainwindow);

	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	pthread_exit(NULL);
}

void gui_init(void)
{
	current_term->TI_cols = 80;
	current_term->TI_lines = 25;
	li = current_term->TI_lines;
	co = current_term->TI_cols;

	pthread_mutex_init(&evmutex, NULL);
	pthread_cond_init(&evcond, NULL);
	pthread_cond_init(&gtkcond, NULL);

	/* Open the IPC pipes */
	pipe(gtkipcin);
	pipe(gtkcommandpipe);
	pipe(mainpipe);
	new_open(gtkipcin[0]);

	predicate = 0;

	if(pthread_create(&gtkthread, NULL, (void *)&gtkbx_init, NULL))
	{
		printf("%s panic! Could not create gtk thread!\n", VERSION);
		exit(500);
	}

	/* Wait for the window to be realized */
	pthread_mutex_lock(&evmutex);
	while(!predicate)
	{
		pthread_cond_wait(&evcond, &evmutex);
	}
	pthread_mutex_unlock(&evmutex);

}

/* Handler for menuitem events */
void menuitemhandler(gpointer *data)
{
	menucmd = (int) *data;

	sendevent(EVMENU, last_input_screen->current_window->refnum);
}

void removetilde(char *dest, char *src)
{
	int z, cur=0;

	for(z=0;z<strlen(src);z++)
	{
		if(src[z] != '~')
			{
				dest[cur] = src[z];
				cur++;
			}
	}
	dest[cur] = 0;
}

/* Create a submenu, used in menubar and popup menu creation */
GtkWidget *newsubmenu(MenuStruct *menutoadd)
{
	GtkWidget 	*tmphandle,
	*tmpmenu;
	MenuList *tmp;
	char expbuf[500];

	tmp = menutoadd->menuorigin;

	/* When you readd shared menu support remove the #ifndef GTK from commands2.c */

	tmpmenu = gtk_menu_new();

	while(tmp!=NULL)
	{
		if(tmp->menutype & GUISEPARATOR)
			tmphandle=gtk_menu_item_new();
		else
		{
			int checkstate = (tmp->menutype & GUICHECKEDMENUITEM);

			/* Using a slightly different approach to shared
			 menus in GTK */
			if(tmp->refnum > 0)
			{
				/* If we already have an entry defined, use it's info */
				MenuRef *tmpref = find_menuref(menutoadd->root, tmp->refnum);
				if(tmpref)
				{
					if(tmpref->checked)
						checkstate = TRUE;
					else
						checkstate = FALSE;

					if(tmpref->menutext)
						strcpy(expbuf, tmpref->menutext);
				}
				else
				{
					/* We don't have an entry so we create a new one */
					removetilde(expbuf, tmp->name);
					if(tmp->menutype & GUICHECKEDMENUITEM)
						new_menuref(&menutoadd->root, tmp->refnum, expbuf, 1);
					else
						new_menuref(&menutoadd->root, tmp->refnum, expbuf, 0);
				}
			}
			else
				removetilde(expbuf, tmp->name);

			if(tmp->menutype & GUICHECKMENUITEM)
			{
				tmphandle=gtk_check_menu_item_new_with_label(expbuf);

				if(tmp->refnum > 0)
				{
					MenuRef *tmpref = find_menuref(menutoadd->root, tmp->refnum);
					if(tmpref)
						tmpref->menuhandle = tmphandle;
				}
				gtk_check_menu_item_set_show_toggle(GTK_CHECK_MENU_ITEM(tmphandle), TRUE);

				if(checkstate)
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(tmphandle), checkstate);
			}
			else
				tmphandle=gtk_menu_item_new_with_label(expbuf);
		}
		if(tmp->menutype & GUISUBMENU)
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(tmphandle), newsubmenu((MenuStruct *)findmenu(tmp->submenu)));
		if(tmp->menutype & GUIMENUITEM)
			gtk_signal_connect_object(GTK_OBJECT(tmphandle), "activate", GTK_SIGNAL_FUNC(menuitemhandler), (gpointer) &tmp->menuid);
		/*if((tmp->menutype & GUIBRKSUBMENU) || (tmp->menutype & GUIBRKMENUITEM))
		 gtk_menu_item_right_justify(GTK_MENU_ITEM(tmphandle));*/
		gtk_menu_append(GTK_MENU(tmpmenu), tmphandle);
		gtk_widget_show(tmphandle);
		tmp=tmp->next;
	}

	gtk_widget_show(tmpmenu);
	return tmpmenu;
}

/* Creates a menubar on a given window/screen */
void newmenubar(Screen *menuscreen, MenuStruct *menutoadd)
{
	GtkWidget *tmphandle;
	MenuList *tmp;
	char expbuf[500];

	if(!menutoadd || !menutoadd->menuorigin)
	{
		say("Cannot create blank menu.");
		return;
	}

	tmp = menutoadd->menuorigin;
	if(menuscreen->menubar)
	{
		gtk_widget_destroy(menuscreen->menubar);
		menuscreen->menubar = NULL;
	}

	if(menuscreen->menu)
		new_free(&menuscreen->menu);
	menuscreen->menu = m_strdup(menutoadd->name);

	menuscreen->menubar = gtk_menu_bar_new();
	while(tmp!=NULL)
	{
		if(tmp->menutype & GUISEPARATOR)
			tmphandle = gtk_menu_item_new();
		else
		{
			removetilde(expbuf, tmp->name);
			tmphandle=gtk_menu_item_new_with_label(expbuf);
		}
		if(tmp->menutype & GUISUBMENU)
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(tmphandle), newsubmenu((MenuStruct *)findmenu(tmp->submenu)));
		if(tmp->menutype & GUIMENUITEM)
			gtk_signal_connect_object(GTK_OBJECT(tmphandle), "activate", GTK_SIGNAL_FUNC(menuitemhandler), (gpointer) &tmp->menuid);
		gtk_menu_bar_append(GTK_MENU_BAR(menuscreen->menubar), tmphandle);
		gtk_widget_show(tmphandle);
		tmp=tmp->next;
	}
	gtk_box_pack_end(GTK_BOX(menuscreen->box), menuscreen->menubar, FALSE, TRUE, 0);
	gtk_widget_show(menuscreen->menubar);
	gtk_sizewindow(menuscreen);
}

/* This section is for portability considerations */
void gui_clreol(void)
{
	gtkprintf(current_term->TI_el);
}

void gui_gotoxy(int col, int row)
{
	col++;row++;
	gtkprintf("\e[%d;%dH", row, col);
}

void gui_clrscr(void)
{
	Screen *tmp;

	for(tmp = screen_list; tmp; tmp = tmp->next)
	{
		if(tmp->alive && tmp->pipe[1])
			write(tmp->pipe[1], "\e[2J", 4);
	}
}

void gui_left(int num)
{
	gtkprintf("\e[D");
}

void gui_right(int num)
{
	gtkprintf("\e[C");
}

void gui_scroll(int top, int bot, int n)
{
	/* Stolen from term.c */
	int i,oneshot=0,rn,sr,er;
	char thing[128], final[128], start[128];

	/* Some basic sanity checks */
	if (n == 0 || top == bot || bot < top)
		return;

	sr = er = 0;
	final[0] = start[0] = thing[0] = 0;

	if (n < 0)
		rn = -n;
	else
		rn = n;

	if (current_term->TI_csr && (current_term->TI_ri || current_term->TI_rin) && (current_term->TI_ind || current_term->TI_indn))
	{
		/*
		 * Previously there was a test to see if the entire scrolling
		 * region was the full screen.  That test *always* fails,
		 * because we never scroll the bottom line of the screen.
		 */
		strcpy(start, (char *)tparm(current_term->TI_csr, top, bot));
		strcpy(final, (char *)tparm(current_term->TI_csr, 0, current_term->TI_lines-1));

		if (n > 0)
		{
			sr = bot;
			er = top;
			strcpy(thing, current_term->TI_ind);
		}
		else
		{
			sr = top;
			er = bot;
			strcpy (thing, current_term->TI_ri);
		}
	}

	if (!thing[0])
		return;

	/* Do the actual work here */
	if (start[0])
		gtkprintf(start);
	
	gui_gotoxy (0, sr);

	if (oneshot)
		gtkprintf(thing);
	else
	{
		for (i = 0; i < rn; i++)
			gtkprintf(thing);
	}
	gui_gotoxy (0, er);
	if (final[0])
		gtkprintf(final);
                                                                                                            
}

void gui_flush(void)
{
}

void gui_puts(unsigned char *buffer)
{
	int i;

	for (i = 0; i < strlen(buffer); i++)
		gtkputc(buffer[i]);

	gtkputc('\n');
}

/* Create a new (non-main) window */
void gui_new_window(Screen *gtknew, Window *win)
{
	predicate = 0;
	gtkcommand.command = GTKNEWWIN;
	gtkcommand.data[0] = (gpointer) gtknew;
	gtkcommand.data[1] = (gpointer) win;
	gtk_sendcommand(TRUE);

	/* Wait for the window to be realized */
	pthread_mutex_lock(&evmutex);
	while(!predicate)
	{
		pthread_cond_wait(&evcond, &evmutex);
	}
	pthread_mutex_unlock(&evmutex);
	/* This is a hack which should be removed ASAP - Brian */
	write(gtkipcin[1], "\r", 1);
}

void gtk_new_window(Screen *gtknew, Window *win)
{
	char *defmenu, *deffont;
	GtkWidget *tmpbox, *label;
	GtkStyle *style;
	MenuStruct *tmpms;
	char *bg = NULL;

	gtknew->page = NULL;

	/* Create the main window */
	if(get_int_var(MDI_VAR))
	{
        gtknew->window = gtk_hbox_new(0,0);
		label = gtk_label_new("<none>");
		gtk_notebook_append_page (GTK_NOTEBOOK(notebook), gtknew->window, label);
	}
	else
	{
		gtknew->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

		gtk_window_set_title(GTK_WINDOW(gtknew->window), VERSION);
		gtk_window_set_wmclass(GTK_WINDOW(gtknew->window), VERSION, VERSION);
		gtk_window_set_policy(GTK_WINDOW (gtknew->window), TRUE, TRUE, TRUE);
		gtk_widget_realize(gtknew->window);
	}

	/* Attach handlers to the events */
	if(!get_int_var(MDI_VAR))
	{
		gtk_signal_connect(GTK_OBJECT(gtknew->window), "delete_event", GTK_SIGNAL_FUNC(window_destroy), gtknew);
		gtk_signal_connect(GTK_OBJECT(gtknew->window), "size_allocate", GTK_SIGNAL_FUNC(gtk_windowsize), gtknew);
		gtk_signal_connect(GTK_OBJECT(gtknew->window), "focus_in_event", GTK_SIGNAL_FUNC(gtk_windowfocus), gtknew);
	}

	gtk_container_set_border_width(GTK_CONTAINER(gtknew->window), 0);

	tmpbox = gtk_hbox_new(FALSE, 0);
	gtknew->box = gtk_vbox_new(FALSE, 0);

	style = gtk_style_new();

	deffont=get_string_var(DEFAULT_FONT_VAR);
	gtknew->viewport = (GtkWidget *)zvt_term_new();
	if(deffont && *deffont)
		zvt_load_font(deffont, gtknew);
	else
		zvt_load_font("fixed", gtknew);

	gtknew->co = co; gtknew->li = li;
	zvt_term_set_scrollback((ZvtTerm *)gtknew->viewport, 0);
	zvt_term_set_blink((ZvtTerm *)gtknew->viewport, TRUE);
	zvt_term_set_bell((ZvtTerm *)gtknew->viewport, TRUE);

	gtk_signal_connect(GTK_OBJECT(gtknew->viewport), "key_press_event", GTK_SIGNAL_FUNC(gtk_keypress), gtknew);

	/* Add a background XPM if it exists */
	if ((bg = path_search(BitchXbg, get_string_var(LOAD_PATH_VAR))))
		zvt_term_set_background((ZvtTerm *)gtknew->viewport, bg, FALSE, FALSE);

	gtk_signal_connect(GTK_OBJECT(gtknew->viewport), "map", GTK_SIGNAL_FUNC(viewport_map), NULL);

	/* Create the vertical scroll bar */
	gtknew->adjust = (GtkAdjustment *)gtk_adjustment_new(get_int_var(SCROLLBACK_VAR), 0, get_int_var(SCROLLBACK_VAR)+22, 1, 22, 22);
	gtknew->scroller = gtk_vscrollbar_new(gtknew->adjust);
	GTK_WIDGET_UNSET_FLAGS(gtknew->scroller, GTK_CAN_FOCUS);

	gtk_signal_connect(GTK_OBJECT(gtknew->adjust), "value_changed", GTK_SIGNAL_FUNC(gtk_scrollerchanged), (gpointer)gtknew);

	/* Create the nicklist */
	gtknew->scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (gtknew->scrolledwindow),
									GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtknew->clist = gtk_clist_new(1);
	gtk_clist_set_column_auto_resize(GTK_CLIST(gtknew->clist), 0, TRUE);
	/* Set the color to black */
	style = gtk_style_copy(gtk_widget_get_style(gtknew->clist));
	style->base[0] = style->base[1] = style->bg[0] = style->bg[1] = colors[0];
	gtk_widget_set_style(gtknew->clist, style);
	gtk_container_add(GTK_CONTAINER(gtknew->scrolledwindow), gtknew->clist);

	gtk_signal_connect(GTK_OBJECT(gtknew->viewport), "button_press_event", GTK_SIGNAL_FUNC(gtk_contextmenu), gtknew);
	gtk_signal_connect(GTK_OBJECT(gtknew->clist), "button_press_event", GTK_SIGNAL_FUNC(gtk_click_nicklist), gtknew);

	/* Add to the layout */
	gtk_container_add(GTK_CONTAINER(gtknew->window), gtknew->box);

	gtk_box_pack_start(GTK_BOX(tmpbox), gtknew->viewport, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(tmpbox), gtknew->scroller, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(tmpbox), gtknew->scrolledwindow, FALSE, TRUE, 0);

	gtk_box_pack_end(GTK_BOX(gtknew->box), tmpbox, TRUE, TRUE, 0);

	gtknew->nicklist = get_int_var(NICKLIST_VAR);

	gtk_windowicon(gtknew->window);

	if(gtknew->nicklist)
		gtk_widget_show(gtknew->scrolledwindow);
	gtk_widget_show(gtknew->clist);
	gtk_widget_show(GTK_WIDGET(gtknew->viewport));
	gtk_widget_show(gtknew->scroller);
	gtk_widget_show(gtknew->box);
	gtk_widget_show(tmpbox);
	gtk_widget_show(gtknew->window);

	gtk_signal_connect (GTK_OBJECT(gtknew->window), "selection_get",
                            GTK_SIGNAL_FUNC (selection_handle), gtknew);

	gtk_signal_connect (GTK_OBJECT(gtknew->window), "selection_received",
			    GTK_SIGNAL_FUNC (gtk_paste), gtknew);

	gtk_selection_add_target (gtknew->window, GDK_SELECTION_PRIMARY,
				  GDK_SELECTION_TYPE_STRING, 0);

	zvt_term_set_color_scheme((ZvtTerm *)gtknew->viewport, bx_red, bx_green, bx_blue);

	/* Open the screen's output pipe */
	pipe(gtknew->pipe);

	/* Add to the gtk IO event list */
	gtknew->gtkio = gdk_input_add(gtknew->pipe[0], GDK_INPUT_READ, (GdkInputFunction)output_info, (gpointer)NULL);

	output_screen = last_input_screen = gtknew;
	make_window_current(win);
	gtk_sizewindow(gtknew);
	zvt_term_set_size((ZvtTerm *)gtknew->viewport, gtknew->co, gtknew->li);
	if(gtknew->nicklist)
		gtk_widget_set_usize(gtknew->scrolledwindow, gtknew->nicklist, 0);

	gtk_widget_realize(gtknew->window);

	gtknew->menubar = (GtkWidget *)NULL;

	gtkcommand.command = GTKNONE;

	defmenu=get_string_var(DEFAULT_MENU_VAR);
	if(defmenu && *defmenu)
	{
		if((tmpms = (MenuStruct *)findmenu(defmenu))!=NULL)
		{
			gtkcommand.command = GTKMENUBAR;
			gtkcommand.data[0] = (gpointer) gtknew;
			gtkcommand.data[1] = (gpointer) m_strdup(defmenu);
			gtkcommand.data[2] = (gpointer) tmpms;
		}
	}

	gtkincommand = TRUE;
	write(gtkcommandpipe[1], "A", 1);

	/* Make the new tab visiable */
	if(get_int_var(MDI_VAR))
		gtk_setfocus(gtknew);
}

void gui_kill_window(Screen *killscreen)
{
	gtkcommand.command = GTKWINKILL;
	gtkcommand.data[0] = (gpointer) killscreen;
	gtk_sendcommand(TRUE);
}

int gui_read(Screen *screen, char *buffer, int maxbufsize)
{
	return read(gtkipcin[0], buffer, maxbufsize);
}

void gui_settitle(char *titletext, Screen *gtkwin)
{
	if(gtkwin->alive == 0)
		return;

	gtkcommand.command = GTKTITLE;
	gtkcommand.data[1] = (gpointer) m_strdup(titletext);
	gtkcommand.data[0] = (gpointer) gtkwin;
	gtk_sendcommand(TRUE);
}

void gui_msgbox(void)
{
	extern char *msgtext;

	gtkcommand.command = GTKMSGBOX;
	gtkcommand.data[0] = (gpointer) msgtext;
	gtk_sendcommand(TRUE);
}

void gui_popupmenu(char *menuname)
{
	gtkcommand.command = GTKMENUPOPUP;
	gtkcommand.data[0] = (gpointer) m_strdup(menuname);
	gtk_sendcommand(TRUE);
}

void gui_font_dialog(Screen *screen)
{
	gtkcommand.command = GTKFONT;
	gtkcommand.data[0] = (gpointer) screen;
	gtk_sendcommand(TRUE);
}


void gui_file_dialog(char *type, char *path, char *title, char *ok, char *apply, char *code, char *szButton)
{
	gtkcommand.command = GTKFILEDIALOG;
	gtkcommand.data[0] = (gpointer) m_strdup(type);
	gtkcommand.data[1] = (gpointer) m_strdup(path);
	gtkcommand.data[2] = (gpointer) m_strdup(title);
	gtkcommand.data[3] = (gpointer) m_strdup(ok);
	gtkcommand.data[4] = (gpointer) m_strdup(apply);
	gtkcommand.data[5] = (gpointer) m_strdup(szButton);
	gtkcommand.data[6] = (gpointer) m_strdup(code);
	gtkcommand.data[7] = (gpointer) current_window->screen;
	gtk_sendcommand(TRUE);
}

void gui_properties_notebook(void)
{
	if(in_properties)
		return;

	gtkcommand.command = GTKPROP;
	gtk_sendcommand(TRUE);
}

#define current_screen      last_input_screen
#define INPUT_BUFFER        current_screen->input_buffer
#define ADD_TO_INPUT(x)     strmcat(INPUT_BUFFER, (x), INPUT_BUFFER_SIZE);

void gtk_paste (GtkWidget *widget, GtkSelectionData *selection_data, gpointer data)
{
	char *sdata;

	/* If we didn't get anything abort */
	if (selection_data->length < 0)
		return;

	/* Make sure we got the data in the expected form */
	if (selection_data->type != GDK_SELECTION_TYPE_STRING)
        return;

	sdata = (char *)selection_data->data;;

	if(!sdata)
		return;

	selectdata = strdup(sdata);

	if(strlen(selectdata) < 1)
	{
		free(selectdata);
		return;
	}

	sendevent(EVPASTE, current_window ? current_window->refnum : 0);
	/* I don't think we need to free "data" */
}

void gtk_main_paste (int refnum)
{
	char   *oclip, *clip, *bit = NULL;
	int    line = 0, i = 0;
	char   *channel = NULL;
	int topic = 0;
	int smartpaste = 0;
	char smart[512]; /* SmartPaste buffer (256) */
	int smartsize = 80;
	int input_line = 0;
	char *args = pasteargs;
	Window *this_window = get_window_by_refnum(refnum);

	if(this_window)
	{
		make_window_current(this_window);
		from_server = this_window->server;
	}

	/* Parse arguments */
	if (!args || !*args)
		channel = get_current_channel_by_refnum(0);
	else
	{
		char *t;
		while (args && *args)
		{
			t = next_arg(args, &args);
			if (*t == '-')
			{
				if (!my_strnicmp(t, "-topic", strlen(t)))
				{
					topic = 1;
				}
				else if (!my_strnicmp(t, "-smart", strlen(t)))
				{
					/* Smartpaste */
					smartpaste = 1;
				}
				else if (!my_strnicmp(t, "-input", strlen(t)))
				{
					/* Smartpaste */
					input_line = 1;
				}
			}
			else
				channel = t;
		}
	}
	if (!channel)
		channel = get_current_channel_by_refnum(0);


	if(!channel && !(current_window || current_window->query_nick) && !topic && !input_line)
		return;

	oclip = (char *)selectdata; line = 0;

	if (!smartpaste)
	{
		/* Ordinary paste */
		clip = oclip; bit = strtok(clip, "\n\r");
		while (bit)
		{
			if (input_line)
			{
				if(bit && *bit)
				{
					ADD_TO_INPUT(bit);
					update_input(UPDATE_FROM_CURSOR);
				}
			} else
				if (!topic)
				{
					if(current_window->query_nick)
					{
						if(*current_window->query_nick && bit && *bit)
							if (do_hook(PASTE_LIST, "%s %s", current_window->query_nick, bit))
								send_text(current_window->query_nick, bit, NULL, 1, 0);
					}
					else
					{
						if(channel && *channel && bit && *bit)
							if (do_hook(PASTE_LIST, "%s %s", channel, bit))
								send_text(channel, bit, NULL, 1, 0);
					}
				} else
				{
					send_to_server("TOPIC %s :%s", channel, bit);
					break;
				}
			line++;
			bit = strtok(NULL, "\n\r");
			if (input_line && bit)
				send_line(0, NULL);

		}
	} else
	{
		/* Rosmo's Incredibly Dull SmartPaste:
		 Fits as much as possible into a 80-nick_length buffer, stripping
		 spaces from beginning and end and then pasting */

		if (!topic)
			smartsize = 512;
		else
			smartsize = 128; /* 'tis correct? */

		clip = oclip;
		while (*clip && *clip == ' ') clip++; /* Strip spaces from the start */

		*smart = '\0';
		while (*clip)
		{
			if (input_line && *smart)
				send_line(0, NULL);

			*smart = '\0';

			i = 0;
			while (*clip && i < smartsize)
			{
				if (*clip != '\n')
				{
					if (*clip != '\r') strncat(smart, clip, 1);
				}
				else
				{
					strcat(smart, " "); clip++; i++;
					while (*clip && *clip == ' ') clip++; /* Strip spaces */
					clip--;
				}
				clip++; i++;
			}

			if (strcmp(smart, "") != 0) /* If our smart buffer has stuff, send it! */
			{
				if (input_line)
				{
					if(*smart)
					{
						ADD_TO_INPUT(smart);
						update_input(UPDATE_FROM_CURSOR);
					}
				} else
					if (!topic)
					{
						if(current_window->query_nick)
						{
							if(*current_window->query_nick && *smart)
								if (do_hook(PASTE_LIST, "%s %s", current_window->query_nick, smart))
									send_text(current_window->query_nick, smart, NULL, 1, 0);
						}
						else
						{
							if(channel && *channel && *smart)
								if (do_hook(PASTE_LIST, "%s %s", channel, smart))
									send_text(channel, smart, NULL, 1, 0);
						}
					}
					else
					{
						send_to_server("TOPIC %s :%s", channel, smart);
						break;
					}
			}
		}

		free(oclip); /* Free */
	}
}

void gui_paste(char *args)
{
	gtkcommand.command = GTKPASTE;
	gtkcommand.data[0] = (gpointer) strdup(args);
	gtk_sendcommand(TRUE);
}

void gui_setfocus(Screen *screen)
{
	gtkcommand.command = GTKSETFOCUS;
	gtkcommand.data[0] = (gpointer) screen;
	gtk_sendcommand(TRUE);
}

void gui_scrollerchanged(Screen *screen, int position)
{
	int *pos;

	pos = malloc(sizeof(int));
	*pos = position;

	gtkcommand.command = GTKSCROLL;
	gtkcommand.data[0] = (gpointer) screen;
	gtkcommand.data[1] = (gpointer) pos;
	gtk_sendcommand(TRUE);
}

/* Assuming fontinfo is allocated as 100 bytes in function_winitem() */
void gui_query_window_info(Screen *screen, char *fontinfo, int *x, int *y, int *cx, int *cy)
{
	if(screen && screen->window)
	{
		gdk_window_get_position(screen->window->window, x, y);
		gdk_window_get_size(screen->window->window, cx, cy);
		if(screen->fontname)
			strlcpy(fontinfo, screen->fontname, 100);
		else
			strcpy(fontinfo, "unknown");
	}
}

/* Sound is implemented using ESD at the moment */
void gui_play_sound(char *filename)
{
#ifdef SOUND
	esd_play_file(VERSION, filename, 0);
#endif
}

int gui_send_mci_string(char *mcistring, char *retstring)
{
	return 0;
}

void gui_get_sound_error(int errnum, char *errstring)
{
}

void gui_menu(Screen *screen, char *addmenu)
{
	MenuStruct *menutoadd;
	menutoadd = findmenu(addmenu);

	gtkcommand.command = GTKMENUBAR;
	gtkcommand.data[0] = (gpointer) screen;
	gtkcommand.data[1] = (gpointer) m_strdup(addmenu);
	gtkcommand.data[2] = (gpointer) menutoadd;
	gtk_sendcommand(TRUE);
}

int gui_isset(Screen *screen, fd_set *rd, int what)
{
	if(screen == last_input_screen)
		return FD_ISSET(gtkipcin[0], rd);
	else
		return FALSE;
}

int gui_putc(int c)
{
	return gtkputc((unsigned char)c);
}

void gui_exit(void)
{
#ifdef SOUND
	if(esd_connection >= 0)
		esd_close(esd_connection);
#endif

	gtkcommand.command = GTKEXIT;
	gtk_sendcommand(FALSE);
	usleep(1000);
	exit(0);
}

void gui_screen(Screen *gtknew)
{
    /* Copy the global variables into the screen struct */
	gtknew->old_li=gtknew->li;
	gtknew->old_co=gtknew->co;
	gtknew->window = mainwindow;
	gtknew->viewport = mainviewport;
	gtknew->menubar = mainmenubar;
	gtknew->scroller = mainscroller;
	gtknew->scrolledwindow = mainscrolledwindow;
	gtknew->clist = mainclist;
	gtknew->box = mainbox;
	gtknew->adjust = mainadjust;
	gtknew->font = mainfont;
	gtknew->fontname = mainfontname;
	gtknew->gtkio = gtkio;
	gtknew->pipe[0] = mainpipe[0];
	gtknew->pipe[1] = mainpipe[1];
	gtknew->maxfontwidth = ((ZvtTerm *)gtknew->viewport)->charwidth;
	gtknew->maxfontheight = ((ZvtTerm *)gtknew->viewport)->charheight;
	gtknew->page = NULL;
	main_screen = gtknew;
	/* Clean out the globals */
	mainwindow = (GtkWidget *)NULL;
	mainviewport = (GtkWidget *)NULL;
	mainmenubar = (GtkWidget *)NULL;
	mainscroller = (GtkWidget *)NULL;
	mainscrolledwindow = (GtkWidget *)NULL;
	mainclist = (GtkWidget *)NULL;
	mainbox = (GtkWidget *)NULL;

	gtk_signal_disconnect_by_data (GTK_OBJECT(gtknew->window), gtknew->window);
	gtk_signal_disconnect_by_data (GTK_OBJECT(gtknew->clist), gtknew->window);
	gtk_signal_disconnect_by_data (GTK_OBJECT(gtknew->viewport), gtknew->window);
	/* Re-attach handlers to the events */
	gtk_signal_connect(GTK_OBJECT(gtknew->window), "delete_event", GTK_SIGNAL_FUNC(window_destroy), gtknew);
	gtk_signal_connect(GTK_OBJECT(gtknew->viewport), "key_press_event", GTK_SIGNAL_FUNC(gtk_keypress), gtknew);
	gtk_signal_connect(GTK_OBJECT(gtknew->window), "size_allocate", GTK_SIGNAL_FUNC(gtk_windowsize), gtknew);
	gtk_signal_connect(GTK_OBJECT(gtknew->window), "focus_in_event", GTK_SIGNAL_FUNC(gtk_windowfocus), gtknew);
	gtk_signal_connect(GTK_OBJECT(gtknew->viewport), "button_press_event", GTK_SIGNAL_FUNC(gtk_contextmenu), (gpointer)gtknew);
	gtk_signal_connect(GTK_OBJECT(gtknew->adjust), "value_changed", GTK_SIGNAL_FUNC(gtk_scrollerchanged), gtknew);
	gtk_signal_connect(GTK_OBJECT(gtknew->clist), "button_press_event", GTK_SIGNAL_FUNC(gtk_click_nicklist), (gpointer)gtknew);
	gtk_signal_connect (GTK_OBJECT(gtknew->window), "selection_get",
						GTK_SIGNAL_FUNC (selection_handle), gtknew);
	gtk_signal_connect (GTK_OBJECT(gtknew->window), "selection_received",
						GTK_SIGNAL_FUNC (gtk_paste), gtknew);
}

void gui_resize(Screen *gtknew)
{
	gtk_resize(gtknew);
}

/* Load the font now that the Saves are loaded */
void gui_font_init(void)
{
	char *font;

	if((font=get_string_var(DEFAULT_FONT_VAR))!=NULL)
		gui_font_set(font, main_screen);
	else
		gui_font_set(mainfontname, main_screen);
}

void gui_font_set(char *font, Screen *screen)
{
	gtkcommand.command = GTKSETFONT;
	gtkcommand.data[0] = (gpointer)strdup(font);
	gtkcommand.data[1] = (gpointer)screen;
	gtk_sendcommand(TRUE);
}

int gui_screen_width(void)
{
	return gdk_screen_width();
}

int gui_screen_height(void)
{
	return gdk_screen_height();
}

void gui_setwindowpos(Screen *screen, int x, int y, int cx, int cy, int top, int bottom, int min, int max, int restore, int activate, int size, int position)
{
	if(screen && screen->window)
	{
		gtkcommand.command = GTKSETWINDOWPOS;
		gtkcommand.data[0] = (gpointer)screen->window;
		gtkcommand.data[1] = (gpointer)top;
		gtkcommand.data[2] = (gpointer)bottom;
		gtkcommand.data[3] = (gpointer)size;
		gtkcommand.data[4] = (gpointer)position;
		gtkcommand.data[5] = (gpointer)min;
		gtkcommand.data[6] = (gpointer)restore;
		gtkcommand.data[7] = (gpointer)cx;
		gtkcommand.data[8] = (gpointer)cy;
		gtkcommand.data[9] = (gpointer)x;
		gtkcommand.data[10] = (gpointer)y;
		gtk_sendcommand(TRUE);
	}
}

void BX_gui_mutex_lock(void)
{
	gdk_threads_enter();
}

void BX_gui_mutex_unlock(void)
{
	gdk_threads_leave();
}

int gui_setmenuitem(char *menuname, int refnum, char *what, char *param)
{
	MenuRef *tmp = NULL;
	MenuStruct *tmpmenu = findmenu(menuname);
	MenuList *thismenu;

	if(!tmpmenu)
		return FALSE;

	tmp = find_menuref(tmpmenu->root, refnum);

	thismenu = tmpmenu->menuorigin;

	while(thismenu && thismenu->next && thismenu->refnum != refnum)
	{
		thismenu = thismenu->next;
	}

	if(!thismenu || thismenu->refnum != refnum)
		return FALSE;

	if(my_stricmp(what, "check")==0)
	{
		int checkstate = my_atol(param);

		if(checkstate)
			thismenu->menutype |= GUICHECKEDMENUITEM;
		else
			thismenu->menutype &= ~GUICHECKEDMENUITEM;

		if(!tmp)
			return TRUE;

		if(checkstate)
			tmp->checked = TRUE;
		else
			tmp->checked = FALSE;

		if(tmp->menuhandle && GTK_CHECK_MENU_ITEM(tmp->menuhandle)->active != tmp->checked)
		{
			tmp->checked = GTK_CHECK_MENU_ITEM(tmp->menuhandle)->active;
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(tmp->menuhandle), tmp->checked);
		}
	}
	else if(my_stricmp(what, "text")==0)
	{
		if(thismenu->name)
		{
			new_free(&thismenu->name);
			thismenu->name = m_strdup(param);
		}

		if(!tmp)
			return TRUE;

		if(tmp->menutext)
			new_free(&tmp->menutext);
		tmp->menutext = m_strdup(param);
		gtkcommand.command = GTKREDRAWMENUS;
		gtk_sendcommand(TRUE);
	}
	else
		return FALSE;

	return TRUE;
}

/* Hrm... for some reason I was thinking there was something platform
 * specific about this subroutine but... now looking at it once it is
 * coded it doesn't appear to be... maybe it will cease to be a gui_*
 * function in the short future.
 */
void gui_remove_menurefs(char *menu)
{
	MenuStruct *cleanmenu = findmenu(menu);

	if(cleanmenu)
	{
		MenuList *tmplist = cleanmenu->menuorigin;

		while(tmplist)
		{
			if(tmplist->refnum > 0)
				remove_menuref(&cleanmenu->root, tmplist->refnum);
			if(tmplist->menutype == GUISUBMENU || tmplist->menutype == GUIBRKSUBMENU)
				gui_remove_menurefs(tmplist->submenu);
			tmplist = tmplist->next;
		}
	}
}

void gui_mdi(Window *window, char *text, int value)
{
	if(current_mdi_mode == value)
		return;

	gtkcommand.command = GTKMDI;
	gtkcommand.data[0] = (gpointer)value;
	gtk_sendcommand(TRUE);
}

void gui_update_nicklist(char *channel)
{
	gtkcommand.command = GTKNICKLIST;
	gtkcommand.data[0] = (gpointer) channel;
	gtk_sendcommand(TRUE);
}

void gui_nicklist_width(int width, Screen *this_screen)
{
	gtkcommand.command = GTKNICKLISTWIDTH;
	gtkcommand.data[0] = (gpointer)this_screen;
	gtkcommand.data[1] = (gpointer)width;
	gtk_sendcommand(TRUE);
}

void gui_startup(int argc, char *argv[])
{
	pipe(guiipc);
	new_open(guiipc[0]);

	gtk_set_locale();
	{
		char *p;
		if ((p = path_search("gtkrc", "/usr/share/themes/Default/gtk:/usr/local/share/themes/Default/gtk")))
			gtk_rc_add_default_file(p);
	}
	g_thread_init(NULL);
	gtk_init(&argc, &argv);
#ifdef USE_IMLIB
	gdk_imlib_init();
#endif
}

GtkWidget *about_window = NULL;

void gtk_about_box(char *about_text)
{
	GtkWidget *vbox, *label, *hbox;
	GtkWidget *button;
	GdkBitmap *bitmap;
	GdkPixmap *icon_pixmap = NULL;
#ifndef USE_IMLIB
	GtkStyle *style;
#endif
	GtkWidget *pixmap;
	gchar *text;

	if (!about_window)
	{
		about_window = gtk_window_new(GTK_WINDOW_DIALOG);
		text = g_strdup_printf("About %s", _VERSION_);
		gtk_window_set_title(GTK_WINDOW(about_window), text);
		g_free(text);
		gtk_window_set_policy(GTK_WINDOW(about_window), FALSE, FALSE, FALSE);
		gtk_window_position(GTK_WINDOW(about_window), GTK_WIN_POS_MOUSE);
		gtk_container_set_border_width(GTK_CONTAINER(about_window), 10);
		gtk_signal_connect(GTK_OBJECT(about_window), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &about_window);
		gtk_widget_realize(about_window);

#ifndef USE_IMLIB
		style = gtk_widget_get_style(about_window);
		if (!icon_pixmap)
			icon_pixmap = gdk_pixmap_create_from_xpm_d(about_window->window, &bitmap, &style->bg[GTK_STATE_NORMAL], BitchX);
#else
		gdk_imlib_data_to_pixmap(BitchX, &icon_pixmap, &bitmap);
#endif

		gdk_window_set_icon(about_window->window, NULL, icon_pixmap, bitmap);
		pixmap = gtk_pixmap_new(icon_pixmap, bitmap);

		vbox = gtk_vbox_new(FALSE, 5);
		gtk_container_add(GTK_CONTAINER(about_window), vbox);
		gtk_container_add(GTK_CONTAINER(vbox), pixmap);
		gtk_widget_show(vbox);
		gtk_widget_show(pixmap);

		label = gtk_label_new(VERSION);
		gtk_container_add(GTK_CONTAINER(vbox), label);
		gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
		gtk_widget_show(label);

		label = gtk_label_new("Copyright (c) 1996-2002 Colten Edwards\nAll rights reserved.\n");
		gtk_container_add(GTK_CONTAINER(vbox), label);
		gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
		gtk_widget_show(label);

		label = gtk_label_new(about_text);
		gtk_container_add(GTK_CONTAINER(vbox), label);
		gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
		gtk_widget_show(label);

		hbox = gtk_hbox_new(TRUE, 2);
		gtk_container_add(GTK_CONTAINER(vbox), hbox);
		gtk_widget_show(hbox);

		button = gtk_button_new_with_label("OK");
		gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);
		gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
		GTK_WIDGET_SET_FLAGS(GTK_WIDGET(button), GTK_CAN_DEFAULT);
		gtk_widget_grab_default(button);
		gtk_signal_connect_object(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(about_window));
		gtk_widget_show(button);

		gtk_widget_show(about_window);
	}
}

void gui_about_box(char *about_text)
{
	gtkcommand.command = GTKABOUTBOX;
	gtkcommand.data[0] = (gpointer) about_text;
	gtk_sendcommand(TRUE);
}

void gui_activity(int color)
{
	if(get_int_var(MDI_VAR))
	{
		gtkcommand.command = GTKACTIVITY;
		gtkcommand.data[0] = (gpointer) color;
		gtkcommand.data[1] = (gpointer) target_window;
		gtk_sendcommand(TRUE);
	}
}

void gui_setfileinfo(char *filename, char *nick, int server)
{
}

void gui_setfd(fd_set *rd)
{
	/* Set the GUI IPC pipe readable for select() */
	FD_SET(guiipc[0], rd);
	FD_SET(gtkipcin[0], rd);
}
