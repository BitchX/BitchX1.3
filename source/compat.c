/*
 * Everything that im not directly responsible for i put in here.  Almost
 * all of this stuff is either borrowed from somewhere else (for those poor
 * saps that dont have something you need), or i wrote (and put into the 
 * public domain) in order to make epic compile on some of the more painful
 * systems.  None of this is part of EPIC-proper, so dont feel that youre 
 * going to hurt my feelings if you re-use this.
 */

#include "defs.h"
#include "ircaux.h"
#include "irc_std.h"
#define MAIN_SOURCE
#include "modval.h"


/* --- start of tparm.c --- */
#ifndef HAVE_TPARM
/*
 * tparm.c
 *
 * By Ross Ridge
 * Public Domain
 * 92/02/01 07:30:36
 *
 */
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifndef MAX_PUSHED
#define MAX_PUSHED	32
#endif

#define ARG	1
#define NUM	2

#define INTEGER	1
#define STRING	2

#define MAX_LINE 640

typedef void* anyptr;

typedef struct stack_str {
	int	type;
	int	argnum;
	int	value;
} stack;

static stack S[MAX_PUSHED];
static stack vars['z'-'a'+1];
static int pos = 0;

static struct arg_str {
	int type;
	int	integer;
	char	*string;
} arg_list[10];

static int argcnt;

static va_list tparm_args;

static int pusharg(int arg)
{
	if (pos == MAX_PUSHED)
		return 1;
	S[pos].type = ARG;
	S[pos++].argnum = arg;
	return 0;
}

static int pushnum(int num)
{
	if (pos == MAX_PUSHED)
		return 1;
	S[pos].type = NUM;
	S[pos++].value = num;
	return 0;
}

/* VARARGS2 */
static int getarg(int argnum, int type, anyptr p)
{
	while (argcnt < argnum) {
		arg_list[argcnt].type = INTEGER;
		arg_list[argcnt++].integer = (int) va_arg(tparm_args, int);
	}
	if (argcnt > argnum) {
		if (arg_list[argnum].type != type)
			return 1;
		else if (type == STRING)
			*(char **)p = arg_list[argnum].string;
		else
			*(int *)p = arg_list[argnum].integer;
	} else {
		arg_list[argcnt].type = type;
		if (type == STRING)
			*(char **)p = arg_list[argcnt++].string
				= (char *) va_arg(tparm_args, char *);
		else
			*(int *)p = arg_list[argcnt++].integer = (int) va_arg(tparm_args, int);
	}
	return 0;
}


static int popstring(char **str)
{
	if (pos-- == 0)
		return 1;
	if (S[pos].type != ARG)
		return 1;
	return(getarg(S[pos].argnum, STRING, (anyptr) str));
}

static int popnum(int *num)
{
	if (pos-- == 0)
		return 1;
	switch (S[pos].type) {
	case ARG:
		return (getarg(S[pos].argnum, INTEGER, (anyptr) num));
	case NUM:
		*num = S[pos].value;
		return 0;
	}
	return 1;
}

static int cvtchar(const char *sp, char *c)
{
	switch(*sp) {
	case '\\':
		switch(*++sp) {
		case '\'':
		case '$':
		case '\\':
		case '%':
			*c = *sp;
			return 2;
		case '\0':
			*c = '\\';
			return 1;
		case '0':
			if (sp[1] == '0' && sp[2] == '0') {
				*c = '\0';
				return 4;
			}
			*c = '\200'; /* '\0' ???? */
			return 2;
		default:
			*c = *sp;
			return 2;
		}
	default:
		*c = *sp;
		return 1;
	}
}

static int termcap;

/* sigh... this has got to be the ugliest code I've ever written.
   Trying to handle everything has its cost, I guess.

   It actually isn't to hard to figure out if a given % code is supposed
   to be interpeted with its termcap or terminfo meaning since almost
   all terminfo codes are invalid unless something has been pushed on
   the stack and termcap strings will never push things on the stack
   (%p isn't used by termcap). So where we have a choice we make the
   decision by wether or not somthing has been pushed on the stack.
   The static variable termcap keeps track of this; it starts out set
   to 1 and is incremented as each argument processed by a termcap % code,
   however if something is pushed on the stack it's set to 0 and the
   rest of the % codes are interpeted as terminfo % codes. Another way
   of putting it is that if termcap equals one we haven't decided either
   way yet, if it equals zero we're looking for terminfo codes, and if
   its greater than 1 we're looking for termcap codes.

   Terminfo % codes:

	%%	output a '%'
	%[[:][-+# ][width][.precision]][doxXs]
		output pop according to the printf format
	%c	output pop as a char
	%'c'	push character constant c.
	%{n}	push decimal constant n.
	%p[1-9] push paramter [1-9]
	%g[a-z] push variable [a-z]
	%P[a-z] put pop in variable [a-z]
	%l	push the length of pop (a string)
	%+	add pop to pop and push the result
	%-	subtract pop from pop and push the result
	%*	multiply pop and pop and push the result
	%&	bitwise and pop and pop and push the result
	%|	bitwise or pop and pop and push the result
	%^	bitwise xor pop and pop and push the result
	%~	push the bitwise not of pop
	%=	compare if pop and pop are equal and push the result
	%>	compare if pop is less than pop and push the result
	%<	compare if pop is greater than pop and push the result
	%A	logical and pop and pop and push the result
	%O	logical or pop and pop and push the result
	%!	push the logical not of pop
	%? condition %t if_true [%e if_false] %;
		if condtion evaulates as true then evaluate if_true,
		else evaluate if_false. elseif's can be done:
%? cond %t true [%e cond2 %t true2] ... [%e condN %t trueN] [%e false] %;
	%i	add one to parameters 1 and 2. (ANSI)

  Termcap Codes:

	%%	output a %
	%.	output parameter as a character
	%d	kutput parameter as a decimal number
	%2	output parameter in printf format %02d
	%3	output parameter in printf format %03d
	%+x	add the character x to parameter and output it as a character
(UW)	%-x	subtract parameter FROM the character x and output it as a char
(UW)	%ax	add the character x to parameter
(GNU)	%a[+*-/=][cp]x
		GNU arithmetic.
(UW)	%sx	subtract parameter FROM the character x
	%>xy	if parameter > character x then add character y to parameter
	%B	convert to BCD (parameter = (parameter/10)*16 + parameter%16)
	%D	Delta Data encode (parameter = parameter - 2*(paramter%16))
	%i	increment the first two parameters by one
	%n	xor the first two parameters by 0140
(GNU)	%m	xor the first two parameters by 0177
	%r	swap the first two parameters
(GNU)	%b	backup to previous parameter
(GNU)	%f	skip this parameter

  Note the two definitions of %a, the GNU defintion is used if the characters
  after the 'a' are valid, otherwise the UW definition is used.

  (GNU) used by GNU Emacs termcap libraries
  (UW) used by the University of Waterloo (MFCF) termcap libraries

*/

