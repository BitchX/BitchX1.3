/* pmbitchx.c Written by: Brian Smith (NuKe) and rosmo for BitchX */


#include <sys/stat.h>
/* These headers from the Warp 4 toolkit */
#include <os2/mciapi.h>
#include <os2/mcios2.h>
#include <io.h>
#include <stddef.h>
/* We now require dynamic windows */
#include <dw.h>
#include "window.h"
#include "gui.h"
#include "server.h"
#include "hook.h"
#include "list.h"
#include "commands.h"
#include "hash2.h"
#include "status.h"

/* Window data, located at QWP_USER */
typedef struct {
	HVPS    hvps;
	Screen *screen;
} MYWINDATA, *PMYWINDATA;

int	VIO_staticx=200;
int	VIO_staticy=100;
int	VIO_font_width=6;
int	VIO_font_height=10;

int cxvs, cxsb, cysb, cytb;

/* These are for $lastclickline() function */
char *lastclicklinedata = NULL;
int lastclickcol, lastclickrow;

/* These are for the context menu */
HWND contextmenu;
int contextx, contexty;

HAB	mainhab = 0;
HMQ	mainhmq = 0;

HAB	hab = 0;		 /* handle to the anchor block */
HMQ	hmq = 0;		 /* handle to the message queue */
HWND	hwndClient = 0;		 /* handle to the client */
HWND	hwndFrame = 0; 		 /* handle to the frame window */
HWND	hwndMenu = 0;            /* handle to the menu */
HWND    hwndLeft = 0, hwndRight = 0; /* For nicklist */
HWND    hwndscroll = 0, hwndnickscroll = 0; /* scrollerbars */
HWND    MDIClient = 0,
        MDIFrame = 0;            /* MDI handles */
QMSG	qmsg = { 0 };		 /* message-queue structure */
HDC	hdc = 0;		 /* handle to the device context */
HVPS	hvps = 0,
        mainHvps = 0,         /* handle to the AVIO presentation space */
	    hvpsnick = 0;
static HEV sem = 0;
VIOMODEINFO viomode;
FILE	*debug;
static unsigned char output_buf[256];
int     output_tail = 0;
LONG    menuY;
clock_t flush_counter;
int     pmthread = -1;

HMTX    mutex = 0;

ULONG  cx, cy;
int    just_resized = 0, dont_resize = 0, no_resize;
Screen *focus_screen, *just_resized_screen;
extern MenuStruct *morigin;

/* Used by window_menu_stub() */
ULONG   menuroot=4000;
ULONG   tmpmenuid=3000;

Screen *cursor_screen = NULL;

/* Used by popupmsg */
extern char *msgtext;

/* Used by the scrollers */
int newscrollerpos=0, lastscrollerpos=0, lastscrollerwindow=-1, lastpos=0;

/* Used by file dialog to make it non blocking */
char *codeptr, *paramptr;

/* Used by properties notebook */
static HWND hwndPage1;
static HWND hwndPage2;
static HWND hwndPage3;
static HWND hwndPage4;
static HWND hwndPage5;
int p1e, p2e, p3e, p4e, inproperties=0;
ChannelList *chan = NULL;
Window *nb_window=NULL;
FONTMETRICS fm;
int in_fontdialog=0;

IrcVariable *return_irc_var(int nummer);
IrcVariable *return_fset_var(int nummer);
CSetArray *return_cset_var(int nummer);
WSetArray *return_wset_var(int nummer);

int fontstart=0;

HMODULE hmod;
ULONG EXPENTRY (*DLL_mciPlayFile)(HWND, PSZ, ULONG, PSZ, HWND) = NULL;
ULONG EXPENTRY (*DLL_mciGetErrorString)(ULONG, PSZ, USHORT) = NULL;
ULONG EXPENTRY (*DLL_mciSendString)(PSZ, PSZ,USHORT, HWND, USHORT) = NULL;


#include "input.h"
#define         WIDTH   10

/* needed for rclick */
#include "keys.h"
unsigned long menucmd;

int guiipc[2];

int new_menuref(MenuRef **root, int refnum, HWND menuhandle, int menuid)
{
	MenuRef *new = new_malloc(sizeof(MenuRef));

	if(new)
	{
		new->refnum = refnum;
		new->menuhandle = menuhandle;
		new->menuid = menuid;
		new->next = NULL;

		if (!*root)
			*root = new;
		else
		{
			MenuRef *prev = NULL, *tmp = *root;
			while(tmp)
			{
				prev = tmp;
				tmp = tmp->next;
			}
			if(prev)
				prev->next = new;
			else
				*root = new;
		}
		return TRUE;
	}
	else
		return FALSE;
}

int remove_menuref(MenuRef **root, int refnum)
{
	MenuRef *prev = NULL, *tmp = *root;

	while(tmp)
	{
		if(tmp->refnum == refnum)
		{
			if(!prev)
			{
				new_free(&tmp);
                                *root = NULL;
                                return 0;
			}
			else
			{
				prev->next = tmp->next;
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

	while(tmp)
	{
		if(tmp->refnum == refnum)
			return tmp;
		tmp = tmp->next;
	}
	return NULL;
}


void gui_settitle(char *titletext, Screen *os2win) {
	WinSetWindowText(os2win->hwndFrame, titletext);
}

void aflush() {
	HMTX h = mutex;

	/* flush */
	if (output_tail > 0)
	{
		DosOpenMutexSem(NULL, &h);
		DosRequestMutexSem(h, SEM_INDEFINITE_WAIT);

		VioWrtTTY((PCH)output_buf, output_tail, (output_screen ? output_screen->hvps : hvps));
		output_tail = 0;
		DosReleaseMutexSem(h);
		DosCloseMutexSem(h);
	}
}

void sendevent(unsigned int event, unsigned int window)
{
	unsigned int evbuf[2];

	if(guiipc[1])
	{
		evbuf[0] = event;
		evbuf[1] = window;

		write(guiipc[1], (void *)evbuf, sizeof(unsigned int)*2);
	}
	return;
}

void flush_thread(void *param)
{
	clock_t foo;

	while (1 & 1)
	{
		foo = clock();
		if ((foo - flush_counter) > 10)
		{ if (output_tail > 0) { aflush(); } }
		DosSleep(100L);
	}
}

void cursor_off(HVPS this_hvps)
{
	VIOCURSORINFO bleah;

	VioGetCurType(&bleah, this_hvps);
	bleah.attr = -1;
	VioSetCurType(&bleah, this_hvps);
}

void cursor_thread(void)
{
	VIOCURSORINFO bleah;
	HVPS hvps;;

	while(1 & 1)
	{
		DosSleep(500L);
		if(cursor_screen && (hvps = cursor_screen->hvps))
		{
			VioGetCurType(&bleah, hvps);
			bleah.attr = 0;
			VioSetCurType(&bleah, hvps);
		}
		DosSleep(500L);
		if(cursor_screen && (hvps = cursor_screen->hvps))
		{
			VioGetCurType(&bleah, hvps);
			bleah.attr = -1;
			VioSetCurType(&bleah, hvps);
		}
	}
}

void aputc(int c) {
	HMTX h;

	DosOpenMutexSem(NULL, &h);
	DosRequestMutexSem(h, SEM_INDEFINITE_WAIT);

	/* buffer */
	if (output_tail < 256)
	{
		output_buf[output_tail] = (unsigned char) c;
		output_tail++;
		flush_counter = clock();
	}

	/* flush */
	if (output_tail == 256 || c == '\n' || c == '\r')
	{
		VioWrtTTY((PCH)output_buf, output_tail, (output_screen ? output_screen->hvps : hvps));
		output_tail = 0;
	}

	DosReleaseMutexSem(h);
	DosCloseMutexSem(h);
}

int gui_read(Screen *screen, char *buffer, int maxbufsize) {
	int i;	

	if (strlen(screen->aviokbdbuffer)> maxbufsize) {
		for(i=0;i<=strlen(screen->aviokbdbuffer);i++)
		{
			if (i<maxbufsize) {
				buffer[i]=screen->aviokbdbuffer[i];
			}
			else if(i==maxbufsize) {
				buffer[i]=0;
				screen->aviokbdbuffer[0]=screen->aviokbdbuffer[i];
			}
			else {
				screen->aviokbdbuffer[i-maxbufsize]=screen->aviokbdbuffer[i];
			}
		}
	} else {
		strcpy(buffer,screen->aviokbdbuffer);
		screen->aviokbdbuffer[0]='\0';
	}
	return strlen(buffer);
}

void aprintf(char *format, ...) {
	va_list args;
	char putbuf[AVIO_BUFFER+1], bluf[AVIO_BUFFER];
	USHORT  i, o;

	va_start(args, format);
	vsprintf(putbuf, format, args);
	va_end(args);

	i = o = 0;
	while (putbuf[i] != '\n' && putbuf[i] != 0) {
		i++;
		if (putbuf[i] == '\n' || putbuf[i] == 0) {
			strcpy(bluf, "");
			strncat(bluf, &putbuf[o], i - o);
			if (putbuf[i] != 0) strcat(bluf, "\r\n");
			VioWrtTTY((PCH)bluf,
				  strlen(bluf),
				  (output_screen ? output_screen->hvps : hvps));
			i++; o = i;
		}
	}
}

MRESULT EXPENTRY FontDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
	ULONG Remfonts = 0, Fonts = 0, i, i2;
	static PFONTMETRICS fonts;
	char buf[256];
	ULONG maxX, maxY, len;
	SWP menupos;
	unsigned char *scr;
	char fontsize[10];
	static Screen *changescreen, *savedscreen = NULL;

	switch (msg) {
	case WM_DESTROY:
		free(fonts);
	break;
	case WM_INITDLG:
		savedscreen = current_window ? current_window->screen : main_screen;
		VioQueryFonts(&Remfonts,
			      NULL,
			      0,
			      &Fonts,
			      "System VIO",
			      VQF_PUBLIC,
			      hvps);

		fonts = malloc(sizeof(FONTMETRICS) * Remfonts);

		Fonts = Remfonts;
		VioQueryFonts(&Remfonts,
			      fonts,
			      sizeof(FONTMETRICS),
			      &Fonts,
			      "System VIO",
			      VQF_PUBLIC,
			      hvps);

		for (i = 0; i < Fonts; i++)
		{
			if(fonts[i].lAveCharWidth != 5)
			{
				sprintf(buf, "%s %ux%u", fonts[i].szFacename, (unsigned int)fonts[i].lMaxBaselineExt, (unsigned int)fonts[i].lAveCharWidth);
				WinSendMsg(WinWindowFromID(hwnd, 101),
						   LM_INSERTITEM,
						   MPFROMSHORT(LIT_END),
						   MPFROMP(buf));
			}
			else
				fontstart=i;
		}
		fontstart++;
		break;
	case WM_COMMAND:
        switch (COMMANDMSG(&msg)->cmd)
	{
	case DID_CANCEL:
		WinDismissDlg(hwnd, 0);
		break;
	case DID_OK:
		i = (ULONG)WinSendMsg(WinWindowFromID(hwnd, LB_FONTLIST),
				      LM_QUERYSELECTION,
				      MPFROMSHORT(LIT_CURSOR),
				      0);

		if(i == LIT_NONE)
		{
			WinDismissDlg(hwnd, 0);
			return (MRESULT)0;
		}

		i+=fontstart;

		i2 = (ULONG)WinSendMsg(WinWindowFromID(hwnd, CB_DEFAULT),
				       BM_QUERYCHECK,
				       0,
				       0);
		if(i2)
		{
			sprintf(fontsize, "%dx%d", (int)fonts[i].lAveCharWidth, (int)fonts[i].lMaxBaselineExt);
			set_string_var(DEFAULT_FONT_VAR, fontsize);
		}

		i2 = (ULONG)WinSendMsg(WinWindowFromID(hwnd, CB_CHANGEFORALL),
				       BM_QUERYCHECK,
				       0, 0);

		if(i2)
			changescreen = screen_list;
		else
			changescreen = savedscreen;

		while(changescreen)
		{
			if(changescreen->alive)
			{
				len = (changescreen->co + 1) * changescreen->li * 2;
				scr = malloc(len);
				VioReadCellStr(scr, (PUSHORT)&len, 0, 0, changescreen->hvps);

				changescreen->VIO_font_width = fonts[i].lAveCharWidth;
				changescreen->VIO_font_height = fonts[i].lMaxBaselineExt;

				co = changescreen->co;
				li = changescreen->li;

				cx = (co - 1) * changescreen->VIO_font_width;
				cy = li * changescreen->VIO_font_height;

				maxX = WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN);
				maxY = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);

				/* No point in making bigger windows than the screen */
				if (cx > maxX)
					changescreen->co = (maxX / changescreen->VIO_font_width) -1;

				if (cy > maxY)
					changescreen->li = (maxY / changescreen->VIO_font_height) -1;

				/* Recalculate in case we modified the values */
				cx = (co - 1) * changescreen->VIO_font_width;
				cy = li * changescreen->VIO_font_height;

				if(changescreen->nicklist)
					cx += (changescreen->VIO_font_width * changescreen->nicklist) + cxvs;

				if(changescreen->hwndMenu==(HWND)NULL)
					WinSetWindowPos(changescreen->hwndFrame, HWND_TOP, 0, 0, cx + (cxsb*2) + cxvs, cy + (cysb*2)+cytb, SWP_SIZE);
				else
				{
					WinQueryWindowPos(changescreen->hwndMenu, &menupos);
					WinSetWindowPos(changescreen->hwndFrame, HWND_TOP, 0, 0, cx + (cxsb*2) + cxvs, cy + (cxsb*2)+cytb + menupos.cy, SWP_SIZE);
				}
				VioSetDeviceCellSize(changescreen->VIO_font_height, changescreen->VIO_font_width, changescreen->hvps);
				VioSetDeviceCellSize(changescreen->VIO_font_height, changescreen->VIO_font_width, changescreen->hvpsnick);

				VioWrtCellStr(scr, len, 0, 0, changescreen->hvps);
				free(scr);
			}

			if(i2)
				changescreen = changescreen->next;
			else
				changescreen = NULL;
		}
		WinDismissDlg(hwnd, 0);
		break;
	default: break;
	}
	return((MRESULT)0);
	break;
	}
	return (WinDefDlgProc(hwnd, msg, mp1, mp2));
}

void font_dialog_stub(Screen *screen)
{
	HAB hnbab;
	HMQ hnbmq;

	hnbab = WinInitialize(0);
	hnbmq = WinCreateMsgQueue(hnbab, 0);

	WinDlgBox(HWND_DESKTOP, screen->hwndFrame, FontDlgProc, NULLHANDLE, FONTDIALOG, NULL);
	in_fontdialog=0;

	WinDestroyMsgQueue(hnbmq);
	WinTerminate(hnbab);
}

void gui_font_dialog(Screen *screen)
{
	if(in_fontdialog)
		return;

	in_fontdialog=1;
	_beginthread((void *)font_dialog_stub, NULL, 0xFFFF, (void *)screen);
}

void set_right_window(HWND hwnd)
{
	Screen *this_screen = NULL;
	static Screen *last_screen = NULL;

	this_screen = (Screen *)WinQueryWindowPtr(WinQueryWindow(hwnd, QW_PARENT), 0);
	if (!this_screen)
		this_screen = (Screen *)WinQueryWindowPtr(hwnd, 0);
	if (!this_screen)
		this_screen = output_screen;

	if (this_screen != last_screen)
		make_window_current(this_screen->current_window);

	last_screen = this_screen;
}

int mesg(char *format, ...) {
	va_list args;
	char outbuf[256];

	va_start(args, format);
	vsprintf(outbuf, format, args);
	va_end(args);

	WinMessageBox(HWND_DESKTOP, HWND_DESKTOP, outbuf, "PMBitchX", 0, MB_OK | MB_INFORMATION | MB_MOVEABLE);

	return strlen(outbuf);
}

