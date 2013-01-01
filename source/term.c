/*
 * term.c -- termios and termcap handlers
 *
 * Written By Matthew Green, based on window.c by Michael Sandrof
 * Copyright(c) 1993 Matthew Green
 * Significant modifications by Jeremy Nelson
 * Copyright 1997 EPIC Software Labs,
 * Significant additions by J. Kean Johnston
 * Copyright 1998 J. Kean Johnston, used with permission
 * Modifications Copyright Colten Edwards 1997-1998.
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT
 */

#include "irc.h"
static char cvsrevision[] = "$Id: term.c 17 2008-03-10 06:38:25Z keaston $";
CVS_REVISION(term_c)
#include "struct.h"
#include "screen.h"
#include "ircaux.h"

#ifdef __EMX__
static BYTE pair[2];
char default_pair[2];
VIOMODEINFO vmode;
int	vio_screen = 0;
#endif


#ifdef WINNT
HANDLE ghstdout;
CONSOLE_SCREEN_BUFFER_INFO gscrbuf;
CONSOLE_CURSOR_INFO gcursbuf;
DWORD gdwPlatform;
#endif

#include "vars.h"
#include "struct.h"
#include "ircterm.h"
#include "window.h"
#include "screen.h"
#include "output.h"
#ifdef TRANSLATE
#include "translat.h"
#endif

#define MAIN_SOURCE
#include "modval.h"

#if defined(_ALL_SOURCE) || defined(__EMX__) || defined(__QNX__) || defined(__FreeBSD__)
#include <termios.h>
#else
#include <sys/termios.h>
#endif

#include <sys/ioctl.h>

static	int		tty_des;		/* descriptor for the tty */

static	struct	termios	oldb, newb;

	char		my_PC, *BC, *UP;
	int		BClen, UPlen;
extern	int		already_detached;

#ifdef WTERM_C
int use_socks = 0;
char *get_string_var(enum VAR_TYPES var) { return NULL; }
char username[40];
void put_it(const char *str, ...) { return; }
#endif

                
#if !defined(WTERM_C)

/* Systems cant seem to agree where to put these... */
#ifdef HAVE_TERMINFO
extern	int		setupterm();
extern	char		*tigetstr();
extern	int		tigetnum();
extern	int		tigetflag();
#define Tgetstr(x, y) 	tigetstr(x.iname)
#define Tgetnum(x) 	tigetnum(x.iname);
#define Tgetflag(x) 	tigetflag(x.iname);
#else
extern	int		tgetent();
extern	char		*tgetstr();
extern	int		tgetnum();
extern	int		tgetflag();
#define Tgetstr(x, y) 	tgetstr(x.tname, &y)
#define Tgetnum(x) 	tgetnum(x.tname)
#define Tgetflag(x) 	tgetflag(x.tname)
#endif

extern  char    *getenv();

/*
 * The old code assumed termcap. termcap is almost always present, but on
 * many systems that have both termcap and terminfo, termcap is deprecated
 * and its databases often out of date. Configure will try to use terminfo
 * if at all possible. We define here a mapping between the termcap / terminfo
 * names, and where we store the information.
 * NOTE: The terminfo portions are NOT implemented yet.
 */
#define CAP_TYPE_BOOL   0
#define CAP_TYPE_INT    1
#define CAP_TYPE_STR    2

typedef struct cap2info
{
	const char *	longname;
	const char *	iname;
	const char *	tname;
	int 		type;
	void *		ptr;
} cap2info;

struct	term_struct TIS;

