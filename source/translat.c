/*
 * translat.c:  Stuff for handling character translation tables
 * and a digraph entry facility.  Support an international IRC!
 *
 * I listen to Sex Pistols, so I assume everyone in this world,
 * and more specifically, all servers, are using ISO 8859/1
 * (Latin-1).  And in case of doubt, please consult Jarkko 'Wiz'
 * Oikarinen's document "Internet Relay Chat Protocol" (doc/Comms
 * in the ircd package), paragraph 2.2.  Besides, all of the sane
 * world has already converted to this set.  (X-Windows, Digital,
 * MS-Windows, etc.)
 * If someone please would forward me some documentation on other
 * international sets, like 8859/2 - 8859/10 etc, please do so!
 * Moreover, feedback on the tables in the definition files would
 * be greatly appreciated!
 * Another idea, to be implemented some beautiful day, would be
 * to add transliteration of the Kanji/Katakana sets used in
 * the far east.  8-)
 * Tomten <tomten@solace.hsh.se> / <tomten@lysator.liu.se>
 */

#include "irc.h"
static char cvsrevision[] = "$Id: translat.c 3 2008-02-25 09:49:14Z keaston $";
CVS_REVISION(translat_c)
#include "struct.h"

#if defined(TRANSLATE) || defined(HEBREW)
#include "vars.h"
#include "translat.h"
#include "ircaux.h"
#include "window.h"
#include "screen.h"
#include "output.h"
#include "hebrew.h"
#define MAIN_SOURCE
#include "modval.h"


#ifndef TRANSLATION_PATH
/* we have this here as a safety feature if for some reason TRANSLATION_PATH
 * is not defined.
 */
#define TRANSLATION_PATH "/usr/local/lib/bx/translation"
#endif


static	unsigned char	my_getarg (char **);

/* Globals */
unsigned char	transToClient[256];    /* Server to client translation. */
unsigned char	transFromClient[256];  /* Client to server translation. */
char	translation = 0;	/* 0 for transparent (no) translation. */
char	digraph_changed = 0;

/*
 * dig_table_lo[] and dig_table_hi[] contain the character pair that
 * will result in the digraph in dig_table_di[].  To avoid searching
 * both tables, I take the lower character of the pair, and only
 * search dig_table_lo[].  Thus, dig_table_lo[] must always contain
 * the lower character of the pair.
 *
 * The digraph tables are based on those in the excellent editor Elvis,
 * with some additions for those, like me, who are used to VT320 or
 * VT420 terminals.
 */

#define	DiLo(x)	x,
#define	DiHi(x)
#define	DiDi(x)

/*
 * Digraph tables.  Note that, when adding a new digraph, the character
 * of the pair with the lowest value, *must* be in the DiLo column.
 * The higher of the pair goes in DiHi, and the digraph itself in DiDi.
 */

unsigned char	dig_table_lo[DIG_TABLE_SIZE] =
{
#include "digraph.inc"
	0
};


#undef	DiLo
#undef	DiHi
#undef	DiDi
#define	DiLo(x)
#define	DiHi(x)	x,
#define	DiDi(x)

unsigned char	dig_table_hi[DIG_TABLE_SIZE] =
{
#include "digraph.inc"
	0
};


#undef	DiLo
#undef	DiHi
#undef	DiDi
#define	DiLo(x)
#define	DiHi(x)
#define	DiDi(x)	x,

unsigned char	dig_table_di[DIG_TABLE_SIZE] =
{
#include "digraph.inc"
	0
};


/*
 * enter_digraph:  The BIND function ENTER_DIGRAPH.
 */
void	enter_digraph(char key, char *ptr)
{
	last_input_screen->digraph_hit = 1;  /* Just stuff away first character. */
}

/*
 * get_digraph:  Called by edit_char() when a digraph entry is activated.
 * Looks up a digraph given char c1 and the global char
 * last_input_screen->digraph_hit.
 */