char *tparm(const char *str, ...) {
	static char OOPS[] = "OOPS";
	static char buf[MAX_LINE];
	register const char *sp;
	register char *dp;
	register char *fmt;
	char conv_char;
	char scan_for;
	int scan_depth = 0, if_depth;
	static int i, j;
	static char *s, c;
	char fmt_buf[MAX_LINE];
	char sbuf[MAX_LINE];
	if (!str)
		return NULL;
	va_start(tparm_args, str);

	sp = str;
	dp = buf;
	scan_for = 0;
	if_depth = 0;
	argcnt = 0;
	pos = 0;
	termcap = 1;
	while(*sp != '\0') {
		switch(*sp) {
		case '\\':
			if (scan_for) {
				if (*++sp != '\0')
					sp++;
				break;
			}
			*dp++ = *sp++;
			if (*sp != '\0')
				*dp++ = *sp++;
			break;
		case '%':
			sp++;
			if (scan_for) {
				if (*sp == scan_for && if_depth == scan_depth) {
					if (scan_for == ';')
						if_depth--;
					scan_for = 0;
				} else if (*sp == '?')
					if_depth++;
				else if (*sp == ';') {
					if (if_depth == 0)
						return OOPS;
					else
						if_depth--;
				}
				sp++;
				break;
			}
			fmt = NULL;
			switch(*sp) {
			case '%':
				*dp++ = *sp++;
				break;
			case '+':
				if (!termcap) {
					if (popnum(&j) || popnum(&i))
						return OOPS;
					i += j;
					if (pushnum(i))
						return OOPS;
					sp++;
					break;
				}
				;/* FALLTHROUGH */
			case 'C':
				if (*sp == 'C') {
					if (getarg(termcap - 1, INTEGER, &i))
						return OOPS;
					if (i >= 96) {
						i /= 96;
						if (i == '$')
							*dp++ = '\\';
						*dp++ = i;
					}
				}
				fmt = "%c";
				/* FALLTHROUGH */
			case 'a':
				if (!termcap)
					return OOPS;
				if (getarg(termcap - 1, INTEGER, (anyptr) &i))
					return OOPS;
				if (*++sp == '\0')
					return OOPS;
				if ((sp[1] == 'p' || sp[1] == 'c')
			            && sp[2] != '\0' && fmt == NULL) {
					/* GNU aritmitic parameter, what they
					   realy need is terminfo.	      */
					int val, lc;
					if (sp[1] == 'p'
					    && getarg(termcap - 1 + sp[2] - '@',
						      INTEGER, (anyptr) &val))
						return OOPS;
					if (sp[1] == 'c') {
						lc = cvtchar(sp + 2, &c) + 2;
					/* Mask out 8th bit so \200 can be
					   used for \0 as per GNU doc's    */
						val = c & 0177;
					} else
						lc = 2;
					switch(sp[0]) {
					case '=':
						break;
					case '+':
						val = i + val;
						break;
					case '-':
						val = i - val;
						break;
					case '*':
						val = i * val;
						break;
					case '/':
						val = i / val;
						break;
					default:
					/* Not really GNU's %a after all... */
						lc = cvtchar(sp, &c);
						val = c + i;
						break;
					}
					arg_list[termcap - 1].integer = val;
					sp += lc;
					break;
				}
				sp += cvtchar(sp, &c);
				arg_list[termcap - 1].integer = c + i;
				if (fmt == NULL)
					break;
				sp--;
				/* FALLTHROUGH */
			case '-':
				if (!termcap) {
					if (popnum(&j) || popnum(&i))
						return OOPS;
					i -= j;
					if (pushnum(i))
						return OOPS;
					sp++;
					break;
				}
				fmt = "%c";
				/* FALLTHROUGH */
			case 's':
				if (termcap && (fmt == NULL || *sp == '-')) {
					if (getarg(termcap - 1, INTEGER, &i))
						return OOPS;
					if (*++sp == '\0')
						return OOPS;
					sp += cvtchar(sp, &c);
					arg_list[termcap - 1].integer = c - i;
					if (fmt == NULL)
						break;
					sp--;
				}
				if (!termcap)
					return OOPS;
				;/* FALLTHROUGH */
			case '.':
				if (termcap && fmt == NULL)
					fmt = "%c";
				;/* FALLTHROUGH */
			case 'd':
				if (termcap && fmt == NULL)
					fmt = "%d";
				;/* FALLTHROUGH */
			case '2':
				if (termcap && fmt == NULL)
					fmt = "%02d";
				;/* FALLTHROUGH */
			case '3':
				if (termcap && fmt == NULL)
					fmt = "%03d";
				;/* FALLTHROUGH */
			case ':': case ' ': case '#': case 'u':
			case 'x': case 'X': case 'o': case 'c':
			case '0': case '1': case '4': case '5':
			case '6': case '7': case '8': case '9':
				if (fmt == NULL) {
					if (termcap)
						return OOPS;
					if (*sp == ':')
						sp++;
					fmt = fmt_buf;
					*fmt++ = '%';
					while(*sp != 's' && *sp != 'x' && *sp != 'X' && *sp != 'd' && *sp != 'o' && *sp != 'c' && *sp != 'u') {
						if (*sp == '\0')
							return OOPS;
						*fmt++ = *sp++;
					}
					*fmt++ = *sp;
					*fmt = '\0';
					fmt = fmt_buf;
				}
				conv_char = fmt[strlen(fmt) - 1];
				if (conv_char == 's') {
					if (popstring(&s))
						return OOPS;
					sprintf(sbuf, fmt, s);
				} else {
					if (termcap) {
						if (getarg(termcap++ - 1,
							   INTEGER, &i))
							return OOPS;
					} else
						if (popnum(&i))
							return OOPS;
					if (i == 0 && conv_char == 'c')
						*sbuf = 0;
					else
						sprintf(sbuf, fmt, i);
				}
				sp++;
				fmt = sbuf;
				while(*fmt != '\0') {
					if (*fmt == '$')
						*dp++ = '\\';
					*dp++ = *fmt++;
				}
				break;
			case 'r':
				if (!termcap || getarg(1, INTEGER, &i))
					return OOPS;
				arg_list[1].integer = arg_list[0].integer;
				arg_list[0].integer = i;
				sp++;
				break;
			case 'i':
				if (getarg(1, INTEGER, &i)
				    || arg_list[0].type != INTEGER)
					return OOPS;
				arg_list[1].integer++;
				arg_list[0].integer++;
				sp++;
				break;
			case 'n':
				if (!termcap || getarg(1, INTEGER, &i))
					return OOPS;
				arg_list[0].integer ^= 0140;
				arg_list[1].integer ^= 0140;
				sp++;
				break;
			case '>':
				if (!termcap) {
					if (popnum(&j) || popnum(&i))
						return OOPS;
					i = (i > j);
					if (pushnum(i))
						return OOPS;
					sp++;
					break;
				}
				if (getarg(termcap-1, INTEGER, &i))
					return OOPS;
				sp += cvtchar(sp, &c);
				if (i > c) {
					sp += cvtchar(sp, &c);
					arg_list[termcap-1].integer += c;
				} else
					sp += cvtchar(sp, &c);
				sp++;
				break;
			case 'B':
				if (!termcap || getarg(termcap-1, INTEGER, &i))
					return OOPS;
				arg_list[termcap-1].integer = 16*(i/10)+i%10;
				sp++;
				break;
			case 'D':
				if (!termcap || getarg(termcap-1, INTEGER, &i))
					return OOPS;
				arg_list[termcap-1].integer = i - 2 * (i % 16);
				sp++;
				break;
			case 'p':
				if (termcap > 1)
					return OOPS;
				if (*++sp == '\0')
					return OOPS;
				if (*sp == '0')
					i = 9;
				else
					i = *sp - '1';
				if (i < 0 || i > 9)
					return OOPS;
				if (pusharg(i))
					return OOPS;
				termcap = 0;
				sp++;
				break;
			case 'P':
				if (termcap || *++sp == '\0')
					return OOPS;
				i = *sp++ - 'a';
				if (i < 0 || i > 25)
					return OOPS;
				if (pos-- == 0)
					return OOPS;
				switch(vars[i].type = S[pos].type) {
				case ARG:
					vars[i].argnum = S[pos].argnum;
					break;
				case NUM:
					vars[i].value = S[pos].value;
					break;
				}
				break;
			case 'g':
				if (termcap || *++sp == '\0')
					return OOPS;
				i = *sp++ - 'a';
				if (i < 0 || i > 25)
					return OOPS;
				switch(vars[i].type) {
				case ARG:
					if (pusharg(vars[i].argnum))
						return OOPS;
					break;
				case NUM:
					if (pushnum(vars[i].value))
						return OOPS;
					break;
				}
				break;
			case '\'':
				if (termcap > 1)
					return OOPS;
				if (*++sp == '\0')
					return OOPS;
				sp += cvtchar(sp, &c);
				if (pushnum(c) || *sp++ != '\'')
					return OOPS;
				termcap = 0;
				break;
			case '{':
				if (termcap > 1)
					return OOPS;
				i = 0;
				sp++;
				while(isdigit((unsigned char)*sp))
					i = 10 * i + *sp++ - '0';
				if (*sp++ != '}' || pushnum(i))
					return OOPS;
				termcap = 0;
				break;
			case 'l':
				if (termcap || popstring(&s))
					return OOPS;
				i = strlen(s);
				if (pushnum(i))
					return OOPS;
				sp++;
				break;
			case '*':
				if (termcap || popnum(&j) || popnum(&i))
					return OOPS;
				i *= j;
				if (pushnum(i))
					return OOPS;
				sp++;
				break;
			case '/':
				if (termcap || popnum(&j) || popnum(&i))
					return OOPS;
				i /= j;
				if (pushnum(i))
					return OOPS;
				sp++;
				break;
			case 'm':
				if (termcap) {
					if (getarg(1, INTEGER, &i))
						return OOPS;
					arg_list[0].integer ^= 0177;
					arg_list[1].integer ^= 0177;
					sp++;
					break;
				}
				if (popnum(&j) || popnum(&i))
					return OOPS;
				i %= j;
				if (pushnum(i))
					return OOPS;
				sp++;
				break;
			case '&':
				if (popnum(&j) || popnum(&i))
					return OOPS;
				i &= j;
				if (pushnum(i))
					return OOPS;
				sp++;
				break;
			case '|':
				if (popnum(&j) || popnum(&i))
					return OOPS;
				i |= j;
				if (pushnum(i))
					return OOPS;
				sp++;
				break;
			case '^':
				if (popnum(&j) || popnum(&i))
					return OOPS;
				i ^= j;
				if (pushnum(i))
					return OOPS;
				sp++;
				break;
			case '=':
				if (popnum(&j) || popnum(&i))
					return OOPS;
				i = (i == j);
				if (pushnum(i))
					return OOPS;
				sp++;
				break;
			case '<':
				if (popnum(&j) || popnum(&i))
					return OOPS;
				i = (i < j);
				if (pushnum(i))
					return OOPS;
				sp++;
				break;
			case 'A':
				if (popnum(&j) || popnum(&i))
					return OOPS;
				i = (i && j);
				if (pushnum(i))
					return OOPS;
				sp++;
				break;
			case 'O':
				if (popnum(&j) || popnum(&i))
					return OOPS;
				i = (i || j);
				if (pushnum(i))
					return OOPS;
				sp++;
				break;
			case '!':
				if (popnum(&i))
					return OOPS;
				i = !i;
				if (pushnum(i))
					return OOPS;
				sp++;
				break;
			case '~':
				if (popnum(&i))
					return OOPS;
				i = ~i;
				if (pushnum(i))
					return OOPS;
				sp++;
				break;
			case '?':
				if (termcap > 1)
					return OOPS;
				termcap = 0;
				if_depth++;
				sp++;
				break;
			case 't':
				if (popnum(&i) || if_depth == 0)
					return OOPS;
				if (!i) {
					scan_for = 'e';
					scan_depth = if_depth;
				}
				sp++;
				break;
			case 'e':
				if (if_depth == 0)
					return OOPS;
				scan_for = ';';
				scan_depth = if_depth;
				sp++;
				break;
			case ';':
				if (if_depth-- == 0)
					return OOPS;
				sp++;
				break;
			case 'b':
				if (--termcap < 1)
					return OOPS;
				sp++;
				break;
			case 'f':
				if (!termcap++)
					return OOPS;
				sp++;
				break;
			}
			break;
		default:
			if (scan_for)
				sp++;
			else
				*dp++ = *sp++;
			break;
		}
	}
	va_end(tparm_args);
	*dp = '\0';
	return buf;
}
#endif
/* --- end of tparm.c --- */