/* Routines dealing with menus */
HWND newsubmenu(MenuStruct *menutoadd, HWND location, int pulldown)
{
	HWND tmphandle;
	MenuList *tmp;
	MENUITEM miSubMenu;
	HWND hwndSubmenu;
	ULONG itemmask, flag = 0L, id = 0L;

	if(menutoadd->sharedhandle && WinIsWindow(hab, menutoadd->sharedhandle))
		return menutoadd->sharedhandle;

	if(pulldown)
	{
		flag = MS_ACTIONBAR;
        id = FID_MENU;
	}

	tmphandle=WinCreateWindow(location,
							  WC_MENU,
							  NULL,
							  flag,
							  0,0,0,0,
							  location,
							  HWND_TOP,
							  id,
							  NULL,
							  NULL);

	/* Go through all menuitems adding items and submenus (and subitems) as neccessary */

	tmp = menutoadd->menuorigin;
	while(tmp!=NULL)
	{
		itemmask=0;
		hwndSubmenu=(HWND)NULLHANDLE;

		if(tmp->menutype & GUISUBMENU)
		{
			if(tmp->menutype & GUISHARED)
			{
				MenuStruct *tmpmenu = findmenu(tmp->submenu);

				if(!tmpmenu->sharedhandle || !WinIsWindow(hab, tmpmenu->sharedhandle))
				{
					hwndSubmenu=tmpmenu->sharedhandle=(HWND)newsubmenu(tmpmenu, HWND_OBJECT, FALSE);

					/* By mikh's recommendation :) */
					WinSetWindowULong(hwndSubmenu, QWL_STYLE,
									  WinQueryWindowULong(hwndSubmenu, QWL_STYLE) & ~MS_ACTIONBAR);
				}
				else
			hwndSubmenu=(HWND)tmpmenu->sharedhandle;
			}
			else
				hwndSubmenu=(HWND)newsubmenu((MenuStruct *)findmenu(tmp->submenu), tmphandle, FALSE);
		}
		miSubMenu.iPosition=MIT_END;
		miSubMenu.afStyle=MIS_TEXT;
		if(tmp->menutype & GUISEPARATOR)
			miSubMenu.afStyle = MIS_SEPARATOR;
		if(tmp->menutype & GUIBRKMENUITEM)
			miSubMenu.afStyle |= MIS_BREAKSEPARATOR;
		if(tmp->menutype & GUIBRKSUBMENU)
			miSubMenu.afStyle |= MIS_BREAKSEPARATOR;
		if(tmp->menutype & GUIIAMENUITEM)
			itemmask |= MIA_DISABLED;
		if(tmp->menutype & GUICHECKEDMENUITEM)
			itemmask |= MIA_CHECKED;
		if(tmp->menutype & GUINDMENUITEM)
			itemmask |= MIA_NODISMISS;

		miSubMenu.afAttribute=0;
		miSubMenu.id=tmp->menuid;
		miSubMenu.hwndSubMenu=hwndSubmenu;
		miSubMenu.hItem=NULLHANDLE;

		WinSendMsg((HWND)tmphandle,
				   MM_INSERTITEM,
				   MPFROMP(&miSubMenu),
				   MPFROMP(tmp->name));

		if(tmp->refnum > 0)
			new_menuref(&menutoadd->root, tmp->refnum, tmphandle, tmp->menuid);
		/* Set other options */
		if(itemmask != 0)
			WinSendMsg((HWND)tmphandle, MM_SETITEMATTR, MPFROM2SHORT(tmp->menuid, TRUE), MPFROM2SHORT(itemmask, itemmask));
		if(tmp->menutype & GUIDEFMENUITEM)
		{
			WinSetWindowBits((HWND)tmphandle, QWL_STYLE, MS_CONDITIONALCASCADE,MS_CONDITIONALCASCADE );
			/* Set cascade menu default */
			WinSendMsg((HWND)tmphandle, MM_SETDEFAULTITEMID, MPFROMSHORT(tmp->menuid), NULL );
		}

		tmp=tmp->next;
	}
	return tmphandle;
}

/* This is so I can create a menu easily from elsewhere in the code */

HWND menucreatestub(char *addthismenu, HWND location, int pulldown)
{
	MenuStruct *menutoadd;

	if((menutoadd = (MenuStruct *)findmenu(addthismenu))==NULL)
	{
		say("Menu not found.");
		return 0;
	}

	if(!menutoadd->menuorigin)
	{
		say("Cannot create blank menu.");
		return 0;
	}
	if(menutoadd->sharedhandle && WinIsWindow(hab, menutoadd->sharedhandle))
		return menutoadd->sharedhandle;
	else
        return newsubmenu(menutoadd, location, pulldown);
}

void detach_shared(Screen *this_screen)
{
	int itemid = (int)WinSendMsg(this_screen->hwndMenu, MM_ITEMIDFROMPOSITION, (MPARAM)0, (MPARAM)NULL);
	/* Set any owners of shared menus to HWND_OBJECT so the won't get destroyed - thanks mikh */
	if(this_screen->menu && WinIsWindow(hab, this_screen->hwndMenu))
	{
		MenuStruct *tmpmenu = findmenu(this_screen->menu);

        if(tmpmenu)
	{
		MenuList *current = tmpmenu->menuorigin;

		if(tmpmenu->sharedhandle && WinIsWindow(hab, tmpmenu->sharedhandle))
			WinSetOwner(tmpmenu->sharedhandle, HWND_OBJECT);
		else
		{
			while(current)
			{
				MenuStruct *submenu;

				if((current->menutype & GUISHARED) && (submenu = findmenu(current->submenu)))
				{
					if(submenu->sharedhandle && WinIsWindow(hab, submenu->sharedhandle))
						WinSetOwner(submenu->sharedhandle, HWND_OBJECT);
				}
				current = current->next;
			}
		}
	}
	}
	while(itemid != -1)
	{
		/* This may cause memory leaks... unless they are all shared  (menu.bx is) */
		WinSendMsg(this_screen->hwndMenu, MM_REMOVEITEM, MPFROM2SHORT(itemid, FALSE), NULL);
		itemid = (int)WinSendMsg(this_screen->hwndMenu, MM_ITEMIDFROMPOSITION, (MPARAM)0, (MPARAM)NULL);
	}
}

void window_menu_stub(Screen *menuscreen, char *addthismenu)
{
	int menustate=0, newmenustate=0;

	tmpmenuid=3000;

	if(!menuscreen)
		return;

	if(menuscreen->hwndMenu && WinIsWindow(hab, menuscreen->hwndMenu))
	{
		menustate=1;
		detach_shared(menuscreen);
		WinDestroyWindow(menuscreen->hwndMenu);
        menuscreen->hwndMenu = NULLHANDLE;
		new_free(&menuscreen->menu);
	}

	/* Create toplevel menu */

	aflush();
	if(addthismenu && strcmp(addthismenu, "-delete")!=0)
	{
		if((menuscreen->hwndMenu=(HWND)menucreatestub(addthismenu, menuscreen->hwndFrame, TRUE))!=(HWND)NULL)
		newmenustate=1;
	}
	else
        menuscreen->hwndMenu=(HWND)NULL;

	if(newmenustate==1)
		menuscreen->menu=m_strdup(addthismenu);
	else
		menuscreen->menu=NULL;

	/* Tell the Window to resize because of menu */
	WinSendMsg(menuscreen->hwndFrame, WM_UPDATEFRAME, 0, 0);

	if(menustate != newmenustate)   
		WinSendMsg(menuscreen->hwndFrame, WM_USER, 0, 0);
	new_free(&addthismenu);
}

void pm_seticon(Screen *screen)
{
	static HPOINTER icon = 0;

	if(!screen)
		return;

	/* For some reason the icon gets unset when spawning */
    if(!icon)
		icon = WinLoadPointer(HWND_DESKTOP,NULLHANDLE,IDM_MAINMENU);

	WinSendMsg(screen->hwndFrame, WM_SETICON, (MPARAM)icon, 0);
}

void pm_mdi_on(void)
{
	Screen *tmp = screen_list;
	HSWITCH hswitch;
	SWCNTRL swcntrl;
	SWP swpw, swpf;
	PID pid;
	ULONG width, height, x, y;

	WinSetParent(MDIFrame, HWND_DESKTOP, TRUE);

	WinQueryWindowProcess(MDIFrame, &pid, NULL);
 
	/* initialize switch structure */
	swcntrl.hwnd = MDIFrame;
	swcntrl.hwndIcon = NULLHANDLE;
	swcntrl.hprog = NULLHANDLE;
	swcntrl.idProcess = pid;
	swcntrl.idSession = 0;
	swcntrl.uchVisibility = SWL_VISIBLE;
	swcntrl.fbJump = SWL_JUMPABLE;
	strcpy(swcntrl.szSwtitle,irc_version);
 
	hswitch = WinCreateSwitchEntry(mainhab, &swcntrl);

	width = (ULONG)((((float)WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN)))*0.75);
	height = (ULONG)((((float)WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN)))*0.75);
	x = (ULONG)((WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN)-width)/2);
	y = (ULONG)((WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN)-height)/2);

	WinSetWindowPos(MDIFrame, 0, x, y, width, height, SWP_MOVE | SWP_SIZE | SWP_SHOW);
	WinShowWindow(MDIFrame, TRUE);

	WinQueryWindowPos(MDIClient, &swpf);
	WinSetWindowULong(MDIFrame, QWP_USER, swpf.cy);

	while(tmp)
	{

		if(tmp->alive)
		{
			WinQueryWindowPos(tmp->hwndFrame, &swpw);
			hswitch = WinQuerySwitchHandle(tmp->hwndFrame, 0);
			if(hswitch)
			{
				WinQuerySwitchEntry(hswitch, &swcntrl);
				swcntrl.uchVisibility = SWL_INVISIBLE;
				WinChangeSwitchEntry(hswitch, &swcntrl);
			}
			WinSetParent(tmp->hwndFrame, MDIClient, TRUE);
			WinSetWindowPos(tmp->hwndFrame, NULLHANDLE, swpw.x, swpw.y - (WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN) - swpf.cy), 0, 0, SWP_MOVE);
		}
		tmp = tmp->next;
	}
}

void pm_mdi_off(void)
{
	Screen *tmp = screen_list;
	HSWITCH hswitch;
	SWCNTRL swcntrl;
	SWP swpw, swpf;

	WinQueryWindowPos(MDIClient, &swpf);

	hswitch = WinQuerySwitchHandle(MDIFrame, 0);
	if(hswitch)
		WinRemoveSwitchEntry(hswitch);

	while(tmp)
	{
		if(tmp->alive)
		{
			WinQueryWindowPos(tmp->hwndFrame, &swpw);
			WinSetParent(tmp->hwndFrame, HWND_DESKTOP, TRUE);
			WinSetWindowPos(tmp->hwndFrame, NULLHANDLE, swpw.x, swpw.y + (WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN) - swpf.cy), 0, 0, SWP_MOVE);
			hswitch = WinQuerySwitchHandle(tmp->hwndFrame, 0);
			if(hswitch)
			{
				WinQuerySwitchEntry(hswitch, &swcntrl);
				swcntrl.uchVisibility = SWL_VISIBLE;
				WinChangeSwitchEntry(hswitch, &swcntrl);
			}
		}
		tmp = tmp->next;
	}

	WinShowWindow(MDIFrame, FALSE);
	WinSetParent(MDIFrame, HWND_OBJECT, TRUE);
}

int test_callback(HWND window, void *data)
{
	dw_window_destroy((HWND)data);
	return -1;
}

void pm_aboutbox(char *about_text)
{
	HWND mainwindow, text, stext, logo,
	     okbutton, buttonbox, lbbox;

	ULONG flStyle = FCF_SYSMENU | FCF_TITLEBAR |
		FCF_SHELLPOSITION | FCF_TASKLIST | FCF_DLGBORDER | FCF_SIZEBORDER | FCF_MINMAX;

	mainwindow = dw_window_new(HWND_DESKTOP, "About PMBitchX", flStyle);

	dw_window_set_icon(mainwindow, IDM_MAINMENU);

	lbbox = dw_box_new(BOXVERT, 10);

	dw_box_pack_start(mainwindow, lbbox, 0, 0, TRUE, TRUE, 0);

	stext = dw_text_new("PMBitchX (c) 2002 Colten Edwards, Brian Smith", 0);

	dw_window_set_style(stext, DT_VCENTER, DT_VCENTER);
	dw_window_set_style(stext, DT_CENTER, DT_CENTER);

	dw_box_pack_start(lbbox, stext, 130, 20, TRUE, FALSE, 10);

	buttonbox = dw_box_new(BOXHORZ, 0);

	dw_box_pack_start(lbbox, buttonbox, 0, 0, TRUE, FALSE, 0);

	dw_window_set_color(buttonbox, CLR_PALEGRAY, CLR_PALEGRAY);

	logo = dw_bitmap_new(0);

	dw_window_set_bitmap(logo, IDM_LOGO);

    dw_box_pack_start(buttonbox, 0, 52, 52, TRUE, FALSE, 5);
	dw_box_pack_start(buttonbox, logo, 52, 52, FALSE, FALSE, 5);
    dw_box_pack_start(buttonbox, 0, 52, 52, TRUE, FALSE, 5);

	text = dw_text_new(about_text, 0);

	dw_window_set_style(text, DT_CENTER, DT_CENTER);
	dw_window_set_style(text, DT_WORDBREAK, DT_WORDBREAK);

	dw_box_pack_start(lbbox, text, 130, 120, TRUE, TRUE, 10);

	buttonbox = dw_box_new(BOXHORZ, 0);

	dw_box_pack_start(lbbox, buttonbox, 0, 0, TRUE, FALSE, 0);

	okbutton = dw_button_new("Ok", 1001L);

    dw_box_pack_start(buttonbox, 0, 50, 30, TRUE, FALSE, 5);
	dw_box_pack_start(buttonbox, okbutton, 50, 30, FALSE, FALSE, 5);
    dw_box_pack_start(buttonbox, 0, 50, 30, TRUE, FALSE, 5);

	dw_window_set_usize(mainwindow, 435, 340);

	dw_window_show(mainwindow);

	dw_signal_connect(okbutton, "clicked", DW_SIGNAL_FUNC(test_callback), (void *)mainwindow);
	dw_signal_connect(mainwindow, "delete_event", DW_SIGNAL_FUNC(test_callback), (void *)mainwindow);
}

void pm_resize(Screen *this_screen)
{
	if(this_screen->co < 20) this_screen->old_co=this_screen->co=20;
	if(this_screen->li < 10) this_screen->old_li=this_screen->li=10;
	if(this_screen->co > 199) this_screen->old_co=this_screen->co=199;
	if(this_screen->li > 99) this_screen->old_li=this_screen->li=99;
	cx = this_screen->co * this_screen->VIO_font_width;
	cy = this_screen->li * this_screen->VIO_font_height;

	co = this_screen->co; li = this_screen->li;

	/* Recalculate some stuff that was done in input.c previously */
	this_screen->input_line = this_screen->li-1;

	this_screen->input_zone_len = this_screen->co - (WIDTH * 2);
	if (this_screen->input_zone_len < 10)
		this_screen->input_zone_len = 10;		/* Take that! */

	this_screen->input_start_zone = WIDTH;
	this_screen->input_end_zone = this_screen->co - WIDTH;
}