unsigned char	get_digraph(unsigned char c1)
{
	int	i = 0;
	unsigned char	c,
		c2 = last_input_screen->digraph_first;

	last_input_screen->digraph_hit = 0;
	if (c1 > c2)	/* Make sure we have the lowest one in c1. */
		c = c1,	c1 = c2, c2 = c;
	while (dig_table_lo[i])
	{	/* Find digraph and return it. */
		if ((dig_table_lo[i] == c1) && (dig_table_hi[i] == c2))
			return dig_table_di[i];
		i++;
	}
	return 0;		/* Failed lookup. */
}


/*
 * set_translation:  Called when the TRANSLATION variable is SET.
 * Attempts to load a new translation table.
 */
void set_translation(Window *win, char *tablename, int unused)
{
	FILE	*table;
	unsigned char	temp_table[512];
	char	*filename = NULL, *s;
	int	inputs[8];
	int	j,
		c = 0;
	char	buffer[BIG_BUFFER_SIZE + 1];

	if (!tablename)
	{
		translation = 0;
		return;
	}
	for (s = tablename; *s; s++)
	{
		if (isspace((unsigned char)*s))
		{
			*s = '\0';
			break;
		}			
	}
	
	tablename = upper(tablename);

	/* Check for transparent mode; ISO-8859/1, Latin-1 */
	if (!strcmp("LATIN_1", tablename))
	{
		translation = 0;
		return;
	}

	/* Else try loading the translation table from disk. */
	malloc_strcpy(&filename, TRANSLATION_PATH "/");
	malloc_strcat(&filename, tablename);
	if ( !(table = uzfopen(&filename, ".", 0)) )
	{
		say("Cannot open character table definition \"%s\" !",
			tablename);
		set_string_var(TRANSLATION_VAR, NULL);
		new_free(&filename);
		return;
	}

	/* Any problems in the translation tables between hosts are
	 * almost certain to be caused here.
	 * many scanf implementations do not work as defined. In particular,
	 * scanf should ignore white space including new lines (many stop
	 * at the new line character, hence the fgets and sscanf workaround),
	 * many fail to read 0xab as a hexadecimal number (failing on the
	 * x) despite the 0x being defined as optionally existing on input,
	 * and others zero out all the output variables if there is trailing
	 * non white space in the format string which doesn't appear on the
	 * input. Overall, the standard I/O libraries have a tendancy not
	 * to be very standard.
	 */

	while (fgets(buffer, 80, table))
	{
		sscanf(buffer, "0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x",
		    inputs+0, inputs+1, inputs+2, inputs+3,
		    inputs+4, inputs+5, inputs+6, inputs+7);
		for (j = 0; j<8; j++)
			temp_table[c++] = (unsigned char) inputs[j];
	}
	fclose(table);
	new_free(&filename);
	if (c == 512)
	{
		for (c = 0; c <= 255; c++)
		{
			transToClient[c] = temp_table[c];
			transFromClient[c] = temp_table[c | 256];
		}
#if 0
		for (c = 0; c <= 255; c++)
			transToClient[c] = c;
#endif

		translation = 1;
	}
	else
	{
		say("Error loading translation table \"%s\" !", tablename);
		set_string_var(TRANSLATION_VAR, NULL);
	}
}

/*
 * digraph:  The /DIGRAPH command with facilities.
 * This routine is *NOT* finished yet.
 */