/* --- start of strtoul.c --- */
#ifndef HAVE_STRTOUL
/*
 * Copyright (c) 1990 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE. 
 */

/*
 * Convert a string to an unsigned long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
unsigned long strtoul (const char *nptr, char **endptr, int base)
{
	const char *s;
	unsigned long acc, cutoff;
	int c;
	int neg, any, cutlim;

	s = nptr;
	do
		c = *s++;
	while (isspace(c));

	if (c == '-') 
	{
		neg = 1;
		c = *s++;
	} 
	else 
	{
		neg = 0;
		if (c == '+')
			c = *s++;
	}

	if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X')) 
	{
		c = s[1];
		s += 2;
		base = 16;
	}

	if (base == 0)
		base = c == '0' ? 8 : 10;

#ifndef ULONG_MAX
#define ULONG_MAX (unsigned long) -1
#endif

	cutoff = ULONG_MAX / (unsigned long)base;
	cutlim = ULONG_MAX % (unsigned long)base;

	for (acc = 0, any = 0;; c = *s++) 
	{
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;

		if (c >= base)
			break;

		if (any < 0)
			continue;

		if (acc > cutoff || acc == cutoff && c > cutlim) 
		{
			any = -1;
			acc = ULONG_MAX;
			errno = ERANGE;
		}
		else 
		{
			any = 1;
			acc *= (unsigned long)base;
			acc += c;
		}
	}
	if (neg && any > 0)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char *) (any ? s - 1 : nptr);
	return (acc);
}
#endif /* DO NOT HAVE STRTOUL */
/* --- end of strtoul.c --- */

/* --- start of scandir.c --- */
#ifndef HAVE_SCANDIR
/*
 * Scandir.c -- A painful file for painful operating systems
 *
 * Technically, scandir is not a "standard" function.  It belongs to
 * 4.2BSD derived systems, and most everone that has any repsect for their
 * customers implements it sanely.  Which probably explains why its broken
 * on Solaris 2.
 *
 * I removed the 4BSD scandir function because it required intimite knowledge
 * of what was inside the DIR type, which sort of defeats the point.  What I
 * left was this extremely generic scandir function that only depends on
 * opendir(), readdir(), and closedir(), and perhaps the DIRSIZ macro.
 * The only member of struct dirent we peer into is d_name.
 *
 * Public domain
 */


#define RESIZEDIR(x, y, z) x = realloc((void *)(x), sizeof(y) * (z))

/* Initial guess at directory size. */
#define INITIAL_SIZE	30

typedef struct dirent DIRENT;

/*
 * If the system doesnt have a way to tell us how big a directory entry
 * is, then we make a wild guess.  This shouldnt ever be SMALLER than 
 * the actual size, and if its larger, so what?  This will probably not
 * be a size thats divisible by 4, so the memcpy() may not be super
 * efficient.  But so what?  Any system that cant provide a decent scandir
 * im not worried about efficiency.
 */
/* The SCO hack is at the advice of FireClown, thanks! =) */
#if defined(_SCO_DS)
# undef DIRSIZ
#endif

#ifndef DIRSIZ
#  define DIRSIZ(d) (sizeof(DIRENT) + strlen(d->d_name) + 1)
#endif


/*
 * Scan the directory dirname calling select to make a list of selected
 * directory entries then sort using qsort and compare routine dcomp. Returns
 * the number of entries and a pointer to a list of pointers to struct direct
 * (through namelist). Returns -1 if there were any errors. 
 */
int	scandir (const char *name, 
		 DIRENT ***list,
		 int 	(*selector) (DIRENT *), 
		 int 	(*sorter) (const void *, const void *))
{
		DIRENT	**names;
	static	DIRENT	*e;
    		DIR	*dp;
    		int	i;
    		int	size = INITIAL_SIZE;

	if (!(names = (DIRENT **)malloc(size * sizeof(DIRENT *))))
		return -1;

	if (access(name, R_OK | X_OK))
		return -1;

	if (!(dp = opendir(name)))
		return -1;

	/* Read entries in the directory. */
	for (i = 0; (e = readdir(dp));)
	{
		if (!selector || (*selector)(e))
		{
			if (i + 1 >= size)
			{
				size <<= 1;
				RESIZEDIR(names, DIRENT *, size);
				if (!names)
				{
					closedir(dp);
					return (-1);
				}
			}
			names[i] = (DIRENT *)malloc(DIRSIZ(e));
			if (names[i] == NULL)
			{
				int j;
				for (j = 0; j < i; j++)
					free(names[j]);
				free(names);
				closedir(dp);
				return -1;
			}
			memcpy(names[i], e, DIRSIZ(e));
			i++;
		}
	}

	/*
	 * Truncate the "names" array down to its actual size (why?)
	 */
	RESIZEDIR(names, DIRENT *, i + 2);
	names[i + 1] = 0;
	*list = names;
	closedir(dp);

	/*
	 * Sort if neccesary...
	 */
	if (i && sorter)
		qsort(names, i, sizeof(DIRENT *), sorter);

	return i;
}
#endif
/* --- end of scandir.c --- */