void drawnicklist(Screen *this_screen)
{
	NickList *n, *list = NULL;
	ChannelList *cptr;
	char minibuffer[NICKNAME_LEN+1], spaces[100];
	int nickcount=0, k;

	if(!this_screen)
		return;

	memset(spaces, ' ', 100);

	if(this_screen->current_window && this_screen->current_window->current_channel)
	{
		cptr = lookup_channel(this_screen->current_window->current_channel, this_screen->current_window->server, 0);
		list = sorted_nicklist(cptr, get_int_var(NICKLIST_SORT_VAR));

		if(this_screen->spos<0)
			this_screen->spos=0;
		for(n = list; n; n = n->next)
		{
			if(nickcount >= this_screen->spos && nickcount < this_screen->spos + this_screen->li)
			{
				minibuffer[0]=' ';
				minibuffer[1]='\0';
				if(nick_isvoice(n))
					strcpy(minibuffer, "+");
				if(nick_isop(n))
					strcpy(minibuffer, "@");
				if(nick_isircop(n))
				{
					if(minibuffer[0] == ' ')
						strcpy(minibuffer, "*");
					else
						strcat(minibuffer, "*");
				}
				strcat(minibuffer, n->nick);
				for(k=strlen(minibuffer);k<(this_screen->nicklist+1);k++)
					minibuffer[k]=' ';
				minibuffer[k]='\0';
				VioSetCurPos(nickcount-this_screen->spos, 0, this_screen->hvpsnick);
				if(this_screen->mpos == nickcount)
					VioWrtTTY("\e[0;1;37;44m", 12, this_screen->hvpsnick);
				VioWrtTTY(minibuffer, strlen(minibuffer), this_screen->hvpsnick);
				VioWrtTTY("\e[0m", 4, this_screen->hvpsnick);
			}
			nickcount++;
		}
	}

	if(nickcount < this_screen->li)
	{
		for(k=nickcount;k<this_screen->li;k++)
		{
			VioSetCurPos(k, 0, this_screen->hvpsnick);
			VioWrtTTY(spaces, NICKNAME_LEN, this_screen->hvpsnick);
		}
	}
	else if (this_screen->spos > nickcount-this_screen->li)
	{
		this_screen->spos=nickcount-this_screen->li;
		drawnicklist(this_screen);
	}

	WinSendMsg(this_screen->hwndnickscroll, SBM_SETSCROLLBAR, (MPARAM)this_screen->spos, MPFROM2SHORT(0, nickcount - this_screen->li));
	WinSendMsg(this_screen->hwndnickscroll, SBM_SETTHUMBSIZE, MPFROM2SHORT(this_screen->li, nickcount), (MPARAM)NULL);
    if(list)
		clear_sorted_nicklist(&list);
}

void pm_focus(char key)
{
	int count = 1, page = (int)(key - '0');
    Screen *tmp = screen_list;

	if(key == '0')
		page = 10;

	while(tmp)
	{
		if(count == page)
			WinSetFocus(HWND_DESKTOP, tmp->hwndClient);

		count++;
		tmp = tmp->next;
	}
}

MRESULT EXPENTRY FrameWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
	PMYWINDATA wd;
	HPS hps;
	Screen *this_screen = NULL;

	wd = (PMYWINDATA)WinQueryWindowPtr(hwnd, QWP_USER);

    if(wd)
		this_screen = wd->screen;

	switch (msg) {
	case WM_BUTTON1MOTIONSTART:
	case WM_BUTTON2MOTIONSTART:
	case WM_BUTTON1UP:
	case WM_BUTTON2UP:
	case WM_BUTTON1DOWN:
	case WM_BUTTON2DOWN:
	case WM_SETFOCUS:
	case WM_COMMAND:
	case WM_CONTEXTMENU:
	case WM_CHAR:
		{
			if(this_screen)
				return GenericWndProc(this_screen->hwndClient, msg, mp1, mp2);
			else
				return NULL;
		}
		break;
	case WM_PAINT:
			hps = WinBeginPaint(hwnd, hvps, NULL);
			if (this_screen)
			{
					VioShowPS(this_screen->li+1,
							  this_screen->co+1,
							  0,
							  this_screen->hvps);
					if(this_screen->nicklist)
							VioShowPS(this_screen->li+1,
									  this_screen->nicklist + 1,
									  0,
									  this_screen->hvpsnick);
			}
			WinEndPaint(hps);
			return (WinDefAVioWindowProc(hwnd, msg, (ULONG)mp1, (ULONG)mp2));
	}
	return (MRESULT)WinDefWindowProc(hwnd, msg, mp1, mp2);
}

MRESULT EXPENTRY GenericWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{

	HPS   hps;
	int tmp, statusstart;
	SHORT x, y, i;
	SWP swp, menupos;
	static HVPS hvps;
	Screen *this_screen = NULL;
	PMYWINDATA wd;
	static int sx, sy, tx, ty;
	static unsigned char *mark = NULL;
	ULONG len;
	POINTL ptl;
	static PVOID shared;
	TRACKINFO ti;
	APIRET rc;
	char *selectionbuffer;
	VIOCURSORINFO bleah;

	wd = (PMYWINDATA)WinQueryWindowPtr(hwnd, QWP_USER);
	if (!wd)
		wd = (PMYWINDATA)WinQueryWindowPtr(WinWindowFromID(hwnd, FID_CLIENT), QWP_USER);

	if (wd)
	{
		hvps = wd->hvps;
		this_screen = wd->screen;
		if (!this_screen) { wd->screen = main_screen; this_screen = wd->screen; }
	}

	switch (msg) {
		/* rosmo's awesome copy functions */
	case WM_BUTTON1MOTIONSTART:
		if(this_screen)
	{
		WinQueryPointerPos(HWND_DESKTOP, &ptl);
		WinMapWindowPoints(HWND_DESKTOP, hwnd, &ptl, 1);

		WinQueryWindowPos(hwnd, &swp);
		ti.rclTrack.xLeft   = ti.rclTrack.xRight = (int)(((int)(ptl.x/this_screen->VIO_font_width))*this_screen->VIO_font_width)+1;
		ti.rclTrack.yBottom = ti.rclTrack.yTop   = (int)(((int)(ptl.y/this_screen->VIO_font_height)+1)*this_screen->VIO_font_height)+1;
		ti.rclTrack.xLeft--;
		ti.rclTrack.yBottom--;

		WinQueryWindowRect(this_screen->hwndLeft, &ti.rclBoundary);
		ti.cxBorder	= ti.cyBorder = 1;
		ti.cxGrid 	= this_screen->VIO_font_width;
		ti.cyGrid 	= this_screen->VIO_font_height;
		ti.cxKeyboard = 0;
		ti.cyKeyboard = 0;
		ti.ptlMinTrackSize.x = 0;
		ti.ptlMinTrackSize.y = 0;
		ti.ptlMaxTrackSize.x = swp.cx + this_screen->VIO_font_width;
		ti.ptlMaxTrackSize.y = swp.cy + this_screen->VIO_font_height;
		ti.fs = TF_BOTTOM | TF_RIGHT;

		/* Allocate a buffer to store the entire screen so it won't change
		 during selection - NuKe */

		selectionbuffer = new_malloc((this_screen->li * this_screen->co) + 1);

		for(i=0;i<this_screen->li;i++)
			VioReadCharStr(&selectionbuffer[i*this_screen->co], (PUSHORT)&this_screen->co, i, 0, hvps);

		if (WinTrackRect(hwnd, NULLHANDLE, &ti))
		{
			sx = (int)(ti.rclTrack.xLeft/this_screen->VIO_font_width);
			ty = this_screen->li - (int)(ti.rclTrack.yBottom/this_screen->VIO_font_height);
			tx = (int)(ti.rclTrack.xRight/this_screen->VIO_font_width);
			sy = this_screen->li - (int)(ti.rclTrack.yTop/this_screen->VIO_font_height);

			mark = new_malloc((ty - sy) * (tx - sx + 2) + 1); i = 0;
			for (y = sy; y < ty; y++)
			{
				len = tx - sx;
				memcpy(&mark[i], &selectionbuffer[(y * this_screen->co) + sx], len);
				i += len - 1;
				/* get rid of trailing white space */
				while(mark[i] == ' ' && i > 0)
					i--;
				mark[i + 1] = '\r';
				mark[i + 2] = '\n';
				i += 2;
			}


			mark[i] = '\0';

			/* Ok, we got the stuff - now place it in the clipboard */
			WinOpenClipbrd(hab); /* Open clipboard */
			WinEmptyClipbrd(hab); /* Empty clipboard */

			/* Ok, clipboard wants giveable unnamed shared memory */

			shared = NULL;
			rc = DosAllocSharedMem(&shared,
					       NULL,
					       i,
					       OBJ_GIVEABLE | PAG_COMMIT | PAG_READ | PAG_WRITE);

			if (rc == 0)
			{
				memcpy(shared, mark, i);

				WinSetClipbrdData(hab,
						  (ULONG)shared,
						  CF_TEXT,
						  CFI_POINTER);
			}

			WinCloseClipbrd(hab); /* Close clipboard */

			new_free(&selectionbuffer);
			new_free(&mark);
		}
	}
		break;
	case WM_SETFOCUS:
        if (SHORT1FROMMP(mp2)==TRUE)
	{
		/* If we have a shared menu attached to the window make it owned by the
		 * current window so it opens properly
		 */
		cursor_screen = this_screen;

		if(this_screen && this_screen->current_window)
		{
			if(this_screen->menu)
			{
				MenuStruct *tmpmenu = findmenu(this_screen->menu);

				if(tmpmenu)
				{
					MenuList *current = tmpmenu->menuorigin;

					if(tmpmenu->sharedhandle && WinIsWindow(hab, tmpmenu->sharedhandle))
						WinSetOwner(tmpmenu->sharedhandle, this_screen->hwndClient);
					else
					{
						while(current)
						{
							HWND hwndmenu;
							MenuStruct *submenu;

							if((current->menutype & GUISHARED) && (submenu = findmenu(current->submenu)))
							{
								if(submenu->sharedhandle && WinIsWindow(hab, submenu->sharedhandle) && (hwndmenu = WinWindowFromID(this_screen->hwndFrame, FID_MENU)))
									WinSetOwner(submenu->sharedhandle, hwndmenu);
							}
							current = current->next;
						}
					}
				}
			}
            WinSetCp(hmq, this_screen->codepage);
            WinSetCp(mainhmq, this_screen->codepage);
			/* Tell the main thread to set this window as current */
			sendevent(EVFOCUS, this_screen->current_window->refnum);
		}
	}
	else
	{
		cursor_screen = NULL;
		if(this_screen)
		{
			VioGetCurType(&bleah, this_screen->hvps);
			bleah.attr = -1;
			VioSetCurType(&bleah, this_screen->hvps);
		}
	}
	break;
	case WM_COMMAND:
		menucmd=COMMANDMSG(&msg)->cmd;
		sendevent(EVMENU, this_screen->current_window->refnum);
		return((MRESULT)0);
		break;
	case WM_CLOSE:
#ifdef WINDOW_CREATE
		kill_screen(this_screen);
#else
		irc_exit(1, NULL, "%s rocks your world!", VERSION);
#endif
		break;
	case WM_PAINT:
		hps = WinBeginPaint(hwnd, hvps, NULL);
		if (this_screen)
		{
			VioShowPS(this_screen->li+1,
				  this_screen->co+1,
				  0,
				  this_screen->hvps);
                        if(this_screen->nicklist)
				VioShowPS(this_screen->li+1,
					  this_screen->nicklist + 1,
					  0,
					  this_screen->hvpsnick);
		}
		WinEndPaint(hps);
		return (WinDefAVioWindowProc(hwnd, msg, (ULONG)mp1, (ULONG)mp2));
	case WM_CHAR:

		if((SHORT1FROMMP(mp1) & KC_KEYUP)) break;
		if(strlen(this_screen->aviokbdbuffer)==254) {
			DosBeep(1000, 200);
				 } else {
			if(SHORT1FROMMP(mp2) != 0) {
				tmp=strlen(this_screen->aviokbdbuffer);
				if (CHAR1FROMMP(mp2) == 224) {
					this_screen->aviokbdbuffer[tmp]='\e';
					this_screen->aviokbdbuffer[tmp+1]=CHAR2FROMMP(mp2);
					this_screen->aviokbdbuffer[tmp+2]=0;
				} else {
					/* Fake a SIGINT on CTRL-C */
#if 0
					if((SHORT1FROMMP(mp1) & KC_CTRL) && SHORT1FROMMP(mp2) == 99)
						raise(SIGINT);
#endif
					if((SHORT1FROMMP(mp1) & KC_ALT) && SHORT1FROMMP(mp2) >= '0' && SHORT1FROMMP(mp2) <= '9')
						pm_focus((char)SHORT1FROMMP(mp2));
					else if ((SHORT1FROMMP(mp1) & KC_CTRL) && (SHORT1FROMMP(mp2) > 96) && (SHORT1FROMMP(mp2) < 122))
						this_screen->aviokbdbuffer[tmp]=SHORT1FROMMP(mp2)-96;
					else
						this_screen->aviokbdbuffer[tmp]=SHORT1FROMMP(mp2);
					this_screen->aviokbdbuffer[tmp+1]=0;
				}
			}
				 }
		/* Force select() to exit */
		sendevent(EVNONE, 0);
		break;

	case WM_BUTTON1UP:
	case WM_BUTTON2UP:
	case WM_BUTTON3UP:
	case WM_BUTTON1DBLCLK:
	case WM_BUTTON2DBLCLK:
	case WM_BUTTON3DBLCLK:
		{
			/* Save the server, and restore it when we are done. */
			int save_server = from_server;

			WinQueryPointerPos(HWND_DESKTOP, &ptl);
			WinMapWindowPoints(HWND_DESKTOP, this_screen->hwndClient, &ptl, 1);
			contextx=ptl.x;contexty=ptl.y;
			lastclickrow=this_screen->li - ((int)(ptl.y/this_screen->VIO_font_height)) - 1;
			lastclickcol=((int)(ptl.x/this_screen->VIO_font_width));
			last_input_screen=output_screen=this_screen;
			make_window_current(this_screen->current_window);

			from_server = current_window->server;

			if(this_screen->nicklist && lastclickcol > this_screen->co)
			{
				USHORT len = this_screen->nicklist + 1;

				lastclickcol = 0;
				if(lastclicklinedata != NULL)
				{
					VioReadCharStr((PCH)lastclicklinedata,(PUSHORT)&len,lastclickrow,0,this_screen->hvpsnick);
					lastclicklinedata[len] = 0;
				}
				if(lastclicklinedata[0] == ' ')
					memmove(lastclicklinedata, &lastclicklinedata[1], strlen(lastclicklinedata));

				this_screen->mpos = lastclickrow + this_screen->spos;
				drawnicklist(this_screen);

				switch(msg)
				{
				case WM_BUTTON1UP:
					wm_process(NICKLISTLCLICK);
					break;
				case WM_BUTTON2UP:
					wm_process(NICKLISTRCLICK);
					break;
				case WM_BUTTON3UP:
					wm_process(NICKLISTMCLICK);
					break;
				case WM_BUTTON1DBLCLK:
					wm_process(NICKLISTLDBLCLICK);
					break;
				case WM_BUTTON2DBLCLK:
					wm_process(NICKLISTRDBLCLICK);
					break;
				case WM_BUTTON3DBLCLK:
					wm_process(NICKLISTMDBLCLICK);
					break;
				}

				from_server = save_server;

				return NULL;
			}

			if(lastclicklinedata != NULL)
				VioReadCharStr((PCH)lastclicklinedata,(PUSHORT)&this_screen->co,lastclickrow,0,this_screen->hvps);
			statusstart=this_screen->li - 2;

			/* I was hoping there was an easier way but....I gave up */

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
				switch(msg)
				{
				case WM_BUTTON1UP:
					wm_process(STATUSLCLICK);
					break;
				case WM_BUTTON2UP:
					wm_process(STATUSRCLICK);
					break;
				case WM_BUTTON3UP:
					wm_process(STATUSMCLICK);
					break;
				case WM_BUTTON1DBLCLK:
					wm_process(STATUSLDBLCLICK);
					break;
				case WM_BUTTON2DBLCLK:
					wm_process(STATUSRDBLCLICK);
					break;
				case WM_BUTTON3DBLCLK:
					wm_process(STATUSMDBLCLICK);
					break;
				}
			}
			else
			{
				switch(msg)
				{
				case WM_BUTTON1UP:
					wm_process(LCLICK);
					break;
				case WM_BUTTON2UP:
					wm_process(RCLICK);
					break;
				case WM_BUTTON3UP:
					wm_process(MCLICK);
					break;
				case WM_BUTTON1DBLCLK:
					wm_process(LDBLCLICK);
					break;
				case WM_BUTTON2DBLCLK:
					wm_process(RDBLCLICK);
					break;
				case WM_BUTTON3DBLCLK:
					wm_process(MDBLCLICK);
					break;
				}
			}

			from_server = save_server;
		}
		break;
	case WM_VSCROLL:
		if(SHORT1FROMMP(mp1) == 1000)
		{   /* Channel is being scrolled */
			switch(SHORT2FROMMP(mp2))
			{
			case SB_LINEUP:
				sendevent(EVSUP, this_screen->current_window->refnum);
				break;
			case SB_LINEDOWN:
				sendevent(EVSDOWN, this_screen->current_window->refnum);
				break;
			case SB_PAGEUP:
				sendevent(EVSUPPG, this_screen->current_window->refnum);
				break;
			case SB_PAGEDOWN:
                        sendevent(EVSDOWNPG, this_screen->current_window->refnum);
			break;
			case SB_SLIDERTRACK:
				newscrollerpos=SHORT1FROMMP(mp2);
				if(newscrollerpos<0)
					newscrollerpos=0;
				if(newscrollerpos>get_int_var(SCROLLBACK_VAR))
					newscrollerpos=get_int_var(SCROLLBACK_VAR);
				sendevent(EVSTRACK, this_screen->current_window->refnum);
				break;
			}
		} else if(SHORT1FROMMP(mp1) == 1001)
		{   /* Nicklist is scrolling */
			switch(SHORT2FROMMP(mp2))
			{
			case SB_LINEUP:
				this_screen->spos--;
				break;
			case SB_LINEDOWN:
				this_screen->spos++;
				break;
			case SB_PAGEUP:
				this_screen->spos -= this_screen->li;
				break;
			case SB_PAGEDOWN:
				this_screen->spos += this_screen->li;
				break;
			case SB_SLIDERPOSITION:
			case SB_SLIDERTRACK:
				this_screen->spos=SHORT1FROMMP(mp2);
				break;
			}
			drawnicklist(this_screen);
		}
		break;
	case WM_USER:
		dont_resize=1;
		if(this_screen->hwndMenu==(HWND)NULL)
		{
			WinSetWindowPos(this_screen->hwndFrame, HWND_TOP, 0, 0, (this_screen->VIO_font_width * this_screen->nicklist) + cx + (cxsb*2) + (cxvs+(this_screen->nicklist ? cxvs : 0)), cy + (cysb*2) + cytb + 1, SWP_SIZE);
		}
		else
		{
			WinQueryWindowPos(this_screen->hwndMenu, &menupos);
			if(menupos.cy<3)
				WinSetWindowPos(this_screen->hwndFrame, HWND_TOP, 0, 0, (this_screen->VIO_font_width * this_screen->nicklist) + cx + (cxvs+(this_screen->nicklist ? cxvs : 0)) + (cxsb*2), cy + (cysb*2)+cytb + WinQuerySysValue(HWND_DESKTOP, SV_CYMENU), SWP_SIZE);
			else
				WinSetWindowPos(this_screen->hwndFrame, HWND_TOP, 0, 0, (this_screen->VIO_font_width * this_screen->nicklist) + cx + (cxvs+(this_screen->nicklist ? cxvs : 0)) + (cxsb*2), cy + (cysb*2)+cytb + menupos.cy, SWP_SIZE);
		}
		drawnicklist(this_screen);
		sendevent(EVREFRESH, this_screen->current_window->refnum);
		break;
	case WM_USER+2:
		pm_new_window((Screen *)mp1, (Window *)mp2);
		break;
	case WM_USER+3:
		if(mp1)
			pm_mdi_on();
		else
			pm_mdi_off();
		break;
	case WM_USER+4:
		gui_kill_window((Screen *)mp1);
		break;
	case WM_USER+5:
        pm_aboutbox((char *)mp1);
        break;
	case WM_USER+6:
		dont_resize=1;
		window_menu_stub((Screen *)mp1, (char *)mp2);
		break;
	case 0x041e:
		if(just_resized == 1 && this_screen == just_resized_screen)
		{
			WinPostMsg(hwnd, WM_USER, 0, 0);
			just_resized=0;
		}
		break;
	case 0x041f:
		if(just_resized == 1 && this_screen == just_resized_screen)
		{
			WinPostMsg(hwnd, WM_USER, 0, 0);
			  just_resized=0;
		}
		break;
	case WM_MINMAXFRAME:
		{
			PSWP pSwp = PVOIDFROMMP(mp1);

			if(pSwp->fl & SWP_MAXIMIZE)
			{
				dont_resize=1;
				WinPostMsg(hwnd, WM_USER, 0, 0);
			}
		}
		break;
	case WM_SIZE:
		if (!this_screen) return((MRESULT)0);

		x=SHORT1FROMMP(mp2); y=SHORT2FROMMP(mp2);
		if(x && y) {
			int nx, ny, ncx, ncy;
			HDC hdc;

			this_screen->old_co = this_screen->co=((int)((x-(cxvs+(this_screen->nicklist ? cxvs : 0)))/this_screen->VIO_font_width)) - this_screen->nicklist;
			this_screen->old_li = this_screen->li=((int)(y/this_screen->VIO_font_height));

			nx = 0;
			ny = 0;
			ncx = (x-(cxvs+(this_screen->nicklist ? cxvs : 0))) - (this_screen->nicklist * this_screen->VIO_font_width);
			ncy = y;


			VioAssociate((HDC)NULL, this_screen->hvps);
			WinSetWindowPos(this_screen->hwndLeft, NULLHANDLE, nx, ny-((this_screen->VIO_font_height*VIO_staticy)-ncy), ncx, ncy+abs((this_screen->VIO_font_height*VIO_staticy)-ncy), SWP_MOVE | SWP_SIZE);
			hdc = WinOpenWindowDC(this_screen->hwndLeft);
			VioAssociate(hdc, this_screen->hvps);

			nx += ncx;
			ncx = cxvs;

			WinSetWindowPos(this_screen->hwndscroll, NULLHANDLE, nx, ny, ncx, ncy, SWP_MOVE | SWP_SIZE);

			if(this_screen->nicklist)
			{
				nx += ncx;
				ncx = this_screen->nicklist * this_screen->VIO_font_width;

				VioAssociate((HDC)NULL, this_screen->hvpsnick);
				WinSetWindowPos(this_screen->hwndRight, NULLHANDLE, nx, ny-((this_screen->VIO_font_height*VIO_staticy)-ncy), ncx, ncy+abs((this_screen->VIO_font_height*VIO_staticy)-ncy), SWP_MOVE | SWP_SIZE);
				hdc = WinOpenWindowDC(this_screen->hwndRight);
				VioAssociate(hdc, this_screen->hvpsnick);

				nx += ncx;
				ncx =  cxvs;

				WinSetWindowPos(this_screen->hwndnickscroll, NULLHANDLE, nx, ny, ncx, ncy, SWP_MOVE | SWP_SIZE);
			}

			pm_resize(this_screen);

			recalculate_windows(this_screen);
			make_window_current(this_screen->current_window);
			if(dont_resize == 1)
				dont_resize = 0;
			else
			{
				just_resized_screen=this_screen;
				just_resized=1;
			}
		}
		WinDefAVioWindowProc(hwnd, msg, (ULONG)mp1, (ULONG)mp2);
		break;
	}
	return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

