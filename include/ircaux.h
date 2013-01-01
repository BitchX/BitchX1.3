/*
 * ircaux.h: header file for ircaux.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT
 *
 * @(#)$Id: ircaux.h 80 2009-11-24 10:21:30Z keaston $
 */

#ifndef _IRCAUX_H_
#define _IRCAUX_H_

#include "irc.h"
#include "irc_std.h"
#include <stdio.h>
#ifdef WANT_TCL
#undef USE_TCLALLOC
#include <tcl.h>
#endif

typedef int comp_len_func (char *, char *, int);
typedef int comp_func (char *, char *);

extern unsigned char stricmp_table[];


char *	BX_check_nickname 		(char *);
char *	BX_next_arg 		(char *, char **);
char *	BX_new_next_arg 		(char *, char **);
char *	BX_new_new_next_arg 	(char *, char **, char *);
char *	BX_last_arg 		(char **);
char *	BX_expand_twiddle 		(char *);
char *	BX_upper 			(char *);
char *	BX_lower 			(char *);
char *	BX_sindex			(register char *, char *);
char *	BX_rsindex 		(register char *, char *, char *, int);
char *	BX_path_search 		(char *, char *);
char *	BX_double_quote 		(const char *, const char *, char *);
char *	quote_it 		(const char *, const char *, char *);

char *	n_malloc_strcpy		(char **, const char *, const char *, const char *, const int);
char *	BX_malloc_str2cpy		(char **, const char *, const char *);
char *	n_malloc_strcat 	(char **, const char *, const char *, const char *, const int);

char *	BX_m_s3cat_s 		(char **, const char *, const char *);
char *	BX_m_s3cat 		(char **, const char *, const char *);
char *	BX_m_3cat 			(char **, const char *, const char *);
char *	BX_m_e3cat 		(char **, const char *, const char *);
char *	BX_m_2dup 			(const char *, const char *);
char *	BX_m_3dup 			(const char *, const char *, const char *);
char *	BX_m_opendup 		(const char *, ...);
char *	n_m_strdup 		(const char *, const char *, const char *, const int);
char *	BX_malloc_sprintf 		(char **, const char *, ...);
char *	BX_m_sprintf 		(const char *, ...);
int	BX_is_number 		(const char *);
char *	BX_my_ctime 		(time_t);

#if 0
#define my_stricmp(x, y) strcasecmp(x, y) /* unable to use these for reasons of case sensitivity and finish */
#define my_strnicmp(x, y, n) strncasecmp(x, y, n)
#else
int	BX_my_stricmp 	(const char *, const char *);
int	BX_my_strnicmp	(const char *, const char *, size_t);
#endif