cap2info tcaps[] =
{
	{ "auto_left_margin",		"bw",		"bw",	CAP_TYPE_BOOL,	(void *)&TIS.TI_bw },
	{ "auto_right_margin",		"am",		"am",	CAP_TYPE_BOOL,	(void *)&TIS.TI_am },
	{ "no_esc_ctlc",		"xsb",		"xb",	CAP_TYPE_BOOL,	(void *)&TIS.TI_xsb },
	{ "ceol_standout_glitch",	"xhp",		"xs",	CAP_TYPE_BOOL,	(void *)&TIS.TI_xhp },
	{ "eat_newline_glitch",		"xenl",		"xn",	CAP_TYPE_BOOL,	(void *)&TIS.TI_xenl },
	{ "erase_overstrike",		"eo",		"eo",	CAP_TYPE_BOOL,	(void *)&TIS.TI_eo },
	{ "generic_type",		"gn",		"gn",	CAP_TYPE_BOOL,	(void *)&TIS.TI_gn },
	{ "hard_copy",			"hc",		"hc",	CAP_TYPE_BOOL,	(void *)&TIS.TI_hc },
	{ "has_meta_key",		"km",		"km",	CAP_TYPE_BOOL,	(void *)&TIS.TI_km },
	{ "has_status_line",		"hs",		"hs",	CAP_TYPE_BOOL,	(void *)&TIS.TI_hs },
	{ "insert_null_glitch",		"in",		"in",	CAP_TYPE_BOOL,	(void *)&TIS.TI_in },
	{ "memory_above",		"da",		"da",	CAP_TYPE_BOOL,	(void *)&TIS.TI_da },
	{ "memory_below",		"db",		"db",	CAP_TYPE_BOOL,	(void *)&TIS.TI_db },
	{ "move_insert_mode",		"mir",		"mi",	CAP_TYPE_BOOL,	(void *)&TIS.TI_mir },
	{ "move_standout_mode",		"msgr",		"ms",	CAP_TYPE_BOOL,	(void *)&TIS.TI_msgr },
	{ "over_strike",		"os",		"os",	CAP_TYPE_BOOL,	(void *)&TIS.TI_os },
	{ "status_line_esc_ok",		"eslok",	"es",	CAP_TYPE_BOOL,	(void *)&TIS.TI_eslok },
	{ "dest_tabs_magic_smso",	"xt",		"xt",	CAP_TYPE_BOOL,	(void *)&TIS.TI_xt },
	{ "tilde_glitch",		"hz",		"hz",	CAP_TYPE_BOOL,	(void *)&TIS.TI_hz },
	{ "transparent_underline",	"ul",		"ul",	CAP_TYPE_BOOL,	(void *)&TIS.TI_ul },
	{ "xon_xoff",			"xon",		"xo",	CAP_TYPE_BOOL,	(void *)&TIS.TI_xon },
	{ "needs_xon_xoff",		"nxon",		"nx",	CAP_TYPE_BOOL,	(void *)&TIS.TI_nxon },
	{ "prtr_silent",		"mc5i",		"5i",	CAP_TYPE_BOOL,	(void *)&TIS.TI_mc5i },
	{ "hard_cursor",		"chts",		"HC",	CAP_TYPE_BOOL,	(void *)&TIS.TI_chts },
	{ "non_rev_rmcup",		"nrrmc",	"NR",	CAP_TYPE_BOOL,	(void *)&TIS.TI_nrrmc },
	{ "no_pad_char",		"npc",		"NP",	CAP_TYPE_BOOL,	(void *)&TIS.TI_npc },
	{ "non_dest_scroll_region",	"ndscr",	"ND",	CAP_TYPE_BOOL,	(void *)&TIS.TI_ndscr },
	{ "can_change",			"ccc",		"cc",	CAP_TYPE_BOOL,	(void *)&TIS.TI_ccc },
	{ "back_color_erase",		"bce",		"ut",	CAP_TYPE_BOOL,	(void *)&TIS.TI_bce },
	{ "hue_lightness_saturation",	"hls",		"hl",	CAP_TYPE_BOOL,	(void *)&TIS.TI_hls },
	{ "col_addr_glitch",		"xhpa",		"YA",	CAP_TYPE_BOOL,	(void *)&TIS.TI_xhpa },
	{ "cr_cancels_micro_mode",	"crxm",		"YB",	CAP_TYPE_BOOL,	(void *)&TIS.TI_crxm },
	{ "has_print_wheel",		"daisy",	"YC",	CAP_TYPE_BOOL,	(void *)&TIS.TI_daisy },
	{ "row_addr_glitch",		"xvpa",		"YD",	CAP_TYPE_BOOL,	(void *)&TIS.TI_xvpa },
	{ "semi_auto_right_margin",	"sam",		"YE",	CAP_TYPE_BOOL,	(void *)&TIS.TI_sam },
	{ "cpi_changes_res",		"cpix",		"YF",	CAP_TYPE_BOOL,	(void *)&TIS.TI_cpix },
	{ "lpi_changes_res",		"lpix",		"YG",	CAP_TYPE_BOOL,	(void *)&TIS.TI_lpix },
	{ "columns",			"cols",		"co",	CAP_TYPE_INT,	(void *)&TIS.TI_cols },
	{ "init_tabs",			"it",		"it",	CAP_TYPE_INT,	(void *)&TIS.TI_it },
	{ "lines",			"lines",	"li",	CAP_TYPE_INT,	(void *)&TIS.TI_lines },
	{ "lines_of_memory",		"lm",		"lm",	CAP_TYPE_INT,	(void *)&TIS.TI_lm },
	{ "magic_cookie_glitch",	"xmc",		"sg",	CAP_TYPE_INT,	(void *)&TIS.TI_xmc },
	{ "padding_baud_rate",		"pb",		"pb",	CAP_TYPE_INT,	(void *)&TIS.TI_pb },
	{ "virtual_terminal",		"vt",		"vt",	CAP_TYPE_INT,	(void *)&TIS.TI_vt },
	{ "width_status_line",		"wsl",		"ws",	CAP_TYPE_INT,	(void *)&TIS.TI_wsl },
	{ "num_labels",			"nlab",		"Nl",	CAP_TYPE_INT,	(void *)&TIS.TI_nlab },
	{ "label_height",		"lh",		"lh",	CAP_TYPE_INT,	(void *)&TIS.TI_lh },
	{ "label_width",		"lw",		"lw",	CAP_TYPE_INT,	(void *)&TIS.TI_lw },
	{ "max_attributes",		"ma",		"ma",	CAP_TYPE_INT,	(void *)&TIS.TI_ma },
	{ "maximum_windows",		"wnum",		"MW",	CAP_TYPE_INT,	(void *)&TIS.TI_wnum },
	{ "max_colors",			"colors",	"Co",	CAP_TYPE_INT,	(void *)&TIS.TI_colors },
	{ "max_pairs",			"pairs",	"pa",	CAP_TYPE_INT,	(void *)&TIS.TI_pairs },
	{ "no_color_video",		"ncv",		"NC",	CAP_TYPE_INT,	(void *)&TIS.TI_ncv },
	{ "buffer_capacity",		"bufsz",	"Ya",	CAP_TYPE_INT,	(void *)&TIS.TI_bufsz },
	{ "dot_vert_spacing",		"spinv",	"Yb",	CAP_TYPE_INT,	(void *)&TIS.TI_spinv },
	{ "dot_horz_spacing",		"spinh",	"Yc",	CAP_TYPE_INT,	(void *)&TIS.TI_spinh },
	{ "max_micro_address",		"maddr",	"Yd",	CAP_TYPE_INT,	(void *)&TIS.TI_maddr },
	{ "max_micro_jump",		"mjump",	"Ye",	CAP_TYPE_INT,	(void *)&TIS.TI_mjump },
	{ "micro_col_size",		"mcs",		"Yf",	CAP_TYPE_INT,	(void *)&TIS.TI_mcs },
	{ "micro_line_size",		"mls",		"Yg",	CAP_TYPE_INT,	(void *)&TIS.TI_mls },
	{ "number_of_pins",		"npins",	"Yh",	CAP_TYPE_INT,	(void *)&TIS.TI_npins },
	{ "output_res_char",		"orc",		"Yi",	CAP_TYPE_INT,	(void *)&TIS.TI_orc },
	{ "output_res_line",		"orl",		"Yj",	CAP_TYPE_INT,	(void *)&TIS.TI_orl },
	{ "output_res_horz_inch",	"orhi",		"Yk",	CAP_TYPE_INT,	(void *)&TIS.TI_orhi },
	{ "output_res_vert_inch",	"orvi",		"Yl",	CAP_TYPE_INT,	(void *)&TIS.TI_orvi },
	{ "print_rate",			"cps",		"Ym",	CAP_TYPE_INT,	(void *)&TIS.TI_cps },
	{ "wide_char_size",		"widcs",	"Yn",	CAP_TYPE_INT,	(void *)&TIS.TI_widcs },
	{ "buttons",			"btns",		"BT",	CAP_TYPE_INT,	(void *)&TIS.TI_btns },
	{ "bit_image_entwining",	"bitwin",	"Yo",	CAP_TYPE_INT,	(void *)&TIS.TI_bitwin },
	{ "bit_image_type",		"bitype",	"Yp",	CAP_TYPE_INT,	(void *)&TIS.TI_bitype },
	{ "back_tab",			"cbt",		"bt",	CAP_TYPE_STR,	&TIS.TI_cbt },
	{ "bell",			"bel",		"bl",	CAP_TYPE_STR,	&TIS.TI_bel },
	{ "carriage_return",		"cr",		"cr",	CAP_TYPE_STR,	&TIS.TI_cr },
	{ "change_scroll_region",	"csr",		"cs",	CAP_TYPE_STR,	&TIS.TI_csr },
	{ "clear_all_tabs",		"tbc",		"ct",	CAP_TYPE_STR,	&TIS.TI_tbc },
	{ "clear_screen",		"clear",	"cl",	CAP_TYPE_STR,	&TIS.TI_clear },
	{ "clr_eol",			"el",		"ce",	CAP_TYPE_STR,	&TIS.TI_el },
	{ "clr_eos",			"ed",		"cd",	CAP_TYPE_STR,	&TIS.TI_ed },
	{ "column_address",		"hpa",		"ch",	CAP_TYPE_STR,	&TIS.TI_hpa },
	{ "command_character",		"cmdch",	"CC",	CAP_TYPE_STR,	&TIS.TI_cmdch },
	{ "cursor_address",		"cup",		"cm",	CAP_TYPE_STR,	&TIS.TI_cup },
	{ "cursor_down",		"cud1",		"do",	CAP_TYPE_STR,	&TIS.TI_cud1 },
	{ "cursor_home",		"home",		"ho",	CAP_TYPE_STR,	&TIS.TI_home },
	{ "cursor_invisible",		"civis",	"vi",	CAP_TYPE_STR,	&TIS.TI_civis },
	{ "cursor_left",		"cub1",		"le",	CAP_TYPE_STR,	&TIS.TI_cub1 },
	{ "cursor_mem_address",		"mrcup",	"CM",	CAP_TYPE_STR,	&TIS.TI_mrcup },
	{ "cursor_normal",		"cnorm",	"ve",	CAP_TYPE_STR,	&TIS.TI_cnorm },
	{ "cursor_right",		"cuf1",		"nd",	CAP_TYPE_STR,	&TIS.TI_cuf1 },
	{ "cursor_to_ll",		"ll",		"ll",	CAP_TYPE_STR,	&TIS.TI_ll },
	{ "cursor_up",			"cuu1",		"up",	CAP_TYPE_STR,	&TIS.TI_cuu1 },
	{ "cursor_visible",		"cvvis",	"vs",	CAP_TYPE_STR,	&TIS.TI_cvvis },
	{ "delete_character",		"dch1",		"dc",	CAP_TYPE_STR,	&TIS.TI_dch1 },
	{ "delete_line",		"dl1",		"dl",	CAP_TYPE_STR,	&TIS.TI_dl1 },
	{ "dis_status_line",		"dsl",		"ds",	CAP_TYPE_STR,	&TIS.TI_dsl },
	{ "down_half_line",		"hd",		"hd",	CAP_TYPE_STR,	&TIS.TI_hd },
	{ "enter_alt_charset_mode",	"smacs",	"as",	CAP_TYPE_STR,	&TIS.TI_smacs },
	{ "enter_blink_mode",		"blink",	"mb",	CAP_TYPE_STR,	&TIS.TI_blink },
	{ "enter_bold_mode",		"bold",		"md",	CAP_TYPE_STR,	&TIS.TI_bold },
	{ "enter_ca_mode",		"smcup",	"ti",	CAP_TYPE_STR,	&TIS.TI_smcup },
	{ "enter_delete_mode",		"smdc",		"dm",	CAP_TYPE_STR,	&TIS.TI_smdc },
	{ "enter_dim_mode",		"dim",		"mh",	CAP_TYPE_STR,	&TIS.TI_dim },
	{ "enter_insert_mode",		"smir",		"im",	CAP_TYPE_STR,	&TIS.TI_smir },
	{ "enter_secure_mode",		"invis",	"mk",	CAP_TYPE_STR,	&TIS.TI_invis },
	{ "enter_protected_mode",	"prot",		"mp",	CAP_TYPE_STR,	&TIS.TI_prot },
	{ "enter_reverse_mode",		"rev",		"mr",	CAP_TYPE_STR,	&TIS.TI_rev },
	{ "enter_standout_mode",	"smso",		"so",	CAP_TYPE_STR,	&TIS.TI_smso },
	{ "enter_underline_mode",	"smul",		"us",	CAP_TYPE_STR,	&TIS.TI_smul },
	{ "erase_chars",		"ech",		"ec",	CAP_TYPE_STR,	&TIS.TI_ech },
	{ "exit_alt_charset_mode",	"rmacs",	"ae",	CAP_TYPE_STR,	&TIS.TI_rmacs },
	{ "exit_attribute_mode",	"sgr0",		"me",	CAP_TYPE_STR,	&TIS.TI_sgr0 },
	{ "exit_ca_mode",		"rmcup",	"te",	CAP_TYPE_STR,	&TIS.TI_rmcup },
	{ "exit_delete_mode",		"rmdc",		"ed",	CAP_TYPE_STR,	&TIS.TI_rmdc },
	{ "exit_insert_mode",		"rmir",		"ei",	CAP_TYPE_STR,	&TIS.TI_rmir },
	{ "exit_standout_mode",		"rmso",		"se",	CAP_TYPE_STR,	&TIS.TI_rmso },
	{ "exit_underline_mode",	"rmul",		"ue",	CAP_TYPE_STR,	&TIS.TI_rmul },
	{ "flash_screen",		"flash",	"vb",	CAP_TYPE_STR,	&TIS.TI_flash },
	{ "form_feed",			"ff",		"ff",	CAP_TYPE_STR,	&TIS.TI_ff },
	{ "from_status_line",		"fsl",		"fs",	CAP_TYPE_STR,	&TIS.TI_fsl },
	{ "init_1string",		"is1",		"i1",	CAP_TYPE_STR,	&TIS.TI_is1 },
	{ "init_2string",		"is2",		"is",	CAP_TYPE_STR,	&TIS.TI_is2 },
	{ "init_3string",		"is3",		"i3",	CAP_TYPE_STR,	&TIS.TI_is3 },
	{ "init_file",			"if",		"if",	CAP_TYPE_STR,	&TIS.TI_if },
	{ "insert_character",		"ich1",		"ic",	CAP_TYPE_STR,	&TIS.TI_ich1 },
	{ "insert_line",		"il1",		"al",	CAP_TYPE_STR,	&TIS.TI_il1 },
	{ "insert_padding",		"ip",		"ip",	CAP_TYPE_STR,	&TIS.TI_ip },
	{ "key_backspace",		"kbs",		"kb",	CAP_TYPE_STR,	&TIS.TI_kbs },
	{ "key_catab",			"ktbc",		"ka",	CAP_TYPE_STR,	&TIS.TI_ktbc },
	{ "key_clear",			"kclr",		"kC",	CAP_TYPE_STR,	&TIS.TI_kclr },
	{ "key_ctab",			"kctab",	"kt",	CAP_TYPE_STR,	&TIS.TI_kctab },
	{ "key_dc",			"kdch1",	"kD",	CAP_TYPE_STR,	&TIS.TI_kdch1 },
	{ "key_dl",			"kdl1",		"kL",	CAP_TYPE_STR,	&TIS.TI_kdl1 },
	{ "key_down",			"kcud1",	"kd",	CAP_TYPE_STR,	&TIS.TI_kcud1 },
	{ "key_eic",			"krmir",	"kM",	CAP_TYPE_STR,	&TIS.TI_krmir },
	{ "key_eol",			"kel",		"kE",	CAP_TYPE_STR,	&TIS.TI_kel },
	{ "key_eos",			"ked",		"kS",	CAP_TYPE_STR,	&TIS.TI_ked },
	{ "key_f0",			"kf0",		"k0",	CAP_TYPE_STR,	&TIS.TI_kf0 },
	{ "key_f1",			"kf1",		"k1",	CAP_TYPE_STR,	&TIS.TI_kf1 },
	{ "key_f10",			"kf10",		"k;",	CAP_TYPE_STR,	&TIS.TI_kf10 },
	{ "key_f2",			"kf2",		"k2",	CAP_TYPE_STR,	&TIS.TI_kf2 },
	{ "key_f3",			"kf3",		"k3",	CAP_TYPE_STR,	&TIS.TI_kf3 },
	{ "key_f4",			"kf4",		"k4",	CAP_TYPE_STR,	&TIS.TI_kf4 },
	{ "key_f5",			"kf5",		"k5",	CAP_TYPE_STR,	&TIS.TI_kf5 },
	{ "key_f6",			"kf6",		"k6",	CAP_TYPE_STR,	&TIS.TI_kf6 },
	{ "key_f7",			"kf7",		"k7",	CAP_TYPE_STR,	&TIS.TI_kf7 },
	{ "key_f8",			"kf8",		"k8",	CAP_TYPE_STR,	&TIS.TI_kf8 },
	{ "key_f9",			"kf9",		"k9",	CAP_TYPE_STR,	&TIS.TI_kf9 },
	{ "key_home",			"khome",	"kh",	CAP_TYPE_STR,	&TIS.TI_khome },
	{ "key_ic",			"kich1",	"kI",	CAP_TYPE_STR,	&TIS.TI_kich1 },
	{ "key_il",			"kil1",		"kA",	CAP_TYPE_STR,	&TIS.TI_kil1 },
	{ "key_left",			"kcub1",	"kl",	CAP_TYPE_STR,	&TIS.TI_kcub1 },
	{ "key_ll",			"kll",		"kH",	CAP_TYPE_STR,	&TIS.TI_kll },
	{ "key_npage",			"knp",		"kN",	CAP_TYPE_STR,	&TIS.TI_knp },
	{ "key_ppage",			"kpp",		"kP",	CAP_TYPE_STR,	&TIS.TI_kpp },
	{ "key_right",			"kcuf1",	"kr",	CAP_TYPE_STR,	&TIS.TI_kcuf1 },
	{ "key_sf",			"kind",		"kF",	CAP_TYPE_STR,	&TIS.TI_kind },
	{ "key_sr",			"kri",		"kR",	CAP_TYPE_STR,	&TIS.TI_kri },
	{ "key_stab",			"khts",		"kT",	CAP_TYPE_STR,	&TIS.TI_khts },
	{ "key_up",			"kcuu1",	"ku",	CAP_TYPE_STR,	&TIS.TI_kcuu1 },
	{ "keypad_local",		"rmkx",		"ke",	CAP_TYPE_STR,	&TIS.TI_rmkx },
	{ "keypad_xmit",		"smkx",		"ks",	CAP_TYPE_STR,	&TIS.TI_smkx },
	{ "lab_f0",			"lf0",		"l0",	CAP_TYPE_STR,	&TIS.TI_lf0 },
	{ "lab_f1",			"lf1",		"l1",	CAP_TYPE_STR,	&TIS.TI_lf1 },
	{ "lab_f10",			"lf10",		"la",	CAP_TYPE_STR,	&TIS.TI_lf10 },
	{ "lab_f2",			"lf2",		"l2",	CAP_TYPE_STR,	&TIS.TI_lf2 },
	{ "lab_f3",			"lf3",		"l3",	CAP_TYPE_STR,	&TIS.TI_lf3 },
	{ "lab_f4",			"lf4",		"l4",	CAP_TYPE_STR,	&TIS.TI_lf4 },
	{ "lab_f5",			"lf5",		"l5",	CAP_TYPE_STR,	&TIS.TI_lf5 },
	{ "lab_f6",			"lf6",		"l6",	CAP_TYPE_STR,	&TIS.TI_lf6 },
	{ "lab_f7",			"lf7",		"l7",	CAP_TYPE_STR,	&TIS.TI_lf7 },
	{ "lab_f8",			"lf8",		"l8",	CAP_TYPE_STR,	&TIS.TI_lf8 },
	{ "lab_f9",			"lf9",		"l9",	CAP_TYPE_STR,	&TIS.TI_lf9 },
	{ "meta_off",			"rmm",		"mo",	CAP_TYPE_STR,	&TIS.TI_rmm },
	{ "meta_on",			"smm",		"mm",	CAP_TYPE_STR,	&TIS.TI_smm },
	{ "newline",			"nel",		"nw",	CAP_TYPE_STR,	&TIS.TI_nel },
	{ "pad_char",			"pad",		"pc",	CAP_TYPE_STR,	&TIS.TI_pad },
	{ "parm_dch",			"dch",		"DC",	CAP_TYPE_STR,	&TIS.TI_dch },
	{ "parm_delete_line",		"dl",		"DL",	CAP_TYPE_STR,	&TIS.TI_dl },
	{ "parm_down_cursor",		"cud",		"DO",	CAP_TYPE_STR,	&TIS.TI_cud },
	{ "parm_ich",			"ich",		"IC",	CAP_TYPE_STR,	&TIS.TI_ich },
	{ "parm_index",			"indn",		"SF",	CAP_TYPE_STR,	&TIS.TI_indn },
	{ "parm_insert_line",		"il",		"AL",	CAP_TYPE_STR,	&TIS.TI_il },
	{ "parm_left_cursor",		"cub",		"LE",	CAP_TYPE_STR,	&TIS.TI_cub },
	{ "parm_right_cursor",		"cuf",		"RI",	CAP_TYPE_STR,	&TIS.TI_cuf },
	{ "parm_rindex",		"rin",		"SR",	CAP_TYPE_STR,	&TIS.TI_rin },
	{ "parm_up_cursor",		"cuu",		"UP",	CAP_TYPE_STR,	&TIS.TI_cuu },
	{ "pkey_key",			"pfkey",	"pk",	CAP_TYPE_STR,	&TIS.TI_pfkey },
	{ "pkey_local",			"pfloc",	"pl",	CAP_TYPE_STR,	&TIS.TI_pfloc },
	{ "pkey_xmit",			"pfx",		"px",	CAP_TYPE_STR,	&TIS.TI_pfx },
	{ "print_screen",		"mc0",		"ps",	CAP_TYPE_STR,	&TIS.TI_mc0 },
	{ "prtr_off",			"mc4",		"pf",	CAP_TYPE_STR,	&TIS.TI_mc4 },
	{ "prtr_on",			"mc5",		"po",	CAP_TYPE_STR,	&TIS.TI_mc5 },
	{ "repeat_char",		"rep",		"rp",	CAP_TYPE_STR,	&TIS.TI_rep },
	{ "reset_1string",		"rs1",		"r1",	CAP_TYPE_STR,	&TIS.TI_rs1 },
	{ "reset_2string",		"rs2",		"r2",	CAP_TYPE_STR,	&TIS.TI_rs2 },
	{ "reset_3string",		"rs3",		"r3",	CAP_TYPE_STR,	&TIS.TI_rs3 },
	{ "reset_file",			"rf",		"rf",	CAP_TYPE_STR,	&TIS.TI_rf },
	{ "restore_cursor",		"rc",		"rc",	CAP_TYPE_STR,	&TIS.TI_rc },
	{ "row_address",		"vpa",		"cv",	CAP_TYPE_STR,	&TIS.TI_vpa },
	{ "save_cursor",		"sc",		"sc",	CAP_TYPE_STR,	&TIS.TI_sc },
	{ "scroll_forward",		"ind",		"sf",	CAP_TYPE_STR,	&TIS.TI_ind },
	{ "scroll_reverse",		"ri",		"sr",	CAP_TYPE_STR,	&TIS.TI_ri },
	{ "set_attributes",		"sgr",		"sa",	CAP_TYPE_STR,	&TIS.TI_sgr },
	{ "set_tab",			"hts",		"st",	CAP_TYPE_STR,	&TIS.TI_hts },
	{ "set_window",			"wind",		"wi",	CAP_TYPE_STR,	&TIS.TI_wind },
	{ "tab",			"ht",		"ta",	CAP_TYPE_STR,	&TIS.TI_ht },
	{ "to_status_line",		"tsl",		"ts",	CAP_TYPE_STR,	&TIS.TI_tsl },
	{ "underline_char",		"uc",		"uc",	CAP_TYPE_STR,	&TIS.TI_uc },
	{ "up_half_line",		"hu",		"hu",	CAP_TYPE_STR,	&TIS.TI_hu },
	{ "init_prog",			"iprog",	"iP",	CAP_TYPE_STR,	&TIS.TI_iprog },
	{ "key_a1",			"ka1",		"K1",	CAP_TYPE_STR,	&TIS.TI_ka1 },
	{ "key_a3",			"ka3",		"K3",	CAP_TYPE_STR,	&TIS.TI_ka3 },
	{ "key_b2",			"kb2",		"K2",	CAP_TYPE_STR,	&TIS.TI_kb2 },
	{ "key_c1",			"kc1",		"K4",	CAP_TYPE_STR,	&TIS.TI_kc1 },
	{ "key_c3",			"kc3",		"K5",	CAP_TYPE_STR,	&TIS.TI_kc3 },
	{ "prtr_non",			"mc5p",		"pO",	CAP_TYPE_STR,	&TIS.TI_mc5p },
	{ "char_padding",		"rmp",		"rP",	CAP_TYPE_STR,	&TIS.TI_rmp },
	{ "acs_chars",			"acsc",		"ac",	CAP_TYPE_STR,	&TIS.TI_acsc },
	{ "plab_norm",			"pln",		"pn",	CAP_TYPE_STR,	&TIS.TI_pln },
	{ "key_btab",			"kcbt",		"kB",	CAP_TYPE_STR,	&TIS.TI_kcbt },
	{ "enter_xon_mode",		"smxon",	"SX",	CAP_TYPE_STR,	&TIS.TI_smxon },
	{ "exit_xon_mode",		"rmxon",	"RX",	CAP_TYPE_STR,	&TIS.TI_rmxon },
	{ "enter_am_mode",		"smam",		"SA",	CAP_TYPE_STR,	&TIS.TI_smam },
	{ "exit_am_mode",		"rmam",		"RA",	CAP_TYPE_STR,	&TIS.TI_rmam },
	{ "xon_character",		"xonc",		"XN",	CAP_TYPE_STR,	&TIS.TI_xonc },
	{ "xoff_character",		"xoffc",	"XF",	CAP_TYPE_STR,	&TIS.TI_xoffc },
	{ "ena_acs",			"enacs",	"eA",	CAP_TYPE_STR,	&TIS.TI_enacs },
	{ "label_on",			"smln",		"LO",	CAP_TYPE_STR,	&TIS.TI_smln },
	{ "label_off",			"rmln",		"LF",	CAP_TYPE_STR,	&TIS.TI_rmln },
	{ "key_beg",			"kbeg",		"@1",	CAP_TYPE_STR,	&TIS.TI_kbeg },
	{ "key_cancel",			"kcan",		"@2",	CAP_TYPE_STR,	&TIS.TI_kcan },
	{ "key_close",			"kclo",		"@3",	CAP_TYPE_STR,	&TIS.TI_kclo },
	{ "key_command",		"kcmd",		"@4",	CAP_TYPE_STR,	&TIS.TI_kcmd },
	{ "key_copy",			"kcpy",		"@5",	CAP_TYPE_STR,	&TIS.TI_kcpy },
	{ "key_create",			"kcrt",		"@6",	CAP_TYPE_STR,	&TIS.TI_kcrt },
	{ "key_end",			"kend",		"@7",	CAP_TYPE_STR,	&TIS.TI_kend },
	{ "key_enter",			"kent",		"@8",	CAP_TYPE_STR,	&TIS.TI_kent },
	{ "key_exit",			"kext",		"@9",	CAP_TYPE_STR,	&TIS.TI_kext },
	{ "key_find",			"kfnd",		"@0",	CAP_TYPE_STR,	&TIS.TI_kfnd },
	{ "key_help",			"khlp",		"%1",	CAP_TYPE_STR,	&TIS.TI_khlp },
	{ "key_mark",			"kmrk",		"%2",	CAP_TYPE_STR,	&TIS.TI_kmrk },
	{ "key_message",		"kmsg",		"%3",	CAP_TYPE_STR,	&TIS.TI_kmsg },
	{ "key_move",			"kmov",		"%4",	CAP_TYPE_STR,	&TIS.TI_kmov },
	{ "key_next",			"knxt",		"%5",	CAP_TYPE_STR,	&TIS.TI_knxt },
	{ "key_open",			"kopn",		"%6",	CAP_TYPE_STR,	&TIS.TI_kopn },
	{ "key_options",		"kopt",		"%7",	CAP_TYPE_STR,	&TIS.TI_kopt },
	{ "key_previous",		"kprv",		"%8",	CAP_TYPE_STR,	&TIS.TI_kprv },
	{ "key_print",			"kprt",		"%9",	CAP_TYPE_STR,	&TIS.TI_kprt },
	{ "key_redo",			"krdo",		"%0",	CAP_TYPE_STR,	&TIS.TI_krdo },
	{ "key_reference",		"kref",		"&1",	CAP_TYPE_STR,	&TIS.TI_kref },
	{ "key_refresh",		"krfr",		"&2",	CAP_TYPE_STR,	&TIS.TI_krfr },
	{ "key_replace",		"krpl",		"&3",	CAP_TYPE_STR,	&TIS.TI_krpl },
	{ "key_restart",		"krst",		"&4",	CAP_TYPE_STR,	&TIS.TI_krst },
	{ "key_resume",			"kres",		"&5",	CAP_TYPE_STR,	&TIS.TI_kres },
	{ "key_save",			"ksav",		"&6",	CAP_TYPE_STR,	&TIS.TI_ksav },
	{ "key_suspend",		"kspd",		"&7",	CAP_TYPE_STR,	&TIS.TI_kspd },
	{ "key_undo",			"kund",		"&8",	CAP_TYPE_STR,	&TIS.TI_kund },
	{ "key_sbeg",			"kBEG",		"&9",	CAP_TYPE_STR,	&TIS.TI_kBEG },
	{ "key_scancel",		"kCAN",		"&0",	CAP_TYPE_STR,	&TIS.TI_kCAN },
	{ "key_scommand",		"kCMD",		"*1",	CAP_TYPE_STR,	&TIS.TI_kCMD },
	{ "key_scopy",			"kCPY",		"*2",	CAP_TYPE_STR,	&TIS.TI_kCPY },
	{ "key_screate",		"kCRT",		"*3",	CAP_TYPE_STR,	&TIS.TI_kCRT },
	{ "key_sdc",			"kDC",		"*4",	CAP_TYPE_STR,	&TIS.TI_kDC },
	{ "key_sdl",			"kDL",		"*5",	CAP_TYPE_STR,	&TIS.TI_kDL },
	{ "key_select",			"kslt",		"*6",	CAP_TYPE_STR,	&TIS.TI_kslt },
	{ "key_send",			"kEND",		"*7",	CAP_TYPE_STR,	&TIS.TI_kEND },
	{ "key_seol",			"kEOL",		"*8",	CAP_TYPE_STR,	&TIS.TI_kEOL },
	{ "key_sexit",			"kEXT",		"*9",	CAP_TYPE_STR,	&TIS.TI_kEXT },
	{ "key_sfind",			"kFND",		"*0",	CAP_TYPE_STR,	&TIS.TI_kFND },
	{ "key_shelp",			"kHLP",		"#1",	CAP_TYPE_STR,	&TIS.TI_kHLP },
	{ "key_shome",			"kHOM",		"#2",	CAP_TYPE_STR,	&TIS.TI_kHOM },
	{ "key_sic",			"kIC",		"#3",	CAP_TYPE_STR,	&TIS.TI_kIC },
	{ "key_sleft",			"kLFT",		"#4",	CAP_TYPE_STR,	&TIS.TI_kLFT },
	{ "key_smessage",		"kMSG",		"%a",	CAP_TYPE_STR,	&TIS.TI_kMSG },
	{ "key_smove",			"kMOV",		"%b",	CAP_TYPE_STR,	&TIS.TI_kMOV },
	{ "key_snext",			"kNXT",		"%c",	CAP_TYPE_STR,	&TIS.TI_kNXT },
	{ "key_soptions",		"kOPT",		"%d",	CAP_TYPE_STR,	&TIS.TI_kOPT },
	{ "key_sprevious",		"kPRV",		"%e",	CAP_TYPE_STR,	&TIS.TI_kPRV },
	{ "key_sprint",			"kPRT",		"%f",	CAP_TYPE_STR,	&TIS.TI_kPRT },
	{ "key_sredo",			"kRDO",		"%g",	CAP_TYPE_STR,	&TIS.TI_kRDO },
	{ "key_sreplace",		"kRPL",		"%h",	CAP_TYPE_STR,	&TIS.TI_kRPL },
	{ "key_sright",			"kRIT",		"%i",	CAP_TYPE_STR,	&TIS.TI_kRIT },
	{ "key_srsume",			"kRES",		"%j",	CAP_TYPE_STR,	&TIS.TI_kRES },
	{ "key_ssave",			"kSAV",		"!1",	CAP_TYPE_STR,	&TIS.TI_kSAV },
	{ "key_ssuspend",		"kSPD",		"!2",	CAP_TYPE_STR,	&TIS.TI_kSPD },
	{ "key_sundo",			"kUND",		"!3",	CAP_TYPE_STR,	&TIS.TI_kUND },
	{ "req_for_input",		"rfi",		"RF",	CAP_TYPE_STR,	&TIS.TI_rfi },
	{ "key_f11",			"kf11",		"F1",	CAP_TYPE_STR,	&TIS.TI_kf11 },
	{ "key_f12",			"kf12",		"F2",	CAP_TYPE_STR,	&TIS.TI_kf12 },
	{ "key_f13",			"kf13",		"F3",	CAP_TYPE_STR,	&TIS.TI_kf13 },
	{ "key_f14",			"kf14",		"F4",	CAP_TYPE_STR,	&TIS.TI_kf14 },
	{ "key_f15",			"kf15",		"F5",	CAP_TYPE_STR,	&TIS.TI_kf15 },
	{ "key_f16",			"kf16",		"F6",	CAP_TYPE_STR,	&TIS.TI_kf16 },
	{ "key_f17",			"kf17",		"F7",	CAP_TYPE_STR,	&TIS.TI_kf17 },
	{ "key_f18",			"kf18",		"F8",	CAP_TYPE_STR,	&TIS.TI_kf18 },
	{ "key_f19",			"kf19",		"F9",	CAP_TYPE_STR,	&TIS.TI_kf19 },
	{ "key_f20",			"kf20",		"FA",	CAP_TYPE_STR,	&TIS.TI_kf20 },
	{ "key_f21",			"kf21",		"FB",	CAP_TYPE_STR,	&TIS.TI_kf21 },
	{ "key_f22",			"kf22",		"FC",	CAP_TYPE_STR,	&TIS.TI_kf22 },
	{ "key_f23",			"kf23",		"FD",	CAP_TYPE_STR,	&TIS.TI_kf23 },
	{ "key_f24",			"kf24",		"FE",	CAP_TYPE_STR,	&TIS.TI_kf24 },
	{ "key_f25",			"kf25",		"FF",	CAP_TYPE_STR,	&TIS.TI_kf25 },
	{ "key_f26",			"kf26",		"FG",	CAP_TYPE_STR,	&TIS.TI_kf26 },
	{ "key_f27",			"kf27",		"FH",	CAP_TYPE_STR,	&TIS.TI_kf27 },
	{ "key_f28",			"kf28",		"FI",	CAP_TYPE_STR,	&TIS.TI_kf28 },
	{ "key_f29",			"kf29",		"FJ",	CAP_TYPE_STR,	&TIS.TI_kf29 },
	{ "key_f30",			"kf30",		"FK",	CAP_TYPE_STR,	&TIS.TI_kf30 },
	{ "key_f31",			"kf31",		"FL",	CAP_TYPE_STR,	&TIS.TI_kf31 },
	{ "key_f32",			"kf32",		"FM",	CAP_TYPE_STR,	&TIS.TI_kf32 },
	{ "key_f33",			"kf33",		"FN",	CAP_TYPE_STR,	&TIS.TI_kf33 },
	{ "key_f34",			"kf34",		"FO",	CAP_TYPE_STR,	&TIS.TI_kf34 },
	{ "key_f35",			"kf35",		"FP",	CAP_TYPE_STR,	&TIS.TI_kf35 },
	{ "key_f36",			"kf36",		"FQ",	CAP_TYPE_STR,	&TIS.TI_kf36 },
	{ "key_f37",			"kf37",		"FR",	CAP_TYPE_STR,	&TIS.TI_kf37 },
	{ "key_f38",			"kf38",		"FS",	CAP_TYPE_STR,	&TIS.TI_kf38 },
	{ "key_f39",			"kf39",		"FT",	CAP_TYPE_STR,	&TIS.TI_kf39 },
	{ "key_f40",			"kf40",		"FU",	CAP_TYPE_STR,	&TIS.TI_kf40 },
	{ "key_f41",			"kf41",		"FV",	CAP_TYPE_STR,	&TIS.TI_kf41 },
	{ "key_f42",			"kf42",		"FW",	CAP_TYPE_STR,	&TIS.TI_kf42 },
	{ "key_f43",			"kf43",		"FX",	CAP_TYPE_STR,	&TIS.TI_kf43 },
	{ "key_f44",			"kf44",		"FY",	CAP_TYPE_STR,	&TIS.TI_kf44 },
	{ "key_f45",			"kf45",		"FZ",	CAP_TYPE_STR,	&TIS.TI_kf45 },
	{ "key_f46",			"kf46",		"Fa",	CAP_TYPE_STR,	&TIS.TI_kf46 },
	{ "key_f47",			"kf47",		"Fb",	CAP_TYPE_STR,	&TIS.TI_kf47 },
	{ "key_f48",			"kf48",		"Fc",	CAP_TYPE_STR,	&TIS.TI_kf48 },
	{ "key_f49",			"kf49",		"Fd",	CAP_TYPE_STR,	&TIS.TI_kf49 },
	{ "key_f50",			"kf50",		"Fe",	CAP_TYPE_STR,	&TIS.TI_kf50 },
	{ "key_f51",			"kf51",		"Ff",	CAP_TYPE_STR,	&TIS.TI_kf51 },
	{ "key_f52",			"kf52",		"Fg",	CAP_TYPE_STR,	&TIS.TI_kf52 },
	{ "key_f53",			"kf53",		"Fh",	CAP_TYPE_STR,	&TIS.TI_kf53 },
	{ "key_f54",			"kf54",		"Fi",	CAP_TYPE_STR,	&TIS.TI_kf54 },
	{ "key_f55",			"kf55",		"Fj",	CAP_TYPE_STR,	&TIS.TI_kf55 },
	{ "key_f56",			"kf56",		"Fk",	CAP_TYPE_STR,	&TIS.TI_kf56 },
	{ "key_f57",			"kf57",		"Fl",	CAP_TYPE_STR,	&TIS.TI_kf57 },
	{ "key_f58",			"kf58",		"Fm",	CAP_TYPE_STR,	&TIS.TI_kf58 },
	{ "key_f59",			"kf59",		"Fn",	CAP_TYPE_STR,	&TIS.TI_kf59 },
	{ "key_f60",			"kf60",		"Fo",	CAP_TYPE_STR,	&TIS.TI_kf60 },
	{ "key_f61",			"kf61",		"Fp",	CAP_TYPE_STR,	&TIS.TI_kf61 },
	{ "key_f62",			"kf62",		"Fq",	CAP_TYPE_STR,	&TIS.TI_kf62 },
	{ "key_f63",			"kf63",		"Fr",	CAP_TYPE_STR,	&TIS.TI_kf63 },
	{ "clr_bol",			"el1",		"cb",	CAP_TYPE_STR,	&TIS.TI_el1 },
	{ "clear_margins",		"mgc",		"MC",	CAP_TYPE_STR,	&TIS.TI_mgc },
	{ "set_left_margin",		"smgl",		"ML",	CAP_TYPE_STR,	&TIS.TI_smgl },
	{ "set_right_margin",		"smgr",		"MR",	CAP_TYPE_STR,	&TIS.TI_smgr },
	{ "label_format",		"fln",		"Lf",	CAP_TYPE_STR,	&TIS.TI_fln },
	{ "set_clock",			"sclk",		"SC",	CAP_TYPE_STR,	&TIS.TI_sclk },
	{ "display_clock",		"dclk",		"DK",	CAP_TYPE_STR,	&TIS.TI_dclk },
	{ "remove_clock",		"rmclk",	"RC",	CAP_TYPE_STR,	&TIS.TI_rmclk },
	{ "create_window",		"cwin",		"CW",	CAP_TYPE_STR,	&TIS.TI_cwin },
	{ "goto_window",		"wingo",	"WG",	CAP_TYPE_STR,	&TIS.TI_wingo },
	{ "hangup",			"hup",		"HU",	CAP_TYPE_STR,	&TIS.TI_hup },
	{ "dial_phone",			"dial",		"DI",	CAP_TYPE_STR,	&TIS.TI_dial },
	{ "quick_dial",			"qdial",	"QD",	CAP_TYPE_STR,	&TIS.TI_qdial },
	{ "tone",			"tone",		"TO",	CAP_TYPE_STR,	&TIS.TI_tone },
	{ "pulse",			"pulse",	"PU",	CAP_TYPE_STR,	&TIS.TI_pulse },
	{ "flash_hook",			"hook",		"fh",	CAP_TYPE_STR,	&TIS.TI_hook },
	{ "fixed_pause",		"pause",	"PA",	CAP_TYPE_STR,	&TIS.TI_pause },
	{ "wait_tone",			"wait",		"WA",	CAP_TYPE_STR,	&TIS.TI_wait },
	{ "user0",			"u0",		"u0",	CAP_TYPE_STR,	&TIS.TI_u0 },
	{ "user1",			"u1",		"u1",	CAP_TYPE_STR,	&TIS.TI_u1 },
	{ "user2",			"u2",		"u2",	CAP_TYPE_STR,	&TIS.TI_u2 },
	{ "user3",			"u3",		"u3",	CAP_TYPE_STR,	&TIS.TI_u3 },
	{ "user4",			"u4",		"u4",	CAP_TYPE_STR,	&TIS.TI_u4 },
	{ "user5",			"u5",		"u5",	CAP_TYPE_STR,	&TIS.TI_u5 },
	{ "user6",			"u6",		"u6",	CAP_TYPE_STR,	&TIS.TI_u6 },
	{ "user7",			"u7",		"u7",	CAP_TYPE_STR,	&TIS.TI_u7 },
	{ "user8",			"u8",		"u8",	CAP_TYPE_STR,	&TIS.TI_u8 },
	{ "user9",			"u9",		"u9",	CAP_TYPE_STR,	&TIS.TI_u9 },
	{ "orig_pair",			"op",		"op",	CAP_TYPE_STR,	&TIS.TI_op },
	{ "orig_colors",		"oc",		"oc",	CAP_TYPE_STR,	&TIS.TI_oc },
	{ "initialize_color",		"initc",	"Ic",	CAP_TYPE_STR,	&TIS.TI_initc },
	{ "initialize_pair",		"initp",	"Ip",	CAP_TYPE_STR,	&TIS.TI_initp },
	{ "set_color_pair",		"scp",		"sp",	CAP_TYPE_STR,	&TIS.TI_scp },
	{ "set_foreground",		"setf",		"Sf",	CAP_TYPE_STR,	&TIS.TI_setf },
	{ "set_background",		"setb",		"Sb",	CAP_TYPE_STR,	&TIS.TI_setb },
	{ "change_char_pitch",		"cpi",		"ZA",	CAP_TYPE_STR,	&TIS.TI_cpi },
	{ "change_line_pitch",		"lpi",		"ZB",	CAP_TYPE_STR,	&TIS.TI_lpi },
	{ "change_res_horz",		"chr",		"ZC",	CAP_TYPE_STR,	&TIS.TI_chr },
	{ "change_res_vert",		"cvr",		"ZD",	CAP_TYPE_STR,	&TIS.TI_cvr },
	{ "define_char",		"defc",		"ZE",	CAP_TYPE_STR,	&TIS.TI_defc },
	{ "enter_doublewide_mode",	"swidm",	"ZF",	CAP_TYPE_STR,	&TIS.TI_swidm },
	{ "enter_draft_quality",	"sdrfq",	"ZG",	CAP_TYPE_STR,	&TIS.TI_sdrfq },
	{ "enter_italics_mode",		"sitm",		"ZH",	CAP_TYPE_STR,	&TIS.TI_sitm },
	{ "enter_leftward_mode",	"slm",		"ZI",	CAP_TYPE_STR,	&TIS.TI_slm },
	{ "enter_micro_mode",		"smicm",	"ZJ",	CAP_TYPE_STR,	&TIS.TI_smicm },
	{ "enter_near_letter_quality",	"snlq",		"ZK",	CAP_TYPE_STR,	&TIS.TI_snlq },
	{ "enter_normal_quality",	"snrmq",	"ZL",	CAP_TYPE_STR,	&TIS.TI_snrmq },
	{ "enter_shadow_mode",		"sshm",		"ZM",	CAP_TYPE_STR,	&TIS.TI_sshm },
	{ "enter_subscript_mode",	"ssubm",	"ZN",	CAP_TYPE_STR,	&TIS.TI_ssubm },
	{ "enter_superscript_mode",	"ssupm",	"ZO",	CAP_TYPE_STR,	&TIS.TI_ssupm },
	{ "enter_upward_mode",		"sum",		"ZP",	CAP_TYPE_STR,	&TIS.TI_sum },
	{ "exit_doublewide_mode",	"rwidm",	"ZQ",	CAP_TYPE_STR,	&TIS.TI_rwidm },
	{ "exit_italics_mode",		"ritm",		"ZR",	CAP_TYPE_STR,	&TIS.TI_ritm },
	{ "exit_leftward_mode",		"rlm",		"ZS",	CAP_TYPE_STR,	&TIS.TI_rlm },
	{ "exit_micro_mode",		"rmicm",	"ZT",	CAP_TYPE_STR,	&TIS.TI_rmicm },
	{ "exit_shadow_mode",		"rshm",		"ZU",	CAP_TYPE_STR,	&TIS.TI_rshm },
	{ "exit_subscript_mode",	"rsubm",	"ZV",	CAP_TYPE_STR,	&TIS.TI_rsubm },
	{ "exit_superscript_mode",	"rsupm",	"ZW",	CAP_TYPE_STR,	&TIS.TI_rsupm },
	{ "exit_upward_mode",		"rum",		"ZX",	CAP_TYPE_STR,	&TIS.TI_rum },
	{ "micro_column_address",	"mhpa",		"ZY",	CAP_TYPE_STR,	&TIS.TI_mhpa },
	{ "micro_down",			"mcud1",	"ZZ",	CAP_TYPE_STR,	&TIS.TI_mcud1 },
	{ "micro_left",			"mcub1",	"Za",	CAP_TYPE_STR,	&TIS.TI_mcub1 },
	{ "micro_right",		"mcuf1",	"Zb",	CAP_TYPE_STR,	&TIS.TI_mcuf1 },
	{ "micro_row_address",		"mvpa",		"Zc",	CAP_TYPE_STR,	&TIS.TI_mvpa },
	{ "micro_up",			"mcuu1",	"Zd",	CAP_TYPE_STR,	&TIS.TI_mcuu1 },
	{ "order_of_pins",		"porder",	"Ze",	CAP_TYPE_STR,	&TIS.TI_porder },
	{ "parm_down_micro",		"mcud",		"Zf",	CAP_TYPE_STR,	&TIS.TI_mcud },
	{ "parm_left_micro",		"mcub",		"Zg",	CAP_TYPE_STR,	&TIS.TI_mcub },
	{ "parm_right_micro",		"mcuf",		"Zh",	CAP_TYPE_STR,	&TIS.TI_mcuf },
	{ "parm_up_micro",		"mcuu",		"Zi",	CAP_TYPE_STR,	&TIS.TI_mcuu },
	{ "select_char_set",		"scs",		"Zj",	CAP_TYPE_STR,	&TIS.TI_scs },
	{ "set_bottom_margin",		"smgb",		"Zk",	CAP_TYPE_STR,	&TIS.TI_smgb },
	{ "set_bottom_margin_parm",	"smgbp",	"Zl",	CAP_TYPE_STR,	&TIS.TI_smgbp },
	{ "set_left_margin_parm",	"smglp",	"Zm",	CAP_TYPE_STR,	&TIS.TI_smglp },
	{ "set_right_margin_parm",	"smgrp",	"Zn",	CAP_TYPE_STR,	&TIS.TI_smgrp },
	{ "set_top_margin",		"smgt",		"Zo",	CAP_TYPE_STR,	&TIS.TI_smgt },
	{ "set_top_margin_parm",	"smgtp",	"Zp",	CAP_TYPE_STR,	&TIS.TI_smgtp },
	{ "start_bit_image",		"sbim",		"Zq",	CAP_TYPE_STR,	&TIS.TI_sbim },
	{ "start_char_set_def",		"scsd",		"Zr",	CAP_TYPE_STR,	&TIS.TI_scsd },
	{ "stop_bit_image",		"rbim",		"Zs",	CAP_TYPE_STR,	&TIS.TI_rbim },
	{ "stop_char_set_def",		"rcsd",		"Zt",	CAP_TYPE_STR,	&TIS.TI_rcsd },
	{ "subscript_characters",	"subcs",	"Zu",	CAP_TYPE_STR,	&TIS.TI_subcs },
	{ "superscript_characters",	"supcs",	"Zv",	CAP_TYPE_STR,	&TIS.TI_supcs },
	{ "these_cause_cr",		"docr",		"Zw",	CAP_TYPE_STR,	&TIS.TI_docr },
	{ "zero_motion",		"zerom",	"Zx",	CAP_TYPE_STR,	&TIS.TI_zerom },
	{ "char_set_names",		"csnm",		"Zy",	CAP_TYPE_STR,	&TIS.TI_csnm },
	{ "key_mouse",			"kmous",	"Km",	CAP_TYPE_STR,	&TIS.TI_kmous },
	{ "mouse_info",			"minfo",	"Mi",	CAP_TYPE_STR,	&TIS.TI_minfo },
	{ "req_mouse_pos",		"reqmp",	"RQ",	CAP_TYPE_STR,	&TIS.TI_reqmp },
	{ "get_mouse",			"getm",		"Gm",	CAP_TYPE_STR,	&TIS.TI_getm },
	{ "set_a_foreground",		"setaf",	"AF",	CAP_TYPE_STR,	&TIS.TI_setaf },
	{ "set_a_background",		"setab",	"AB",	CAP_TYPE_STR,	&TIS.TI_setab },
	{ "pkey_plab",			"pfxl",		"xl",	CAP_TYPE_STR,	&TIS.TI_pfxl },
	{ "device_type",		"devt",		"dv",	CAP_TYPE_STR,	&TIS.TI_devt },
	{ "code_set_init",		"csin",		"ci",	CAP_TYPE_STR,	&TIS.TI_csin },
	{ "set0_des_seq",		"s0ds",		"s0",	CAP_TYPE_STR,	&TIS.TI_s0ds },
	{ "set1_des_seq",		"s1ds",		"s1",	CAP_TYPE_STR,	&TIS.TI_s1ds },
	{ "set2_des_seq",		"s2ds",		"s2",	CAP_TYPE_STR,	&TIS.TI_s2ds },
	{ "set3_des_seq",		"s3ds",		"s3",	CAP_TYPE_STR,	&TIS.TI_s3ds },
	{ "set_lr_margin",		"smglr",	"ML",	CAP_TYPE_STR,	&TIS.TI_smglr },
	{ "set_tb_margin",		"smgtb",	"MT",	CAP_TYPE_STR,	&TIS.TI_smgtb },
	{ "bit_image_repeat",		"birep",	"Xy",	CAP_TYPE_STR,	&TIS.TI_birep },
	{ "bit_image_newline",		"binel",	"Zz",	CAP_TYPE_STR,	&TIS.TI_binel },
	{ "bit_image_carriage_return",	"bicr",		"Yv",	CAP_TYPE_STR,	&TIS.TI_bicr },
	{ "color_names",		"colornm",	"Yw",	CAP_TYPE_STR,	&TIS.TI_colornm },
	{ "define_bit_image_region",	"defbi",	"Yx",	CAP_TYPE_STR,	&TIS.TI_defbi },
	{ "end_bit_image_region",	"endbi",	"Yy",	CAP_TYPE_STR,	&TIS.TI_endbi },
	{ "set_color_band",		"setcolor",	"Yz",	CAP_TYPE_STR,	&TIS.TI_setcolor },
	{ "set_page_length",		"slines",	"YZ",	CAP_TYPE_STR,	&TIS.TI_slines },
	{ "display_pc_char",		"dispc",	"S1",	CAP_TYPE_STR,	&TIS.TI_dispc },
	{ "enter_pc_charset_mode",	"smpch",	"S2",	CAP_TYPE_STR,	&TIS.TI_smpch },
	{ "exit_pc_charset_mode",	"rmpch",	"S3",	CAP_TYPE_STR,	&TIS.TI_rmpch },
	{ "enter_scancode_mode",	"smsc",		"S4",	CAP_TYPE_STR,	&TIS.TI_smsc },
	{ "exit_scancode_mode",		"rmsc",		"S5",	CAP_TYPE_STR,	&TIS.TI_rmsc },
	{ "pc_term_options",		"pctrm",	"S6",	CAP_TYPE_STR,	&TIS.TI_pctrm },
	{ "scancode_escape",		"scesc",	"S7",	CAP_TYPE_STR,	&TIS.TI_scesc },
	{ "alt_scancode_esc",		"scesa",	"S8",	CAP_TYPE_STR,	&TIS.TI_scesa },
	{ "enter_horizontal_hl_mode",	"ehhlm",	"Xh",	CAP_TYPE_STR,	&TIS.TI_ehhlm },
	{ "enter_left_hl_mode",		"elhlm",	"Xl",	CAP_TYPE_STR,	&TIS.TI_elhlm },
	{ "enter_low_hl_mode",		"elohlm",	"Xo",	CAP_TYPE_STR,	&TIS.TI_elohlm },
	{ "enter_right_hl_mode",	"erhlm",	"Xr",	CAP_TYPE_STR,	&TIS.TI_erhlm },
	{ "enter_top_hl_mode",		"ethlm",	"Xt",	CAP_TYPE_STR,	&TIS.TI_ethlm },
	{ "enter_vertical_hl_mode",	"evhlm",	"Xv",	CAP_TYPE_STR,	&TIS.TI_evhlm },
	{ "set_a_attributes",		"sgr1",		"sA",	CAP_TYPE_STR,	&TIS.TI_sgr1 },
	{ "set_pglen_inch",		"slength",	"sL",	CAP_TYPE_STR,	&TIS.TI_slength },
	{ "termcap_init2",		"OTi2",		"i2",	CAP_TYPE_STR,	&TIS.TI_OTi2 },
	{ "termcap_reset",		"OTrs",		"rs",	CAP_TYPE_STR,	&TIS.TI_OTrs },
	{ "magic_cookie_glitch_ul",	"OTug",		"ug",	CAP_TYPE_INT,	(char **)&TIS.TI_OTug },
	{ "backspaces_with_bs",		"OTbs",		"bs",	CAP_TYPE_BOOL,	(char **)&TIS.TI_OTbs },
	{ "crt_no_scrolling",		"OTns",		"ns",	CAP_TYPE_BOOL,	(char **)&TIS.TI_OTns },
	{ "no_correctly_working_cr",	"OTnc",		"nc",	CAP_TYPE_BOOL,	(char **)&TIS.TI_OTnc },
	{ "carriage_return_delay",	"OTdC",		"dC",	CAP_TYPE_INT,	(char **)&TIS.TI_OTdC },
	{ "new_line_delay",		"OTdN",		"dN",	CAP_TYPE_INT,	(char **)&TIS.TI_OTdN },
	{ "linefeed_if_not_lf",		"OTnl",		"nl",	CAP_TYPE_STR,	&TIS.TI_OTnl },
	{ "backspace_if_not_bs",	"OTbc",		"bc",	CAP_TYPE_STR,	&TIS.TI_OTbc },
	{ "gnu_has_meta_key",		"OTMT",		"MT",	CAP_TYPE_BOOL,	(char **)&TIS.TI_OTMT },
	{ "linefeed_is_newline",	"OTNL",		"NL",	CAP_TYPE_BOOL,	(char **)&TIS.TI_OTNL },
	{ "backspace_delay",		"OTdB",		"dB",	CAP_TYPE_INT,	(char **)&TIS.TI_OTdB },
	{ "horizontal_tab_delay",	"OTdT",		"dT",	CAP_TYPE_INT,	(char **)&TIS.TI_OTdT },
	{ "number_of_function_keys",	"OTkn",		"kn",	CAP_TYPE_INT,	(char **)&TIS.TI_OTkn },
	{ "other_non_function_keys",	"OTko",		"ko",	CAP_TYPE_STR,	&TIS.TI_OTko },
	{ "arrow_key_map",		"OTma",		"ma",	CAP_TYPE_STR,	&TIS.TI_OTma },
	{ "has_hardware_tabs",		"OTpt",		"pt",	CAP_TYPE_BOOL,	(char **)&TIS.TI_OTpt },
	{ "return_does_clr_eol",	"OTxr",		"xr",	CAP_TYPE_BOOL,	(char **)&TIS.TI_OTxr },
	{ "acs_ulcorner",		"OTG2",		"G2",	CAP_TYPE_STR,	&TIS.TI_OTG2 },
	{ "acs_llcorner",		"OTG3",		"G3",	CAP_TYPE_STR,	&TIS.TI_OTG3 },
	{ "acs_urcorner",		"OTG1",		"G1",	CAP_TYPE_STR,	&TIS.TI_OTG1 },
	{ "acs_lrcorner",		"OTG4",		"G4",	CAP_TYPE_STR,	&TIS.TI_OTG4 },
	{ "acs_ltee",			"OTGR",		"GR",	CAP_TYPE_STR,	&TIS.TI_OTGR },
	{ "acs_rtee",			"OTGL",		"GL",	CAP_TYPE_STR,	&TIS.TI_OTGL },
	{ "acs_btee",			"OTGU",		"GU",	CAP_TYPE_STR,	&TIS.TI_OTGU },
	{ "acs_ttee",			"OTGD",		"GD",	CAP_TYPE_STR,	&TIS.TI_OTGD },
	{ "acs_hline",			"OTGH",		"GH",	CAP_TYPE_STR,	&TIS.TI_OTGH },
	{ "acs_vline",			"OTGV",		"GV",	CAP_TYPE_STR,	&TIS.TI_OTGV },
	{ "acs_plus",			"OTGC",		"GC",	CAP_TYPE_STR,	&TIS.TI_OTGC },
	{ "memory_lock",		"meml",		"ml",	CAP_TYPE_STR,	&TIS.TI_meml },
	{ "memory_unlock",		"memu",		"mu",	CAP_TYPE_STR,	&TIS.TI_memu },
	{ "box_chars_1",		"box1",		"bx",	CAP_TYPE_STR,	&TIS.TI_box1 },
};

	struct	term_struct *current_term = &TIS;
	int 	numcaps = sizeof(tcaps) / sizeof(cap2info);

	int	meta_mode = 2;
	int	can_color = 0;
	int	need_redraw = 0;
