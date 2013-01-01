/*
 * term.h: header file for term.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: ircterm.h 3 2008-02-25 09:49:14Z keaston $
 */

#ifndef _TERM_H_
# define _TERM_H_

#ifdef HAVE_NCURSES_H
# include <ncurses.h>
# ifdef HAVE_NCURSES_TERMCAP_H
#  include <ncurses/termcap.h>
# elif defined(HAVE_TERMCAP_H)
#  ifndef __CYGWIN__
#     include <termcap.h>
#  endif
# endif
#else
# ifdef HAVE_CURSES_H
#  include <curses.h>
# endif
# ifdef HAVE_TERMCAP_H
#  include <termcap.h>
# endif
#endif
#ifdef __EMX__
#include <termcap.h>
#endif

#include "irc_std.h"
#include "screen.h"

extern	int	need_redraw;
extern	int	meta_mode;

/* 
 * This puts a character to the current target, whatever it is. 
 * All output everywhere should go through this.
 * This does not mangle its output, so its suitable for outputting
 * escape sequences.
 */
#if !defined(WTERM_C) && !defined(WSERV_C)
#define current_ftarget (output_screen ? output_screen->fpout : stdout)

#ifdef __EMXPM__
void avio_set_var(int aviovar, int value);
int avio_get_var(int aviovar);
void avio_refresh_screen(void);
#define         AVIOREDRAW      1
#define         AVIORESIZED     2
#define         AVIOINSELECT    3
#define         AVIOINWMCHAR    4
#endif

#ifdef GUI
#include "gui.h"
#endif

#ifdef TRANSLATE
#include "translat.h"
__inline__
static int putchar_x (int c) {
#ifdef GUI
#if 1
	return gui_putc((int) (translation ? transToClient[c] : c));
#else
	return gui_putc((int) c);
#endif
#else
#if 1
	return fputc((int) (translation ? transToClient[c] : c), current_ftarget);
#else
	return fputc((int) c, current_ftarget);
#endif
#endif
}
#else
__inline__ 
static int putchar_x (int c) { 
#ifdef GUI
	return gui_putc((int) c);
#else
	return fputc((unsigned int) c, current_ftarget); 
#endif
}
#endif

__inline__
static void term_flush (void) { fflush(current_ftarget); }

#define	TERM_SGR_BOLD_ON	1
#define TERM_SGR_BOLD_OFF	2
#define TERM_SGR_BLINK_ON	3
#define TERM_SGR_BLINK_OFF	4
#define TERM_SGR_UNDL_ON	5
#define TERM_SGR_UNDL_OFF	6
#define TERM_SGR_REV_ON		7
#define TERM_SGR_REV_OFF	8
#define TERM_SGR_NORMAL		9
#define TERM_SGR_RESET		10
#define TERM_SGR_FOREGROUND	11
#define TERM_SGR_BACKGROUND	12
#define TERM_SGR_GCHAR		13
#define TERM_SGR_ALTCHAR_ON	14
#define TERM_SGR_ALTCHAR_OFF	15
#define TERM_SGR_MAXVAL		16

#define TERM_CAN_CUP		1 << 0
#define TERM_CAN_CLEAR		1 << 1
#define TERM_CAN_CLREOL		1 << 2
#define TERM_CAN_RIGHT		1 << 3
#define TERM_CAN_LEFT		1 << 4
#define TERM_CAN_SCROLL		1 << 5
#define TERM_CAN_DELETE		1 << 6
#define TERM_CAN_INSERT		1 << 7
#define TERM_CAN_DELLINES	1 << 8
#define TERM_CAN_INSLINES	1 << 9
#define TERM_CAN_REPEAT		1 << 10
#define TERM_CAN_BOLD		1 << 11
#define TERM_CAN_BLINK		1 << 12
#define TERM_CAN_UNDL		1 << 13
#define TERM_CAN_REVERSE	1 << 14
#define TERM_CAN_COLOR		1 << 15
#define TERM_CAN_GCHAR		1 << 16

#if 0
extern	char	*TI_cr, *TI_nl;
extern	int	TI_lines, TI_cols;
extern	char	*TI_sgrstrs[];
extern	char	*TI_forecolors[];
extern	char	*TI_backcolors[];
#endif

extern	int	termfeatures;


/*      Our variable name   Cap / Info      Description */

