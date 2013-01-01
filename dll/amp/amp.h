/* this file is a part of amp software, (C) tomislav uzelac 1996,1997
*/

/* these should not be touched
*/
#define		SYNCWORD	0xfff

#ifndef TRUE
#define		TRUE		1
#endif
#ifndef FALSE
#define		FALSE		0
#endif

/* 
 * version 
 */
#define		MAJOR		0
#define		MINOR		7
#define		PATCH		6


#include "defs.h"
#include "proto.h"

#ifndef MAX
#define MAX(a,b)	((a) > (b) ? (a) : (b))
#endif
#define MAX3(a,b,c)	((a) > (b) ? MAX(a, c) : MAX(b, c))
#ifndef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif


extern int AUDIO_BUFFER_SIZE;