MRESULT EXPENTRY MDIWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
	switch( msg )
	{
	case WM_PAINT:
		{
		HPS    hps;
		RECTL  rc;

		hps = WinBeginPaint( hwnd, 0L, &rc );
		WinFillRect(hps, &rc, CLR_DARKGRAY);
		WinEndPaint( hps );
		break;
		}
	case WM_SIZE:
		{
			Screen *tmp = screen_list;
			ULONG oldheight = WinQueryWindowULong(MDIFrame, QWP_USER);

			if(!SHORT1FROMMP(mp2) && !SHORT2FROMMP(mp2))
				return NULL;

			if(oldheight && (oldheight-SHORT2FROMMP(mp2)))
			{
				while(tmp)
				{
					if(tmp->alive)
					{
						SWP swp;

						WinQueryWindowPos(tmp->hwndFrame, &swp);
						WinSetWindowPos(tmp->hwndFrame,0, swp.x, swp.y - (oldheight-SHORT2FROMMP(mp2)), 0, 0, SWP_MOVE);
					}
					tmp = tmp->next;
				}
			}
			WinSetWindowULong(MDIFrame, QWP_USER, SHORT2FROMMP(mp2));
		}
		break;
	case WM_CLOSE:
		irc_exit(1, NULL, "%s exiting", VERSION);
		break;
	default:
		return WinDefWindowProc( hwnd, msg, mp1, mp2 );
	}
	return (MRESULT)FALSE;
}

void avio_exit(void) {
	VioAssociate((HDC)NULL, (output_screen ? output_screen->hvps : hvps));
	VioDestroyPS((output_screen ? output_screen->hvps : hvps));
	WinDestroyWindow(hwndFrame);
	WinDestroyMsgQueue(hmq);
	WinTerminate(hab);
	DosExit(EXIT_PROCESS, 0);
}

void avio_init() {

	HEV	 mySem = sem;
	PMYWINDATA mywindata;

	ULONG flStyle = FCF_MINMAX | FCF_SYSMENU | FCF_TITLEBAR |
		FCF_SIZEBORDER | FCF_SHELLPOSITION | FCF_TASKLIST | FCF_ICON;

	pmthread = *_threadid;

	DosOpenEventSem(NULL, (PHEV)&mySem);

	hab = WinInitialize(0);
	hmq = WinCreateMsgQueue(hab, 0);

	cxvs = WinQuerySysValue(HWND_DESKTOP, SV_CXVSCROLL);
	cxsb = WinQuerySysValue(HWND_DESKTOP, SV_CXSIZEBORDER);
	cysb = WinQuerySysValue(HWND_DESKTOP, SV_CYSIZEBORDER);
	cytb = WinQuerySysValue(HWND_DESKTOP, SV_CYTITLEBAR);

	if (!WinRegisterClass(hab, "AVIO", GenericWndProc, CS_SIZEREDRAW, 32))
		DosExit(EXIT_PROCESS, 1);
	WinRegisterClass(hab, "BXMDI", MDIWndProc, CS_SIZEREDRAW | CS_CLIPCHILDREN, 0);

	MDIFrame = WinCreateStdWindow(HWND_DESKTOP,
				      WS_CLIPCHILDREN,
				      &flStyle,
				      "BXMDI",
				      irc_version,
				      WS_CLIPCHILDREN,
				      NULLHANDLE,
				      IDM_MAINMENU,
				      &MDIClient);

	hwndFrame = WinCreateStdWindow(HWND_DESKTOP,
				       0,
				       &flStyle,
				       "AVIO",
				       irc_version,
				       0L,
				       NULLHANDLE,
				       IDM_MAINMENU,
				       &hwndClient);

	hwndLeft = WinCreateWindow(hwndClient,
				   WC_FRAME,
				   NULL,
				   WS_VISIBLE,
				   0,0,2000,1000,
				   hwndClient,
				   HWND_TOP,
				   0L,
				   NULL,
				   NULL);

	hwndRight = WinCreateWindow(hwndClient,
				    WC_FRAME,
				    NULL,
				    WS_VISIBLE,
				    0,0,2000,1000,
				    hwndClient,
				    HWND_TOP,
				    0L,
				    NULL,
				    NULL);

	hwndscroll = WinCreateWindow(hwndClient,
				     WC_SCROLLBAR,
				     NULL,
				     WS_VISIBLE | SBS_VERT,
				     0,0,100,100,
				     hwndClient,
				     HWND_TOP,
				     1000L,
				     NULL,
				     NULL);
	hwndnickscroll = WinCreateWindow(hwndClient,
					 WC_SCROLLBAR,
					 NULL,
					 WS_VISIBLE | SBS_VERT,
					 0,0,100,100,
					 hwndClient,
					 HWND_TOP,
					 1001L,
					 NULL,
					 NULL);
	hwndMenu=(HWND)NULL;

	current_term->TI_cols = co = 81;
	current_term->TI_lines = li = 25;
	cx = (co+14) * VIO_font_width;
	cy = li * VIO_font_height;

	WinSetParent(MDIFrame, HWND_OBJECT, FALSE);
	/* Setup main window */
	hdc = WinOpenWindowDC(hwndLeft);
	VioCreatePS(&hvps, VIO_staticy, VIO_staticx, 0,
		    1, 0);
	VioAssociate(hdc, hvps);

	/* Setup nicklist */
	hdc = WinOpenWindowDC(hwndRight);
	VioCreatePS(&hvpsnick, VIO_staticy, VIO_staticx, 0,
		    1, 0);
	VioAssociate(hdc, hvpsnick);

	cursor_off(hvpsnick);

	VIO_font_width=6;VIO_font_height=10;

	/* Set font size */
	VioSetDeviceCellSize(VIO_font_height, VIO_font_width, hvps);
	VioSetDeviceCellSize(VIO_font_height, VIO_font_width, hvpsnick);

	DosPostEventSem(mySem);
	DosCloseEventSem(mySem);

	WinSendMsg(hwndscroll, SBM_SETSCROLLBAR, (MPARAM)get_int_var(SCROLLBACK_VAR), MPFROM2SHORT(0, get_int_var(SCROLLBACK_VAR)));

	DosCreateMutexSem(NULL, &mutex, 0, FALSE);

	lastclicklinedata = new_malloc(500);

	mywindata = new_malloc(sizeof(MYWINDATA));
	memset(mywindata, '\0', sizeof(MYWINDATA));
	mywindata->hvps  = hvps;
	mywindata->screen = NULL;
	WinSetWindowPtr(hwndClient, QWP_USER, mywindata);
	WinSetWindowPtr(hwndLeft, QWP_USER, mywindata);
	WinSetWindowPtr(hwndRight, QWP_USER, mywindata);

	WinSubclassWindow(hwndLeft, FrameWndProc);
	WinSubclassWindow(hwndRight, FrameWndProc);

	WinSendMsg(hwndnickscroll, SBM_SETSCROLLBAR, (MPARAM)0, MPFROM2SHORT(0, li));
	WinSendMsg(hwndnickscroll, SBM_SETTHUMBSIZE, MPFROM2SHORT(li, 0), (MPARAM)NULL);

	setmode(guiipc[0], O_BINARY);
	setmode(guiipc[1], O_BINARY);

	while (WinGetMsg(hab, &qmsg, (HWND)NULL, 0, 0))
	    WinDispatchMsg(hab, &qmsg);

}