static	int	term_echo_flag = 1;
static	int	li;
static	int	co;

#if !defined(__EMX__) && !defined(WINNT) && !defined(GUI)
#ifndef HAVE_TERMINFO
static	char	termcap[2048];	/* bigger than we need, just in case */
#endif
static	char	termcap2[2048];	/* bigger than we need, just in case */
#endif

/* 
 * Any GUI system modules must be included here to make the GUI support
 *  routines accessable to the rest of BitchX. 
 */
#ifndef WTERM_C
#ifdef __EMXPM__
#include "PMBitchX.c"
#elif defined(GTK)
#include "gtkbitchx.c"
#elif defined(WIN32)
#include "winbitchx.c"
#endif
#endif
/*
 * term_echo: if 0, echo is turned off (all characters appear as blanks), if
 * non-zero, all is normal.  The function returns the old value of the
 * term_echo_flag 
 */
int	term_echo (int flag)
{
	int	echo;

	echo = term_echo_flag;
	term_echo_flag = flag;
	return (echo);
}

/*
 * term_putchar: puts a character to the screen, and displays control
 * characters as inverse video uppercase letters.  NOTE:  Dont use this to
 * display termcap control sequences!  It won't work! 
 *
 * Um... well, it will work if DISPLAY_ANSI_VAR is set to on... (hop)
 */