int	BX_my_strnstr 		(const unsigned char *, const unsigned char *, size_t);
int	BX_scanstr 		(char *, char *);
void	really_free 		(int);
char *	BX_chop 			(char *, int);
char *	BX_strmcpy 		(char *, const char *, int);
char *	BX_strmcat 		(char *, const char *, int);
char *	strmcat_ue 		(char *, const char *, int);
char *	n_m_strcat_ues 		(char **, char *, int, const char *, const char *, const int);
char *	BX_stristr 		(const char *, const char *);
char *	BX_rstristr 		(char *, char *);
FILE *	BX_uzfopen 		(char **, char *, int);
int	BX_end_strcmp 		(const char *, const char *, int);
void	BX_ircpanic		(char *, ...);
int	fw_strcmp 		(comp_len_func *, char *, char *);
int	lw_strcmp 		(comp_func *, char *, char *);
int	open_to 		(char *, int, off_t);
struct timeval BX_get_time 	(struct timeval *);
double 	BX_time_diff 		(struct timeval, struct timeval);
char *	BX_plural 			(int);
int	BX_time_to_next_minute 	(void);
char *	BX_remove_trailing_spaces 	(char *);
char *	BX_my_ltoa			(long);
char *	BX_strformat 		(char *, const char *, int, char);
char *	chop_word 		(char *);
int	BX_splitw 			(char *, char ***);
char *	BX_unsplitw 		(char ***, int);
int	BX_check_val 		(char *);
char *	BX_strextend 		(char *, char, int);
char *	strext			(char *, char *);
char *	BX_pullstr 		(char *, char *);
int 	BX_empty 			(const char *);
char *	safe_new_next_arg 	(char *, char **);
char *	BX_MatchingBracket 	(register char *, register char, register char);
int	BX_word_count 		(char *);
int	BX_parse_number 		(char **);
char *	BX_remove_brackets 	(const char *, const char *, int *);
u_long	hashpjw 		(char *, u_long);
char *	BX_m_dupchar 		(int);
char *	BX_strmccat		(char *, char, int);
off_t	file_size		(char *);
int	is_root			(char *, char *, int);
size_t	BX_streq			(const char *, const char *);
size_t	BX_strieq			(const char *, const char *);
char *	n_m_strndup		(const char *, size_t, const char *, const char *, const int);
char *	BX_on_off 			(int);
char *	BX_rfgets			(char *, int, FILE *);
char *  BX_strmopencat             (char *, int, ...);
long 	BX_my_atol			(const char *);
char *	s_next_arg		(char **);
char *	BX_next_in_comma_list	(char *, char **);
void	BX_strip_control		(const char *, char *);
int	BX_figure_out_address	(char *, char **, char **, char **, char **, int *);
int	count_char		(const unsigned char *, const unsigned char);
char *	BX_strnrchr		(char *, char, int);
void	BX_mask_digits		(char **);
const char *BX_strfill		(char, int);
char *	BX_ov_strcpy		(char *, const char *);
char *	BX_strpcat			(char *, const char *, ...);
char *  BX_strmpcat		(char *, size_t, const char *, ...);
char *	chomp			(char *);
size_t	BX_ccspan			(const char *, int);
u_char *BX_strcpy_nocolorcodes	(u_char *, const u_char *);

u_long	BX_random_number		(u_long);
char *	get_userhost		(void);

char *	urlencode		(const char *);
char *	urldecode		(char *);

/* From words.c */
#define SOS -32767
#define EOS 32767
char	*BX_strsearch			(register char *, char *, char *, int);
char	*BX_move_to_abs_word	(const register char *, char **, int);
char	*BX_move_word_rel		(const register char *, char **, int);
char	*BX_extract		(char *, int, int);
char	*BX_extract2		(const char *, int, int);
int	BX_wild_match		(const char *, const char *);

/* Used for connect_by_number */
#define SERVICE_SERVER 0
#define SERVICE_CLIENT 1
#define PROTOCOL_TCP 0
#define PROTOCOL_UDP 1

/* Used from network.c */
int			BX_connect_by_number (char *, unsigned short *, int, int, int);
struct sockaddr_foobar *	BX_lookup_host(const char *);
char *			BX_host_to_ip (const char *);
char *			BX_ip_to_host (const char *);
char *			BX_one_to_another (const char *);
int			BX_set_blocking (int);
int			BX_set_non_blocking (int);
int			my_accept (int, struct sockaddr *, int *);
int			lame_resolv (const char *, struct sockaddr_foobar *);

#define my_isspace(x) \
	((x) == 9 || (x) == 10 || (x) == 11 || (x) == 12 || (x) == 13 || (x) == 32)
  
#define my_isdigit(x) \
(*x >= '0' && *x <= '9') || \
((*x == '-'  || *x == '+') && (x[1] >= '0' && x[1] <= '9'))

#define LOCAL_COPY(y) strcpy(alloca(strlen((y)) + 1), y)


#define	_1KB	((double) 1000)
#define _1MEG	(_1KB * _1KB)
#define _1GIG	(_1KB * _1KB * _1KB)
#define _1TER	(_1KB * _1KB * _1KB * _1KB)
#define _1ETA	(_1KB * _1KB * _1KB * _1KB * _1KB)

#if 0
#define	_1MEG	(1024.0*1024.0)
#define	_1GIG	(1024.0*1024.0*1024.0)
#define	_1TER	(1024.0*1024.0*1024.0*1024.0)
#define	_1ETA	(1024.0*1024.0*1024.0*1024.0*1024.0)
#endif

#define	_GMKs(x)	( ((double)x > _1ETA) ? "eb" : \
			(((double)x > _1TER) ? "tb" : (((double)x > _1GIG) ? "gb" : \
			(((double)x > _1MEG) ? "mb" : (((double)x > _1KB)? "kb" : "bytes")))))

