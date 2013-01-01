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
#include "module.h"
#include "hook.h"
#include "hash2.h"
#define INIT_MODULE
#include "modval.h"

#include <sys/time.h>

#define cparse convert_output_format

#ifdef __EMXPM__
HAB     hnab = 0;
HMQ     hnmq = 0;
QMSG	nqmsg = { 0 };
HWND	hwndnClient = 0;
HDC	hndc = 0;
HVPS	hnvps = 0;
HWND    hwndNicklist = 0;
HWND    hwndscroller;

char spaces[NICKNAME_LEN+1];

SBCDATA scrollerdata;
int spos;
#elif defined(GTK)
GdkColor Black = { 0, 0x0000, 0x0000, 0x0000 };
GdkColor White = { 0, 0xaaaa, 0xaaaa, 0xaaaa };
GtkWidget *GtkNickList = NULL,
          *clist,
          *scrolledwindow;
char *tmptext[1];
#endif

BUILT_IN_DLL(nicklist_add);
int update_nicklist (char *, char *, char **);

void drawnicklist(void)
{
NickList *n, *list;
ChannelList *cptr;
char minibuffer[NICKNAME_LEN+1];
int nickcount=0, k;

    cptr = lookup_channel(current_window->current_channel, from_server, 0);
    list = sorted_nicklist(cptr, NICKSORT_NORMAL);
#ifdef __EMXPM__
       if(spos<0)
           spos=0;
#elif defined(GTK)
       gui_mutex_lock();
       gtk_clist_clear((GtkCList *)clist);
#endif
       for(n = list; n; n = n->next)
       {
#ifdef __EMXPM__
             if(nickcount >= spos && nickcount < spos + 20)
             {
#endif
                minibuffer[0]='\0';
                if(nick_isvoice(n))
                   strcpy(minibuffer, "+");
                if(nick_isop(n))
                   strcpy(minibuffer, "@");
                if(nick_isircop(n))
                   strcat(minibuffer, "*");
                strcat(minibuffer, n->nick);
                for(k=strlen(minibuffer);k<NICKNAME_LEN;k++)
                   minibuffer[k]=' ';
                minibuffer[NICKNAME_LEN]='\0';
#ifdef __EMXPM__
                VioSetCurPos(nickcount-spos, 0, hnvps);
                VioWrtTTY(minibuffer, strlen(minibuffer), hnvps);
                }
#elif defined(GTK)
             tmptext[0] = &minibuffer[0];
             gtk_clist_append((GtkCList *)clist, tmptext);
             gtk_clist_set_background((GtkCList *)clist, nickcount, &Black);
             gtk_clist_set_foreground((GtkCList *)clist, nickcount, &White);
#endif
             nickcount++;
       }
#ifdef __EMXPM__
    if(nickcount<20)
       {
          for(k=nickcount;k<20;k++)
             {
             VioSetCurPos(k, 0, hnvps);
             VioWrtTTY(spaces, NICKNAME_LEN, hnvps);
             }
       }
    else if (spos > nickcount-20)
       {
       spos=nickcount-20;
       drawnicklist();
       }

    WinSendMsg(hwndscroller, SBM_SETSCROLLBAR, (MPARAM)spos, MPFROM2SHORT(0, nickcount - 20));
    WinSendMsg(hwndscroller, SBM_SETTHUMBSIZE, MPFROM2SHORT(20, nickcount), (MPARAM)NULL);
#elif defined(GTK)
    gui_mutex_unlock();
#endif
    clear_sorted_nicklist(&list);
}

int update_nicklist (char *which, char *str, char **unused)
{
#ifdef __EMXPM__
	if (hwndNicklist)
#elif defined(GTK)
	if (GtkNickList)
#endif
		drawnicklist();
	return 0;
}