struct term_struct {
	int TI_bw;	 /* bw  / bw        cub1 wraps from column 0 to last column */
	int TI_am;	 /* am  / am        terminal has automatic margins */
	int TI_xsb;	 /* xb  / xsb       beehive (f1=escape, f2=ctrl C) */
	int TI_xhp;	 /* xs  / xhp       standout not erased by overwriting (hp) */
	int TI_xenl;	 /* xn  / xenl      newline ignored after 80 cols (concept) */
	int TI_eo;	 /* eo  / eo        can erase overstrikes with a blank */
	int TI_gn;	 /* gn  / gn        generic line type */
	int TI_hc;	 /* hc  / hc        hardcopy terminal */
	int TI_km;	 /* km  / km        Has a meta key (shift, sets parity bit) */
	int TI_hs;	 /* hs  / hs        has extra status line */
	int TI_in;	 /* in  / in        insert mode distinguishes nulls */
	int TI_da;	 /* da  / da        display may be retained above the screen */
	int TI_db;	 /* db  / db        display may be retained below the screen */
	int TI_mir;	 /* mi  / mir       safe to move while in insert mode */
	int TI_msgr;	 /* ms  / msgr      safe to move while in standout mode */
	int TI_os;	 /* os  / os        terminal can overstrike */
	int TI_eslok;	 /* es  / eslok     escape can be used on the status line */
	int TI_xt;	 /* xt  / xt        tabs destructive, magic so char (t1061) */
	int TI_hz;	 /* hz  / hz        can't print ~'s (hazeltine) */
	int TI_ul;	 /* ul  / ul        underline character overstrikes */
	int TI_xon;	 /* xo  / xon       terminal uses xon/xoff handshaking */
	int TI_nxon;	 /* nx  / nxon      padding won't work, xon/xoff required */
	int TI_mc5i;	 /* 5i  / mc5i      printer won't echo on screen */
	int TI_chts;	 /* HC  / chts      cursor is hard to see */
	int TI_nrrmc;	 /* NR  / nrrmc     smcup does not reverse rmcup */
	int TI_npc;	 /* NP  / npc       pad character does not exist */
	int TI_ndscr;	 /* ND  / ndscr     scrolling region is non-destructive */
	int TI_ccc;	 /* cc  / ccc       terminal can re-define existing colors */
	int TI_bce;	 /* ut  / bce       screen erased with background color */
	int TI_hls;	 /* hl  / hls       terminal uses only HLS color notation (Tektronix) */
	int TI_xhpa;	 /* YA  / xhpa      only positive motion for hpa/mhpa caps */
	int TI_crxm;	 /* YB  / crxm      using cr turns off micro mode */
	int TI_daisy;	 /* YC  / daisy     printer needs operator to change character set */
	int TI_xvpa;	 /* YD  / xvpa      only positive motion for vpa/mvpa caps */
	int TI_sam;	 /* YE  / sam       printing in last column causes cr */
	int TI_cpix;	 /* YF  / cpix      changing character pitch changes resolution */
	int TI_lpix;	 /* YG  / lpix      changing line pitch changes resolution */
	int TI_cols;	 /* co  / cols      number of columns in a line */
	int TI_it;	 /* it  / it        tabs initially every # spaces */
	int TI_lines;	 /* li  / lines     number of lines on screen or page */
	int TI_lm;	 /* lm  / lm        lines of memory if > line. 0 means varies */
	int TI_xmc;	 /* sg  / xmc       number of blank characters left by smso or rmso */
	int TI_pb;	 /* pb  / pb        lowest baud rate where padding needed */
	int TI_vt;	 /* vt  / vt        virtual terminal number (CB/unix) */
	int TI_wsl;	 /* ws  / wsl       number of columns in status line */
	int TI_nlab;	 /* Nl  / nlab      number of labels on screen */
	int TI_lh;	 /* lh  / lh        rows in each label */
	int TI_lw;	 /* lw  / lw        columns in each label */
	int TI_ma;	 /* ma  / ma        maximum combined attributes terminal can handle */
	int TI_wnum;	 /* MW  / wnum      maximum number of defineable windows */
	int TI_colors;	 /* Co  / colors    maximum number of colors on screen */
	int TI_pairs;	 /* pa  / pairs     maximum number of color-pairs on the screen */
	int TI_ncv;	 /* NC  / ncv       video attributes that can't be used with colors */
	int TI_bufsz;	 /* Ya  / bufsz     numbers of bytes buffered before printing */
	int TI_spinv;	 /* Yb  / spinv     spacing of pins vertically in pins per inch */
	int TI_spinh;	 /* Yc  / spinh     spacing of dots horizontally in dots per inch */
	int TI_maddr;	 /* Yd  / maddr     maximum value in micro_..._address */
	int TI_mjump;	 /* Ye  / mjump     maximum value in parm_..._micro */
	int TI_mcs;	 /* Yf  / mcs       character step size when in micro mode */
	int TI_mls;	 /* Yg  / mls       line step size when in micro mode */
	int TI_npins;	 /* Yh  / npins     numbers of pins in print-head */
	int TI_orc;	 /* Yi  / orc       horizontal resolution in units per line */
	int TI_orl;	 /* Yj  / orl       vertical resolution in units per line */
	int TI_orhi;	 /* Yk  / orhi      horizontal resolution in units per inch */
	int TI_orvi;	 /* Yl  / orvi      vertical resolution in units per inch */
	int TI_cps;	 /* Ym  / cps       print rate in characters per second */
	int TI_widcs;	 /* Yn  / widcs     character step size when in double wide mode */
	int TI_btns;	 /* BT  / btns      number of buttons on mouse */
	int TI_bitwin;	 /* Yo  / bitwin    number of passes for each bit-image row */
	int TI_bitype;	 /* Yp  / bitype    type of bit-image device */
	char *TI_cbt;	 /* bt  / cbt       back tab (P) */
	char *TI_bel;	 /* bl  / bel       audible signal (bell) (P) */
	char *TI_cr;	 /* cr  / cr        carriage return (P*) (P*) */
	char *TI_csr;	 /* cs  / csr       change region to line #1 to line #2 (P) */
	char *TI_tbc;	 /* ct  / tbc       clear all tab stops (P) */
	char *TI_clear;	 /* cl  / clear     clear screen and home cursor (P*) */
	char *TI_el;	 /* ce  / el        clear to end of line (P) */
	char *TI_ed;	 /* cd  / ed        clear to end of screen (P*) */
	char *TI_hpa;	 /* ch  / hpa       horizontal position #1, absolute (P) */
	char *TI_cmdch;	 /* CC  / cmdch     terminal settable cmd character in prototype !? */
	char *TI_cup;	 /* cm  / cup       move to row #1 columns #2 */
	char *TI_cud1;	 /* do  / cud1      down one line */
	char *TI_home;	 /* ho  / home      home cursor (if no cup) */
	char *TI_civis;	 /* vi  / civis     make cursor invisible */
	char *TI_cub1;	 /* le  / cub1      move left one space */
	char *TI_mrcup;	 /* CM  / mrcup     memory relative cursor addressing */
	char *TI_cnorm;	 /* ve  / cnorm     make cursor appear normal (undo civis/cvvis) */
	char *TI_cuf1;	 /* nd  / cuf1      non-destructive space (move right one space) */
	char *TI_ll;	 /* ll  / ll        last line, first column (if no cup) */
	char *TI_cuu1;	 /* up  / cuu1      up one line */
	char *TI_cvvis;	 /* vs  / cvvis     make cursor very visible */
	char *TI_dch1;	 /* dc  / dch1      delete character (P*) */
	char *TI_dl1;	 /* dl  / dl1       delete line (P*) */
	char *TI_dsl;	 /* ds  / dsl       disable status line */
	char *TI_hd;	 /* hd  / hd        half a line down */
	char *TI_smacs;	 /* as  / smacs     start alternate character set (P) */
	char *TI_blink;	 /* mb  / blink     turn on blinking */
	char *TI_bold;	 /* md  / bold      turn on bold (extra bright) mode */
	char *TI_smcup;	 /* ti  / smcup     string to start programs using cup */
	char *TI_smdc;	 /* dm  / smdc      enter delete mode */
	char *TI_dim;	 /* mh  / dim       turn on half-bright mode */
	char *TI_smir;	 /* im  / smir      enter insert mode */
	char *TI_invis;	 /* mk  / invis     turn on blank mode (characters invisible) */
	char *TI_prot;	 /* mp  / prot      turn on protected mode */
	char *TI_rev;	 /* mr  / rev       turn on reverse video mode */
	char *TI_smso;	 /* so  / smso      begin standout mode */
	char *TI_smul;	 /* us  / smul      begin underline mode */
	char *TI_ech;	 /* ec  / ech       erase #1 characters (P) */
	char *TI_rmacs;	 /* ae  / rmacs     end alternate character set (P) */
	char *TI_sgr0;	 /* me  / sgr0      turn off all attributes */
	char *TI_rmcup;	 /* te  / rmcup     strings to end programs using cup */
	char *TI_rmdc;	 /* ed  / rmdc      end delete mode */
	char *TI_rmir;	 /* ei  / rmir      exit insert mode */
	char *TI_rmso;	 /* se  / rmso      exit standout mode */
	char *TI_rmul;	 /* ue  / rmul      exit underline mode */
	char *TI_flash;	 /* vb  / flash     visible bell (may not move cursor) */
	char *TI_ff;	 /* ff  / ff        hardcopy terminal page eject (P*) */
	char *TI_fsl;	 /* fs  / fsl       return from status line */
	char *TI_is1;	 /* i1  / is1       initialization string */
	char *TI_is2;	 /* is  / is2       initialization string */
	char *TI_is3;	 /* i3  / is3       initialization string */
	char *TI_if;	 /* if  / if        name of initialization file */
	char *TI_ich1;	 /* ic  / ich1      insert character (P) */
	char *TI_il1;	 /* al  / il1       insert line (P*) */
	char *TI_ip;	 /* ip  / ip        insert padding after inserted character */
	char *TI_kbs;	 /* kb  / kbs       backspace key */
	char *TI_ktbc;	 /* ka  / ktbc      clear-all-tabs key */
	char *TI_kclr;	 /* kC  / kclr      clear-screen or erase key */
	char *TI_kctab;	 /* kt  / kctab     clear-tab key */
	char *TI_kdch1;	 /* kD  / kdch1     delete-character key */
	char *TI_kdl1;	 /* kL  / kdl1      delete-line key */
	char *TI_kcud1;	 /* kd  / kcud1     down-arrow key */
	char *TI_krmir;	 /* kM  / krmir     sent by rmir or smir in insert mode */
	char *TI_kel;	 /* kE  / kel       clear-to-end-of-line key */
	char *TI_ked;	 /* kS  / ked       clear-to-end-of-screen key */
	char *TI_kf0;	 /* k0  / kf0       F0 function key */
	char *TI_kf1;	 /* k1  / kf1       F1 function key */
	char *TI_kf10;	 /* k;  / kf10      F10 function key */
	char *TI_kf2;	 /* k2  / kf2       F2 function key */
	char *TI_kf3;	 /* k3  / kf3       F3 function key */
	char *TI_kf4;	 /* k4  / kf4       F4 function key */
	char *TI_kf5;	 /* k5  / kf5       F5 function key */
	char *TI_kf6;	 /* k6  / kf6       F6 function key */
	char *TI_kf7;	 /* k7  / kf7       F7 function key */
	char *TI_kf8;	 /* k8  / kf8       F8 function key */
	char *TI_kf9;	 /* k9  / kf9       F9 function key */
	char *TI_khome;	 /* kh  / khome     home key */
	char *TI_kich1;	 /* kI  / kich1     insert-character key */
	char *TI_kil1;	 /* kA  / kil1      insert-line key */
	char *TI_kcub1;	 /* kl  / kcub1     left-arrow key */
	char *TI_kll;	 /* kH  / kll       lower-left key (home down) */
	char *TI_knp;	 /* kN  / knp       next-page key */
	char *TI_kpp;	 /* kP  / kpp       previous-page key */
	char *TI_kcuf1;	 /* kr  / kcuf1     right-arrow key */
	char *TI_kind;	 /* kF  / kind      scroll-forward key */
	char *TI_kri;	 /* kR  / kri       scroll-backward key */
	char *TI_khts;	 /* kT  / khts      set-tab key */
	char *TI_kcuu1;	 /* ku  / kcuu1     up-arrow key */
	char *TI_rmkx;	 /* ke  / rmkx      leave 'keyboard_transmit' mode */
	char *TI_smkx;	 /* ks  / smkx      enter 'keyboard_transmit' mode */
	char *TI_lf0;	 /* l0  / lf0       label on function key f0 if not f0 */
	char *TI_lf1;	 /* l1  / lf1       label on function key f1 if not f1 */
	char *TI_lf10;	 /* la  / lf10      label on function key f10 if not f10 */
	char *TI_lf2;	 /* l2  / lf2       label on function key f2 if not f2 */
	char *TI_lf3;	 /* l3  / lf3       label on function key f3 if not f3 */
	char *TI_lf4;	 /* l4  / lf4       label on function key f4 if not f4 */
	char *TI_lf5;	 /* l5  / lf5       label on function key f5 if not f5 */
	char *TI_lf6;	 /* l6  / lf6       label on function key f6 if not f6 */
	char *TI_lf7;	 /* l7  / lf7       label on function key f7 if not f7 */
	char *TI_lf8;	 /* l8  / lf8       label on function key f8 if not f8 */
	char *TI_lf9;	 /* l9  / lf9       label on function key f9 if not f9 */
	char *TI_rmm;	 /* mo  / rmm       turn off meta mode */
	char *TI_smm;	 /* mm  / smm       turn on meta mode (8th-bit on) */
	char *TI_nel;	 /* nw  / nel       newline (behave like cr followed by lf) */
	char *TI_pad;	 /* pc  / pad       padding char (instead of null) */
	char *TI_dch;	 /* DC  / dch       delete #1 characters (P*) */
	char *TI_dl;	 /* DL  / dl        delete #1 lines (P*) */
	char *TI_cud;	 /* DO  / cud       down #1 lines (P*) */
	char *TI_ich;	 /* IC  / ich       insert #1 characters (P*) */
	char *TI_indn;	 /* SF  / indn      scroll forward #1 lines (P) */
	char *TI_il;	 /* AL  / il        insert #1 lines (P*) */
	char *TI_cub;	 /* LE  / cub       move #1 characters to the left (P) */
	char *TI_cuf;	 /* RI  / cuf       move #1 characters to the right (P*) */
	char *TI_rin;	 /* SR  / rin       scroll back #1 lines (P) */
	char *TI_cuu;	 /* UP  / cuu       up #1 lines (P*) */
	char *TI_pfkey;	 /* pk  / pfkey     program function key #1 to type string #2 */
	char *TI_pfloc;	 /* pl  / pfloc     program function key #1 to execute string #2 */
	char *TI_pfx;	 /* px  / pfx       program function key #1 to transmit string #2 */
	char *TI_mc0;	 /* ps  / mc0       print contents of screen */
	char *TI_mc4;	 /* pf  / mc4       turn off printer */
	char *TI_mc5;	 /* po  / mc5       turn on printer */
	char *TI_rep;	 /* rp  / rep       repeat char #1 #2 times (P*) */
	char *TI_rs1;	 /* r1  / rs1       reset string */
	char *TI_rs2;	 /* r2  / rs2       reset string */
	char *TI_rs3;	 /* r3  / rs3       reset string */
	char *TI_rf;	 /* rf  / rf        name of reset file */
	char *TI_rc;	 /* rc  / rc        restore cursor to position of last save_cursor */
	char *TI_vpa;	 /* cv  / vpa       vertical position #1 absolute (P) */
	char *TI_sc;	 /* sc  / sc        save current cursor position (P) */
	char *TI_ind;	 /* sf  / ind       scroll text up (P) */
	char *TI_ri;	 /* sr  / ri        scroll text down (P) */
	char *TI_sgr;	 /* sa  / sgr       define video attributes #1-#9 (PG9) */
	char *TI_hts;	 /* st  / hts       set a tab in every row, current columns */
	char *TI_wind;	 /* wi  / wind      current window is lines #1-#2 cols #3-#4 */
	char *TI_ht;	 /* ta  / ht        tab to next 8-space hardware tab stop */
	char *TI_tsl;	 /* ts  / tsl       move to status line */
	char *TI_uc;	 /* uc  / uc        underline char and move past it */
	char *TI_hu;	 /* hu  / hu        half a line up */
	char *TI_iprog;	 /* iP  / iprog     path name of program for initialization */
	char *TI_ka1;	 /* K1  / ka1       upper left of keypad */
	char *TI_ka3;	 /* K3  / ka3       upper right of keypad */
	char *TI_kb2;	 /* K2  / kb2       center of keypad */
	char *TI_kc1;	 /* K4  / kc1       lower left of keypad */
	char *TI_kc3;	 /* K5  / kc3       lower right of keypad */
	char *TI_mc5p;	 /* pO  / mc5p      turn on printer for #1 bytes */
	char *TI_rmp;	 /* rP  / rmp       like ip but when in insert mode */
	char *TI_acsc;	 /* ac  / acsc      graphics charset pairs, based on vt100 */
	char *TI_pln;	 /* pn  / pln       program label #1 to show string #2 */
	char *TI_kcbt;	 /* kB  / kcbt      back-tab key */
	char *TI_smxon;	 /* SX  / smxon     turn on xon/xoff handshaking */
	char *TI_rmxon;	 /* RX  / rmxon     turn off xon/xoff handshaking */
	char *TI_smam;	 /* SA  / smam      turn on automatic margins */
	char *TI_rmam;	 /* RA  / rmam      turn off automatic margins */
	char *TI_xonc;	 /* XN  / xonc      XON character */
	char *TI_xoffc;	 /* XF  / xoffc     XOFF character */
	char *TI_enacs;	 /* eA  / enacs     enable alternate char set */
	char *TI_smln;	 /* LO  / smln      turn on soft labels */
	char *TI_rmln;	 /* LF  / rmln      turn off soft labels */
	char *TI_kbeg;	 /* @1  / kbeg      begin key */
	char *TI_kcan;	 /* @2  / kcan      cancel key */
	char *TI_kclo;	 /* @3  / kclo      close key */
	char *TI_kcmd;	 /* @4  / kcmd      command key */
	char *TI_kcpy;	 /* @5  / kcpy      copy key */
	char *TI_kcrt;	 /* @6  / kcrt      create key */
	char *TI_kend;	 /* @7  / kend      end key */
	char *TI_kent;	 /* @8  / kent      enter/send key */
	char *TI_kext;	 /* @9  / kext      exit key */
	char *TI_kfnd;	 /* @0  / kfnd      find key */
	char *TI_khlp;	 /* %1  / khlp      help key */
	char *TI_kmrk;	 /* %2  / kmrk      mark key */
	char *TI_kmsg;	 /* %3  / kmsg      message key */
	char *TI_kmov;	 /* %4  / kmov      move key */
	char *TI_knxt;	 /* %5  / knxt      next key */
	char *TI_kopn;	 /* %6  / kopn      open key */
	char *TI_kopt;	 /* %7  / kopt      options key */
	char *TI_kprv;	 /* %8  / kprv      previous key */
	char *TI_kprt;	 /* %9  / kprt      print key */
	char *TI_krdo;	 /* %0  / krdo      redo key */
	char *TI_kref;	 /* &1  / kref      reference key */
	char *TI_krfr;	 /* &2  / krfr      refresh key */
	char *TI_krpl;	 /* &3  / krpl      replace key */
	char *TI_krst;	 /* &4  / krst      restart key */
	char *TI_kres;	 /* &5  / kres      resume key */
	char *TI_ksav;	 /* &6  / ksav      save key */
	char *TI_kspd;	 /* &7  / kspd      suspend key */
	char *TI_kund;	 /* &8  / kund      undo key */
	char *TI_kBEG;	 /* &9  / kBEG      shifted begin key */
	char *TI_kCAN;	 /* &0  / kCAN      shifted cancel key */
	char *TI_kCMD;	 /* *1  / kCMD      shifted command key */
	char *TI_kCPY;	 /* *2  / kCPY      shifted copy key */
	char *TI_kCRT;	 /* *3  / kCRT      shifted create key */
	char *TI_kDC;	 /* *4  / kDC       shifted delete-character key */
	char *TI_kDL;	 /* *5  / kDL       shifted delete-line key */
	char *TI_kslt;	 /* *6  / kslt      select key */
	char *TI_kEND;	 /* *7  / kEND      shifted end key */
	char *TI_kEOL;	 /* *8  / kEOL      shifted clear-to-end-of-line key */
	char *TI_kEXT;	 /* *9  / kEXT      shifted exit key */
	char *TI_kFND;	 /* *0  / kFND      shifted find key */
	char *TI_kHLP;	 /* #1  / kHLP      shifted help key */
	char *TI_kHOM;	 /* #2  / kHOM      shifted home key */
	char *TI_kIC;	 /* #3  / kIC       shifted insert-character key */
	char *TI_kLFT;	 /* #4  / kLFT      shifted left-arrow key */
	char *TI_kMSG;	 /* %a  / kMSG      shifted message key */
	char *TI_kMOV;	 /* %b  / kMOV      shifted move key */
	char *TI_kNXT;	 /* %c  / kNXT      shifted next key */
	char *TI_kOPT;	 /* %d  / kOPT      shifted options key */
	char *TI_kPRV;	 /* %e  / kPRV      shifted previous key */
	char *TI_kPRT;	 /* %f  / kPRT      shifted print key */
	char *TI_kRDO;	 /* %g  / kRDO      shifted redo key */
	char *TI_kRPL;	 /* %h  / kRPL      shifted replace key */
	char *TI_kRIT;	 /* %i  / kRIT      shifted right-arrow key */
	char *TI_kRES;	 /* %j  / kRES      shifted resume key */
	char *TI_kSAV;	 /* !1  / kSAV      shifted save key */
	char *TI_kSPD;	 /* !2  / kSPD      shifted suspend key */
	char *TI_kUND;	 /* !3  / kUND      shifted undo key */
	char *TI_rfi;	 /* RF  / rfi       send next input char (for ptys) */
	char *TI_kf11;	 /* F1  / kf11      F11 function key */
	char *TI_kf12;	 /* F2  / kf12      F12 function key */
	char *TI_kf13;	 /* F3  / kf13      F13 function key */
	char *TI_kf14;	 /* F4  / kf14      F14 function key */
	char *TI_kf15;	 /* F5  / kf15      F15 function key */
	char *TI_kf16;	 /* F6  / kf16      F16 function key */
	char *TI_kf17;	 /* F7  / kf17      F17 function key */
	char *TI_kf18;	 /* F8  / kf18      F18 function key */
	char *TI_kf19;	 /* F9  / kf19      F19 function key */
	char *TI_kf20;	 /* FA  / kf20      F20 function key */
	char *TI_kf21;	 /* FB  / kf21      F21 function key */
	char *TI_kf22;	 /* FC  / kf22      F22 function key */
	char *TI_kf23;	 /* FD  / kf23      F23 function key */
	char *TI_kf24;	 /* FE  / kf24      F24 function key */
	char *TI_kf25;	 /* FF  / kf25      F25 function key */
	char *TI_kf26;	 /* FG  / kf26      F26 function key */
	char *TI_kf27;	 /* FH  / kf27      F27 function key */
	char *TI_kf28;	 /* FI  / kf28      F28 function key */
	char *TI_kf29;	 /* FJ  / kf29      F29 function key */
	char *TI_kf30;	 /* FK  / kf30      F30 function key */
	char *TI_kf31;	 /* FL  / kf31      F31 function key */
	char *TI_kf32;	 /* FM  / kf32      F32 function key */
	char *TI_kf33;	 /* FN  / kf33      F33 function key */
	char *TI_kf34;	 /* FO  / kf34      F34 function key */
	char *TI_kf35;	 /* FP  / kf35      F35 function key */
	char *TI_kf36;	 /* FQ  / kf36      F36 function key */
	char *TI_kf37;	 /* FR  / kf37      F37 function key */
	char *TI_kf38;	 /* FS  / kf38      F38 function key */
	char *TI_kf39;	 /* FT  / kf39      F39 function key */
	char *TI_kf40;	 /* FU  / kf40      F40 function key */
	char *TI_kf41;	 /* FV  / kf41      F41 function key */
	char *TI_kf42;	 /* FW  / kf42      F42 function key */
	char *TI_kf43;	 /* FX  / kf43      F43 function key */
	char *TI_kf44;	 /* FY  / kf44      F44 function key */
	char *TI_kf45;	 /* FZ  / kf45      F45 function key */
	char *TI_kf46;	 /* Fa  / kf46      F46 function key */
	char *TI_kf47;	 /* Fb  / kf47      F47 function key */
	char *TI_kf48;	 /* Fc  / kf48      F48 function key */
	char *TI_kf49;	 /* Fd  / kf49      F49 function key */
	char *TI_kf50;	 /* Fe  / kf50      F50 function key */
	char *TI_kf51;	 /* Ff  / kf51      F51 function key */
	char *TI_kf52;	 /* Fg  / kf52      F52 function key */
	char *TI_kf53;	 /* Fh  / kf53      F53 function key */
	char *TI_kf54;	 /* Fi  / kf54      F54 function key */
	char *TI_kf55;	 /* Fj  / kf55      F55 function key */
	char *TI_kf56;	 /* Fk  / kf56      F56 function key */
	char *TI_kf57;	 /* Fl  / kf57      F57 function key */
	char *TI_kf58;	 /* Fm  / kf58      F58 function key */
	char *TI_kf59;	 /* Fn  / kf59      F59 function key */
	char *TI_kf60;	 /* Fo  / kf60      F60 function key */
	char *TI_kf61;	 /* Fp  / kf61      F61 function key */
	char *TI_kf62;	 /* Fq  / kf62      F62 function key */
	char *TI_kf63;	 /* Fr  / kf63      F63 function key */
	char *TI_el1;	 /* cb  / el1       Clear to beginning of line */
	char *TI_mgc;	 /* MC  / mgc       clear right and left soft margins */
	char *TI_smgl;	 /* ML  / smgl      set left soft margin at current column */
	char *TI_smgr;	 /* MR  / smgr      set right soft margin at current column */
	char *TI_fln;	 /* Lf  / fln       label format */
	char *TI_sclk;	 /* SC  / sclk      set clock, #1 hrs #2 mins #3 secs */
	char *TI_dclk;	 /* DK  / dclk      display clock at (#1,#2) */
	char *TI_rmclk;	 /* RC  / rmclk     remove clock */
	char *TI_cwin;	 /* CW  / cwin      define a window #1 from #2,#3 to #4,#5 */
	char *TI_wingo;	 /* WG  / wingo     go to window #1 */
	char *TI_hup;	 /* HU  / hup       hang-up phone */
	char *TI_dial;	 /* DI  / dial      dial number #1 */
	char *TI_qdial;	 /* QD  / qdial     dial number #1 without checking */
	char *TI_tone;	 /* TO  / tone      select touch tone dialing */
	char *TI_pulse;	 /* PU  / pulse     select pulse dialing */
	char *TI_hook;	 /* fh  / hook      flash switch hook */
	char *TI_pause;	 /* PA  / pause     pause for 2-3 seconds */
	char *TI_wait;	 /* WA  / wait      wait for dial-tone */
	char *TI_u0;	 /* u0  / u0        User string #0 */
	char *TI_u1;	 /* u1  / u1        User string #1 */
	char *TI_u2;	 /* u2  / u2        User string #2 */
	char *TI_u3;	 /* u3  / u3        User string #3 */
	char *TI_u4;	 /* u4  / u4        User string #4 */
	char *TI_u5;	 /* u5  / u5        User string #5 */
	char *TI_u6;	 /* u6  / u6        User string #6 */
	char *TI_u7;	 /* u7  / u7        User string #7 */
	char *TI_u8;	 /* u8  / u8        User string #8 */
	char *TI_u9;	 /* u9  / u9        User string #9 */
	char *TI_op;	 /* op  / op        Set default pair to its original value */
	char *TI_oc;	 /* oc  / oc        Set all color pairs to the original ones */
	char *TI_initc;	 /* Ic  / initc     initialize color #1 to (#2,#3,#4) */
	char *TI_initp;	 /* Ip  / initp     Initialize color pair #1 to fg=(#2,#3,#4), bg=(#5,#6,#7) */
	char *TI_scp;	 /* sp  / scp       Set current color pair to #1 */
	char *TI_setf;	 /* Sf  / setf      Set foreground color #1 */
	char *TI_setb;	 /* Sb  / setb      Set background color #1 */
	char *TI_cpi;	 /* ZA  / cpi       Change number of characters per inch */
	char *TI_lpi;	 /* ZB  / lpi       Change number of lines per inch */
	char *TI_chr;	 /* ZC  / chr       Change horizontal resolution */
	char *TI_cvr;	 /* ZD  / cvr       Change vertical resolution */
	char *TI_defc;	 /* ZE  / defc      Define a character */
	char *TI_swidm;	 /* ZF  / swidm     Enter double-wide mode */
	char *TI_sdrfq;	 /* ZG  / sdrfq     Enter draft-quality mode */
	char *TI_sitm;	 /* ZH  / sitm      Enter italic mode */
	char *TI_slm;	 /* ZI  / slm       Start leftward carriage motion */
	char *TI_smicm;	 /* ZJ  / smicm     Start micro-motion mode */
	char *TI_snlq;	 /* ZK  / snlq      Enter NLQ mode */
	char *TI_snrmq;	 /* ZL  / snrmq     Enter normal-quality mode */
	char *TI_sshm;	 /* ZM  / sshm      Enter shadow-print mode */
	char *TI_ssubm;	 /* ZN  / ssubm     Enter subscript mode */
	char *TI_ssupm;	 /* ZO  / ssupm     Enter superscript mode */
	char *TI_sum;	 /* ZP  / sum       Start upward carriage motion */
	char *TI_rwidm;	 /* ZQ  / rwidm     End double-wide mode */
	char *TI_ritm;	 /* ZR  / ritm      End italic mode */
	char *TI_rlm;	 /* ZS  / rlm       End left-motion mode */
	char *TI_rmicm;	 /* ZT  / rmicm     End micro-motion mode */
	char *TI_rshm;	 /* ZU  / rshm      End shadow-print mode */
	char *TI_rsubm;	 /* ZV  / rsubm     End subscript mode */
	char *TI_rsupm;	 /* ZW  / rsupm     End superscript mode */
	char *TI_rum;	 /* ZX  / rum       End reverse character motion */
	char *TI_mhpa;	 /* ZY  / mhpa      Like column_address in micro mode */
	char *TI_mcud1;	 /* ZZ  / mcud1     Like cursor_down in micro mode */
	char *TI_mcub1;	 /* Za  / mcub1     Like cursor_left in micro mode */
	char *TI_mcuf1;	 /* Zb  / mcuf1     Like cursor_right in micro mode */
	char *TI_mvpa;	 /* Zc  / mvpa      Like row_address in micro mode */
	char *TI_mcuu1;	 /* Zd  / mcuu1     Like cursor_up in micro mode */
	char *TI_porder; /* Ze  / porder    Match software bits to print-head pins */
	char *TI_mcud;	 /* Zf  / mcud      Like parm_down_cursor in micro mode */
	char *TI_mcub;	 /* Zg  / mcub      Like parm_left_cursor in micro mode */
	char *TI_mcuf;	 /* Zh  / mcuf      Like parm_right_cursor in micro mode */
	char *TI_mcuu;	 /* Zi  / mcuu      Like parm_up_cursor in micro mode */
	char *TI_scs;	 /* Zj  / scs       Select character set */
	char *TI_smgb;	 /* Zk  / smgb      Set bottom margin at current line */
	char *TI_smgbp;	 /* Zl  / smgbp     Set bottom margin at line #1 or #2 lines from bottom */
	char *TI_smglp;	 /* Zm  / smglp     Set left (right) margin at column #1 (#2) */
	char *TI_smgrp;	 /* Zn  / smgrp     Set right margin at column #1 */
	char *TI_smgt;	 /* Zo  / smgt      Set top margin at current line */
	char *TI_smgtp;	 /* Zp  / smgtp     Set top (bottom) margin at row #1 (#2) */
	char *TI_sbim;	 /* Zq  / sbim      Start printing bit image graphics */
	char *TI_scsd;	 /* Zr  / scsd      Start character set definition */
	char *TI_rbim;	 /* Zs  / rbim      Stop printing bit image graphics */
	char *TI_rcsd;	 /* Zt  / rcsd      End definition of character set */
	char *TI_subcs;	 /* Zu  / subcs     List of subscriptable characters */
	char *TI_supcs;	 /* Zv  / supcs     List of superscriptable characters */
	char *TI_docr;	 /* Zw  / docr      Printing any of these characters causes CR */
	char *TI_zerom;	 /* Zx  / zerom     No motion for subsequent character */
	char *TI_csnm;	 /* Zy  / csnm      List of character set names */
	char *TI_kmous;	 /* Km  / kmous     Mouse event has occurred */
	char *TI_minfo;	 /* Mi  / minfo     Mouse status information */
	char *TI_reqmp;	 /* RQ  / reqmp     Request mouse position */
	char *TI_getm;	 /* Gm  / getm      Curses should get button events */
	char *TI_setaf;	 /* AF  / setaf     Set foreground color using ANSI escape */
	char *TI_setab;	 /* AB  / setab     Set background color using ANSI escape */
	char *TI_pfxl;	 /* xl  / pfxl      Program function key #1 to type string #2 and show string #3 */
	char *TI_devt;	 /* dv  / devt      Indicate language/codeset support */
	char *TI_csin;	 /* ci  / csin      Init sequence for multiple codesets */
	char *TI_s0ds;	 /* s0  / s0ds      Shift to code set 0 (EUC set 0, ASCII) */
	char *TI_s1ds;	 /* s1  / s1ds      Shift to code set 1 */
	char *TI_s2ds;	 /* s2  / s2ds      Shift to code set 2 */
	char *TI_s3ds;	 /* s3  / s3ds      Shift to code set 3 */
	char *TI_smglr;	 /* ML  / smglr     Set both left and right margins to #1, #2 */
	char *TI_smgtb;	 /* MT  / smgtb     Sets both top and bottom margins to #1, #2 */
	char *TI_birep;	 /* Xy  / birep     Repeat bit image cell #1 #2 times */
	char *TI_binel;	 /* Zz  / binel     Move to next row of the bit image */
	char *TI_bicr;	 /* Yv  / bicr      Move to beginning of same row */
	char *TI_colornm; /* Yw  / colornm   Give name for color #1 */
	char *TI_defbi;	 /* Yx  / defbi     Define rectangualar bit image region */
	char *TI_endbi;	 /* Yy  / endbi     End a bit-image region */
	char *TI_setcolor; /* Yz  / setcolor  Change to ribbon color #1 */
	char *TI_slines; /* YZ  / slines    Set page length to #1 lines */
	char *TI_dispc;	 /* S1  / dispc     Display PC character */
	char *TI_smpch;	 /* S2  / smpch     Enter PC character display mode */
	char *TI_rmpch;	 /* S3  / rmpch     Exit PC character display mode */
	char *TI_smsc;	 /* S4  / smsc      Enter PC scancode mode */
	char *TI_rmsc;	 /* S5  / rmsc      Exit PC scancode mode */
	char *TI_pctrm;	 /* S6  / pctrm     PC terminal options */
	char *TI_scesc;	 /* S7  / scesc     Escape for scancode emulation */
	char *TI_scesa;	 /* S8  / scesa     Alternate escape for scancode emulation */
	char *TI_ehhlm;	 /* Xh  / ehhlm     Enter horizontal highlight mode */
	char *TI_elhlm;	 /* Xl  / elhlm     Enter left highlight mode */
	char *TI_elohlm; /* Xo  / elohlm    Enter low highlight mode */
	char *TI_erhlm;	 /* Xr  / erhlm     Enter right highlight mode */
	char *TI_ethlm;	 /* Xt  / ethlm     Enter top highlight mode */
	char *TI_evhlm;	 /* Xv  / evhlm     Enter vertical highlight mode */
	char *TI_sgr1;	 /* sA  / sgr1      Define second set of video attributes #1-#6 */
	char *TI_slength; /* sL  / slength   YI Set page length to #1 hundredth of an inch */
	char *TI_OTi2;	 /* i2  / OTi2      secondary initialization string */
	char *TI_OTrs;	 /* rs  / OTrs      terminal reset string */
	int TI_OTug;	 /* ug  / OTug      number of blanks left by ul */
	int TI_OTbs;	 /* bs  / OTbs      uses ^H to move left */
	int TI_OTns;	 /* ns  / OTns      crt cannot scroll */
	int TI_OTnc;	 /* nc  / OTnc      no way to go to start of line */
	int TI_OTdC;	 /* dC  / OTdC      pad needed for CR */
	int TI_OTdN;	 /* dN  / OTdN      pad needed for LF */
	char *TI_OTnl;	 /* nl  / OTnl      use to move down */
	char *TI_OTbc;	 /* bc  / OTbc      move left, if not ^H */
	int TI_OTMT;	 /* MT  / OTMT      has meta key */
	int TI_OTNL;	 /* NL  / OTNL      move down with \n */
	int TI_OTdB;	 /* dB  / OTdB      padding required for ^H */
	int TI_OTdT;	 /* dT  / OTdT      padding required for ^I */
	int TI_OTkn;	 /* kn  / OTkn      count of function keys */
	char *TI_OTko;	 /* ko  / OTko      list of self-mapped keycaps */
	char *TI_OTma;	 /* ma  / OTma      map arrow keys rogue(1) motion keys */
	int TI_OTpt;	 /* pt  / OTpt      has 8-char tabs invoked with ^I */
	int TI_OTxr;	 /* xr  / OTxr      return clears the line */
	char *TI_OTG2;	 /* G2  / OTG2      single upper left */
	char *TI_OTG3;	 /* G3  / OTG3      single lower left */
	char *TI_OTG1;	 /* G1  / OTG1      single upper right */
	char *TI_OTG4;	 /* G4  / OTG4      single lower right */
	char *TI_OTGR;	 /* GR  / OTGR      tee pointing right */
	char *TI_OTGL;	 /* GL  / OTGL      tee pointing left */
	char *TI_OTGU;	 /* GU  / OTGU      tee pointing up */
	char *TI_OTGD;	 /* GD  / OTGD      tee pointing down */
	char *TI_OTGH;	 /* GH  / OTGH      single horizontal line */
	char *TI_OTGV;	 /* GV  / OTGV      single vertical line */
	char *TI_OTGC;	 /* GC  / OTGC      single intersection */
	char *TI_meml;	 /* ml  / meml      memory lock above */
	char *TI_memu;	 /* mu  / memu      memory unlock */
	char *TI_box1;	 /* bx  / box1      box characters primary set */
	/* non termcap/terminfo terminal info (generated by epic) */
	char TI_normal[256];
	char *TI_sgrstrs[TERM_SGR_MAXVAL];
	char *TI_forecolors[16];
	char *TI_backcolors[16];
	int TI_meta_mode;
/*	int TI_need_redraw ; */
};