void	term_putchar (unsigned char c)
{
	if (!term_echo_flag)
	{
		putchar_x(' ');
		return;
	}
	/* Sheer, raving paranoia */
#ifndef __EMX__
	if (!(newb.c_cflag & CS8) && (c & 0x80))
		c &= ~0x80;
#endif
	/* 
	 * This mangles all the unprintable control characters
	 * except for the escape character.  This will only support
	 * escape sequences that use ESC + any normal, printable
	 * characters (this is an assumption throughout the program).
	 * While you can use a non-vt100 emulation, the output will
	 * probably be mangled in several places.
	 */
	if (c < 0x20 || c == 0x9b) /* all ctl chars except ESC */
	{
		term_standout_on();
		putchar_x((c | 0x40) & 0x7f); /* Convert to printable */
		term_standout_off();
	}
	else if (c == 0x7f) 	/* delete char */
	{
		term_standout_on();
		putchar_x('?');
		term_standout_off();
	}
	else
		putchar_x(c);
}

/*
 * term_reset: sets terminal attributed back to what they were before the
 * program started 
 */
void term_reset (void)
{
	tcsetattr(tty_des, TCSADRAIN, &oldb);

	if (current_term->TI_csr)
		tputs_x((char *)tparm(current_term->TI_csr, 0, current_term->TI_lines - 1));
	term_gotoxy(0, current_term->TI_lines - 1);
#if use_alt_screen
	if (current_term->TI_rmcup)
		tputs_x(current_term->TI_rmcup);
#endif
#if use_automargins
	if (current_term->TI_am && current_term->TI_smam)
		tputs_x(current_term->TI_smam);
#endif
	term_flush();
}