void load_default_font(Screen *font_screen)
{
	char *fontsize, *height, *width;
	int t, cy, cx;
	SWP     menupos;

	if((fontsize=get_string_var(DEFAULT_FONT_VAR))!=NULL)
	{
		width=new_malloc(10);
		strcpy(width, fontsize);
		t=0;
		while(width[t]!='x' && t<9)
			t++;
		height=&width[t+1];
		width[t]=0;
		VIO_font_width=atoi(width);
		VIO_font_height=atoi(height);
		new_free(&width);
		if (VIO_font_width < 6 || VIO_font_height < 8)
		{
			VIO_font_width=6;VIO_font_height=10;
		}
	}
	else
	{
		VIO_font_width=6;VIO_font_height=10;
	}

	font_screen->VIO_font_width=VIO_font_width;
	font_screen->VIO_font_height=VIO_font_height;

	if(VIO_font_width != 6 || VIO_font_height != 10)
	{
		cx = (font_screen->co - 1) * font_screen->VIO_font_width;
		cy = font_screen->li * font_screen->VIO_font_height;
		cx += font_screen->VIO_font_width * font_screen->nicklist;
		if(font_screen->nicklist)
			cx += cxvs;

		VioSetDeviceCellSize(font_screen->VIO_font_height, font_screen->VIO_font_width, font_screen->hvps);			  /* Set the font size */
		VioSetDeviceCellSize(font_screen->VIO_font_height, font_screen->VIO_font_width, font_screen->hvpsnick);
		if(font_screen->hwndMenu==(HWND)NULL)
			WinSetWindowPos(font_screen->hwndFrame, HWND_TOP, 0, 0, cx+(cxsb*2)+1, cy+(cysb+cytb), SWP_SIZE);
		else
		{
			WinQueryWindowPos(font_screen->hwndMenu, &menupos);
			WinSetWindowPos(font_screen->hwndFrame, HWND_TOP, 0, 0, cx+(cxsb*2)+cxvs, cy+(cysb*2)+cytb+menupos.cy, SWP_SIZE);
		}
	}
}

/* Individual Page Procs */
MRESULT EXPENTRY NotebookPage1DlgProc(HWND hWnd, ULONG msg, MPARAM mp1,	MPARAM mp2)
{
	char szBuffer[1024];
	int i;
	char *ptr;

	switch (msg)
	{
		/* Perform dialog initialization	*/
	case WM_INITDLG:
		WinSetDlgItemText(hWnd,	EF_ENTRY1, "");
		p1e=LIT_NONE;
		break;
		/* Process control selections */
	case WM_CONTROL:
		switch(SHORT1FROMMP(mp1))
		{
		case SETS1:
			if(p1e!=LIT_NONE)
			{
				switch(return_irc_var(p1e)->type)
				{
				case BOOL_TYPE_VAR:
					i = (ULONG)WinSendMsg(WinWindowFromID(hWnd, CB_ONOFF1),
							      BM_QUERYCHECK,
							      0,
							      0);
					set_int_var(p1e, i);
					break;
				case CHAR_TYPE_VAR:
					WinQueryDlgItemText(hWnd, EF_ENTRY1,	1024L, szBuffer);
					set_string_var(p1e, szBuffer);
					break;
				case STR_TYPE_VAR:
					WinQueryDlgItemText(hWnd, EF_ENTRY1,	1024L, szBuffer);
					set_string_var(p1e, szBuffer);
					break;
				case INT_TYPE_VAR:
					WinQueryDlgItemText(hWnd, EF_ENTRY1,	1024L, szBuffer);
					set_int_var(p1e, my_atol((char *)&szBuffer));
					break;
				}
			}
			p1e = (ULONG)WinSendMsg(WinWindowFromID(hWnd, SETS1),
						LM_QUERYSELECTION,
						MPFROMSHORT(LIT_CURSOR),
						0);
			if(p1e!=LIT_NONE)
			{
				switch(return_irc_var(p1e)->type)
				{
				case BOOL_TYPE_VAR:
					WinEnableWindow(WinWindowFromID(hWnd, CB_ONOFF1), TRUE);
					WinEnableWindow(WinWindowFromID(hWnd, EF_ENTRY1), FALSE);
					i = get_int_var(p1e);
					WinSendMsg(WinWindowFromID(hWnd, CB_ONOFF1),BM_SETCHECK,MPFROMSHORT(i),0);
					WinSetDlgItemText(hWnd,	EF_ENTRY1, "");
					break;
				case INT_TYPE_VAR:
					WinEnableWindow(WinWindowFromID(hWnd, CB_ONOFF1), FALSE);
					WinEnableWindow(WinWindowFromID(hWnd, EF_ENTRY1), TRUE);
					sprintf(szBuffer, "%d", get_int_var(p1e));
					WinSetDlgItemText(hWnd,	EF_ENTRY1, szBuffer);
					break;
				case STR_TYPE_VAR:
					WinEnableWindow(WinWindowFromID(hWnd, CB_ONOFF1), FALSE);
					WinEnableWindow(WinWindowFromID(hWnd, EF_ENTRY1), TRUE);
					ptr=get_string_var(p1e);
					if(ptr && *ptr)
						strcpy(szBuffer, ptr);
					else
						strcpy(szBuffer, empty_string);
					WinSetDlgItemText(hWnd,	EF_ENTRY1, szBuffer);
					break;
				case CHAR_TYPE_VAR:
					WinEnableWindow(WinWindowFromID(hWnd, CB_ONOFF1), FALSE);
					WinEnableWindow(WinWindowFromID(hWnd, EF_ENTRY1), TRUE);
					ptr = get_string_var(p1e);
					if(ptr && *ptr)
						strcpy(szBuffer, ptr);
					else
						strcpy(szBuffer, empty_string);
					WinSetDlgItemText(hWnd,	EF_ENTRY1, szBuffer);
					break;
				}
			}
			break;
		}
		break;
		/* Process push	button selections */
	case WM_COMMAND:
		switch ( SHORT1FROMMP(mp1) )
		{
		case DID_OK:
			if(p1e!=LIT_NONE)
			{
				switch(return_irc_var(p1e)->type)
				{
				case BOOL_TYPE_VAR:
					i = (ULONG)WinSendMsg(WinWindowFromID(hWnd, CB_ONOFF1),BM_QUERYCHECK,0,0);
					set_int_var(p1e, i);
					break;
				case CHAR_TYPE_VAR:
					WinQueryDlgItemText(hWnd, EF_ENTRY1,	1024L, szBuffer);
					set_string_var(p1e, szBuffer);
					break;
				case STR_TYPE_VAR:
					WinQueryDlgItemText(hWnd, EF_ENTRY1,	1024L, szBuffer);
					set_string_var(p1e, szBuffer);
					break;
				case INT_TYPE_VAR:
					WinQueryDlgItemText(hWnd, EF_ENTRY1,	1024L, szBuffer);
					set_int_var(p1e, my_atol((char *)&szBuffer));
					break;
				}
			}
			break;
		}

		break;
		/* Close requested, exit dialogue */
	case WM_CLOSE:
		WinDismissDlg(hWnd, FALSE);
		break;

		/* Pass	through	unhandled messages */
	default:
		return(WinDefDlgProc(hWnd, msg, mp1, mp2));
	}
	return(0L);
}

MRESULT	EXPENTRY NotebookPage2DlgProc(HWND hWnd, ULONG msg, MPARAM mp1,	MPARAM mp2)

{
	 char szBuffer[1024];
	 int i;
	 char *ptr;
 
	 switch (msg)
	 {
		 /* Perform dialog initialization */
	 case WM_INITDLG :
		 WinSetDlgItemText(hWnd,	EF_ENTRY2, "");
		 if(nb_window && nb_window->current_channel)
			 chan = (ChannelList *) find_in_list((List **)get_server_channels(from_server), nb_window->current_channel, 0);
		 else
			 chan = NULL;
		 p2e=LIT_NONE;
		 break;
		 /* Process control selections */
	 case WM_CONTROL:
		 switch(SHORT1FROMMP(mp1))
		 {
		 case SETS2:
			 if(p2e!=LIT_NONE && chan)
			 {
				 switch(return_cset_var(p2e)->type)
				 {
				 case BOOL_TYPE_VAR:
					 i = (ULONG)WinSendMsg(WinWindowFromID(hWnd, CB_ONOFF2),
							       BM_QUERYCHECK,
							       0,
							       0);
					 set_cset_int_var(chan->csets, p2e, i);
					 break;
				 case CHAR_TYPE_VAR:
					 WinQueryDlgItemText(hWnd, EF_ENTRY2,	1024L, szBuffer);
					 set_cset_str_var(chan->csets, p2e, szBuffer);
					 break;
				 case STR_TYPE_VAR:
					 WinQueryDlgItemText(hWnd, EF_ENTRY2,	1024L, szBuffer);
					 set_cset_str_var(chan->csets, p2e, szBuffer);
					 break;
				 case INT_TYPE_VAR:
					 WinQueryDlgItemText(hWnd, EF_ENTRY2,	1024L, szBuffer);
					 set_cset_int_var(chan->csets, p2e, my_atol((char *)&szBuffer));
					 break;
				 }
			 }
			 p2e = (ULONG)WinSendMsg(WinWindowFromID(hWnd, SETS2),
						 LM_QUERYSELECTION,
						 MPFROMSHORT(LIT_CURSOR),
						 0);
			 if(p2e!=LIT_NONE && chan)
			 {
				 switch(return_cset_var(p2e)->type)
				 {
				 case BOOL_TYPE_VAR:
					 WinEnableWindow(WinWindowFromID(hWnd, CB_ONOFF2), TRUE);
					 WinEnableWindow(WinWindowFromID(hWnd, EF_ENTRY2), FALSE);
					 i = get_cset_int_var(chan->csets, p2e);
					 WinSendMsg(WinWindowFromID(hWnd, CB_ONOFF2),
						    BM_SETCHECK,
						    MPFROMSHORT(i),
						    0);
					 WinSetDlgItemText(hWnd,	EF_ENTRY2, empty_string);
					 break;
				 case INT_TYPE_VAR:
					 WinEnableWindow(WinWindowFromID(hWnd, CB_ONOFF2), FALSE);
					 WinEnableWindow(WinWindowFromID(hWnd, EF_ENTRY2), TRUE);
					 sprintf(szBuffer, "%d", get_cset_int_var(chan->csets, p2e));
					 WinSetDlgItemText(hWnd,	EF_ENTRY2, szBuffer);
					 break;
				 case STR_TYPE_VAR:
					 WinEnableWindow(WinWindowFromID(hWnd, CB_ONOFF2), FALSE);
					 WinEnableWindow(WinWindowFromID(hWnd, EF_ENTRY2), TRUE);
					 ptr=get_cset_str_var(chan->csets, p2e);
					 if(ptr && *ptr)
						 strcpy(szBuffer, ptr);
					 else
						 strcpy(szBuffer, empty_string);
					 WinSetDlgItemText(hWnd,	EF_ENTRY2, szBuffer);
					 break;
				 case CHAR_TYPE_VAR:
					 WinEnableWindow(WinWindowFromID(hWnd, CB_ONOFF2), FALSE);
					 WinEnableWindow(WinWindowFromID(hWnd, EF_ENTRY2), TRUE);
					 ptr = get_cset_str_var(chan->csets, p2e);
					 if(ptr && *ptr)
						 strcpy(szBuffer, ptr);
					 else
						 strcpy(szBuffer, empty_string);
					 WinSetDlgItemText(hWnd,	EF_ENTRY2, szBuffer);
					 break;
				 }
			 }
			 break;
		 }
		 break;
		 /* Process push	button selections */
	 case WM_COMMAND:
	 switch (SHORT1FROMMP(mp1))
	 {
	 case DID_OK	:
		 if(p2e!=LIT_NONE && chan)
		 {
			 switch(return_cset_var(p2e)->type)
			 {
			 case BOOL_TYPE_VAR:
				 i = (ULONG)WinSendMsg(WinWindowFromID(hWnd, CB_ONOFF2),
						       BM_QUERYCHECK,
						       0,
					   0);
				 set_cset_int_var(chan->csets, p2e, i);
				 break;
			 case CHAR_TYPE_VAR:
				 WinQueryDlgItemText(hWnd, EF_ENTRY2,	1024L, szBuffer);
				 set_cset_str_var(chan->csets, p2e, szBuffer);
				 break;
			 case STR_TYPE_VAR:
				 WinQueryDlgItemText(hWnd, EF_ENTRY2,	1024L, szBuffer);
				 set_cset_str_var(chan->csets, p2e, szBuffer);
				 break;
			 case INT_TYPE_VAR:
				 WinQueryDlgItemText(hWnd, EF_ENTRY2,	1024L, szBuffer);
				 set_cset_int_var(chan->csets, p2e, my_atol((char *)&szBuffer));
				 break;
			 }
		 }
		 break;
	 }

	 break;
	 /* Close requested, exit dialogue */
	 case WM_CLOSE:
	 WinDismissDlg(hWnd, FALSE);
	 break;

	 /* Pass through unhandled messages */
	 default:
	     return(WinDefDlgProc(hWnd, msg, mp1, mp2));
	 }
	 return(0L);
}

MRESULT	EXPENTRY NotebookPage3DlgProc(HWND hWnd, ULONG msg, MPARAM mp1,	MPARAM mp2)
{
	char szBuffer[1024]; /* String Buffer */
	char *ptr;

	switch ( msg )
	{
		/* Perform dialog initialization */
	case WM_INITDLG :
		WinSetDlgItemText(hWnd,	EF_ENTRY3, empty_string);
		p3e=LIT_NONE;
		break;
		/* Process control selections */
	case WM_CONTROL:
		switch(SHORT1FROMMP(mp1))
		{
		case SETS3:
			if(p3e!=LIT_NONE)
			{
				WinQueryDlgItemText(hWnd, EF_ENTRY3,	1024L, szBuffer);
				fset_string_var(p3e, szBuffer);
			}
			p3e = (ULONG)WinSendMsg(WinWindowFromID(hWnd, SETS3),
						LM_QUERYSELECTION,
						MPFROMSHORT(LIT_CURSOR),
							  0);
			if(p3e!=LIT_NONE)
			{
				WinEnableWindow(WinWindowFromID(hWnd, CB_ONOFF3), FALSE);
				WinEnableWindow(WinWindowFromID(hWnd, EF_ENTRY3), TRUE);
				ptr=fget_string_var(p3e);
				if(ptr && *ptr)
					strcpy(szBuffer, ptr);
				else
					strcpy(szBuffer, empty_string);
				WinSetDlgItemText(hWnd,	EF_ENTRY3, szBuffer);
			}
			break;
		}
		break;
		/* Process push	button selections */
	case WM_COMMAND:
		switch ( SHORT1FROMMP(mp1) )
		{
		case DID_OK	:
			if(p3e!=LIT_NONE)
			{
				WinQueryDlgItemText(hWnd, EF_ENTRY3, 1024L, szBuffer);
				fset_string_var(p3e, szBuffer);
			}
			break;
		}

		break;
	   /* Close requested, exit dialogue */
	case WM_CLOSE:
		WinDismissDlg(hWnd, FALSE);
		break;

		/* Pass	through	unhandled messages */
	default:
		return(WinDefDlgProc(hWnd, msg, mp1, mp2));
	}
	return(0L);
}

MRESULT	EXPENTRY NotebookPage4DlgProc(HWND hWnd, ULONG msg, MPARAM mp1,	MPARAM mp2)