#ifdef __EMXPM__
MRESULT EXPENTRY NicklistWndProc(hwnd, msg, mp1, mp2)
HWND   hwnd;
USHORT msg;
MPARAM mp1;
MPARAM mp2;
{
HPS   hps;

        switch(msg) {
        case WM_TIMER:
           if(get_dllint_var("nl_always_on_top"))
            WinSetWindowPos(WinQueryWindow(hwnd, QW_PARENT),
                        	HWND_TOP, 0,0,0,0, SWP_ZORDER);
            break;
         case WM_CLOSE:
            VioAssociate((HDC)NULL, hnvps);
            VioDestroyPS(hnvps);
            WinDestroyWindow(hwndNicklist);
            hwndnClient = hwndscroller = hwndNicklist = NULLHANDLE;
            return (MRESULT)TRUE;
            break;
         case WM_VSCROLL:
           switch(SHORT2FROMMP(mp2))
           {
           case SB_LINEUP:
              spos--;
              break;
           case SB_LINEDOWN:
              spos++;
              break;
           case SB_PAGEUP:
              spos -= 20;
              break;
           case SB_PAGEDOWN:
              spos += 20;
              break;
           case SB_SLIDERPOSITION:
           case SB_SLIDERTRACK:
              spos=SHORT1FROMMP(mp2);
              break;
           }
           drawnicklist();
           break;
	case WM_PAINT:
	   hps = WinBeginPaint(hwnd, NULLHANDLE, NULL);
	   VioShowPS(20, NICKNAME_LEN, 0, hnvps);
	   WinEndPaint(hps);
 	   return (0L);
        case WM_SIZE:
 	   return (WinDefAVioWindowProc(hwnd, msg, (ULONG)mp1, (ULONG)mp2));
           break;
        }
return (WinDefWindowProc(hwnd, msg, mp1, mp2));
}

void nicklist_thread(void)
{
ChannelList *chan;
NickList *n;
SWP winpos;
int borderw, borderh, titleh, cx, cy;
ULONG fstyle = FCF_TITLEBAR | FCF_SHELLPOSITION | FCF_TASKLIST | FCF_DLGBORDER;
VIOCURSORINFO bleah;

   hnab = WinInitialize(0);
   hnmq = WinCreateMsgQueue(hnab, 0);
   WinRegisterClass(hnab, "NICKLIST", NicklistWndProc, CS_SIZEREDRAW, 0);
	hwndNicklist = WinCreateStdWindow(HWND_DESKTOP,
								   WS_VISIBLE,
								   &fstyle,
								   "NICKLIST",
								   "Nicks",
								   0L,
								   NULLHANDLE,
								   0L,
								   &hwndnClient);
	hndc = WinOpenWindowDC(hwndnClient);									  /* opens device context */
	VioCreatePS(&hnvps, 20, NICKNAME_LEN + 1, 0, 					  /* creates AVIO PS */
				1, 0);
	VioAssociate(hndc, hnvps);									  /* associates DC and AVIO PS */
	VioSetDeviceCellSize(current_window->screen->VIO_font_height, current_window->screen->VIO_font_width, hnvps);			  /* Set the font size */
	titleh=WinQuerySysValue(HWND_DESKTOP, SV_CYTITLEBAR);
   borderh=WinQuerySysValue(HWND_DESKTOP, SV_CYDLGFRAME);
   borderw=WinQuerySysValue(HWND_DESKTOP, SV_CXDLGFRAME);

        cx=NICKNAME_LEN*current_window->screen->VIO_font_width;
        cy=20*current_window->screen->VIO_font_height;
        WinSetWindowPos(hwndNicklist, HWND_TOP, 0, 0, cx + (borderw * 2) + 15 /* scroller width */, cy + titleh + (borderh * 2), SWP_SIZE);
        scrollerdata.posFirst = 0;
        scrollerdata.posLast = 0;
        scrollerdata.posThumb = 0;
        scrollerdata.cVisible = 0;
        scrollerdata.cTotal = 0;
        hwndscroller = WinCreateWindow(hwndnClient, WC_SCROLLBAR, (PSZ)NULL, SBS_VERT | SBS_THUMBSIZE | SBS_AUTOSIZE | WS_VISIBLE, cx, 0, 15, cy, hwndnClient, HWND_TOP, 1L, &scrollerdata, NULL);
        bleah.yStart = 0;
        bleah.cEnd = 0;
        bleah.cx = 0;
        bleah.attr = -1;
        VioSetCurType(&bleah, hnvps);
        update_nicklist(NULL, NULL, NULL);
        WinStartTimer(0, WinWindowFromID(hwndNicklist, FID_CLIENT), 1, 1000);
	while (WinGetMsg(hnab, &nqmsg, (HWND)NULL, 0, 0))
		 WinDispatchMsg(hnab, &nqmsg);
        WinDestroyMsgQueue(hnmq);
        WinTerminate(hnab);
        _endthread();
}
#elif defined(GTK)
void gtk_nicklist(void)
{
    gui_mutex_lock();
    GtkNickList = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(GtkNickList), "Nicks");
    /*gtk_signal_connect(GTK_OBJECT(GtkNickList), "delete_event", GTK_SIGNAL_FUNC (delete), NULL);*/
    scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_container_add(GTK_CONTAINER(GtkNickList), scrolledwindow);
    gtk_widget_show(scrolledwindow);

    clist = gtk_clist_new(1);
    gtk_container_add(GTK_CONTAINER(scrolledwindow), clist);
    gtk_widget_show(clist);
    gtk_widget_show(GtkNickList);
    gui_mutex_unlock();
    drawnicklist();
}
#endif