void term_reset2 (void)
{
	tcsetattr(tty_des, TCSADRAIN, &oldb);
	term_flush();
}

/*
 * term_cont: sets the terminal back to IRCII stuff when it is restarted
 * after a SIGSTOP.  Somewhere, this must be used in a signal() call 
 */
SIGNAL_HANDLER(term_cont)
{
extern int foreground;
	use_input = foreground = (tcgetpgrp(0) == getpgrp());
	if (foreground)
	{
#if use_alt_screen
		if (current_term->TI_smcup)
			tputs_x(current_term->TI_smcup);
#endif
#if use_automargins
		if (current_term->TI_rmam)
			tputs_x(current_term->TI_rmam);
#endif
		need_redraw = 1;
		tcsetattr(tty_des, TCSADRAIN, &newb);
	}
}

#if !defined(STERM_C)
/*
 * term_pause: sets terminal back to pre-program days, then SIGSTOPs itself. 
 */
extern void term_pause (char unused, char *not_used)
{
#ifndef PUBLIC_ACCESS
#ifdef WINNT
char *shell;
DWORD dwmode;
HANDLE hstdin=GetStdHandle(STD_INPUT_HANDLE);
STARTUPINFO si = { 0 };
PROCESS_INFORMATION pi = { 0 };

	si.cb = sizeof(si);

	if (!(shell = get_string_var(SHELL_VAR)))
	{
		if (gdwPlatform == VER_PLATFORM_WIN32_WINDOWS)
			shell = "command.com";
		else
			shell = "cmd.exe";
	}
	if (!(GetConsoleMode(hstdin,&dwmode)))
		return;

	if (!SetConsoleMode(hstdin, dwmode | ((ENABLE_LINE_INPUT|ENABLE_ECHO_INPUT|ENABLE_PROCESSED_INPUT) & ~ENABLE_WINDOW_INPUT)))
		return;

	if(!CreateProcess(NULL, shell, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi) )
		return;
	CloseHandle(pi.hThread);
	WaitForSingleObject(pi.hProcess,INFINITE);
	CloseHandle(pi.hProcess);

	if (!SetConsoleMode(hstdin, dwmode ) )
		return;
	refresh_screen(0,NULL);
#else

	term_reset();
#ifndef __EMX__
	kill(getpid(), SIGSTOP);
#endif
#endif
#endif /* PUBLIC_ACCESS */
}
#endif /* STERM_C */
#endif /* NOT IN WTERM_C */




/*
 * term_init: does all terminal initialization... reads termcap info, sets
 * the terminal to CBREAK, no ECHO mode.   Chooses the best of the terminal
 * attributes to use ..  for the version of this function that is called for
 * wserv, we set the termial to RAW, no ECHO, so that all the signals are
 * ignored.. fixes quite a few problems...  -phone, jan 1993..
 */
int termfeatures = 0;

int term_init (char *term)
{
#ifndef WTERM_C
	int	i;
	int	desired;


#if !defined(__EMX__) && !defined(WINNT) && !defined(GUI)
	memset(current_term, 0, sizeof(struct term_struct));

	if (dumb_mode)
		ircpanic("term_init called in dumb_mode");
	*termcap2 = 0;
	if (!term && !(term = getenv("TERM")))
	{
		fprintf(stderr, "\n");
		fprintf(stderr, "You do not have a TERM environment variable.\n");
		fprintf(stderr, "So we'll be running in dumb mode...\n");
		return -1;
	}
#ifdef WANT_DETACH
	else if (!already_detached)
		fprintf(stdout, "Using terminal type [%s]\n", term);
#endif
#ifdef HAVE_TERMINFO
	setupterm(NULL, 1, &i);
	if (i != 1)
	{
		fprintf(stderr, "setupterm failed: %d\n", i);
		fprintf(stderr, "So we'll be running in dumb mode...\n");
		return -1;
	}
#else
	if (tgetent(termcap, term) < 1)
	{
		fprintf(stderr, "\n");
		fprintf(stderr, "Your current TERM setting (%s) does not have a termcap entry.\n", term);
		fprintf(stderr, "So we'll be running in dumb mode...\n");
		return -1;
	}
#endif

	for (i = 0; i < numcaps; i++)
	{
		int ival;
		char *cval;

		if (tcaps[i].type == CAP_TYPE_INT)
		{
			ival = Tgetnum(tcaps[i]);
			*(int *)tcaps[i].ptr = ival;
		}
		else if (tcaps[i].type == CAP_TYPE_BOOL)
		{
			ival = Tgetflag(tcaps[i]);
			*(int *)tcaps[i].ptr = ival;
		}
		else
		{
			char *tmp = termcap2;

			cval = Tgetstr(tcaps[i], tmp);
			if (cval == (char *) -1)
				*(char **)tcaps[i].ptr = NULL;
			else
				*(char **)tcaps[i].ptr = cval;
		}
	}

	BC = current_term->TI_cub1;
	UP = current_term->TI_cuu1;
	if (current_term->TI_pad)
		my_PC = current_term->TI_pad[0];
	else
		my_PC = 0;

	if (BC)
		BClen = strlen(BC);
	else
		BClen = 0;
	if (UP)
		UPlen = strlen(UP);
	else
		UPlen = 0;

	li = current_term->TI_lines;
	co = current_term->TI_cols;
	if (!co)
		co = 79;
	if (!li)
		li = 24;

	if (!current_term->TI_nel)
		current_term->TI_nel = "\n";
	if (!current_term->TI_cr)
		current_term->TI_cr = "\r";

	current_term->TI_normal[0] = 0;
	if (current_term->TI_sgr0)
	{
		strcat(current_term->TI_normal, current_term->TI_sgr0);
	}
	if (current_term->TI_rmso)
	{
		if (current_term->TI_sgr0 && strcmp(current_term->TI_rmso, current_term->TI_sgr0))
			strcat(current_term->TI_normal, current_term->TI_rmso);
	}
	if (current_term->TI_rmul)
	{
		if (current_term->TI_sgr0 && strcmp(current_term->TI_rmul, current_term->TI_sgr0))
			strcat (current_term->TI_normal, current_term->TI_rmul);
	}


#else

#if defined(WINNT) && !defined(GUI)
	CONSOLE_SCREEN_BUFFER_INFO scrbuf;
	HANDLE hinput =GetStdHandle(STD_INPUT_HANDLE);
	DWORD dwmode;
	OSVERSIONINFO osver;

	ghstdout = GetStdHandle(STD_OUTPUT_HANDLE);

	if(!GetConsoleScreenBufferInfo(ghstdout, &gscrbuf) ) 
	{
		fprintf(stderr,"Error from getconsolebufinfo %d\n",GetLastError());
		abort();
	}

	GetConsoleMode(hinput,&dwmode);
	SetConsoleMode(hinput,dwmode & (~(ENABLE_LINE_INPUT |ENABLE_ECHO_INPUT
				| ENABLE_PROCESSED_INPUT)| ENABLE_WINDOW_INPUT/* | ENABLE_WRAP_AT_EOL_OUTPUT*/)
				);

	osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osver);
	gdwPlatform = osver.dwPlatformId;

	GetConsoleCursorInfo(hinput, &gcursbuf);

	GetConsoleScreenBufferInfo(ghstdout, &scrbuf);
	li = scrbuf.srWindow.Bottom - scrbuf.srWindow.Top + 1;
	co = scrbuf.srWindow.Right - scrbuf.srWindow.Left + 1;

	memset(current_term, 0, sizeof(struct term_struct));

	current_term->TI_lines = li;
	current_term->TI_cols = co - 1;
#elif defined(GUI)

	memset(current_term, 0, sizeof(struct term_struct));
	gui_init();
#else
	vmode.cb = sizeof(VIOMODEINFO);
	VioGetMode(&vmode, 0);
	default_pair[0] = ' ';
	default_pair[1] = 7;

	memset(current_term, 0, sizeof(struct term_struct));

	current_term->TI_lines = vmode.row;
	current_term->TI_cols = vmode.col;
	li = current_term->TI_lines;
	co = current_term->TI_cols;
#endif

	current_term->TI_cup = strdup("\e[%i%d;%dH");
	current_term->TI_cub1 = strdup("\e[D");
	current_term->TI_clear = strdup("\e[H\e[2J");
	current_term->TI_el = strdup("\e[K");
	current_term->TI_mrcup = strdup("\e[%i%d;%dH");
	current_term->TI_cub = strdup("\e[D");
	current_term->TI_cuf = strdup("\e[C");
	current_term->TI_cuf1 = strdup("\e[C");
	current_term->TI_smso = strdup("\e[7m");
	current_term->TI_rmso = strdup("\e[0m");
#ifdef GTK
	current_term->TI_smul = strdup("\e[4m");
	current_term->TI_rmul = strdup("\e[24m");
#else
	current_term->TI_smul = strdup("\e[1;34m");
	current_term->TI_rmul = strdup("\e[0;37m");
#endif
	current_term->TI_bold = strdup("\e[1;37m");
	current_term->TI_sgr0 = strdup("\e[0;37m");
	current_term->TI_blink = strdup("\e[5m");
	current_term->TI_normal[0]=0;

#ifdef GTK
	current_term->TI_csr = strdup("\e[%i%p1%d;%p2%dr");
#endif
	current_term->TI_ri = strdup("\eM");
	current_term->TI_ind = strdup("\n");
	current_term->TI_il = strdup("\e[%p1%dL");
	current_term->TI_il1 = strdup("\e[L");
	current_term->TI_dl = strdup("\e[%p1%dM");
	current_term->TI_dl1 = strdup("\e[M");
	
	strcat(current_term->TI_normal, current_term->TI_sgr0);
	strcat(current_term->TI_normal, current_term->TI_rmso);

	current_term->TI_cr = strdup("\r");
	current_term->TI_nel = strdup("\n");
	current_term->TI_bel = strdup("\007");
	current_term->TI_cub = strdup("\010");
#ifdef __EMXPM__
	current_term->TI_kend = strdup("\eO");
	current_term->TI_khome = strdup("\eG");
	current_term->TI_knp = strdup("\eQ");
	current_term->TI_kpp = strdup("\eI");
	current_term->TI_kcuf1 = strdup("\eM");
	current_term->TI_kcub1 = strdup("\eK");
	current_term->TI_kcuu1 = strdup("\eH");
	current_term->TI_kcud1 = strdup("\eP");
#else
	current_term->TI_kend = strdup("\e[4~");
	current_term->TI_khome = strdup("\e[1~");
	current_term->TI_knp = strdup("\e[6~");
	current_term->TI_kpp = strdup("\e[5~");
	current_term->TI_kcuf1 = strdup("\e[C");
	current_term->TI_kcub1 = strdup("\e[D");
	current_term->TI_kcuu1 = strdup("\e[A");
	current_term->TI_kcud1 = strdup("\e[B");