{
	char szBuffer[1024]; /* String Buffer */
	char *ptr;

	switch (msg)
	{
		/* Perform dialog initialization */
	case WM_INITDLG :
		WinSetDlgItemText(hWnd,	EF_ENTRY4, empty_string);
		p4e=LIT_NONE;
		break;
		/* Process control selections */
	case WM_CONTROL :
		switch(SHORT1FROMMP(mp1))
		{
		case SETS4:
			if(p4e!=LIT_NONE && nb_window)
			{
				WinQueryDlgItemText(hWnd, EF_ENTRY4,	1024L, szBuffer);
				set_wset_string_var(nb_window->wset, p4e, szBuffer);
			}
			p4e = (ULONG)WinSendMsg(WinWindowFromID(hWnd, SETS4),
						LM_QUERYSELECTION,
						MPFROMSHORT(LIT_CURSOR),
						0);
			if(p4e!=LIT_NONE && nb_window)
			{
				WinEnableWindow(WinWindowFromID(hWnd, CB_ONOFF4), FALSE);
				WinEnableWindow(WinWindowFromID(hWnd, EF_ENTRY4), TRUE);
				ptr=get_wset_string_var(nb_window->wset, p4e);
				if(ptr && *ptr)
					strcpy(szBuffer, ptr);
				else
					strcpy(szBuffer, empty_string);
				WinSetDlgItemText(hWnd,	EF_ENTRY4, szBuffer);
			}
			break;
		}
		break;
		/* Process push	button selections */
	case WM_COMMAND :
		switch ( SHORT1FROMMP(mp1) )
		{
		case DID_OK	:
			if(p4e!=LIT_NONE && nb_window)
			{
				WinQueryDlgItemText(hWnd, EF_ENTRY4,	1024L, szBuffer);
				set_wset_string_var(nb_window->wset, p4e, szBuffer);
			}
			break;
		}

		break;
		/* Close requested, exit dialogue */
	case WM_CLOSE :
		WinDismissDlg(hWnd, FALSE);
		break;

		/* Pass	through	unhandled messages */
	default :
		return(WinDefDlgProc(hWnd, msg, mp1, mp2));
	}
	return(0L);
}



MRESULT	EXPENTRY NotebookPage5DlgProc(HWND hWnd, ULONG msg, MPARAM mp1,	MPARAM mp2)
{
	switch (msg)
	{
	case	WM_CHAR	:
		break;
		/* Perform dialog initialization */
	case	WM_INITDLG :
		break;
		/* Process control selections */
	case	WM_CONTROL :
		switch ( SHORT2FROMMP(mp1) )
		{
		}
		break;
		/* Process push	button selections */
	case	WM_COMMAND :
		switch ( SHORT1FROMMP(mp1) )
		{
		case DID_OK	:
			break;
		}
		break;
		/* Close requested, exit dialogue */
	case	WM_CLOSE :
		WinDismissDlg(hWnd, FALSE);
		break;

		/* Pass	through	unhandled messages */
	default :
		return(WinDefDlgProc(hWnd, msg, mp1, mp2));
	}
	return(0L);
}

MRESULT	EXPENTRY NotebookDlgProc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
	HPS   hPS;			   /* Temporary	Presentation Space */
	HWND  hwndNBK;     		   /* Notebook Window Handle */
	PVOID ppb;			   /* Dialogue Template	Pointer */
	RECTL rcl;			   /* Rectangle	Holder */
	ULONG ulPageID;    		   /* Inserted Page ID */
	int t;
	char szBuffer[200];

	switch ( msg )
	{
		/* Perform dialog initialization */
	case WM_INITDLG:

		inproperties=1;

		nb_window=current_window;

		GpiQueryFontMetrics(hPS = WinGetPS(hWnd), sizeof(FONTMETRICS), &fm);

		if(nb_window->current_channel)
			sprintf(szBuffer, "PMBitchX Properties for %s", nb_window->current_channel);
		else
			strcpy(szBuffer, "PMBitchX Properties");

		hwndNBK = WinWindowFromID(hWnd, ID_PROP);

		WinSetWindowText(WinWindowFromID(hWnd, FID_TITLEBAR), szBuffer);

		/* Page 1 - Sets */

		ulPageID = (ULONG)WinSendMsg(hwndNBK, BKM_INSERTPAGE, 0L,
					     MPFROM2SHORT((BKA_STATUSTEXTON | BKA_AUTOPAGESIZE |	BKA_MAJOR), BKA_LAST));

		WinSendMsg(hwndNBK, BKM_SETSTATUSLINETEXT,
			   MPFROMLONG(ulPageID), MPFROMP("Page 1 of 5"));

		WinSendMsg(hwndNBK, BKM_SETTABTEXT,
			   MPFROMLONG(ulPageID), MPFROMP("~Sets"));

		DosGetResource((HMODULE)NULL, RT_DIALOG, NBKP_SETS1, (PPVOID)&ppb);
		hwndPage1 = WinCreateDlg(HWND_DESKTOP, (HWND)NULL, (PFNWP)NotebookPage1DlgProc,
					 (PDLGTEMPLATE)ppb, NULL);
		DosFreeResource((PVOID)ppb);
		WinSendMsg(hwndNBK, BKM_SETPAGEWINDOWHWND,
			   MPFROMLONG(ulPageID), MPFROMHWND(hwndPage1));

		/* Page 2 - Csets */

		ulPageID = (ULONG)WinSendMsg(hwndNBK, BKM_INSERTPAGE, 0L,
					     MPFROM2SHORT((BKA_STATUSTEXTON | BKA_AUTOPAGESIZE | BKA_MAJOR), BKA_LAST));

		WinSendMsg(hwndNBK, BKM_SETSTATUSLINETEXT,
			   MPFROMLONG(ulPageID), MPFROMP("Page 2 of 5"));

		WinSendMsg(hwndNBK, BKM_SETTABTEXT,
			   MPFROMLONG(ulPageID), MPFROMP("~Csets"));

		DosGetResource((HMODULE)NULL, RT_DIALOG, NBKP_SETS2, (PPVOID)&ppb);
		hwndPage2 = WinCreateDlg(HWND_DESKTOP, (HWND)NULL, (PFNWP)NotebookPage2DlgProc,
					 (PDLGTEMPLATE)ppb, NULL);
		DosFreeResource((PVOID)ppb);
		WinSendMsg(hwndNBK, BKM_SETPAGEWINDOWHWND,
			   MPFROMLONG(ulPageID), MPFROMHWND(hwndPage2));

		/* Page 3 - Fsets */

		ulPageID = (ULONG)WinSendMsg(hwndNBK, BKM_INSERTPAGE, 0L,
					     MPFROM2SHORT((BKA_STATUSTEXTON | BKA_AUTOPAGESIZE | BKA_MAJOR), BKA_LAST));

		WinSendMsg(hwndNBK, BKM_SETSTATUSLINETEXT,
			   MPFROMLONG(ulPageID), MPFROMP("Page 3 of 5"));

		WinSendMsg(hwndNBK, BKM_SETTABTEXT,
			   MPFROMLONG(ulPageID), MPFROMP("~Fsets"));

		DosGetResource((HMODULE)NULL, RT_DIALOG, NBKP_SETS3, (PPVOID)&ppb);
		hwndPage3 = WinCreateDlg(HWND_DESKTOP, (HWND)NULL, (PFNWP)NotebookPage3DlgProc,
					 (PDLGTEMPLATE)ppb, NULL);
		DosFreeResource((PVOID)ppb);
		WinSendMsg(hwndNBK, BKM_SETPAGEWINDOWHWND,
			   MPFROMLONG(ulPageID), MPFROMHWND(hwndPage3));

		/* Page 4 - Wsets */

		ulPageID = (ULONG)WinSendMsg(hwndNBK, BKM_INSERTPAGE, 0L,
                                     MPFROM2SHORT((BKA_STATUSTEXTON | BKA_AUTOPAGESIZE | BKA_MAJOR), BKA_LAST));

		WinSendMsg(hwndNBK, BKM_SETSTATUSLINETEXT,
			   MPFROMLONG(ulPageID), MPFROMP("Page 4 of 5"));

		WinSendMsg(hwndNBK, BKM_SETTABTEXT,
			   MPFROMLONG(ulPageID), MPFROMP("~Wsets"));

		DosGetResource((HMODULE)NULL, RT_DIALOG, NBKP_SETS4, (PPVOID)&ppb);
		hwndPage4 = WinCreateDlg(HWND_DESKTOP, (HWND)NULL, (PFNWP)NotebookPage4DlgProc,
					 (PDLGTEMPLATE)ppb, NULL);
		DosFreeResource((PVOID)ppb);
		WinSendMsg(hwndNBK, BKM_SETPAGEWINDOWHWND,
			   MPFROMLONG(ulPageID), MPFROMHWND(hwndPage4));

		/* Page 5 - OS/2 Settings */

		ulPageID = (ULONG)WinSendMsg(hwndNBK, BKM_INSERTPAGE, 0L,
					     MPFROM2SHORT((BKA_STATUSTEXTON | BKA_AUTOPAGESIZE | BKA_MAJOR), BKA_LAST));

		WinSendMsg(hwndNBK, BKM_SETSTATUSLINETEXT,
			   MPFROMLONG(ulPageID), MPFROMP("Page 5 of 5"));

		WinSendMsg(hwndNBK, BKM_SETTABTEXT,
			   MPFROMLONG(ulPageID), MPFROMP("~OS/2 Settings"));

		DosGetResource((HMODULE)NULL, RT_DIALOG, NBKP_OSSETTINGS, (PPVOID)&ppb);
		hwndPage5 = WinCreateDlg(HWND_DESKTOP, (HWND)NULL, (PFNWP)NotebookPage4DlgProc,
					 (PDLGTEMPLATE)ppb, NULL);
		DosFreeResource((PVOID)ppb);
		WinSendMsg(hwndNBK, BKM_SETPAGEWINDOWHWND,
			   MPFROMLONG(ulPageID), MPFROMHWND(hwndPage5));

		rcl.xLeft = rcl.yBottom = 0L;
		rcl.xRight = rcl.yTop = 400L;

		WinDrawText(hPS = WinGetPS(hWnd), -1, " Second Page ", &rcl, CLR_BLACK, CLR_BLACK,
			    DT_LEFT | DT_BOTTOM | DT_QUERYEXTENT);
		WinReleasePS(hPS);
		WinSendMsg(hwndNBK, BKM_SETDIMENSIONS,
			   MPFROM2SHORT((SHORT)(rcl.xRight - rcl.xLeft),	(SHORT)(fm.lMaxBaselineExt * 2)),
			   MPFROMSHORT(BKA_MAJORTAB));

		/* Set the dimension of the notebook buttons	*/

		WinSendMsg(hwndNBK, BKM_SETDIMENSIONS, MPFROM2SHORT(21, 21),
			   MPFROMLONG(BKA_PAGEBUTTON));

		/* Set the background colours to that of the dialog */

		WinSendMsg(hwndNBK, BKM_SETNOTEBOOKCOLORS, MPFROMLONG(SYSCLR_DIALOGBACKGROUND),
			   MPFROMLONG(BKA_BACKGROUNDPAGECOLORINDEX));
		WinSendMsg(hwndNBK, BKM_SETNOTEBOOKCOLORS, MPFROMLONG(SYSCLR_DIALOGBACKGROUND),
			   MPFROMLONG(BKA_BACKGROUNDMAJORCOLORINDEX));

		for(t=0; t<NUMBER_OF_VARIABLES; t++)
		{
			WinSendMsg(WinWindowFromID(hwndPage1, SETS1),
				   LM_INSERTITEM,
				   MPFROMSHORT(LIT_END),
				   MPFROMP(return_irc_var(t)->name));
		}

		for(t=0; t<NUMBER_OF_CSETS; t++)
		{
			WinSendMsg(WinWindowFromID(hwndPage2, SETS2),
				   LM_INSERTITEM,
				   MPFROMSHORT(LIT_END),
				   MPFROMP(return_cset_var(t)->name));
		}

		for(t=0; t<NUMBER_OF_FSET; t++)
		{
			WinSendMsg(WinWindowFromID(hwndPage3, SETS3),
				   LM_INSERTITEM,
				   MPFROMSHORT(LIT_END),
				   MPFROMP(return_fset_var(t)->name));
		}

		for(t=0; t<NUMBER_OF_WSETS; t++)
		{
			WinSendMsg(WinWindowFromID(hwndPage4, SETS4),
				   LM_INSERTITEM,
				   MPFROMSHORT(LIT_END),
				   MPFROMP(return_wset_var(t)->name));
		}

		break;
	case WM_COMMAND :
		switch (SHORT1FROMMP(mp1))
		{
		case DID_OK	:
			WinSendMsg(hwndPage1, WM_COMMAND,
				   MPFROMLONG(DID_OK), MPFROMLONG(CMDSRC_OTHER));
			WinSendMsg(hwndPage2, WM_COMMAND,
				   MPFROMLONG(DID_OK), MPFROMLONG(CMDSRC_OTHER));
			WinSendMsg(hwndPage3, WM_COMMAND,
				   MPFROMLONG(DID_OK), MPFROMLONG(CMDSRC_OTHER));
			WinSendMsg(hwndPage4, WM_COMMAND,
				   MPFROMLONG(DID_OK), MPFROMLONG(CMDSRC_OTHER));
			inproperties=0;
			WinDismissDlg(hWnd, TRUE);
			break;
		}
		break;
		/* Close requested, exit dialogue */
	case WM_CLOSE :
		inproperties=0;
		WinDismissDlg(hWnd, FALSE);
		break;

		/* Pass	through	unhandled messages */
	default :
		return(WinDefDlgProc(hWnd, msg, mp1, mp2));
	}
	return(0L);
}

void properties_notebook(void)
{
	HAB hnbab;
	HMQ hnbmq;

	hnbab = WinInitialize(0);
	hnbmq = WinCreateMsgQueue(hnbab, 0);
	if(inproperties==0)
		WinDlgBox(HWND_DESKTOP, HWND_DESKTOP, NotebookDlgProc, NULLHANDLE, PROPNBK, NULL);

	WinDestroyMsgQueue(hnbmq);
	WinTerminate(hnbab);
}

void msgbox(void)
{
	HAB hmbab;
	HMQ hmbmq;

	hmbab = WinInitialize(0);
	hmbmq = WinCreateMsgQueue(hmbab, 0);

	WinMessageBox(HWND_DESKTOP, HWND_DESKTOP, msgtext, "PMBitchX", 0, MB_OK | MB_INFORMATION | MB_MOVEABLE);

	new_free(&msgtext);

	WinDestroyMsgQueue(hmbmq);
	WinTerminate(hmbab);
}

void pm_clrscr(Screen *tmp)
{
	VioScrollUp(0, 0, -1, -1, -1, (PBYTE)&default_pair, tmp->hvps);
}

void gui_font_set(char *fontsize, Screen *screen)
{
	char *height, *width;
	int t;

	width=new_malloc(10);
	strcpy(width, fontsize);
	t=0;
	while(width[t]!='x' && t<9)
		t++;
	height=&width[t+1];
	width[t]=0;
	VIO_font_width=atoi(width);
	VIO_font_height=atoi(height);
	new_free(&width);
	if (VIO_font_width < 6 || VIO_font_height < 8)
	{
		VIO_font_width=6;VIO_font_height=10;
	}

	if(VioSetDeviceCellSize(VIO_font_height, VIO_font_width, screen->hvps)!=NO_ERROR)
	{
 		VIO_font_width = screen->VIO_font_width;
		VIO_font_height = screen->VIO_font_height;
	}
	else
	{
		VioSetDeviceCellSize(VIO_font_height, VIO_font_width, screen->hvpsnick);
		screen->VIO_font_width = VIO_font_width;
		screen->VIO_font_height = VIO_font_height;
	}

}