#define	_GMKv(x)	(((double)x > _1ETA) ? \
			((double)x/_1ETA) : (((double)x > _1TER) ? \
			((double)x/_1TER) : (((double)x > _1GIG) ? \
			((double)x/_1GIG) : (((double)x > _1MEG) ? \
			((double)x/_1MEG) : (((double)x > _1KB) ? \
			((double)x/_1KB): (double)x)))) )

void	*n_malloc 	(size_t, const char *, const char *, const int);
void	*n_realloc	(void *, size_t, const char *, const char *, const int);

void	*n_free 	(void *, const char *, const char *, const int);

#define MODULENAME NULL

#define new_malloc(x) n_malloc(x, MODULENAME, __FILE__, __LINE__)
#define new_free(x) (*(x) = n_free(*(x), MODULENAME, __FILE__, __LINE__))

#define RESIZE(x, y, z) ((x) = n_realloc((x), sizeof(y) * (z), MODULENAME, __FILE__, __LINE__))
#define malloc_strcpy(x, y) n_malloc_strcpy((char **)x, (char *)y, MODULENAME, __FILE__, __LINE__)
#define malloc_strcat(x, y) n_malloc_strcat((char **)x, (char *)y, MODULENAME, __FILE__, __LINE__)
#define m_strdup(x) n_m_strdup(x, MODULENAME, __FILE__, __LINE__)
#define m_strcat_ues(x, y, z) n_m_strcat_ues(x, y, z, MODULENAME, __FILE__, __LINE__)
#define m_strndup(x, y) n_m_strndup(x, y, MODULENAME, __FILE__, __LINE__)

char	*encode			(const char *, int);
char	*decode			(const char *);
char	*BX_cryptit		(const char *);
int	checkpass		(const char *, const char *);


/* Used for the inbound mangling stuff */

#define MANGLE_ESCAPES		1 << 0
#define MANGLE_ANSI_CODES	1 << 1
#define STRIP_COLOR		1 << 2
#define STRIP_REVERSE		1 << 3
#define STRIP_UNDERLINE		1 << 4
#define STRIP_BOLD		1 << 5
#define STRIP_BLINK		1 << 6
#define STRIP_ROM_CHAR          1 << 7
#define STRIP_ND_SPACE          1 << 8
#define STRIP_ALL_OFF		1 << 9
#define STRIP_ALT_CHAR		1 << 10
#define PRE_MANGLE		1 << 11

extern	int     outbound_line_mangler;
extern	int     inbound_line_mangler;
extern	int	logfile_line_mangler;
extern	int	operlog_line_mangler;

size_t	BX_mangle_line	(char *, int, size_t);
int	BX_charcount		(const char *, char);
char	*BX_stripdev		(char *);
char	*convert_dos		(char *);
char	*convert_unix		(char *);
int	is_dos			(char *);
void	strip_chars		(char *, char *, char);
char	*longcomma		(long);
char	*ulongcomma		(unsigned long);

#define SAFE(x) (((x) && *(x)) ? (x) : empty_string)

/* Used in compat.c */
#ifndef HAVE_TPARM
	char 	*tparm (const char *, ...);
#endif

	int	my_base64_encode (const void *, int, char **);

#ifndef HAVE_STRTOUL
	unsigned long 	strtoul (const char *, char **, int);
#endif

	char *	bsd_getenv (const char *);
	int	bsd_putenv (const char *);
	int	bsd_setenv (const char *, const char *, int);
	void	bsd_unsetenv (const char *);

#ifndef HAVE_INET_ATON
	int	inet_aton (const char *, struct in_addr *);
#endif

#ifndef HAVE_STRLCPY
	size_t	strlcpy (char *, const char *, size_t);
#endif

#ifndef HAVE_STRLCAT
	size_t	strlcat (char *, const char *, size_t);
#endif

#ifndef HAVE_VSNPRINTF
	int	vsnprintf (char *, size_t, const char *, va_list);
#endif

#ifndef HAVE_SNPRINTF
	int	snprintf (char *, size_t, const char *, ...);
#endif

#ifndef HAVE_SETSID
	int	setsid (void);
#endif

#endif /* _IRCAUX_H_ */