#endif
#endif

	/*
	 * Finally set up the current_term->TI_sgrstrs array.  Clean it out first...
	 */
	for (i = 0; i < TERM_SGR_MAXVAL; i++)
		current_term->TI_sgrstrs[i] = empty_string;


	if (current_term->TI_bold)
		current_term->TI_sgrstrs[TERM_SGR_BOLD_ON-1] = current_term->TI_bold;
	if (current_term->TI_sgr0)
	{
		current_term->TI_sgrstrs[TERM_SGR_BOLD_OFF - 1] = current_term->TI_sgr0;
		current_term->TI_sgrstrs[TERM_SGR_BLINK_OFF - 1] = current_term->TI_sgr0;
	}
	if (current_term->TI_blink)
		current_term->TI_sgrstrs[TERM_SGR_BLINK_ON - 1] = current_term->TI_blink;
	if (current_term->TI_smul)
		current_term->TI_sgrstrs[TERM_SGR_UNDL_ON - 1] = current_term->TI_smul;
	if (current_term->TI_rmul)
		current_term->TI_sgrstrs[TERM_SGR_UNDL_OFF - 1] = current_term->TI_rmul;
	if (current_term->TI_smso)
		current_term->TI_sgrstrs[TERM_SGR_REV_ON - 1] = current_term->TI_smso;
	if (current_term->TI_rmso)
		current_term->TI_sgrstrs[TERM_SGR_REV_OFF - 1] = current_term->TI_rmso;
	if (current_term->TI_smacs)
		current_term->TI_sgrstrs[TERM_SGR_ALTCHAR_ON - 1] = current_term->TI_smacs;
	if (current_term->TI_rmacs)
		current_term->TI_sgrstrs[TERM_SGR_ALTCHAR_OFF - 1] = current_term->TI_rmacs;
	if (current_term->TI_normal[0])
	{
		current_term->TI_sgrstrs[TERM_SGR_NORMAL - 1] = current_term->TI_normal;
		current_term->TI_sgrstrs[TERM_SGR_RESET - 1] = current_term->TI_normal;
	}


	/*
	 * Now figure out whether or not this terminal is "capable enough"
	 * for the client. This is a rather complicated set of tests, as
	 * we have many ways of doing the same thing.
	 * To keep the set of tests easier, we set up a bitfield integer
	 * which will have the desired capabilities added to it. If after
	 * all the checks we dont have the desired mask, we dont have a
	 * capable enough terminal.
	 */
	desired = TERM_CAN_CUP | TERM_CAN_CLEAR | TERM_CAN_CLREOL |
		TERM_CAN_RIGHT | TERM_CAN_LEFT | TERM_CAN_SCROLL;

	termfeatures = 0;
	if (current_term->TI_cup)
		termfeatures |= TERM_CAN_CUP;
	if (current_term->TI_hpa && current_term->TI_vpa)
		termfeatures |= TERM_CAN_CUP;
	if (current_term->TI_clear)
		termfeatures |= TERM_CAN_CLEAR;
	if (current_term->TI_ed)
		termfeatures |= TERM_CAN_CLEAR;
	if (current_term->TI_dl || current_term->TI_dl1)
		termfeatures |= TERM_CAN_CLEAR;
	if (current_term->TI_il || current_term->TI_il1)
		termfeatures |= TERM_CAN_CLEAR;
	if (current_term->TI_el)
		termfeatures |= TERM_CAN_CLREOL;
	if (current_term->TI_cub || current_term->TI_mrcup || current_term->TI_cub1 || current_term->TI_kbs)
		termfeatures |= TERM_CAN_LEFT;
	if (current_term->TI_cuf  || current_term->TI_cuf1 || current_term->TI_mrcup)
		termfeatures |= TERM_CAN_RIGHT;
	if (current_term->TI_dch || current_term->TI_dch1)
		termfeatures |= TERM_CAN_DELETE;
	if (current_term->TI_ich || current_term->TI_ich1)
		termfeatures |= TERM_CAN_INSERT;
	if (current_term->TI_rep)
		termfeatures |= TERM_CAN_REPEAT;
	if (current_term->TI_csr && (current_term->TI_ri || current_term->TI_rin) && (current_term->TI_ind || current_term->TI_indn))
		termfeatures |= TERM_CAN_SCROLL;
	if (current_term->TI_wind && (current_term->TI_ri || current_term->TI_rin) && (current_term->TI_ind || current_term->TI_indn))
		termfeatures |= TERM_CAN_SCROLL;
	if ((current_term->TI_il || current_term->TI_il1) && (current_term->TI_dl || current_term->TI_dl1))
		termfeatures |= TERM_CAN_SCROLL;
	if (current_term->TI_bold && current_term->TI_sgr0)
		termfeatures |= TERM_CAN_BOLD;
	if (current_term->TI_blink && current_term->TI_sgr0)
		termfeatures |= TERM_CAN_BLINK;
	if (current_term->TI_smul && current_term->TI_rmul)
		termfeatures |= TERM_CAN_UNDL;
	if (current_term->TI_smso && current_term->TI_rmso)
		termfeatures |= TERM_CAN_REVERSE;
	if (current_term->TI_dispc)
		termfeatures |= TERM_CAN_GCHAR;
	if ((current_term->TI_setf && current_term->TI_setb) || (current_term->TI_setaf && current_term->TI_setab))
		termfeatures |= TERM_CAN_COLOR;

#if !defined(__EMX__) && !defined(WINNT) && !defined(GUI)
	if ((termfeatures & desired) != desired)
	{
		fprintf(stderr, "\nYour terminal (%s) cannot run IRC II in full screen mode.\n", term);
		fprintf(stderr, "The following features are missing from your TERM setting.\n");
		if (!(termfeatures & TERM_CAN_CUP))
			fprintf (stderr, "\tCursor movement\n");
		if (!(termfeatures & TERM_CAN_CLEAR))
			fprintf(stderr, "\tClear screen\n");
		if (!(termfeatures & TERM_CAN_CLREOL))
			fprintf(stderr, "\tClear to end-of-line\n");
		if (!(termfeatures & TERM_CAN_RIGHT))
			fprintf(stderr, "\tCursor right\n");
		if (!(termfeatures & TERM_CAN_LEFT))
			fprintf(stderr, "\tCursor left\n");
		if (!(termfeatures & TERM_CAN_SCROLL))
			fprintf(stderr, "\tScrolling\n");

		fprintf(stderr, "So we'll be running in dumb mode...\n");
		return -1;
	}
#endif

	if (!current_term->TI_cub1)
	{
		if (current_term->TI_kbs)
			current_term->TI_cub1 = current_term->TI_kbs;
		else
			current_term->TI_cub1 = "\b";
	}
	if (!current_term->TI_bel)
		current_term->TI_bel = "\007";

	/*
	 * Default to no colors. (ick)
	 */
	for (i = 0; i < 16; i++)
		current_term->TI_forecolors[i] = current_term->TI_backcolors[i] = empty_string;

	/*
	 * Set up colors.
	 * Absolute fallbacks are for ansi-type colors
	 */
	for (i = 0; i < 16; i++)
	{
		char cbuf[128];

		cbuf[0] = '\0';
		if (i >= 8)
			strcpy(cbuf, current_term->TI_sgrstrs[TERM_SGR_BOLD_ON-1]);
		if (current_term->TI_setaf) 
			strcat(cbuf, (char *)tparm(current_term->TI_setaf, i & 0x07, 0));
		else if (current_term->TI_setf)
			strcat(cbuf, (char *)tparm(current_term->TI_setf, i & 0x07, 0));
		else if (i >= 8)
			sprintf(cbuf, "\033[1;%dm", (i & 0x07) + 30);
		else
			sprintf(cbuf, "\033[%dm", (i & 0x07) + 30);

		current_term->TI_forecolors[i] = m_strdup(cbuf);
	}
            
	for (i = 0; i < 16; i++)
	{
		char cbuf[128];

		cbuf[0] = '\0';
		if (i >= 8)
			strcpy(cbuf, current_term->TI_sgrstrs[TERM_SGR_BLINK_ON - 1]);

		if (current_term->TI_setab)
			strcat (cbuf, tparm(current_term->TI_setab, i & 0x07, 0));
		else if (current_term->TI_setb)
			strcat (cbuf, tparm(current_term->TI_setb, i & 0x07, 0));
		else if (i >= 8)
			sprintf(cbuf, "\033[1;%dm", (i & 0x07) + 40);
		else
			sprintf(cbuf, "\033[%dm", (i & 0x07) + 40);

		current_term->TI_backcolors[i] = m_strdup(cbuf);
	}
#endif /* NOT IN WTERM_C */
	tty_des = 0;
#ifndef GTK
	reinit_term(tty_des);
#endif
	return 0;
}

void reinit_term(int tty)
{
	/* Set up the terminal discipline */
	tcgetattr(tty, &oldb);

	newb = oldb;
	newb.c_lflag &= ~(ICANON | ECHO); /* set equ. of CBREAK, no ECHO */
	newb.c_cc[VMIN] = 1;	          /* read() satified after 1 char */
	newb.c_cc[VTIME] = 0;	         /* No timer */

#       if !defined(_POSIX_VDISABLE)
#               if defined(HAVE_FPATHCONF)
#                       define _POSIX_VDISABLE fpathconf(tty, _PC_VDISABLE)
#               else
#                       define _POSIX_VDISABLE 0
#               endif
#       endif

	newb.c_cc[VQUIT] = _POSIX_VDISABLE;
#	ifdef VDSUSP
		newb.c_cc[VDSUSP] = _POSIX_VDISABLE;
# 	endif
#	ifdef VSUSP
		newb.c_cc[VSUSP] = _POSIX_VDISABLE;
#	endif

#ifndef WTERM_C
	if (!use_flow_control)
		newb.c_iflag &= ~IXON;	/* No XON/XOFF */
	/* Ugh. =) This is annoying! */
#if use_alt_screen
	if (current_term->TI_smcup)
		tputs_x(current_term->TI_smcup);
#endif
#if use_automargins
	if (current_term->TI_rmam)
		tputs_x(current_term->TI_rmam);
#endif
#endif

	tcsetattr(tty, TCSADRAIN, &newb);
	tty_des = tty;
	return;
}


#ifndef WTERM_C

void tty_dup(int tty)
{
	close (tty_des);
	dup2(tty, tty_des);
}

void reset_lines(int lines)
{
	li = lines;
}

void reset_cols(int cols)
{
	co = cols;
}

/*
 * term_resize: gets the terminal height and width.  Trys to get the info
 * from the tty driver about size, if it can't... uses the termcap values. If
 * the terminal size has changed since last time term_resize() has been
 * called, 1 is returned.  If it is unchanged, 0 is returned. 
 */
int term_resize (void)
{
#ifndef GUI
	static	int	old_li = -1,
			old_co = -1;
#	if defined (TIOCGWINSZ)
	{
		struct winsize window;

		if (ioctl(tty_des, TIOCGWINSZ, &window) < 0)
		{
			current_term->TI_lines = li;
			current_term->TI_cols = co;
		}
		else
		{
			if ((current_term->TI_lines = window.ws_row) == 0)
				current_term->TI_lines = li;
			if ((current_term->TI_cols = (window.ws_col)) == 0)
				current_term->TI_cols = co;
		}
	}
#	else
	{
		current_term->TI_lines = li;
		current_term->TI_cols = co;
	}
#	endif

#if use_automargins
	if (!current_term->TI_am || !current_term->TI_rmam)
	{
		current_term->TI_cols--;
	}
#else
	current_term->TI_cols--;
#endif
	if ((old_li != current_term->TI_lines) || (old_co != current_term->TI_cols))
	{
		old_li = current_term->TI_lines;
		old_co = current_term->TI_cols;
		if (main_screen)
		{
			main_screen->li = current_term->TI_lines;
			main_screen->co = current_term->TI_cols;
		}
		return (1);
	}

#endif /* GUI */
	return (0);
}


/* term_CE_clear_to_eol(): the clear to eol function, right? */
void term_clreol (void)
{
#ifdef GUI
	gui_clreol();
#else
	tputs_x(current_term->TI_el);
#endif
	return;
}

/* Set the cursor position */
void term_gotoxy (int col, int row)
{
#if defined(__EMX__) && !defined(GUI)
	term_flush();
	VioSetCurPos(row,col,0);
#elif defined(GUI)
	gui_gotoxy(col, row);
#else
	
	if (current_term->TI_cup)
		tputs_x((char *)tparm(current_term->TI_cup, row, col));
	else
	{
		tputs_x((char *)tparm(current_term->TI_hpa, col));
		tputs_x((char *)tparm(current_term->TI_vpa, row));
	}
#endif
}

/* A no-brainer. Clear the screen. */
void term_clrscr (void)
{
#if defined(__EMX__) && !defined(__EMXX__) && !defined(GUI)
	VioScrollUp(0, 0, -1, -1, -1, &default_pair, vio_screen);
#elif defined(GUI)
	gui_clrscr();
#else

	int i;

	/* We have a specific cap for clearing the screen */
	if (current_term->TI_clear)
	{
		tputs_x(current_term->TI_clear);
		term_gotoxy (0, 0);
		return;
	}

	term_gotoxy (0, 0);
	/* We can clear by doing an erase-to-end-of-display */
	if (current_term->TI_ed)
	{
		tputs_x (current_term->TI_ed);
		return;
	}
	/* We can also clear by deleteing lines ... */
	else if (current_term->TI_dl)
	{
		tputs_x((char *)tparm(current_term->TI_dl, current_term->TI_lines));
		return;
	}
	/* ... in this case one line at a time */
	else if (current_term->TI_dl1)
	{
		for (i = 0; i < current_term->TI_lines; i++)
			tputs_x (current_term->TI_dl1);
		return;
	}
	/* As a last resort we can insert lines ... */
	else if (current_term->TI_il)
	{
		tputs_x ((char *)tparm(current_term->TI_il, current_term->TI_lines));
		term_gotoxy (0, 0);
		return;
	}
	/* ... one line at a time */
	else if (current_term->TI_il1)
	{
		for (i = 0; i < current_term->TI_lines; i++)
			tputs_x (current_term->TI_il1);
		term_gotoxy (0, 0);
	}
#endif
}

/*
 * Move the cursor NUM spaces to the left, non-destruvtively if we can.
 */
void term_left (int num)
{
#ifdef GUI
	gui_left(num);
#else
	if (current_term->TI_cub)
		tputs_x ((char *)tparm(current_term->TI_cub, num));
	else if (current_term->TI_mrcup)
		tputs_x ((char *)tparm(current_term->TI_mrcup, -num, 0));
	else if (current_term->TI_cub1)
		while (num--)
			tputs_x(current_term->TI_cub1);
	else if (current_term->TI_kbs)
		while (num--)
			tputs_x (current_term->TI_kbs);
#endif
}

/*
 * Move the cursor NUM spaces to the right
 */
void term_right (int num)
{
#ifdef GUI
	gui_right(num);
#else
	if (current_term->TI_cuf)
		tputs_x ((char *)tparm(current_term->TI_cuf, num));
	else if (current_term->TI_mrcup)
		tputs_x ((char *)tparm(current_term->TI_mrcup, num, 0));
	else if (current_term->TI_cuf1)
		while (num--)
			tputs_x(current_term->TI_cuf1);
#endif
}

/*
 * term_delete (int num)
 * Deletes NUM characters at the current position
 */
void term_delete (int num)
{
	if (current_term->TI_smdc)
		tputs_x(current_term->TI_smdc);

	if (current_term->TI_dch)
		tputs_x ((char *)tparm (current_term->TI_dch, num));
	else if (current_term->TI_dch1)
		while (num--)
			tputs_x (current_term->TI_dch1);

	if (current_term->TI_rmdc)
		tputs_x (current_term->TI_rmdc);
}

/*
 * Insert character C at the curent cursor position.
 */