extern	struct	term_struct	*current_term;

#define term_has(x) 	(termfeatures & (x))
#define capstr(x) 	(current_term->TI_sgrstrs[(TERM_SGR_ ## x)-1])
#define outcap(x)	(tputs_x(capstr(x)))

#ifndef TERM_DEBUG
#define tputs_x(s)		(tputs(s, 0, putchar_x))
#else
int	tputs_x(char *);
#endif
#if 0
#define term_underline_on()	outcap(UNDL_ON)
#define term_underline_off()	outcap(UNDL_OFF)
#define term_standout_on()	outcap(REV_ON)
#define term_standout_off()	outcap(REV_OFF)
#define term_blink_on()		outcap(BLINK_ON)
#define term_blink_off()	outcap(BLINK_OFF)
#define term_bold_on()		outcap(BOLD_ON)
#define	term_bold_off()		outcap(BOLD_OFF)
#define term_altcharset_on()	outcap(ALTCHAR_ON)
#define term_altcharset_off()	outcap(ALTCHAR_OFF)
#endif
#define term_underline_on()	if (get_int_var(UNDERLINE_VIDEO_VAR)) \
					outcap(UNDL_ON)
#define term_underline_off()	if (get_int_var(UNDERLINE_VIDEO_VAR)) \
					outcap(UNDL_OFF)