/* --- start of env.c --- */
#if 1
/*
 * Copyright (c) 1987, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 * See above for the neccesary list of conditions on use.
 */

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

int   bsd_setenv(const char *name, const char *value, int rewrite);

/*
 * __findenv --
 *	Returns pointer to value associated with name, if any, else NULL.
 *	Sets offset to be the offset of the name/value combination in the
 *	environmental array, for use by setenv(3) and unsetenv(3).
 *	Explicitly removes '=' in argument name.
 *
 *	This routine *should* be a static; don't use it.
 */
__inline__ static char *__findenv(const char *name, int *offset)
{
	extern char **environ;
	register int len, i;
	register const char *np;
	register char **p, *cp;

	if (name == NULL || environ == NULL)
		return (NULL);
	for (np = name; *np && *np != '='; ++np)
		continue;
	len = np - name;
	for (p = environ; (cp = *p) != NULL; ++p) {
		for (np = name, i = len; i && *cp; i--)
			if (*cp++ != *np++)
				break;
		if (i == 0 && *cp++ == '=') {
			*offset = p - environ;
			return (cp);
		}
	}
	return (NULL);
}

/*
 * getenv --
 *	Returns ptr to value associated with name, if any, else NULL.
 */
char *bsd_getenv(const char *name)
{
	int offset;

	return (__findenv(name, &offset));
}

/*
 * setenv --
 *	Set the value of the environmental variable "name" to be
 *	"value".  If rewrite is set, replace any current value.
 */
int bsd_setenv(const char *name, const char *value, int rewrite)
{
	extern char **environ;
	static int alloced;			/* if allocated space before */
	register char *c;
	int l_value, offset;

	if (*value == '=')			/* no `=' in value */
		++value;
	l_value = strlen(value);
	if ((c = __findenv(name, &offset))) {	/* find if already exists */
		if (!rewrite)
			return (0);
		if (strlen(c) >= l_value) {	/* old larger; copy over */
			while ( (*c++ = *value++) );
			return (0);
		}
	} else {					/* create new slot */
		register int cnt;
		register char **p;

		for (p = environ, cnt = 0; *p; ++p, ++cnt);
		if (alloced) {			/* just increase size */
			environ = (char **)realloc((char *)environ,
			    (size_t)(sizeof(char *) * (cnt + 2)));
			if (!environ)
				return (-1);
		}
		else {				/* get new space */
			alloced = 1;		/* copy old entries into it */
			p = malloc((size_t)(sizeof(char *) * (cnt + 2)));
			if (!p)
				return (-1);
			memcpy(p, environ, cnt * sizeof(char *));
			environ = p;
		}
		environ[cnt + 1] = NULL;
		offset = cnt;
	}
	for (c = (char *)name; *c && *c != '='; ++c);	/* no `=' in name */
	if (!(environ[offset] =			/* name + `=' + value */
	    malloc((size_t)((int)(c - name) + l_value + 2))))
		return (-1);
	for (c = environ[offset]; (*c = *name++) && *c != '='; ++c);
	for (*c++ = '='; (*c++ = *value++); );
	return (0);
}

int bsd_putenv(const char *str)
{
	char *p, *equal;
	int rval;

	if ((p = strdup(str)) == NULL)
		return (-1);
	if ((equal = strchr(p, '=')) == NULL) {
		free(p);
		return (-1);
	}
	*equal = '\0';
	rval = bsd_setenv(p, equal + 1, 1);
	free(p);
	return (rval);
}

/*
 * unsetenv(name) --
 *	Delete environmental variable "name".
 */
void bsd_unsetenv(const char *name)
{
	extern char **environ;
	register char **p;
	int offset;

	while (__findenv(name, &offset))	/* if set multiple times */
		for (p = &environ[offset];; ++p)
			if (!(*p = *(p + 1)))
				break;
}
#endif
/* --- end of env.c --- */

/* --- start of inet_aton.c --- */
#if !defined(HAVE_INET_ATON)
/*
 * Copyright (c) 1983, 1990, 1993
 *    The Regents of the University of California.  All rights reserved.
 * Portions Copyright (c) 1993 by Digital Equipment Corporation.
 * See above for the neccesary list of conditions on use.
 */

/* 
 * Check whether "cp" is a valid ascii representation
 * of an Internet address and convert to a binary address.
 * Returns 1 if the address is valid, 0 if not.
 * This replaces inet_addr, the return value from which
 * cannot distinguish between failure and a local broadcast address.
 */
int inet_aton(const char *cp, struct in_addr *addr)
{
	unsigned long	val;
	int		base, n;
	char		c;
	unsigned	parts[4];
	unsigned	*pp = parts;

	c = *cp;
	for (;;) {
		/*
		 * Collect number up to ``.''.
		 * Values are specified as for C:
		 * 0x=hex, 0=octal, isdigit=decimal.
		 */
		if (!isdigit((unsigned char)c))
			return (0);
		val = 0; base = 10;
		if (c == '0') {
			c = *++cp;
			if (c == 'x' || c == 'X')
				base = 16, c = *++cp;
			else
				base = 8;
		}
		for (;;) {
			if (isascii((unsigned char)c) && isdigit((unsigned char)c)) {
				val = (val * base) + (c - '0');
				c = *++cp;
			} else if (base == 16 && isascii((unsigned char)c) && isxdigit((unsigned char)c)) {
				val = (val << 4) |
					(c + 10 - (islower((unsigned char)c) ? 'a' : 'A'));
				c = *++cp;
			} else
				break;
		}
		if (c == '.') {
			/*
			 * Internet format:
			 *	a.b.c.d
			 *	a.b.c	(with c treated as 16 bits)
			 *	a.b	(with b treated as 24 bits)
			 */
			if (pp >= parts + 3)
				return (0);
			*pp++ = val;
			c = *++cp;
		} else
			break;
	}
	/*
	 * Check for trailing characters.
	 */
	if (c != '\0' && (!isascii((unsigned char)c) || !isspace((unsigned char)c)))
		return (0);
	/*
	 * Concoct the address according to
	 * the number of parts specified.
	 */
	n = pp - parts + 1;
	switch (n) {

	case 0:
		return (0);		/* initial nondigit */

	case 1:				/* a -- 32 bits */
		break;

	case 2:				/* a.b -- 8.24 bits */
		if (val > 0xffffff)
			return (0);
		val |= parts[0] << 24;
		break;

	case 3:				/* a.b.c -- 8.8.16 bits */
		if (val > 0xffff)
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16);
		break;

	case 4:				/* a.b.c.d -- 8.8.8.8 bits */
		if (val > 0xff)
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
		break;
	}
	if (addr)
		addr->s_addr = htonl(val);
	return (1);
}
#endif
/* --- end of inet_aton.c --- */

/* --- start of misc stuff --- */
#if 0
int vsnprintf (char *str, size_t size, const char *format, va_list ap)
{
	int ret = vsprintf(str, format, ap);

	/* If the string ended up overflowing, just give up. */
	if (ret == (int)str && strlen(str) > size)
		ircpanic("Buffer overflow in vsnprintf");
	if (ret != (int)str && ret > size)
		ircpanic("Buffer overflow in vsnprintf");

	return ret;
}
#endif

#if 0
int snprintf (char *str, size_t size, const char *format, ...)
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = vsnprintf(str, size, format, args);
	va_end(args);
	return ret;
}
#endif

/*

  Author: Tomi Salo <ttsalo@ssh.fi>

  Copyright (C) 1996 SSH Communications Security Oy, Espoo, Finland
  See COPYING for distribution conditions.

  Implementation of functions snprintf() and vsnprintf()

  */