void term_insert (unsigned char c)
{
	if (current_term->TI_smir)
		tputs_x (current_term->TI_smir);
	else if (current_term->TI_ich1)
		tputs_x (current_term->TI_ich1);
	else if (current_term->TI_ich)
		tputs_x ((char *)tparm(current_term->TI_ich, 1));

	term_putchar (c);

	if (current_term->TI_rmir)
		tputs_x(current_term->TI_rmir);
}


/*
 * Repeat the character C REP times in the most efficient way.
 */
void term_repeat (unsigned char c, int rep)
{
	if (current_term->TI_rep)
		tputs_x((char *)tparm (current_term->TI_rep, (int)c, rep));
	else
		while (rep--)
			putchar_x (c);
}



/*
 * Scroll the screen N lines between lines TOP and BOT.
 */
void term_scroll (int top, int bot, int n)
{
#ifdef GUI
        gui_scroll(top, bot, n);
#else
      int i,oneshot=0,rn,sr,er;
      char thing[128], final[128], start[128];
#ifdef __EMX__
      pair[0] = ' '; pair[1] = 7;
      if (n > 0) VioScrollUp(top, 0, bot, current_term->TI_cols, n, (PBYTE)&pair, (HVIO) vio_screen);
      else if (n < 0) { n = -n; VioScrollDn(top, 0, bot, current_term->TI_cols, n, (PBYTE)&pair, (HVIO) vio_screen); }
#ifndef __EMXX__
      return;
#endif
#elif defined(WINNT)

	/* It seems that Windows 95/98 has a problem with scrolling
	   when you set the scroll area to something other than the 
	   default, using current_term->TI_csr, so this API call will scroll the 
	   text instead. */

	SMALL_RECT src;
	COORD dest;
	CHAR_INFO chr;
			
	src.Left = gscrbuf.srWindow.Left;
	src.Top = top+n;
	src.Right = gscrbuf.srWindow.Right;
	src.Bottom = bot;
										
	dest.X = 0;
	dest.Y = top;
									
	chr.Char.AsciiChar = ' ';
	chr.Attributes = 0;
	ScrollConsoleScreenBuffer(ghstdout, &src, NULL, dest, &chr);
        return;																        
#endif

	/* Some basic sanity checks */
	if (n == 0 || top == bot || bot < top)
		return;

	sr = er = 0;
	final[0] = start[0] = thing[0] = 0;

	if (n < 0)
		rn = -n;
	else
		rn = n;

	/*
	 * First thing we try to do is set the scrolling region on a 
	 * line granularity.  In order to do this, we need to be able
	 * to have termcap 'cs', and as well, we need to have 'sr' if
	 * we're scrolling down, and 'sf' if we're scrolling up.
	 */
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
			if (current_term->TI_indn)
			{
				oneshot = 1;
				strcpy(thing, (char *)tparm(current_term->TI_indn, rn, rn));
			}
			else
				strcpy(thing, current_term->TI_ind);
		}
		else
		{
			sr = top;
			er = bot;
			if (current_term->TI_rin)
			{
				oneshot = 1;
				strcpy (thing, (char *)tparm(current_term->TI_rin, rn, rn));
			}
			else
				strcpy (thing, current_term->TI_ri);
		}
	}

	else if (current_term->TI_wind && (current_term->TI_ri || current_term->TI_rin) && (current_term->TI_ind || current_term->TI_indn))
	{
		strcpy(start, (char *)tparm(current_term->TI_wind, top, bot, 0, current_term->TI_cols-1));
		strcpy(final, (char *)tparm(current_term->TI_wind, 0, current_term->TI_lines-1, 0, current_term->TI_cols-1));

		if (n > 0)
		{
			sr = bot;
			er = top;
			if (current_term->TI_indn)
			{
				oneshot = 1;
				strcpy (thing, (char *)tparm(current_term->TI_indn, rn, rn));
			}
			else
				strcpy (thing, current_term->TI_ind);
		}
		else
		{
			sr = top;
			er = bot;
			if (current_term->TI_rin)
			{
				oneshot = 1;
				strcpy (thing,(char *)tparm(current_term->TI_rin, rn, rn));
			}
			else
				strcpy (thing, current_term->TI_ri);
		}
	}

	else if ((current_term->TI_il || current_term->TI_il1) && (current_term->TI_dl || current_term->TI_dl1))
	{
		if (n > 0)
		{
			sr = top;
			er = bot;

			if (current_term->TI_dl)
			{
				oneshot = 1;
				strcpy (thing, (char *)tparm(current_term->TI_dl, rn, rn));
			}
			else
				strcpy (thing, current_term->TI_dl1);

			if (current_term->TI_il)
			{
				oneshot = 1;
				strcpy(final, (char *)tparm(current_term->TI_il, rn, rn));
			}
			else
				strcpy(final, current_term->TI_il1);
		}
		else
		{
			sr = bot;
			er = top;
			if (current_term->TI_il)
			{
				oneshot = 1;
				strcpy (thing, (char *)tparm(current_term->TI_il, rn, rn));
			}
			else
				strcpy (thing, current_term->TI_il1);

			if (current_term->TI_dl)
			{
				oneshot = 1;
				strcpy(final, (char *)tparm(current_term->TI_dl, rn, rn));
			}
			else
				strcpy(final, current_term->TI_dl1);
		}
	}


	if (!thing[0])
		return;

	/* Do the actual work here */
	if (start[0])
		tputs_x (start);
	term_gotoxy (0, sr);

	if (oneshot)
		tputs_x (thing);
	else
	{
		for (i = 0; i < rn; i++)
			tputs_x(thing);
	}
	term_gotoxy (0, er);
	if (final[0])
		tputs_x(final);
#endif
}

/*
 * term_getsgr(int opt, int fore, int back)
 * Return the string required to set the given mode. OPT defines what
 * we really want it to do. It can have these values:
 * TERM_SGR_BOLD_ON     - turn bold mode on
 * TERM_SGR_BOLD_OFF    - turn bold mode off
 * TERM_SGR_BLINK_ON    - turn blink mode on
 * TERM_SGR_BLINK_OFF   - turn blink mode off
 * TERM_SGR_UNDL_ON     - turn underline mode on
 * TERM_SGR_UNDL_OFF    - turn underline mode off
 * TERM_SGR_REV_ON      - turn reverse video on
 * TERM_SGR_REV_OFF     - turn reverse video off
 * TERM_SGR_NORMAL      - turn all attributes off
 * TERM_SGR_RESET       - all attributes off and back to default colors
 * TERM_SGR_FOREGROUND  - set foreground color
 * TERM_SGR_BACKGROUND  - set background color
 * TERM_SGR_COLORS      - set foreground and background colors
 * TERM_SGR_GCHAR       - print graphics character
 *
 * The colors are defined as:
 * 0    - black
 * 1    - red
 * 2    - green
 * 3    - brown
 * 4    - blue
 * 5    - magenta
 * 6    - cyan
 * 7    - white
 * 8    - grey (foreground only)
 * 9    - bright red (foreground only)
 * 10   - bright green (foreground only)
 * 11   - bright yellow (foreground only)
 * 12   - bright blue (foreground only)
 * 13   - bright magenta (foreground only)
 * 14   - bright cyan (foreground only)
 * 15   - bright white (foreground only)
 */
char *term_getsgr (int opt, int fore, int back)
{
	char *ret = empty_string;

	switch (opt)
	{
		case TERM_SGR_BOLD_ON:
		case TERM_SGR_BOLD_OFF:
		case TERM_SGR_BLINK_OFF:
		case TERM_SGR_BLINK_ON:
		case TERM_SGR_UNDL_ON:
		case TERM_SGR_UNDL_OFF:
		case TERM_SGR_REV_ON:
		case TERM_SGR_REV_OFF:
		case TERM_SGR_NORMAL:
		case TERM_SGR_RESET:
			ret = current_term->TI_sgrstrs[opt-1];
			break;
		case TERM_SGR_FOREGROUND:
			ret = current_term->TI_forecolors[fore & 0x0f];
			break;
		case TERM_SGR_BACKGROUND:
			ret = current_term->TI_backcolors[fore & 0x0f];
			break;
		case TERM_SGR_GCHAR:
			if (current_term->TI_dispc)
				ret = (char *)tparm(current_term->TI_dispc, fore);
			break;
		default:
			ircpanic ("Unknown option '%d' to term_getsgr", opt);
			break;
	}
	return (ret);
}




extern	int term_eight_bit (void)
{
#ifdef GUI
	return 1;
#else
	if (dumb_mode)
		return 1;
	return (((newb.c_cflag) & CSIZE) == CS8) ? 1 : 0;
#endif
}

extern	void term_beep (void)
{
	if (get_int_var(BEEP_VAR))
	{
#ifdef __EMX__
		DosBeep(1000, 200);
#else
		tputs_x(current_term->TI_bel);
		term_flush();
#endif
	}
}

extern	void	set_term_eight_bit (int value)
{
	if (dumb_mode)
		return;
	if (value == ON)
	{
		newb.c_cflag |= CS8;
		newb.c_iflag &= ~ISTRIP;
	}
	else
	{
		newb.c_cflag &= ~CS8;
		newb.c_iflag |= ISTRIP;
	}
	tcsetattr(tty_des, TCSADRAIN, &newb);
}

void	set_meta_8bit (Window *w, char *u, int value)
{
	if (dumb_mode)
		return;

	if (value == 0)
		meta_mode = 0;
	else if (value == 1)
		meta_mode = 1;
	else if (value == 2)
		meta_mode = (current_term->TI_km == 0 ? 0 : 1);
}

char *	control_mangle (unsigned char *text)
{
static 	u_char	retval[256];
	int 	pos = 0;
	
	*retval = 0;
	if (!text)
		return retval;
		
	for (; *text && (pos < 254); text++, pos++)
	{
		if (*text < 32) 
		{
			retval[pos++] = '^';
			retval[pos] = *text + 64;
		}
		else if (*text == 127)
		{
			retval[pos++] = '^';
			retval[pos] = '?';
		}
		else
			retval[pos] = *text;
	}

	retval[pos] = 0;
	return retval;
}

char *	get_term_capability (char *name, int querytype, int mangle)
{
static	char		retval[128];
	const char *	compare = empty_string;
	int 		x;
	cap2info *	t;

	for (x = 0; x < numcaps; x++)
	{
		t = &tcaps[x];
		if (querytype == 0)
			compare = t->longname;
		else if (querytype == 1)
			compare = t->iname;
		else if (querytype == 2)
			compare = t->tname;

		if (!strcmp(name, compare))
		{

			switch (t->type)
			{
			case CAP_TYPE_BOOL:
			case CAP_TYPE_INT:
			if (!(int *)t->ptr)
				return NULL;
			strcpy(retval, ltoa(* (int *)(t->ptr)));
			return retval;
			case CAP_TYPE_STR:
			if (!(char **)t->ptr || !*(char **)t->ptr)
				return NULL;
			strcpy(retval, mangle ? 
					control_mangle(*(char **)t->ptr) :
					(*(char **)t->ptr));
			return retval;
		}
		}
	}
	return NULL;
}

#endif /* NOT IN WTERM_C */

#if 0
/*
 * term.c -- for windows 95/NT
 *
 * Copyright 1997 Colten Edwards
 *
 */

#include "irc.h"
#include "vars.h"
#include "ircterm.h"
#include "window.h"
#include "screen.h"
#include "output.h"
#ifdef TRANSLATE
#include "translat.h"
#endif

#include <windows.h>

char eolbuf[200];

#ifndef WTERM_C
extern	char	*getenv();

static	int	term_CE_clear_to_eol 	(void);
static	int	term_CS_scroll 		(int, int, int);
static 	int	term_null_function 	(void);

/*
 * Function variables: each returns 1 if the function is not supported on the
 * current term type, otherwise they do their thing and return 0
 */
int	(*term_scroll) (int, int, int);	/* this is set to the best scroll available */
int	(*term_insert) (char);		/* this is set to the best insert available */
int	(*term_delete) (void);		/* this is set to the best delete available */
int	(*term_cursor_left) (void);	/* this is set to the best left available */
int	(*term_cursor_right) (void); 	/* this is set to the best right available */
int	(*term_clear_to_eol) (void); 	/* this is set... figure it out */

int nt_cursor_right(void);
int nt_cursor_left(void);

int	CO = 79,
	LI = 24;
HANDLE ghstdout;
CONSOLE_SCREEN_BUFFER_INFO gscrbuf;
CONSOLE_CURSOR_INFO gcursbuf;
DWORD gdwPlatform;

/*
 * term_reset_flag: set to true whenever the terminal is reset, thus letter
 * the calling program work out what to do
 */
	int	need_redraw = 0;
static	int	term_echo_flag = 1;
static	int	li;
static	int	co;
static	int	def_color = 0x07;
	int	CO, LI;


/*
 * term_echo: if 0, echo is turned off (all characters appear as blanks), if
 * non-zero, all is normal.  The function returns the old value of the
 * term_echo_flag
 */
int	term_echo (int flag)
{
	int	echo;

	echo = term_echo_flag;
	term_echo_flag = flag;
	return (echo);
}

void term_flush (void)
{
	return;
}

/*
 * term_reset: sets terminal attributed back to what they were before the
 * program started
 */
void term_reset (void)
{
	term_move_cursor(0, LI - 1);
}

/*
 * term_cont: sets the terminal back to IRCII stuff when it is restarted
 * after a SIGSTOP.  Somewhere, this must be used in a signal() call
 */

SIGNAL_HANDLER(term_cont)
{
	refresh_screen(0, NULL);
}

/*
 * term_pause: sets terminal back to pre-program days, then SIGSTOPs itself.
 */
extern void term_pause (char unused, char *not_used)
{
char *shell;
DWORD dwmode;
HANDLE hstdin=GetStdHandle(STD_INPUT_HANDLE);
STARTUPINFO si = { 0 };
PROCESS_INFORMATION pi = { 0 };

	si.cb = sizeof(si);

	if (!(shell = get_string_var(SHELL_VAR)))
	{
		if (gdwPlatform == VER_PLATFORM_WIN32_WINDOWS)
			shell = "command.com";
		else
			shell = "cmd.exe";
	}
	if (!(GetConsoleMode(hstdin,&dwmode)))
		return;

	if (!SetConsoleMode(hstdin, dwmode | ((ENABLE_LINE_INPUT|ENABLE_ECHO_INPUT|ENABLE_PROCESSED_INPUT) & ~ENABLE_WINDOW_INPUT)))
		return;

	if(!CreateProcess(NULL, shell, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi) )
		return;
	CloseHandle(pi.hThread);
	WaitForSingleObject(pi.hProcess,INFINITE);
	CloseHandle(pi.hProcess);

	if (!SetConsoleMode(hstdin, dwmode ) )
		return;
	refresh_screen(0,NULL);
}
#endif /* NOT IN WTERM_C */



/*
 * term_init: does all terminal initialization... reads termcap info, sets
 * the terminal to CBREAK, no ECHO mode.   Chooses the best of the terminal
 * attributes to use ..  for the version of this function that is called for
 * wserv, we set the termial to RAW, no ECHO, so that all the signals are
 * ignored.. fixes quite a few problems...  -phone, jan 1993..
 */
