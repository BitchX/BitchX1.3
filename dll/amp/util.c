/* this file is a part of amp software

	util.c: created by Andrew Richards

*/

#define AMP_UTIL
#include "amp.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "audio.h"

/* die - for terminal conditions prints the error message and exits */
/* can not be suppressed with -q,-quiet	*/
void
die(char *fmt, ...)
{
#if 0
	va_list ap;
	va_start(ap,fmt);
	vfprintf(stderr, fmt, ap);
#endif
	_exit(-1);
}


/* warn - for warning messages. Can be suppressed by -q,-quiet			*/
void
warn(char *fmt, ...)
{
#if 0
	va_list ap;
	va_start(ap,fmt);
	if (!A_QUIET) {
		fprintf(stderr,"Warning: ");
		vfprintf(stderr, fmt, ap);
	}
#endif
}


/* msg - for general output. Can be suppressed by -q,-quiet. Output */
/* goes to stderr so it doesn't conflict with stdout output */
void
msg(char *fmt, ...)
{
#if 0
	va_list ap;
	va_start(ap,fmt);
 
	if (!A_QUIET)
	  if (A_MSG_STDOUT) {
	    vfprintf(stdout, fmt, ap);
	    fflush(stdout);
	  } else {
	    vfprintf(stderr, fmt, ap);
	    fflush(stderr);
	  }
#endif
}