BUILT_IN_COMMAND(digraph)
{
	char	*arg;
	u_char	c1,
		c2 = '\0',
		c3 = '\0';
	int	len,
		i;

	if ((arg = next_arg(args, &args)) && (*arg == '-'))
	{
		arg++;
		if ((len = strlen(arg)) == 0)
		{
			say("Unknown or missing flag.");
			return;
		}
		if (my_strnicmp(arg, "add", len) == 0)
		{
			/*
			 * Add a digraph to the table.
			 * I *know*.  This *is* a kludge.
			 */
			if ((i = strlen((char *)dig_table_lo)) ==
					DIG_TABLE_SIZE - 1)
				say("Sorry, digraph table full.");
			else
			{
				while ((c1 = my_getarg(&args)) &&
				    (c2 = my_getarg(&args)) &&
				    (c3 = my_getarg(&args)))
				{
					/* Pass c1 to get_digraph() */
					last_input_screen->digraph_first = c1;	    
					if (get_digraph(c2) == 0)
					{
						dig_table_di[i] = c3;
						/* Make sure c1 <= c2 */
						if (c1 > c2)
						    c3 = c1, c1 = c2, c2 = c3;
						dig_table_lo[i] = c1;
						dig_table_hi[i] = c2;
						i++;
						dig_table_lo[i] =
							dig_table_hi[i] =
					 		dig_table_di[i] =
							(unsigned char) 0;
						digraph_changed = 1;
						say("Digraph added to table.");
					}
					else
					{
				say("Digraph already defined in table.");
				break;
					}
				}
				if (!c2 || !c3)
					say("Unknown or missing argument.");
			}

		}
		else if (my_strnicmp(arg, "remove", len) == 0)
		{

			/* Remove a digraph from the table. */
			if ((i = strlen((char *)dig_table_lo)) == 0)
				say("Digraph table is already empty.");
			else
			{
				if ((c1 = my_getarg(&args)) &&
						(c2 = my_getarg(&args)))
				{
					i = 0;
					if (c1 > c2)
						c3 = c1, c1 = c2, c2 = c3;
					while (dig_table_lo[i])
					{
						if ((dig_table_lo[i] == c1) &&
						    (dig_table_hi[i] == c2))
					/*
					 * strcpy() is not guaranteed for
					 * overlapping copying, but this one
					 * is high -> low. Ought to be fixed.
					 */
	/* re-indent this block - phone, jan 1993. */
				{
					strcpy(((char *)dig_table_lo + i),
					    (char *)(dig_table_lo + (i + 1)));
					strcpy((char *)(dig_table_hi + i),
					    (char *)(dig_table_hi + (i + 1)));
					strcpy((char *)(dig_table_di + i),
					    (char *)(dig_table_di + (i + 1)));
					digraph_changed = 1;
					put_it("Digraph removed from table.");
					return;
				}
	/* much better */
						i++;
					}
					say("Digraph not found.");
				}
			}
		}
		else if (my_strnicmp(arg, "clear", len) == 0)
		{

			/* Clear digraph table. */
			dig_table_lo[0] = dig_table_hi[0] = dig_table_di[0] =
				(unsigned char) 0;
			digraph_changed = 1;
			say("Digraph table cleared.");

		}
		else
			say("Unknown flag.");
	}
	else
	{

		/* Display digraph table. */
		char	buffer1[8];
		char	buffer2[192];

		say("Digraph table:");
		buffer2[0] = (char) 0;
		i = 0;
		while(dig_table_lo[i])
		{
			sprintf(buffer1, "%c%c %c   ", dig_table_lo[i],
			    dig_table_hi[i], dig_table_di[i]);
			strcat(buffer2, buffer1);
			if ((++i % 10) == 0)
			{
				put_it(buffer2);
				buffer2[0] = (char) 0;
			}
		}
		if (buffer2[0])
			put_it(buffer2);
		sprintf(buffer2, "%d digraphs listed.", i);
		say(buffer2);
	}
}

static	unsigned char my_getarg(char **args)
{
	unsigned char *arg;

	arg = (unsigned char *)next_arg(*args, args);
	if (!args || !*args || !arg)
		return '\0';
	/* Don't trust isdigit() with 8 bits. */
	if ((*arg <= '9') && (*arg >= '0'))
	{
		unsigned char i = *arg & 0x0f;
		while ( *(++arg) )
			i = (i * 10) + (*arg & 0x0f);
		return i;
	}
	else if ( (*arg == '!') && (*(arg + 1)) )
		return *(arg + 1) | 0x80;
	return *arg;
}

void	save_digraphs(FILE *fp)
{
	if (digraph_changed)
	{

		int	i = 0;
		char	*command = "\nDIGRAPH -ADD ";

		fprintf(fp, "DIGRAPH -CLEAR");
		fprintf(fp, command);
		while(1)
		{
			fprintf(fp, "%d %d %d  ", dig_table_lo[i],
				dig_table_hi[i], dig_table_di[i]);
			if (!dig_table_lo[++i])
				break;
			if (!(i % 5))
				fprintf(fp, command);
		}
		fputc('\n', fp);

	}
}

#endif