/*
 * $Id: compat.c 3 2008-02-25 09:49:14Z keaston $
 * $Log: compat.c,v $
 * Revision 1.1.1.1  2003/04/11 01:09:07  dan
 * inital import into backup cvs server
 *
 *
 * Revision 1.3  2002/10/24 11:00:42  nuke
 * Added hebrew patches.  Fixed problems with isalpha() etc.  Cleaned up some
 * #ifdefs and other general code cleanups.
 *
 * Revision 1.2  2001/05/07 08:44:56  nuke
 * All exported functions are now called through the global table internally.
 *
 * Revision 1.1  2001/03/05 19:39:37  nuke
 * Added BitchX to the archive... slightly post 1.0c18
 *
 * Revision 1.3  2000/10/29 04:55:52  cvs
 * Attemped fixes for Close from window list hanging PMBX.  Fix for
 * input line being updated incorrectly.
 *
 * Revision 1.2  2000/06/09 10:08:39  nuke
 * Various fixes and code cleanup in exec.c  Fixed a crash in detach_shared
 * when a menu didn't exist. Added virtual host support on OS/2.
 *
 * Revision 1.1.1.1  2000/05/10 06:52:03  edwards
 * BitchX Source
 *
 *
 * 1999/05/28 00:59:04 rlm
 *      Adapted code to fit frequency, fixed ambiguous else
 *
 * Revision 1.3  1999/02/23 08:30:59  tri
 * 	Downgraded includes to ssh1 style.
 *
 * Revision 1.2  1999/02/23 07:06:17  tri
 *      Fixed compilation errors.
 *
 * Revision 1.1  1999/02/21 19:52:37  ylo
 *      Intermediate commit of ssh1.2.27 stuff.
 *      Main change is sprintf -> snprintf; however, there are also
 *      many other changes.
 *
 * Revision 1.23  1998/11/05 21:14:05  ylo
 *      Changed to ignore return value from sprintf (it returns char *
 *      on some systems, e.g. SunOS 4.1.x).
 *
 * Revision 1.22  1998/10/28 05:19:06  ylo
 *      Changed to compile in the kernel environment if KERNEL is defined.
 *
 * Revision 1.21  1998/10/06 11:32:34  sjl
 *      Fixed previous.
 *
 * Revision 1.20  1998/10/05 10:16:01  sjl
 *      Added (char) cast to format_str_ptr, as suggested by someone
 *      who compiled with IRIX (or Ultrix).
 *
 * Revision 1.19  1998/06/03 00:45:30  ylo
 *      Fixed initialization of prefix and flags.
 *
 * Revision 1.18  1998/03/28  13:18:57  ylo
 *      Changed snprintf to return the length of the resulting string
 *      (not len+1) when the string fits in the buffer.
 *
 * Revision 1.17  1998/01/28 10:14:31  ylo
 *      Major changes in util library.  Several files were renamed,
 *      and many very commonly used functions and types were renamed
 *      (mostly to add ssh prefix).  The event loop and timeout
 *      interface was changed, and split to portable and
 *      machine-dependent parts.  Thousands of calls to
 *      renamed/modified functions were changed accordingly.
 *
 * Revision 1.16  1997/12/28 02:22:25  ylo
 *      Removed unused variable tmp.
 *
 * Revision 1.15  1997/09/18 06:07:41  kivinen
 *      Added check that if we print NULL pointer print (null) just
 *      like normal printf will do.
 *
 * Revision 1.14  1997/06/04 21:54:51  ylo
 *      Added include snprintf.h.
 *      Removed obsolete xmalloc/xfree pair.
 *
 * Revision 1.13  1996/11/03 19:41:29  ttsalo
 *       Kludged & fixed snprintf
 *
 * Revision 1.12  1996/11/03 18:47:56  ttsalo
 *       Changed buffering in snprintf_convert_float
 *
 * Revision 1.11  1996/11/03 17:46:24  ttsalo
 *       Removed references to bsd's float conversion
 *
 * Revision 1.10  1996/11/03 17:44:18  ttsalo
 *       Re-implemented float conversion, uses system's sprintf
 *
 * Revision 1.9  1996/08/21 15:32:47  kivinen
 *      Removed xmalloc.h and fatal.h includes, as they are now in
 *      includes.h.
 *
 * Revision 1.8  1996/08/09 14:44:03  ttsalo
 *      Fixed a problem in non-null-terminated string formatting
 *
 * Revision 1.7  1996/08/09 13:41:59  ttsalo
 *        Don't pad left-justified numbers with zeros
 *
 * Revision 1.6  1996/08/09 12:11:10  ttsalo
 *      More fine-tuning of float-printing
 *
 * Revision 1.5  1996/08/09 11:37:20  ttsalo
 *      Ansi C - conforming snprintf and vsnprintf.
 *
 * Revision 1.4  1996/08/07 10:12:50  ttsalo
 *      Fixed the banner again
 *
 * Revision 1.3  1996/08/07 10:08:24  ttsalo
 *      Fixed the banner
 *
 * Revision 1.2  1996/08/07 09:52:07  ttsalo
 *      Removed diagnostic printf:s
 *
 * Revision 1.1  1996/08/06 19:00:40  ttsalo
 *      Implementation of snprintf and vsnprintf. Everything except
 *      floating point conversions ready.
 *
 * $EndLog$
 */

#ifndef HAVE_VSNPRINTF

#undef isdigit
#define isdigit(ch) ((ch) >= '0' && (ch) <= '9')

#define MINUS_FLAG 0x1
#define PLUS_FLAG 0x2
#define SPACE_FLAG 0x4
#define HASH_FLAG 0x8
#define CONV_TO_SHORT 0x10
#define IS_LONG_INT 0x20
#define IS_LONG_DOUBLE 0x40
#define X_UPCASE 0x80
#define IS_NEGATIVE 0x100
#define UNSIGNED_DEC 0x200
#define ZERO_PADDING 0x400

#undef sprintf

/* Extract a formatting directive from str. Str must point to a '%'. 
   Returns number of characters used or zero if extraction failed. */

static int
snprintf_get_directive(const char *str, int *flags, int *width,
                       int *precision, char *format_char, va_list *ap)
{
  int length, value;
  const char *orig_str = str;

  *flags = 0; 
  *width = 0;
  *precision = 0;
  *format_char = (char)0;

  if (*str == '%')
    {
      /* Get the flags */
      str++;
      while (*str == '-' || *str == '+' || *str == ' ' 
             || *str == '#' || *str == '0')
        {
          switch (*str)
            {
            case '-':
              *flags |= MINUS_FLAG;
              break;
            case '+':
              *flags |= PLUS_FLAG;
              break;
            case ' ':
              *flags |= SPACE_FLAG;
              break;
            case '#':
              *flags |= HASH_FLAG;
              break;
            case '0':
              *flags |= ZERO_PADDING;
              break;
            }
          str++;
        }

      /* Don't pad left-justified numbers withs zeros */
      if ((*flags & MINUS_FLAG) && (*flags & ZERO_PADDING))
        *flags &= ~ZERO_PADDING;
        
      /* Is width field present? */
      if (isdigit(*str))
        {
          for (value = 0; *str && isdigit(*str); str++)
            value = 10 * value + *str - '0';
          *width = value;
        }
      else
        if (*str == '*')
          {
            *width = va_arg(*ap, int);
            str++;
          }

      /* Is the precision field present? */
      if (*str == '.')
        {
          str++;
          if (isdigit(*str))
            {
              for (value = 0; *str && isdigit(*str); str++)
                value = 10 * value + *str - '0';
              *precision = value;
            }
          else
            if (*str == '*')
              {
                *precision = va_arg(*ap, int);
                str++;
              }
            else
              *precision = 0;
        }

      /* Get the optional type character */
      if (*str == 'h')
        {
          *flags |= CONV_TO_SHORT;
          str++;
        }
      else
        {
          if (*str == 'l')
            {
              *flags |= IS_LONG_INT;
              str++;
            }
          else
            {
              if (*str == 'L')
                {
                  *flags |= IS_LONG_DOUBLE;
                  str++;
                }
            }
        }

      /* Get and check the formatting character */

      *format_char = *str;
      str++;
      length = str - orig_str;

      switch (*format_char)
        {
        case 'i': case 'd': case 'o': case 'u': case 'x': case 'X': 
        case 'f': case 'e': case 'E': case 'g': case 'G': 
        case 'c': case 's': case 'p': case 'n':
          if (*format_char == 'X')
            *flags |= X_UPCASE;
          if (*format_char == 'o')
            *flags |= UNSIGNED_DEC;
          return length;

        default:
          return 0;
        }
    }
  else
    {
      return 0;
    }
}

