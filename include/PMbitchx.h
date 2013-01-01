
/* PM specific stuff */
#ifndef PM_BitchX_h
#define PM_BitchX_h

#define FONTDIALOG      256

#define LB_FONTLIST     101
#define CB_DEFAULT      102
#define CB_CHANGEFORALL 103 

#define IDM_LOGO        355
#define IDM_MAINMENU    356

/* Properties book */
#define PROPNBK         357
#define ID_PROP         359
#define PB_DISMISS      360
#define NBKP_SETS1      362
#define SPB_SETS        363
#define SETS1           364
#define EF_ENTRY1       365
#define CB_ONOFF1       366
#define NBKP_OSSETTINGS 367
#define NBKP_SETS2      368
#define SETS2           369
#define EF_ENTRY2       370
#define CB_ONOFF2       371
#define NBKP_SETS3      372
#define SETS3           373
#define EF_ENTRY3       374
#define CB_ONOFF3       375
#define NBKP_SETS4      376
#define SETS4           377
#define EF_ENTRY4       378
#define CB_ONOFF4       379
#define CB_LFN          380
#define CB_SOUND        381

void pm_resize(Screen *this_screen);
void pm_clrscr(Screen *tmp);
void load_default_font(Screen *font_screen);

void pm_new_window(Screen *new, Window *win);
void pm_seticon(Screen *screen);
void wm_process(int param);

MRESULT EXPENTRY GenericWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY FrameWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

MenuStruct *findmenu(char *menuname);

#define QWP_USER 0

/* Not sure how to decide this */
#define MMPM 1

#endif