BUILT_IN_DLL(nicklist_add)
{
#ifdef __EMXPM__
    if(hwndNicklist)
        WinPostMsg(hwndnClient, WM_CLOSE, 0L, 0L);
    else
        _beginthread((void *)nicklist_thread, NULL, 0xFFFF, NULL);
#elif defined(GTK)
    if(GtkNickList)
    {
        gui_mutex_lock();
        gtk_widget_destroy(GtkNickList);
        GtkNickList = NULL;
        gui_mutex_unlock();
    }
    else
        gtk_nicklist();
#endif
}

int Nicklist_Lock(IrcCommandDll **intp, Function_ptr *global_table)
{
	return 1;
}

int Nicklist_Init(IrcCommandDll **intp, Function_ptr *global_table)
{
#ifdef GTK
/* For paranoia's sake make sure the colors are in the color map */
GdkColormap *cmap;
#endif
	initialize_module("Nicklist");

#ifdef __EMXPM__
        memset(spaces, ' ', NICKNAME_LEN);
        spaces[NICKNAME_LEN]=0;
#endif

#ifdef GTK
/*        gui_mutex_lock();*/
	cmap = gdk_colormap_get_system();
       	gdk_color_alloc(cmap, &White);
        gdk_color_alloc(cmap, &Black);
/*        gui_mutex_unlock();*/
#endif

	add_module_proc(COMMAND_PROC, "Nicklist", "nl", "Add a nick list to window", 0, 0, nicklist_add, NULL);
	add_module_proc(VAR_PROC, "Nicklist", "nl_always_on_top", NULL, BOOL_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(HOOK_PROC, "Nicklist", NULL, "*", KICK_LIST, 1, NULL, update_nicklist);
	add_module_proc(HOOK_PROC, "Nicklist", NULL, "*", JOIN_LIST, 1, NULL, update_nicklist);
	add_module_proc(HOOK_PROC, "Nicklist", NULL, "*", MODE_LIST, 1, NULL, update_nicklist);
	add_module_proc(HOOK_PROC, "Nicklist", NULL, "*", LEAVE_LIST, 1, NULL, update_nicklist);
	add_module_proc(HOOK_PROC, "Nicklist", NULL, "*", SIGNOFF_LIST, 1, NULL, update_nicklist);
	add_module_proc(HOOK_PROC, "Nicklist", NULL, "*", NICKNAME_LIST, 1, NULL, update_nicklist);
	add_module_proc(HOOK_PROC, "Nicklist", NULL, "*", WINDOW_FOCUS_LIST, 1, NULL, update_nicklist);
	add_module_proc(HOOK_PROC, "Nicklist", NULL, "*", CHANNEL_SYNCH_LIST, 1, NULL, update_nicklist);
	add_module_proc(HOOK_PROC, "Nicklist", NULL, "*", CHANNEL_SWITCH_LIST, 1, NULL, update_nicklist);

	return 0;
}