/* Convert a integer from unsigned long int representation 
   to string representation. This will insert prefixes if needed 
   (leading zero for octal and 0x or 0X for hexadecimal) and
   will write at most buf_size characters to buffer.
   tmp_buf is used because we want to get correctly truncated
   results.
   */

static int
snprintf_convert_ulong(char *buffer, size_t buf_size, int base, char *digits,
                       unsigned long int ulong_val, int flags, int width,
                       int precision)
{
  int tmp_buf_len = 100 + width, len;
  char *tmp_buf, *tmp_buf_ptr, prefix[2];
  tmp_buf = alloca(tmp_buf_len+1);

  prefix[0] = '\0';
  prefix[1] = '\0';
  
  /* Make tmp_buf_ptr point just past the last char of buffer */
  tmp_buf_ptr = tmp_buf + tmp_buf_len;

  /* Main conversion loop */
  do 
    {
      *--tmp_buf_ptr = digits[ulong_val % base];
      ulong_val /= base;
      precision--;
    } 
  while ((ulong_val != 0 || precision > 0) && tmp_buf_ptr > tmp_buf);
 
  /* Get the prefix */
  if (!(flags & IS_NEGATIVE))
    {
      if (base == 16 && (flags & HASH_FLAG))
	{
          if (flags && X_UPCASE)
            {
              prefix[0] = 'x';
              prefix[1] = '0';
            }
          else
            {
              prefix[0] = 'X';
              prefix[1] = '0';
            }
          }
      if (base == 8 && (flags & HASH_FLAG))
          prefix[0] = '0';
      
      if (base == 10 && !(flags & UNSIGNED_DEC) && (flags & PLUS_FLAG))
          prefix[0] = '+';
      else
        if (base == 10 && !(flags & UNSIGNED_DEC) && (flags & SPACE_FLAG))
          prefix[0] = ' ';
    }
  else
      prefix[0] = '-';

  if (prefix[0] != '\0' && tmp_buf_ptr > tmp_buf)
    {
      *--tmp_buf_ptr = prefix[0];
      if (prefix[1] != '\0' && tmp_buf_ptr > tmp_buf)
        *--tmp_buf_ptr = prefix[1];
    }
  
  len = (tmp_buf + tmp_buf_len) - tmp_buf_ptr;

  if (len <= buf_size)
    {
      if (len < width)
        {
          if (width > (tmp_buf_ptr - tmp_buf))
            width = (tmp_buf_ptr - tmp_buf);
          if (flags & MINUS_FLAG)
            {
              memcpy(buffer, tmp_buf_ptr, len);
              memset(buffer + len, (flags & ZERO_PADDING)?'0':' ',
                     width - len);
              len = width;
            }
          else
            {
              memset(buffer, (flags & ZERO_PADDING)?'0':' ',
                     width - len);
              memcpy(buffer + width - len, tmp_buf_ptr, len);
              len = width;
            }
        }
      else
        {
          memcpy(buffer, tmp_buf_ptr, len);
        }
      return len;
    }
  else
    {
      memcpy(buffer, tmp_buf_ptr, buf_size);
      return buf_size;
    }
}

#ifndef KERNEL

static int
snprintf_convert_float(char *buffer, size_t buf_size,
                       double dbl_val, int flags, int width,
                       int precision, char format_char)
{
  char print_buf[160], print_buf_len = 0;
  char format_str[80], *format_str_ptr;

  format_str_ptr = format_str;

  if (width > 155) width = 155;
  if (precision <= 0)
    precision = 6;
  if (precision > 120)
    precision = 120;

  /* Construct the formatting string and let system's sprintf
     do the real work. */
  
  *format_str_ptr++ = '%';

  if (flags & MINUS_FLAG)
    *format_str_ptr++ = '-';
  if (flags & PLUS_FLAG)
    *format_str_ptr++ = '+';
  if (flags & SPACE_FLAG)
    *format_str_ptr++ = ' ';
  if (flags & ZERO_PADDING)
    *format_str_ptr++ = '0';
  if (flags & HASH_FLAG)
    *format_str_ptr++ = '#';
    
  sprintf(format_str_ptr, "%d.%d", width, precision);
  format_str_ptr += strlen(format_str_ptr);

  if (flags & IS_LONG_DOUBLE)
    *format_str_ptr++ = 'L';
  *format_str_ptr++ = format_char;
  *format_str_ptr++ = '\0';

  sprintf(print_buf, format_str, dbl_val);
  print_buf_len = strlen(print_buf);

  if (print_buf_len > buf_size)
    print_buf_len = buf_size;
  strncpy(buffer, print_buf, print_buf_len);
  return print_buf_len;
}

#endif /* KERNEL */
#endif /* HAVE_VSNPRINTF */

#ifndef HAVE_SNPRINTF
extern int snprintf(char *str, size_t size, const char *format, ...)
{
  int ret;
  va_list ap;
  va_start(ap, format);
  ret = vsnprintf(str, size, format, ap);
  va_end(ap);

  return ret;
}
#endif

#ifndef HAVE_VSNPRINTF

extern int vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
  int status, left = (int)size - 1;
  const char *format_ptr = format;
  int flags, width, precision, i;
  char format_char, *orig_str = str;
  int *int_ptr;
  long int long_val;
  unsigned long int ulong_val;
  char *str_val;
#ifndef KERNEL
  double dbl_val;
#endif /* KERNEL */

  flags = 0;
  while (format_ptr < format + strlen(format))
    {
      if (*format_ptr == '%')
        {
          if (format_ptr[1] == '%' && left > 0)
            {
              *str++ = '%';
              left--;
              format_ptr += 2;
            }
          else
            {
              if (left <= 0)
                {
                  *str = '\0';
                  return size;
                }
              else
                {
                  status = snprintf_get_directive(format_ptr, &flags, &width,
                                                  &precision, &format_char, 
                                                  &ap);
                  if (status == 0)
                    {
                      *str = '\0';
                      return 0;
                    }
                  else
                    {
                      format_ptr += status;
                      /* Print a formatted argument */
                      switch (format_char)
                        {
                        case 'i': case 'd':
                          /* Convert to unsigned long int before
                             actual conversion to string */
                          if (flags & IS_LONG_INT)
                            long_val = va_arg(ap, long int);
                          else
                            long_val = (long int) va_arg(ap, int);
                          
                          if (long_val < 0)
                            {
                              ulong_val = (unsigned long int) -long_val;
                              flags |= IS_NEGATIVE;
                            }
                          else
                            {
                              ulong_val = (unsigned long int) long_val;
                            }
                          status = snprintf_convert_ulong(str, left, 10,
                                                          "0123456789",
                                                          ulong_val, flags,
                                                          width, precision);
                          str += status;
                          left -= status;
                          break;

                        case 'x':
                          if (flags & IS_LONG_INT)
                            ulong_val = va_arg(ap, unsigned long int);
                          else
                            ulong_val = 
                              (unsigned long int) va_arg(ap, unsigned int);

                          status = snprintf_convert_ulong(str, left, 16,
                                                          "0123456789abcdef",
                                                          ulong_val, flags,
                                                          width, precision);
                          str += status;
                          left -= status;
                          break;

                        case 'X':
                          if (flags & IS_LONG_INT)
                            ulong_val = va_arg(ap, unsigned long int);
                          else
                            ulong_val = 
                              (unsigned long int) va_arg(ap, unsigned int);

                          status = snprintf_convert_ulong(str, left, 16,
                                                          "0123456789ABCDEF",
                                                          ulong_val, flags,
                                                          width, precision);
                          str += status;
                          left -= status;
                          break;

                        case 'o':
                          if (flags & IS_LONG_INT)
                            ulong_val = va_arg(ap, unsigned long int);
                          else
                            ulong_val = 
                              (unsigned long int) va_arg(ap, unsigned int);

                          status = snprintf_convert_ulong(str, left, 8,
                                                          "01234567",
                                                          ulong_val, flags,
                                                          width, precision);
                          str += status;
                          left -= status;
                          break;

                        case 'u':
                          if (flags & IS_LONG_INT)
                            ulong_val = va_arg(ap, unsigned long int);
                          else
                            ulong_val = 
                              (unsigned long int) va_arg(ap, unsigned int);

                          status = snprintf_convert_ulong(str, left, 10,
                                                          "0123456789",
                                                          ulong_val, flags,
                                                          width, precision);
                          str += status;
                          left -= status;
                          break;

                        case 'p':
                          break;
                          
                        case 'c':
                          if (flags & IS_LONG_INT)
                            ulong_val = va_arg(ap, unsigned long int);
                          else
                            ulong_val = 
                              (unsigned long int) va_arg(ap, unsigned int);
                          *str++ = (unsigned char)ulong_val;
                          left--;
                          break;

                        case 's':
                          str_val = va_arg(ap, char *);

                          if (str_val == NULL)
                            str_val = "(null)";
                          
                          if (precision == 0)
                            precision = strlen(str_val);
                          else
                            {
                              if (memchr(str_val, 0, precision) != NULL)
                                precision = strlen(str_val);
                            }
                          if (precision > left)
                            precision = left;

                          if (width > left)
                            width = left;
                          if (width < precision)
                            width = precision;
                          i = width - precision;

                          if (flags & MINUS_FLAG)
                            {
                              strncpy(str, str_val, precision);
                              memset(str + precision,
                                     (flags & ZERO_PADDING)?'0':' ', i);
                            }
                          else
                            {
                              memset(str, (flags & ZERO_PADDING)?'0':' ', i);
                              strncpy(str + i, str_val, precision);
                            }
                          str += width;
                          left -= width;
                          break;

                        case 'n':
                          int_ptr = va_arg(ap, int *);
                          *int_ptr = str - orig_str;
                          break;

#ifndef KERNEL
                        case 'f': case 'e': case 'E': case 'g': case 'G':
                          if (flags & IS_LONG_DOUBLE)
                            dbl_val = (double) va_arg(ap, long double);
                          else
                            dbl_val = va_arg(ap, double);
                          status =
                            snprintf_convert_float(str, left, dbl_val, flags,
                                                   width, precision,
                                                   format_char);
                          str += status;
                          left -= status;
                          break;
#endif /* KERNEL */
                          
                        default:
                          break;
                        }
                    }
                }
            }
        }
      else
        {
          if (left > 0)
            {
              *str++ = *format_ptr++;
              left--;
            }
          else
            {
              *str = '\0';
              return size;
            }
        }
    }
  *str = '\0';
  return size - left - 1;
}
#endif