void gui_font_init(void)
{
	char *fontsize;
	int cy, cx, containerh, codepage = 0;

	VIO_font_width=6;VIO_font_height=10;
	main_screen->VIO_font_width=VIO_font_width;
	main_screen->VIO_font_height=VIO_font_height;

	if((fontsize=get_string_var(DEFAULT_FONT_VAR))!=NULL)
		gui_font_set(fontsize, main_screen);

	main_screen->nicklist = get_int_var(NICKLIST_VAR);

	main_screen->co = 84; main_screen->li = 25;
	cx = (main_screen->co - 1) * main_screen->VIO_font_width;
	cy = main_screen->li * main_screen->VIO_font_height;
	cx += main_screen->VIO_font_width * main_screen->nicklist;
	if(main_screen->nicklist)
		cx += cxvs;

	WinSetWindowPos(main_screen->hwndFrame, HWND_TOP, 0, 0, 0 ,0, SWP_HIDE);

	if(get_int_var(MDI_VAR))
	{
		SWP swp;

		WinQueryWindowPos(MDIClient, &swp);

		containerh = swp.cy;
		WinSetParent(main_screen->hwndFrame, MDIClient, FALSE);
	}
	else
	{
		containerh = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);
		WinSetParent(main_screen->hwndFrame, HWND_DESKTOP, FALSE);
	}

	if(main_screen->hwndMenu==(HWND)NULL)
		WinSetWindowPos(main_screen->hwndFrame, HWND_TOP, 32 * main_screen->screennum, containerh - ((32 * main_screen->screennum) + cy + (cysb*2)+cytb), cx+cxsb, cy+(cysb*2)+WinQuerySysValue(HWND_DESKTOP, SV_CYTITLEBAR), SWP_SIZE | SWP_MOVE | SWP_SHOW | SWP_ACTIVATE | SWP_ZORDER);
	else
		WinSetWindowPos(main_screen->hwndFrame, HWND_TOP, 32 * main_screen->screennum, containerh - ((32 * main_screen->screennum) + cy + (cysb*2)+cytb), cx+cxsb, cy+(cysb*2)+WinQuerySysValue(HWND_DESKTOP, SV_CYTITLEBAR) + WinQuerySysValue(HWND_DESKTOP, SV_CYMENU) + 1, SWP_SIZE | SWP_MOVE | SWP_SHOW | SWP_ACTIVATE | SWP_ZORDER);

	if((codepage = get_int_var(DEFAULT_CODEPAGE_VAR)))
	{
		VioSetCp(0, codepage, main_screen->hvps);
		VioSetCp(0, codepage, main_screen->hvpsnick);
		WinSetCp(hmq, codepage);
		WinSetCp(mainhmq, codepage);
	}

	DosSleep(1000);

	/*if(!get_int_var(SCROLLERBARS_VAR))
	 WinSetParent();*/
}

/* This section is for portability considerations */
void gui_init(void)
{
	LONG add = 100;
	ULONG new;

	/* Create an unnamed semaphore */
	DosCreateEventSem(NULL,
					  (PHEV)&sem,
					  DC_SEM_SHARED,
					  FALSE);

	li = 25;
	co = 80;

#ifdef SOUND
	if(DosLoadModule(NULL, 0, "MCIAPI", &hmod) == NO_ERROR)
	{
		if(DosQueryProcAddr(hmod, 0, "mciPlayFile", (PFN *)&DLL_mciPlayFile) != NO_ERROR)
			DLL_mciPlayFile = NULL;
		if(DosQueryProcAddr(hmod, 0, "mciSendString", (PFN *)&DLL_mciSendString) != NO_ERROR)
			DLL_mciSendString = NULL;
		if(DosQueryProcAddr(hmod, 0, "mciGetErrorString", (PFN *)&DLL_mciGetErrorString) != NO_ERROR)
			DLL_mciGetErrorString = NULL;
	}
#endif

	DosSetRelMaxFH(&add, &new);
	_beginthread((void *)avio_init, NULL, (4*0xFFFF), main_screen );
	_beginthread((void *)flush_thread, NULL, 0xFFFF, &hvps);
	_beginthread((void *)cursor_thread, NULL, 0xFFFF, NULL);

	DosWaitEventSem(sem, SEM_INDEFINITE_WAIT);
	DosCloseEventSem(sem);


	default_pair[0] = ' ';
	default_pair[1] = 7;
}

void gui_clreol(void)
{
	USHORT row, col;
	char tmpbuf[201];

	aflush();
	if(!output_screen)
		return;

	VioGetCurPos((PUSHORT)&row, (PUSHORT)&col, output_screen->hvps);
	if((output_screen->co - col + 1) > 0)
	{
		memset(tmpbuf, ' ', 200);
		tmpbuf[201] = 0;
		VioWrtTTY((PCH)tmpbuf, (output_screen->co - col) + 1, output_screen->hvps);
		VioSetCurPos((USHORT)row, (USHORT)col, output_screen->hvps);
	}
}

void gui_gotoxy(int col, int row)
{
	aflush();
	VioSetCurPos((USHORT)row, (USHORT)col, output_screen ? output_screen->hvps : hvps);
}

void gui_clrscr(void)
{
	Screen *tmp;

	aflush();
	for (tmp = screen_list; tmp; tmp = tmp->next)
        if(tmp->alive)
			pm_clrscr(tmp);
}

void gui_left(int num)
{
	unsigned int row,col;
	aflush();
	VioGetCurPos((PUSHORT)&row, (PUSHORT)&col, output_screen ? output_screen->hvps : hvps);
	if(col>num)
		col=col-num;
	else
		col=0;
	VioSetCurPos((USHORT)row, (USHORT)col, output_screen ? output_screen->hvps : hvps);
}

void gui_right(int num)
{
	unsigned int row,col;
	aflush();
	VioGetCurPos((PUSHORT)&row, (PUSHORT)&col, output_screen->hvps);
	if(output_screen->co > num)
		col=col+num;
	else
		col=output_screen->co;
	VioSetCurPos((USHORT)row, (USHORT)col, output_screen->hvps);
}

void gui_scroll(int top, int bot, int n)
{
	aflush();
	if (n > 0) VioScrollUp(top, 0, bot, output_screen->co, n, (PBYTE)&pair, output_screen->hvps);
	else if (n < 0) { n = -n; VioScrollDn(top, 0, bot, output_screen->co, n, (PBYTE)&pair, output_screen->hvps); }
	return;
}

void gui_flush(void)
{
	aflush();
}

void gui_puts(unsigned char *buffer)
{
	int i;
	for (i = 0; i < strlen(buffer); i++) aputc(buffer[i]);
	aputc('\n'); aputc('\r');
}

void gui_new_window(Screen *new, Window *win)
{
	WinSendMsg(main_screen->hwndClient, WM_USER+2, MPFROMP(new), MPFROMP(win));
}

void pm_new_window(Screen *new, Window *win)
{
	int	cx,
	cy;
	HDC	hdc;
	ULONG	flStyle = FCF_MINMAX | FCF_SYSMENU | FCF_TITLEBAR | FCF_SIZEBORDER | FCF_ICON;
	PMYWINDATA	windata;
	MenuStruct *menutoadd;
	char    *defmenu, *fontsize, *width, *height;
	int     t, containerh, codepage = 0;

	windata = new_malloc(sizeof(MYWINDATA));
	memset(windata, '\0', sizeof(MYWINDATA));

	if(!get_int_var(MDI_VAR))
		flStyle |= FCF_TASKLIST;

	new->hwndFrame = WinCreateStdWindow(HWND_DESKTOP,
					    0,
					    &flStyle,
					    "AVIO",
					    irc_version,
					    0L,
					    NULLHANDLE,
					    IDM_MAINMENU,
					    &new->hwndClient);

	WinSetWindowPtr(new->hwndClient, QWP_USER, NULL);

	new->hwndLeft = WinCreateWindow(new->hwndClient,
					WC_FRAME,
					NULL,
					WS_VISIBLE,
					0,0,2000,1000,
					new->hwndClient,
					HWND_TOP,
					0L,
					NULL,
					NULL);

	new->hwndRight = WinCreateWindow(new->hwndClient,
					 WC_FRAME,
					 NULL,
					 WS_VISIBLE,
					 0,0,2000,1000,
					 new->hwndClient,
					 HWND_TOP,
					 0L,
					 NULL,
					 NULL);

	new->hwndscroll = WinCreateWindow(new->hwndClient,
					  WC_SCROLLBAR,
					  NULL,
					  WS_VISIBLE | SBS_VERT,
					  0,0,100,100,
					  new->hwndClient,
					  HWND_TOP,
					  1000L,
					  NULL,
					  NULL);

	new->hwndnickscroll = WinCreateWindow(new->hwndClient,
					      WC_SCROLLBAR,
					      NULL,
					      WS_VISIBLE | SBS_VERT,
					      0,0,100,100,
					      new->hwndClient,
					      HWND_TOP,
					      1001L,
					      NULL,
					      NULL);

	if (main_screen)
	{
		new->co = main_screen->co;
		new->li = main_screen->li;
	}
	else
	{
		new->co = 81;
		new->li = 25;
	}

	if (main_screen)
	{
		new->VIO_font_width  = main_screen->VIO_font_width;
		new->VIO_font_height = main_screen->VIO_font_height;
	}
	else
	{
		new->VIO_font_width  = VIO_font_width;
		new->VIO_font_height = VIO_font_height;
	}

	/* Get the default font and set the new window with it */

	if((fontsize=get_string_var(DEFAULT_FONT_VAR))!=NULL)
	{
		width=new_malloc(10);
		strcpy(width, fontsize);
		t=0;
		while(width[t]!='x' && t<9)
			t++;
		height=&width[t+1];
		width[t]=0;
		VIO_font_width=atoi(width);
		VIO_font_height=atoi(height);
		new_free(&width);
		if (VIO_font_width < 6 || VIO_font_height < 8)
		{
			VIO_font_width=6;VIO_font_height=10;
		}
	}
	else
	{
		VIO_font_width=6;VIO_font_height=10;
	}

	new->VIO_font_width  = VIO_font_width;
	new->VIO_font_height = VIO_font_height;

	/* Get the default menu and add it to the new window */

	defmenu=get_string_var(DEFAULT_MENU_VAR);
	if(defmenu && *defmenu)
	{
		if((menutoadd = (MenuStruct *)findmenu(defmenu))==NULL)
			say("Menu not found.");
		else if(menutoadd->menuorigin==NULL)
			say("Cannot create blank menu.");
		else
		{
			new->hwndMenu=newsubmenu((MenuStruct *)menutoadd, new->hwndFrame, TRUE);
			new->menu=m_strdup(defmenu);
		}
	}
	else
		new->hwndMenu=(HWND)NULL;

	cx = new->co * new->VIO_font_width;
	cy = new->li * new->VIO_font_height;
	new->nicklist = get_int_var(NICKLIST_VAR);
	cx += new->nicklist * new->VIO_font_width;
	if(new->nicklist)
		cx += cxvs;

	hdc = WinOpenWindowDC(new->hwndLeft);

	/* VIO_staticx and VIO_staticy */
	VioCreatePS(&new->hvps, VIO_staticy, VIO_staticx, 0, 1, 0);
	VioAssociate(hdc, new->hvps);

	hdc = WinOpenWindowDC(new->hwndRight);
	VioCreatePS(&new->hvpsnick, VIO_staticy, VIO_staticx, 0, 1, 0);
	VioAssociate(hdc, new->hvpsnick);

	cursor_off(new->hvpsnick);

	VioSetDeviceCellSize(new->VIO_font_height, new->VIO_font_width, new->hvps);
	VioSetDeviceCellSize(new->VIO_font_height, new->VIO_font_width, new->hvpsnick);

	windata->hvps   = new->hvps;
	windata->screen = new;
	WinSendMsg(new->hwndscroll, SBM_SETSCROLLBAR, (MPARAM)get_int_var(SCROLLBACK_VAR), MPFROM2SHORT(0, get_int_var(SCROLLBACK_VAR)));
	WinSetWindowPtr(new->hwndClient, QWP_USER, windata);
	WinSetWindowPtr(new->hwndLeft, QWP_USER, windata);
	WinSetWindowPtr(new->hwndRight, QWP_USER, windata);

	WinSubclassWindow(new->hwndLeft, FrameWndProc);
	WinSubclassWindow(new->hwndRight, FrameWndProc);

	WinSetWindowPos(new->hwndFrame, HWND_TOP, 0, 0, 0 ,0, SWP_HIDE);

	WinSendMsg(new->hwndnickscroll, SBM_SETSCROLLBAR, (MPARAM)0, MPFROM2SHORT(0, new->li));
	WinSendMsg(new->hwndnickscroll, SBM_SETTHUMBSIZE, MPFROM2SHORT(new->li, 0), (MPARAM)NULL);

	if(get_int_var(MDI_VAR))
	{
		SWP swp;

		WinQueryWindowPos(MDIClient, &swp);

		containerh = swp.cy;
		WinSetParent(new->hwndFrame, MDIClient, FALSE);
	}
	else
	{
		containerh = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);
		WinSetParent(new->hwndFrame, HWND_DESKTOP, FALSE);
	}

	if(new->hwndMenu==(HWND)NULL)
		WinSetWindowPos(new->hwndFrame, HWND_TOP, 32 * new->screennum, containerh - ((32 * new->screennum) + cy + (cysb*2)+ cytb), cx+(cxsb*2) + cxvs, cy + (cysb*2)+ WinQuerySysValue(HWND_DESKTOP, SV_CYTITLEBAR), SWP_SIZE | SWP_ZORDER | SWP_ACTIVATE | SWP_SHOW | SWP_MOVE);
	else
		WinSetWindowPos(new->hwndFrame, HWND_TOP, 32 * new->screennum, containerh - ((32 * new->screennum) + cy + (cysb*2)+ cytb), cx+(cxsb*2) + cxvs, cy + (cysb*2)+ WinQuerySysValue(HWND_DESKTOP, SV_CYTITLEBAR) + /*menupos.cy*/ WinQuerySysValue(HWND_DESKTOP, SV_CYMENU) + 1, SWP_SIZE | SWP_ZORDER | SWP_ACTIVATE | SWP_SHOW | SWP_MOVE);

	if((codepage = get_int_var(DEFAULT_CODEPAGE_VAR)))
	{
		VioSetCp(0, codepage, new->hvps);
		VioSetCp(0, codepage, new->hvpsnick);
		WinSetCp(hmq, codepage);
		WinSetCp(mainhmq, codepage);
	}

}

void gui_kill_window(Screen *killscreen)
{
	PMYWINDATA wd = (PMYWINDATA)WinQueryWindowPtr(killscreen->hwndClient, QWP_USER);
	
	if(pmthread == *_threadid)
	{
		if(killscreen->hwndMenu && WinIsWindow(hab, killscreen->hwndMenu))
			detach_shared(killscreen);
		VioAssociate((HDC)NULL, killscreen->hvps);
		VioDestroyPS(killscreen->hvps);
		killscreen->hvps = 0;
		VioAssociate((HDC)NULL, killscreen->hvpsnick);
		VioDestroyPS(killscreen->hvpsnick);
		killscreen->hvpsnick = 0;
		WinDestroyWindow(killscreen->hwndFrame);
	} else {
		WinSendMsg(killscreen->hwndFrame, WM_USER+4, (MPARAM)killscreen, NULL);
		return;
	}
	if(wd)
		new_free(&wd);
}

void gui_msgbox(void)
{
	_beginthread((void *)msgbox, NULL, 0xFFFF, NULL);
}

void gui_popupmenu(char *menuname)
{
	if (contextmenu && WinIsWindow(hab, contextmenu))
		WinDestroyWindow(contextmenu);
	contextmenu=(HWND)menucreatestub(menuname, last_input_screen->hwndFrame, FALSE);
	WinPopupMenu(last_input_screen->hwndFrame, last_input_screen->hwndFrame, contextmenu, contextx, contexty, 0, PU_KEYBOARD | PU_MOUSEBUTTON1 | PU_VCONSTRAIN | PU_HCONSTRAIN);
}

/* These are support routines for gui_file_dialog() */
MRESULT EXPENTRY FileFilterProc(HWND hwnd, ULONG message, MPARAM mp1, MPARAM mp2 )
{
	return WinDefFileDlgProc( hwnd, message, mp1, mp2 ) ;
}

/* Ripped from functions.c because I couldn't get it to link from there */
BUILT_IN_FUNCTION(fencode)
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