#define term_standout_on()	if (get_int_var(INVERSE_VIDEO_VAR)) \
					outcap(REV_ON)
#define term_standout_off()	if (get_int_var(INVERSE_VIDEO_VAR)) \
					outcap(REV_OFF)
#define term_blink_on()		if (get_int_var(BLINK_VIDEO_VAR)) \
					outcap(BLINK_ON)
#define term_blink_off()	if (get_int_var(BLINK_VIDEO_VAR)) \
					outcap(BLINK_OFF)
#define term_bold_on()		if (get_int_var(BOLD_VIDEO_VAR)) \
					outcap(BOLD_ON)
#define	term_bold_off()		if (get_int_var(BOLD_VIDEO_VAR)) \
					outcap(BOLD_OFF)
#define term_altcharset_on()	if (get_int_var(ALT_CHARSET_VAR)) \
					outcap(ALTCHAR_ON)
#define term_altcharset_off()	if (get_int_var(ALT_CHARSET_VAR)) \
					outcap(ALTCHAR_OFF)

#define term_set_foreground(x)	tputs_x(current_term->TI_forecolors[(x) & 0x0f])
#define term_set_background(x)	tputs_x(current_term->TI_backcolors[(x) & 0x0f])
#define term_set_attribs(f,b)	tputs_x(term_getsgr(TERM_SGR_COLORS,(f),(b)))
#define term_putgchar(x)	tputs_x(term_getsgr(TERM_SGR_GCHAR,(x),0))
#define term_move_cursor(c, r)	term_gotoxy((c),(r))
#define term_cr()		tputs_x(current_term->TI_cr)
#define term_newline()		tputs_x(current_term->TI_nel)
#define term_cursor_left()	term_left(1)
#define term_cursor_right()	term_right(1)
#define term_clear_to_eol()	term_clreol()
#define term_clear_screen()	term_clrscr()