/**************************************************************************/
/* ---------------------- strlcpy, strlcat ----------------------------- */
/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 * All rights reserved.
 * Licensed under the 3-clase BSD license
 */

#ifndef HAVE_STRLCPY
/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t strlcpy (char *dst, const char *src, size_t siz)
{
        char *d = dst;
        const char *s = src;
        size_t n = siz;

        /* Copy as many bytes as will fit */
        if (n != 0 && --n != 0) {
                do {
                        if ((*d++ = *s++) == 0)
                                break;
                } while (--n != 0);
        }

        /* Not enough room in dst, add NUL and traverse rest of src */
        if (n == 0) {
                if (siz != 0)
                        *d = '\0';              /* NUL-terminate dst */
                while (*s++)
                        ;
        }

        return(s - src - 1);    /* count does not include NUL */
}
#endif

#ifndef HAVE_STRLCAT
/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t strlcat(char *dst, const char *src, size_t siz)
{
        char *d = dst;
        const char *s = src;
        size_t n = siz;
        size_t dlen;

        /* Find the end of dst and adjust bytes left but don't go past end */
        while (*d != '\0' && n-- != 0)
                d++;
        dlen = d - dst;
        n = siz - dlen;

        if (n == 0)
                return(dlen + strlen(s));
        while (*s != '\0') {
                if (n != 1) {
                        *d++ = *s;
                        n--;
                }
                s++;
        }
        *d = '\0';

        return(dlen + (s - src));       /* count does not include NUL */
}
#endif


#ifndef HAVE_SETSID
int	setsid	(void)
{
#ifdef __EMX__
	return 0;
#else
	return setpgrp(getpid(), getpid());
#endif
}
#endif

#ifndef HAVE_MEMMOVE
/*  $Revision: 3 $
**
**  This file has been modified to get it to compile more easily
**  on pre-4.4BSD systems.  Rich $alz, June 1991.
*/

/*
 * sizeof(word) MUST BE A POWER OF TWO
 * SO THAT wmask BELOW IS ALL ONES
 */
typedef	int word;		/* "word" used for optimal copy speed */

#define	wsize	sizeof(word)
#define	wmask	(wsize - 1)

/*
 * Copy a block of memory, handling overlap.
 * This is the routine that actually implements
 * (the portable versions of) bcopy, memcpy, and memmove.
 */
void * memmove(char *dst0, const char *src0, register size_t length)
{
	register char *dst = dst0;
	register const char *src = src0;
	register size_t t;

	if (length == 0 || dst == src)		/* nothing to do */
		goto retval;

	/*
	 * Macros: loop-t-times; and loop-t-times, t>0
	 */
#define	TLOOP(s) if (t) TLOOP1(s)
#define	TLOOP1(s) do { s; } while (--t)

	if ((unsigned long)dst < (unsigned long)src) {
		/*
		 * Copy forward.
		 */
		t = (int)src;	/* only need low bits */
		if ((t | (int)dst) & wmask) {
			/*
			 * Try to align operands.  This cannot be done
			 * unless the low bits match.
			 */
			if ((t ^ (int)dst) & wmask || length < wsize)
				t = length;
			else
				t = wsize - (t & wmask);
			length -= t;
			TLOOP1(*dst++ = *src++);
		}
		/*
		 * Copy whole words, then mop up any trailing bytes.
		 */
		t = length / wsize;
		TLOOP(*(word *)dst = *(word *)src; src += wsize; dst += wsize);
		t = length & wmask;
		TLOOP(*dst++ = *src++);
	} else {
		/*
		 * Copy backwards.  Otherwise essentially the same.
		 * Alignment works as before, except that it takes
		 * (t&wmask) bytes to align, not wsize-(t&wmask).
		 */
		src += length;
		dst += length;
		t = (int)src;
		if ((t | (int)dst) & wmask) {
			if ((t ^ (int)dst) & wmask || length <= wsize)
				t = length;
			else
				t &= wmask;
			length -= t;
			TLOOP1(*--dst = *--src);
		}
		t = length / wsize;
		TLOOP(src -= wsize; dst -= wsize; *(word *)dst = *(word *)src);
		t = length & wmask;
		TLOOP(*--dst = *--src);
	}
retval:
	return(dst0);
}
#endif

#ifdef CLOAKED

char proctitlestr[140];
char **Argv = NULL;             /* pointer to argument vector */
char *LastArgv = NULL;          /* end of argv */

/**  SETPROCTITLE -- set process title for ps
**
**	Parameters:
**		fmt -- a printf style format string.
**		a, b, c -- possible parameters to fmt.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Clobbers argv of our main procedure so ps(1) will
**		display the title.
*/

#define SPT_NONE	0	/* don't use it at all */
#define SPT_REUSEARGV	1	/* cover argv with title information */
#define SPT_BUILTIN	2	/* use libc builtin */
#define SPT_PSTAT	3	/* use pstat(PSTAT_SETCMD, ...) */
#define SPT_PSSTRINGS	4	/* use PS_STRINGS->... */
#define SPT_SYSMIPS	5	/* use sysmips() supported by NEWS-OS 6 */
#define SPT_SCO		6	/* write kernel u. area */
#define SPT_CHANGEARGV	7	/* write our own strings into argv[] */
#define SPACELEFT(buf, ptr)  (sizeof buf - ((ptr) - buf))

#ifndef SPT_TYPE
# define SPT_TYPE	SPT_REUSEARGV
#endif

#if SPT_TYPE != SPT_NONE && SPT_TYPE != SPT_BUILTIN