void pm_file_dialog(char *data[7])
{
	/* 0 type
	 1 path
	 2 title
	 3 ok
	 4 apply
	 5 szButton
	 6 code
	 */
	HAB hfdab;
	HMQ hfdmq;

	FILEDLG fild;
	char *okbut, *title, *tmp, *tmp2;
	int z;

	HWND hwndFile;

	hfdab = WinInitialize(0);
	hfdmq = WinCreateMsgQueue(hfdab, 0);

	convert_dos(data[1]);
	okbut = m_strdup(data[5]);
	title = m_strdup(data[2]);

	fild.cbSize = sizeof(FILEDLG);
	fild.fl = /*FDS_HELPBUTTON |*/ FDS_CENTER | FDS_OPEN_DIALOG;
	fild.pszTitle = title;
	fild.pszOKButton = okbut;
	fild.ulUser = 0L;
	fild.pfnDlgProc = (PFNWP)FileFilterProc;
	fild.lReturn = 0L;
	fild.lSRC = 0L;
	fild.hMod = 0;
	fild.x = 0;
	fild.y = 0;
	fild.pszIType       = (PSZ)NULL;
	fild.papszITypeList = (PAPSZ)NULL;
	fild.pszIDrive      = (PSZ)NULL;
	fild.papszIDriveList= (PAPSZ)NULL;
	fild.sEAType        = (SHORT)0;
	fild.papszFQFilename= (PAPSZ)NULL;
	fild.ulFQFCount     = 0L;
	strcpy(fild.szFullFile, data[1]);

	tmp = m_strdup(data[6]);

	for(z=0;z<7;z++)
	{
		if(data[z])
			new_free(&data[z]);
	}

	hwndFile = WinFileDlg(HWND_DESKTOP, HWND_DESKTOP, &fild);
	if(hwndFile)
	{
		switch(fild.lReturn)
		{
		case DID_OK:
			paramptr = m_strdup("OK ");
			convert_unix(fild.szFullFile);
			tmp2 = fencode(NULL, fild.szFullFile);
			malloc_strcat(&paramptr, tmp2);
			new_free(&tmp2);
			break;
		case DID_CANCEL:
			paramptr = m_strdup("CANCEL");
			break;
		default:
			paramptr = m_strdup(empty_string);
			break;
		}

		new_free(&title);
		new_free(&okbut);

		codeptr = tmp;
		sendevent(EVFILE, current_window->refnum);
	}

	WinDestroyMsgQueue(hfdmq);
	WinTerminate(hfdab);
}

void gui_file_dialog(char *type, char *path, char *title, char *ok, char *apply, char *code, char *szButton)
{
	char *data[7];

	if(!type)
		data[0] = m_strdup(empty_string);
    else
		data[0] = m_strdup(type);
	if(!path)
		data[1] = m_strdup(empty_string);
    else
		data[1] = m_strdup(path);
	if(!title)
		data[2] = m_strdup(empty_string);
    else
		data[2] = m_strdup(title);
	if(!ok)
		data[3] = m_strdup(empty_string);
    else
		data[3] = m_strdup(ok);
	if(!apply)
		data[4] = m_strdup(empty_string);
    else
		data[4] = m_strdup(apply);
	if(!szButton)
		data[5] = m_strdup(empty_string);
    else
		data[5] = m_strdup(szButton);
	if(!code)
		data[6] = m_strdup(empty_string);
    else
		data[6] = m_strdup(code);
	_beginthread((void *)pm_file_dialog, NULL, 0xFFFF, data);
}


void gui_properties_notebook(void)
{
	_beginthread((void *)properties_notebook, NULL, 0xFFFF, NULL);
}

#define current_screen      last_input_screen
#define INPUT_BUFFER        current_screen->input_buffer
#define ADD_TO_INPUT(x)     strmcat(INPUT_BUFFER, (x), INPUT_BUFFER_SIZE);

void gui_paste(char *args)
{
	ULONG  fmtInfo;
	APIRET rc;
	PSZ    pszClipText;
	char   *oclip, *clip, *bit;
	int    line = 0, i = 0;
	char   *channel = NULL;
	int topic = 0;
	int smartpaste = 0;
	char smart[512];
	int smartsize = 80;
	int input_line = 0;

	from_server = current_window->server;

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


	WinOpenClipbrd(hab);

	rc = WinQueryClipbrdFmtInfo(hab,
				    CF_TEXT,
				    &fmtInfo);
	if (rc) /* Text data in clipboard */
	{
		pszClipText = (PSZ)WinQueryClipbrdData(hab, CF_TEXT); /* Query data handle */
		oclip = clip = malloc(strlen(pszClipText) + 1); line = 0;
		while ((*clip++ = *pszClipText++)); /* Copy to own memory */

		if (!smartpaste)
		{
			/* Ordinary paste */
			clip = oclip; bit = strtok(clip, "\n\r");
			while (bit)
			{
				if (input_line)
				{
					ADD_TO_INPUT(bit);
					update_input(UPDATE_FROM_CURSOR);
				} else
					if (!topic)
					{
						if(current_window->query_nick)
						{
							if (do_hook(PASTE_LIST, "%s %s", current_window->query_nick, bit))
								send_text(current_window->query_nick, bit, NULL, 1, 0);
						}
						else
						{
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
						ADD_TO_INPUT(smart);
						update_input(UPDATE_FROM_CURSOR);
					} else
						if (!topic)
						{
							if(current_window->query_nick)
							{
								if (do_hook(PASTE_LIST, "%s %s", current_window->query_nick, smart))
									send_text(current_window->query_nick, smart, NULL, 1, 0);
							}
							else
							{
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
		}

		free(oclip); /* Free */
	}
	WinCloseClipbrd(hab);
}

void gui_setfocus(Screen *screen)
{
	WinSetFocus(HWND_DESKTOP, screen->hwndClient);
}

void gui_scrollerchanged(Screen *screen, int position)
{
	WinSendMsg(screen->hwndscroll, SBM_SETSCROLLBAR, (MPARAM)position, MPFROM2SHORT(0, get_int_var(SCROLLBACK_VAR)));
}

void gui_query_window_info(Screen *screen, char *fontinfo, int *x, int *y, int *cx, int *cy)
{

	SWP wpos;

	if(screen)
	{
		WinQueryWindowPos(screen->hwndFrame, &wpos);
		*x = wpos.x;
		*y = wpos.y;
		*cx = wpos.cx;
		*cy = wpos.cy;
	}
	else
	{
		*x=*y=*cx=*cy=0;
	}
	sprintf(fontinfo, "%dx%d", screen ? screen->VIO_font_width : 0, screen ? screen->VIO_font_height : 0);
}

void play_sound(char *filename)
{
#ifdef SOUND
	struct stat tmpstat;
	HAB soundhab = 0;
	HMQ soundhmq = 0;

	soundhab = WinInitialize(0);
	soundhmq = WinCreateMsgQueue(soundhab, 0);

	convert_dos(filename);
	if((stat(filename,&tmpstat))==0)
		DLL_mciPlayFile((HWND)NULL, filename, 0, 0, 0);

	free(filename);

	WinDestroyMsgQueue(soundhmq);
	WinTerminate(soundhab);
#endif
}

void gui_play_sound(char *filename)
{
#ifdef SOUND
	char *freeme = strdup(filename);

	if(!DLL_mciPlayFile)
		return;

	_beginthread((void *)play_sound, NULL, 0xFFFF, (char *)freeme);
#endif
}

int gui_send_mci_string(char *mcistring, char *retstring)
{
#ifdef SOUND
	if(DLL_mciSendString)
		return DLL_mciSendString((PSZ)mcistring, (PSZ)retstring, 500, 0, 0);
	else
#endif
	return 0;
}

void gui_get_sound_error(int errnum, char *errstring)
{
#ifdef SOUND
	if(DLL_mciGetErrorString)
		DLL_mciGetErrorString(errnum, (PSZ)errstring, 500);
#endif
}

void gui_menu(Screen *screen, char *addmenu)
{
	/* Finish creating the menu in the Window thread */
	WinSendMsg(screen->hwndClient, WM_USER+6, (MPARAM)screen, (MPARAM)addmenu);
}

int gui_isset(Screen *screen, fd_set *rd, int what)
{
	if (screen->aviokbdbuffer[0] != 0)
		return TRUE;
	else
		return FALSE;

}

int gui_putc(int c)
{
	aputc(c);
	return 1;
}

void gui_exit(void)
{
#ifdef SOUND
	DosFreeModule(hmod);
#endif
	avio_exit();
}

void gui_screen(Screen *new)
{
	new->hvps = hvps;
	new->hvpsnick = hvpsnick;
	new->VIO_font_width = VIO_font_width;
	new->VIO_font_height = VIO_font_height;
	new->hwndFrame = hwndFrame;
	new->hwndClient = hwndClient;
	new->hwndLeft = hwndLeft;
	new->hwndRight = hwndRight;
	new->hwndscroll = hwndscroll;
	new->hwndnickscroll = hwndnickscroll;
	new->menu=(HWND)NULL;
	new->old_li=new->li;
	new->old_co=new->co;
	new->nicklist = get_int_var(NICKLIST_VAR);


	main_screen = new;
}

void gui_resize(Screen *new)
{
	pm_resize(new);
}

int gui_screen_width(void)
{
	return WinQuerySysValue(HWND_DESKTOP,SV_CXSCREEN);
}

int gui_screen_height(void)
{
	return WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN);
}

void gui_setwindowpos(Screen *screen, int x, int y, int cx, int cy, int top, int bottom, int min, int max, int restore, int activate, int size, int position)
{
	ULONG flags = 0;
	HWND pos = HWND_BOTTOM;

	if(top || bottom)
		flags |= SWP_ZORDER;

	if(top)
		pos = HWND_TOP;

	if(min)
		flags |= SWP_MINIMIZE;

	if(max)
		flags |= SWP_MAXIMIZE;

	if(restore)
		flags |= SWP_RESTORE;

	if(activate)
		flags |= SWP_ACTIVATE;

	if(size)
		flags |= SWP_SIZE;

	if(position)
		flags |= SWP_MOVE;

	WinSetWindowPos(screen->hwndFrame, pos, x, y, cx, cy, flags);
}

void BX_gui_mutex_lock(void)
{
}

void BX_gui_mutex_unlock(void)
{
}

int gui_setmenuitem(char *menuname, int refnum, char *what, char *param)
{
	MenuRef *tmp = NULL;
	MenuStruct *thismenu = findmenu(menuname);

	if(thismenu)
		tmp = find_menuref(thismenu->root, refnum);

	if(!tmp)
		return FALSE;

	if(my_stricmp(what, "check")==0)
	{
		if(my_atol(param))
			WinSendMsg(tmp->menuhandle, MM_SETITEMATTR, MPFROM2SHORT(tmp->menuid, TRUE), MPFROM2SHORT(MIA_CHECKED, MIA_CHECKED));
		else
			WinSendMsg(tmp->menuhandle, MM_SETITEMATTR, MPFROM2SHORT(tmp->menuid, TRUE), MPFROM2SHORT(MIA_CHECKED, 0));
	}
	else if(my_stricmp(what, "text")==0)
	{
		WinSendMsg(tmp->menuhandle, MM_SETITEMTEXT, MPFROMSHORT(tmp->menuid), MPFROMP(param));
	}
	else
		return FALSE;

	return TRUE;

}

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
	WinSendMsg(main_screen->hwndClient, WM_USER+3, (MPARAM)value, 0);
}

void gui_update_nicklist(char *channel)
{
	if(channel)
	{
		Window *this_window;
		ChannelList *cptr = lookup_channel(channel, from_server, 0);

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

void gui_nicklist_width(int width, Screen *this_screen)
{
	SWP swp;
	int ocx = this_screen->VIO_font_width * this_screen->nicklist;
	int ncx = this_screen->VIO_font_width * width;

	if(ocx)
		ocx += cxvs;
	if(width)
		ncx += cxvs;

	if(this_screen->nicklist && !width)
	{
		WinSetParent(this_screen->hwndRight, HWND_OBJECT, FALSE);
		WinSetParent(this_screen->hwndnickscroll, HWND_OBJECT, FALSE);
	}
	if(!this_screen->nicklist && width)
	{
		WinSetParent(this_screen->hwndRight, this_screen->hwndClient, FALSE);
		WinSetParent(this_screen->hwndnickscroll, this_screen->hwndClient, FALSE);
	}
		this_screen->nicklist = width;
	WinQueryWindowPos(this_screen->hwndFrame, &swp);
	WinSetWindowPos(this_screen->hwndFrame, HWND_TOP, swp.x, swp.y, swp.cx + (ncx - ocx), swp.cy, SWP_SIZE);
}

void gui_startup(int argc, char *argv[])
{
	mainhab = WinInitialize(0);
	mainhmq = WinCreateMsgQueue(mainhab, 0);

	pipe(guiipc);
	new_open(guiipc[0]);

	dw_init(FALSE, argc, argv);
}

void gui_about_box(char *about_text)
{
	WinSendMsg(main_screen->hwndFrame, WM_USER+5, MPFROMP(about_text), NULL);
}

void gui_activity(int color)
{
	HWND hwnd;
    ULONG ulColor;

	if(target_window && target_window->screen)
		hwnd = target_window->screen->hwndFrame;
	else
		return;

	switch(color)
	{
	case COLOR_ACTIVE:
		ulColor = CLR_RED;
		break;
	case COLOR_HIGHLIGHT:
		ulColor = CLR_BLUE;
		break;
	default:
		ulColor = CLR_PALEGRAY;
        break;
	}

#if 0
	WinSetPresParam(hwnd,
					PP_FOREGROUNDCOLORINDEX,
					sizeof(ulColor),
					&ulColor);
#endif
}

void gui_setfileinfo(char *filename, char *nick, int server)
{
	const unsigned fea2listsize = 6000;
	char *pData, buffer[200], *s, *t, *fullname, *tmp = NULL;
	EAOP2 eaop2;
	PFEA2 pFEA2;
	extern char	*time_format;
	APIRET rc;

	malloc_sprintf(&tmp, "%s/%s", get_string_var(DCC_DLDIR_VAR), filename);
	fullname = expand_twiddle(tmp);
	convert_dos(fullname);
	new_free(&tmp);

	s = get_server_network(server);

	time_format = "%c";
	update_clock(RESET_TIME);
	t = update_clock(GET_TIME);

	sprintf(buffer, "%s@%s %s", nick, s ? s : "IRC", t ? t : "clock problem");

	time_format = NULL;
	update_clock(RESET_TIME);

	eaop2.fpGEA2List = 0;
	eaop2.fpFEA2List = (PFEA2LIST)malloc(fea2listsize);
	pFEA2 = &eaop2.fpFEA2List->list[0];

	pFEA2->fEA = 0;
    /* .COMMENTS is 9 characters long */
	pFEA2->cbName = 9;

	/* space for the type and length field. */
	pFEA2->cbValue = strlen(buffer)+2*sizeof(USHORT);

	strcpy(pFEA2->szName, ".COMMENTS");
	pData = pFEA2->szName+pFEA2->cbName+1;
	/* data begins at first byte after the name */

	*(USHORT*)pData = EAT_ASCII;             /* type */
	*((USHORT*)pData+1) = strlen(buffer);  /* length */
	strcpy(pData+2*sizeof(USHORT), buffer);/* content */

	pFEA2->oNextEntryOffset = 0;

	eaop2.fpFEA2List->cbList = ((PCHAR)pData+2*sizeof(USHORT)+
									 pFEA2->cbValue)-((PCHAR)eaop2.fpFEA2List);

	rc = DosSetPathInfo(fullname,
						FIL_QUERYEASIZE,
						&eaop2,
						sizeof(eaop2),
						0);

	free((void *)eaop2.fpFEA2List);
}

void gui_setfd(fd_set *rd)
{
	/* Set the GUI IPC pipe readable for select() */
	FD_SET(guiipc[0], rd);
}