int term_init (void)
{
#ifndef WTERM_C

	CONSOLE_SCREEN_BUFFER_INFO scrbuf;
	HANDLE hinput =GetStdHandle(STD_INPUT_HANDLE);
	DWORD dwmode;
	OSVERSIONINFO osver;

	ghstdout = GetStdHandle(STD_OUTPUT_HANDLE);

	if(!GetConsoleScreenBufferInfo(ghstdout, &gscrbuf) ) 
	{
		fprintf(stderr,"Error from getconsolebufinfo %d\n",GetLastError());
		abort();
	}

	GetConsoleMode(hinput,&dwmode);
	SetConsoleMode(hinput,dwmode & (~(ENABLE_LINE_INPUT |ENABLE_ECHO_INPUT
				| ENABLE_PROCESSED_INPUT)| ENABLE_WINDOW_INPUT/* | ENABLE_WRAP_AT_EOL_OUTPUT*/)
				);

	osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osver);
	gdwPlatform = osver.dwPlatformId;

	GetConsoleCursorInfo(hinput, &gcursbuf);

	GetConsoleScreenBufferInfo(ghstdout, &scrbuf);
	li = scrbuf.srWindow.Bottom - scrbuf.srWindow.Top + 1;
	co = scrbuf.srWindow.Right - scrbuf.srWindow.Left + 1;
	LI = li;
	CO = co - 1;
	memset(eolbuf,' ',sizeof(eolbuf)-2);
	eolbuf[sizeof(eolbuf)-1] = 0;
	def_color = gscrbuf.wAttributes;

	term_clear_to_eol = term_CE_clear_to_eol;
	term_cursor_right = nt_cursor_right;
	term_cursor_left  = nt_cursor_left;
	term_scroll 	  = term_CS_scroll/* : term_param_ALDL_scroll*/;
	term_delete	  = term_null_function;
	term_insert       = term_null_function;
#endif /* NOT IN WTERM_C */
	return 0;
}


#ifndef WTERM_C
/*
 * term_resize: gets the terminal height and width.  Trys to get the info
 * from the tty driver about size, if it can't... uses the termcap values. If
 * the terminal size has changed since last time term_resize() has been
 * called, 1 is returned.  If it is unchanged, 0 is returned.
 */
int term_resize (void)
{
	static	int	old_li = -1,
			old_co = -1;

	CONSOLE_SCREEN_BUFFER_INFO scrbuf;
	ghstdout = GetStdHandle(STD_OUTPUT_HANDLE);

	GetConsoleScreenBufferInfo(ghstdout, &scrbuf);
	li = scrbuf.srWindow.Bottom - scrbuf.srWindow.Top + 1;
	co = scrbuf.srWindow.Right - scrbuf.srWindow.Left + 1;
	LI = li;
	CO = co - 1;
	if ((old_li != LI) || (old_co != CO))
	{
		old_li = LI;
		old_co = CO;
		return (1);
	}
	return (0);
}


static 	int	term_null_function _((void))
{
	return 1;
}

/* term_CE_clear_to_eol(): the clear to eol function, right? */

static	int term_CE_clear_to_eol _((void))
{
	CONSOLE_SCREEN_BUFFER_INFO scrbuf;
	HANDLE hStdout = ghstdout;
	DWORD numwrote;
	int num=0;
	COORD savepos;

	if(!GetConsoleScreenBufferInfo(hStdout, &scrbuf) )
		return 0 ;

	num = scrbuf.srWindow.Right - scrbuf.dwCursorPosition.X;
	savepos = scrbuf.dwCursorPosition;

	if (!WriteConsole(hStdout,eolbuf,num,&numwrote,NULL))
		return 0;

	SetConsoleCursorPosition(hStdout, savepos);
	return (0);
}

/*
 * term_CS_scroll: should be used if the terminal has the CS capability by
 * setting term_scroll equal to it
 */
static	int	term_CS_scroll (int line1, int line2, int n)
{
	HANDLE hStdout = ghstdout;
	SMALL_RECT src;
	SMALL_RECT clip;
	COORD dest = {0};
	CHAR_INFO chr;

	src.Left = gscrbuf.srWindow.Left;
	src.Right = gscrbuf.srWindow.Right;
	chr.Char.AsciiChar = ' ';
	chr.Attributes = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
	if (n > 0)
	{
		src.Bottom = gscrbuf.srWindow.Top + line2;
		src.Top =  gscrbuf.srWindow.Top + line1 + n;
		dest.Y =  gscrbuf.srWindow.Top + line1;
		ScrollConsoleScreenBuffer(hStdout, &src, NULL, dest, &chr);
	}
	else
	{
		clip = src;
        	src.Bottom = gscrbuf.srWindow.Top + line2 + line1 + n;
		src.Top = gscrbuf.srWindow.Top + line1;
		dest.Y = gscrbuf.srWindow.Top + line1 - n;
		clip.Top = gscrbuf.srWindow.Top + line1;
		clip.Bottom = gscrbuf.srWindow.Top + line1 + line2;
		ScrollConsoleScreenBuffer(hStdout, &src, &clip, dest, &chr);
	}
	return 0;
}

extern	void	copy_window_size (int *lines, int *columns)
{
	*lines = LI;
	*columns = CO;
}

extern	int term_eight_bit (void)
{
	return 1;
}

extern	void term_beep (void)
{
	if (get_int_var(BEEP_VAR))
	{
       		Beep(0x637, 60/8);
	}
}

void ScrollBuf(HANDLE hOut, CONSOLE_SCREEN_BUFFER_INFO *scrbuf,int where)
{
	SMALL_RECT src;
	COORD dest;
	CHAR_INFO chr;

	src.Left = scrbuf->srWindow.Left;
	src.Top = scrbuf->srWindow.Top+where ;
	src.Right = scrbuf->srWindow.Right;
	src.Bottom = scrbuf->srWindow.Bottom;

	dest.X = 0;
	dest.Y = 0;

	chr.Char.AsciiChar = ' ';
	chr.Attributes = 0;
	ScrollConsoleScreenBuffer(hOut, &src, NULL, dest, &chr);

}

void NT_MoveToLineOrChar(void *fd, int where,int line)
{
	CONSOLE_SCREEN_BUFFER_INFO gscrbuf;
	GetConsoleScreenBufferInfo((HANDLE)fd, &gscrbuf);
	if (line)
	{
		if ( ((gscrbuf.dwCursorPosition.Y+where)> (gscrbuf.srWindow.Bottom-1))
			&&( where >0))
			ScrollBuf((HANDLE)fd,&gscrbuf,where);
		else
			gscrbuf.dwCursorPosition.Y += where;
	}
	else
		gscrbuf.dwCursorPosition.X += where;
	SetConsoleCursorPosition((HANDLE)fd, gscrbuf.dwCursorPosition);
}

void NT_MoveToLineOrCharAbs(void *fd, int where,int line)
{

	CONSOLE_SCREEN_BUFFER_INFO scrbuf;
        GetConsoleScreenBufferInfo((HANDLE)fd, &scrbuf);

	if (line)
		scrbuf.dwCursorPosition.Y = where+scrbuf.srWindow.Top;
	else
		scrbuf.dwCursorPosition.X = where;
	SetConsoleCursorPosition((HANDLE)fd, scrbuf.dwCursorPosition);
}

void term_move_cursor(int cursor, int line)
{
	HANDLE hStdout =ghstdout;
	COORD dest;
	dest.X = cursor;
	dest.Y = line + gscrbuf.srWindow.Top;
	SetConsoleCursorPosition(hStdout, dest);
}


void NT_ClearScreen(void)
{
	CONSOLE_SCREEN_BUFFER_INFO scrbuf;
	DWORD numwrote;
	COORD origin = {0, 0};
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	GetConsoleScreenBufferInfo(hStdout, &scrbuf);
	FillConsoleOutputCharacter(hStdout,' ',scrbuf.dwSize.X*scrbuf.dwSize.Y,
		origin,&numwrote);
	FillConsoleOutputAttribute(hStdout,scrbuf.wAttributes,
		scrbuf.dwSize.X*scrbuf.dwSize.Y,scrbuf.dwCursorPosition,&numwrote);
	origin.X = scrbuf.srWindow.Left;
	origin.Y = scrbuf.srWindow.Top;
	SetConsoleCursorPosition(hStdout, origin);
	return;
}

int nt_cursor_right(void)
{
	NT_MoveToLineOrChar(GetStdHandle(STD_OUTPUT_HANDLE), 1,0);
	return 0;
}

int nt_cursor_left(void)
{
	NT_MoveToLineOrChar(GetStdHandle(STD_OUTPUT_HANDLE), -1,0);
	return 0;
}

extern	void	set_term_eight_bit (int value)
{
	return;
}

#define NPAR 16

static unsigned int color_table[] = {
        0x00,   0x04,   0x02,   0x06,    0x01,   0x05,   0x03,   0x07,
        0x00|8, 0x04|8, 0x02|8, 0x06|8,  0x01|8, 0x05|8, 0x03|8, 0x07|8
};

enum VC_STATES { ESnormal = 0, ESesc, ESsquare, ESgetpars, ESgotpars, ESfunckey };
enum VC_STATES vc_state;
unsigned long par[NPAR+1] = {0};
unsigned long npar = 0;
int intensity = 1;
int blink = 0;
int underline = 0;
int reverse_ch = 0;


void reset_attrib(void *fd, int *fg, int *bg)
{
	intensity = 0;
	underline = 0;
	reverse_ch = 0;
	blink = 0;
	*fg=FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
	*bg=0;
}


void csi_m(void *fd)
{
int i;
static int fg = 7;
static int bg = 0;
	CONSOLE_SCREEN_BUFFER_INFO scrbuf;
	GetConsoleScreenBufferInfo((HANDLE)fd, &scrbuf);
	for (i=0; i <= npar; i++)
	{
        	switch(par[i])
        	{
        		case 0:
                        	/* all off */
				reset_attrib(fd, &fg, &bg);
                        	break;
        		case 1: /* bold */
        			intensity = FOREGROUND_INTENSITY;
        			break;
			case 2: /* all off */
				fg = 0;
				bg = 0;
				intensity = 0;
				break;
        		case 4: /* underline */
		               	fg=FOREGROUND_BLUE | FOREGROUND_RED;
		               	intensity=FOREGROUND_INTENSITY;
		               	bg=0;
                        	break;
        		case 5: /* blink */
				fg=FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
				bg=0;
				intensity=FOREGROUND_INTENSITY | BACKGROUND_INTENSITY;
        			break;
        		case 7: /* reverse */
				fg=0;
				bg=BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED;
				intensity=0;
				break;
        		case 49:
        			fg = (def_color & 0xf0) | bg;
        			break;
        		default:
				if (par[i] >=30 && par[i] <= 37)
					fg = color_table[par[i]-30];
				else if (par[i] >=40 && par[i] <= 47)
					bg = color_table[par[i]-40]<<4;
        			break;

        	}
	}
	/* update attribs now */
	SetConsoleTextAttribute(fd, fg | bg | intensity);
}

void set_term_intensity(int flag)
{
	par[0] = flag ? 1 : 0;
	npar = 0;
	csi_m(GetStdHandle(STD_OUTPUT_HANDLE));
}
void set_term_inverse(int flag)
{
	par[0] = flag ? 7 : 0;
	npar = 0;
	csi_m(GetStdHandle(STD_OUTPUT_HANDLE));
}
void set_term_underline(int flag)
{
	par[0] = flag ? 4 : 0;
	npar = 0;
	csi_m(GetStdHandle(STD_OUTPUT_HANDLE));
}

void csi_K(void *fd, int parm)
{
COORD save_curs = {0};
CONSOLE_SCREEN_BUFFER_INFO scrbuf;
DWORD bytes_rtn;
int num = 0;
	switch(parm)
	{
		case 2:
			NT_MoveToLineOrCharAbs(fd, 0, 0);
		case 0:
			term_CE_clear_to_eol();
			break;
		case 1:
			GetConsoleScreenBufferInfo((HANDLE)fd, &scrbuf);
			save_curs.Y = scrbuf.dwCursorPosition.Y;
			SetConsoleCursorPosition((HANDLE)fd, save_curs);
			num = scrbuf.dwCursorPosition.X - 1;
			if (num > 0)
	                        WriteConsole((HANDLE)fd, eolbuf, num, &bytes_rtn, NULL);
			SetConsoleCursorPosition((HANDLE)fd, scrbuf.dwCursorPosition);
			break;
	}
}

int nt_write(void * fd, unsigned char *buf, register int howmany)
{
	DWORD bytes_rtn;
	int num = 0;
	int len = -1;
	register int c;
	int ok;
	register unsigned char *buf1 = buf;
	vc_state = ESnormal;

	while (howmany)
	{
		howmany--;
                num++;
                ok = 0;
		c = *buf1;
		buf1++;
		len++;
		if (!ok && c)
		{
		        switch(c)
			{
		                case 0x1b:
		                case REV_TOG:
		                case UND_TOG:
		                case BOLD_TOG:
		                case ALL_OFF:
		                	ok = 0;
                                	break;
		                default:
					ok = 1;
                	}
		}
		if (vc_state == ESnormal && ok)
		{
                        WriteConsole((HANDLE)fd, buf+len,1, &bytes_rtn, NULL);
			continue;
		}

        	switch(c)
        	{
			case REV_TOG:
				if (reverse_ch) reverse_ch = 0; else reverse_ch = 1;
				set_term_inverse(reverse_ch);
				continue;
			case BOLD_TOG:
				if (intensity) intensity = 0; else intensity = 1;
				set_term_intensity(intensity);
				continue;
			case UND_TOG:
				if (underline) underline = 0; else underline = 1;
                        	set_term_underline(underline);
                        	continue;
			case ALL_OFF:
				set_term_underline(0);
				set_term_intensity(0);
				set_term_inverse(0); /* This turns it all off */
				intensity = 0;
				underline = 0;
				reverse_ch = 0;
				if (!buf1)	/* We're the end of line */
					term_bold_off();
				continue;
                	case 7:
                		Beep(0x637, 60/8);
                        	continue;
                	case 8:
                        	continue;
                	case 9:
                        	continue;
                	case 10: case 11: case 12:
				NT_MoveToLineOrChar(fd, 1, 1);
			case 13:
				NT_MoveToLineOrCharAbs(fd, 0, 0);
                        	continue;
                	case 24: case 26:
                		vc_state = ESnormal;
                        	continue;
			case 27:
				vc_state = ESesc;
                        	continue;
                	case 128+27:
                		vc_state = ESsquare;
                        	continue;
        	}
        	switch(vc_state)
        	{
        		case ESesc:
        			vc_state = ESnormal;
                        	switch(c)
                        	{
                        		case '[':
                        			vc_state = ESsquare;
                        			continue;
                        	}
                        	continue;
                	case ESsquare:
                		vc_state = ESgetpars;
				for(npar = 0 ; npar < NPAR ; npar++)
					par[npar] = 0;
				npar = 0;
				vc_state = ESgetpars;
				if (c == '[') { /* Function key */
					vc_state=ESfunckey;
					continue;
				}
			case ESgetpars:
				if (c==';' && npar<NPAR-1)
				{
					npar++;
					continue;
				} else if (c>='0' && c<='9')
				{
					par[npar] *= 10;
					par[npar] += c-'0';
					continue;
				} else
					vc_state=ESgotpars;
			case ESgotpars:
				vc_state = ESnormal;
				switch(c)
				{
					case 'C': case 'a':
						NT_MoveToLineOrChar(fd, par[0], 0);
						continue;
					case 'K':
						csi_K(fd, par[0]);
						continue;
                                	case 'm':
                                		csi_m(fd);
	                               		continue;
					case 'r':
					{
						int top, bottom;
						if (!par[0]) par[0]++;
						if (!par[1]) par[1] = li;
						if (par[0] < par[1] && par[1] < li)
						{
							top = par[0]-1;
							bottom = par[1];
                                                	term_CS_scroll(top, bottom, 1);
						}
						continue;
					}
				}
			        continue;
			default:
				vc_state = ESnormal;

        	}
	}
	return num;
}

#endif /* NOT IN WTERM_C */
#endif