# if SPT_TYPE == SPT_PSTAT
#  include <sys/pstat.h>
# endif
# if SPT_TYPE == SPT_PSSTRINGS
#  include <machine/vmparam.h>
#  include <sys/exec.h>
#  ifndef PS_STRINGS	/* hmmmm....  apparently not available after all */
#   undef SPT_TYPE
#   define SPT_TYPE	SPT_REUSEARGV
#  else
#   ifndef NKPDE			/* FreeBSD 2.0 */
#    define NKPDE 63
typedef unsigned int	*pt_entry_t;
#   endif
#  endif
# endif

# if SPT_TYPE == SPT_PSSTRINGS || SPT_TYPE == SPT_CHANGEARGV
#  define SETPROC_STATIC	static
# else
#  define SETPROC_STATIC
# endif

# if SPT_TYPE == SPT_SYSMIPS
#  include <sys/sysmips.h>
#  include <sys/sysnews.h>
# endif

# if SPT_TYPE == SPT_SCO
#  include <sys/immu.h>
#  include <sys/dir.h>
#  include <sys/user.h>
#  include <sys/fs/s5param.h>
#  if PSARGSZ > MAXLINE
#   define SPT_BUFSIZE	PSARGSZ
#  endif
# endif

# ifndef SPT_PADCHAR
#  define SPT_PADCHAR	' '
# endif

# ifndef SPT_BUFSIZE
#  define SPT_BUFSIZE	140
# endif

#endif /* SPT_TYPE != SPT_NONE && SPT_TYPE != SPT_BUILTIN */
extern char **Argv;
extern char *LastArgv;
                    
/*
**  Pointers for setproctitle.
**	This allows "ps" listings to give more useful information.
*/
void initsetproctitle(int argc, char **argv, char **envp)
{
	register int i, envpsize = 0;
	extern char **environ;

	/*
	**  Move the environment so setproctitle can use the space at
	**  the top of memory.
	*/

	for (i = 0; envp[i] != NULL; i++)
		envpsize += strlen(envp[i]) + 1;
	environ = (char **) malloc(sizeof (char *) * (i + 1));
	for (i = 0; envp[i] != NULL; i++)
		environ[i] = strdup(envp[i]);
	environ[i] = NULL;

	/*
	**  Save start and extent of argv for setproctitle.
	*/

	Argv = argv;

	/*
	**  Find the last environment variable within sendmail's
	**  process memory area.
	*/
	while (i > 0 && (envp[i - 1] < argv[0] ||
			 envp[i - 1] > (argv[argc - 1] +
					strlen(argv[argc - 1]) + 1 + envpsize)))
		i--;

	if (i > 0)
		LastArgv = envp[i - 1] + strlen(envp[i - 1]);
	else
		LastArgv = argv[argc - 1] + strlen(argv[argc - 1]);
}

void setproctitle(const char *fmt, ...)
{
#if defined(CLOAKED)
/* this was removed from wu-ftpd which removed it from sendmail */
	register char *p;
	register int i;
	SETPROC_STATIC char buf[SPT_BUFSIZE];
#if SPT_TYPE == SPT_PSTAT
	union pstun pst;
#endif
#if SPT_TYPE == SPT_SCO
	off_t seek_off;
	static int kmem = -1;
	static int kmempid = -1;
	struct user u;
#endif
	va_list args;
	
	p = buf;

	*p = 0;
	p += strlen(p);

	/* print the argument string */
	va_start(args, fmt);
	(void) vsnprintf(p, SPACELEFT(buf, p), fmt, args);
	va_end(args);

	i = strlen(buf);

#if SPT_TYPE == SPT_PSTAT
	pst.pst_command = buf;
	pstat(PSTAT_SETCMD, pst, i, 0, 0);
#endif
#if SPT_TYPE == SPT_PSSTRINGS
	PS_STRINGS->ps_nargvstr = 1;
	PS_STRINGS->ps_argvstr = buf;
#endif
#if SPT_TYPE == SPT_SYSMIPS
	sysmips(SONY_SYSNEWS, NEWS_SETPSARGS, buf);
#endif
#if SPT_TYPE == SPT_SCO
	if (kmem < 0 || kmempid != getpid())
	{
		if (kmem >= 0)
			close(kmem);
		kmem = open(_PATH_KMEM, O_RDWR, 0);
		if (kmem < 0)
			return;
		(void) fcntl(kmem, F_SETFD, 1);
		kmempid = getpid();
	}
	buf[PSARGSZ - 1] = '\0';
	seek_off = UVUBLK + (off_t) u.u_psargs - (off_t) &u;
	if (lseek(kmem, (off_t) seek_off, SEEK_SET) == seek_off)
		(void) write(kmem, buf, PSARGSZ);
#endif
#if SPT_TYPE == SPT_REUSEARGV
	if (i > LastArgv - Argv[0] - 2)
	{
		i = LastArgv - Argv[0] - 2;
		buf[i] = '\0';
	}
	(void) strcpy(Argv[0], buf);
	p = &Argv[0][i];
	while (p < LastArgv)
		*p++ = SPT_PADCHAR;
	Argv[1] = NULL;
#endif
#if SPT_TYPE == SPT_CHANGEARGV
	Argv[0] = buf;
	Argv[1] = 0;
#endif
#endif /* SPT_TYPE != SPT_NONE */
}
#endif /* CLOAKED */

/* --- end of misc stuff --- */

#ifdef __EMX__
#include <sys/utsname.h>
int uname(struct utsname *buf)
{
	ULONG aulBuffer[4];
	APIRET rc;
	if (!buf) return EFAULT;
	rc = DosQuerySysInfo(QSV_VERSION_MAJOR, QSV_MS_COUNT,(void *)aulBuffer, 4*sizeof(ULONG));
	strcpy(buf->sysname,"OS/2");
	strcpy(buf->release, "1.x");
	if (aulBuffer[0] == 20)
	{
		int i = (unsigned int)aulBuffer[1];
		strcpy(buf->release, ltoa(i));
		if (i > 20)
		{
			strcpy(buf->sysname,"Warp");
			sprintf(buf->release, "%d.%d", (int)i/10, i-(((int)i/10)*10));
		}
		else if (i == 10)
			strcpy(buf->release, "2.1");
		else if (i == 0)
			strcpy(buf->release, "2.0");
		strcpy(buf->machine, "i386");
		if(getenv("HOSTNAME") != NULL)
			strcpy(buf->nodename, getenv("HOSTNAME"));
		else
			strcpy(buf->nodename, "unknown");
	}
	return 0;
}
#endif


#ifdef WINNT
int tputs(const unsigned char *str, int nlines, int (*outfun)(int))
{
register unsigned int count = 0;
  /* Safety check. */
	if (str)
	{
		/* Now output the capability string. */
		while (*str)
			(*outfun) (*str++), count++;
	}
	return count;
}
#endif

/* ----------------------- start of base64 stuff ---------------------------*/
/*
 * Copyright (c) 1995-2001 Kungliga Tekniska Högskolan
 * (Royal Institute of Technology, Stockholm, Sweden).
 * All rights reserved.
 *
 * This is licensed under the 3-clause BSD license, which is found above.
 */

static char base64_chars[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 * Return a malloced, base64 string representation of the first 'size' bytes
 * starting at 'data'.  Returns strlen(*str).
 */
int	my_base64_encode (const void *data, int size, char **str)
{
    char *s, *p;
    int i;
    unsigned c;
    const unsigned char *q;

// XXX
//    p = s = (char *)new_malloc(size * 4 / 3 + 4);
    p = s = (char *)malloc(size * 4 / 3 + 4);
    if (p == NULL)
	return -1;
    q = (const unsigned char *) data;
    i = 0;
    for (i = 0; i < size;) {
	c = (unsigned)(unsigned char)q[i++];
	c *= 256;
	if (i < size)
	    c += (unsigned)(unsigned char)q[i];
	i++;
	c *= 256;
	if (i < size)
	    c += (unsigned)(unsigned char)q[i];
	i++;
	p[0] = base64_chars[(c & 0x00fc0000) >> 18];
	p[1] = base64_chars[(c & 0x0003f000) >> 12];
	p[2] = base64_chars[(c & 0x00000fc0) >> 6];
	p[3] = base64_chars[(c & 0x0000003f) >> 0];
	if (i > size)
	    p[3] = '=';
	if (i > size + 1)
	    p[2] = '=';
	p += 4;
    }
    *p = 0;
    *str = s;
    return strlen(s);
}