#ifdef __EMX__
extern int vio_screen;
extern char default_pair[2];
#endif
#endif

	RETSIGTYPE 	term_cont 		(int);
	void 		term_beep 		(void);
	int		term_echo 		(int);
	int		term_init 		(char *);
	int		term_resize 		(void);
	void		term_pause 		(char, char *);
	void		term_putchar 		(unsigned char);
	void		term_scroll 		(int, int, int);
	void		term_insert 		(unsigned char);
	void		term_delete 		(int);
	void		term_repeat		(unsigned char, int);
	void		term_right		(int);
	void		term_left		(int);
	void		term_clreol		(void);
	void		term_clrscr		(void);
	void		term_gotoxy		(int, int);
	void		term_reset		(void);
	int		term_eight_bit		(void);
	void		set_term_eight_bit	(int);
	void		set_meta_8bit		(Window *, char *, int);
	char *		term_getsgr		(int, int, int);
	void		reinit_term		(int);
	void		term_reset2		(void);
	void		reset_cols		(int);
	void		reset_lines		(int);
	char *		term_getsgr		(int, int, int);
	char *		get_term_capability	(char *, int, int);

#if 0
#ifndef TPUTS_DECLARED
int	tputs (const unsigned char *, int, int (*)(int));	
#endif
#endif

#endif /* _TERM_H_ */
