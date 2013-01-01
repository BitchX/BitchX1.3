/*
 * The original was spagetti. I have replaced Michael's code with some of
 * my own which is a thousand times more readable and can also handle '%',
 * which substitutes anything except a space. This should enable people
 * to position things better based on argument. I have also added '?', which
 * substitutes to any single character. And of course it still handles '*'.
 * this should be more efficient than the previous version too.
 *
 * Thus this whole file becomes:
 *
 * Written By Troy Rollo
 * Copyright(c) 1992
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */


#include "irc.h"
static char cvsrevision[] = "$Id: reg.c 80 2009-11-24 10:21:30Z keaston $";
CVS_REVISION(reg_c)
#include "ircaux.h"
#include "output.h"
#define MAIN_SOURCE
#include "modval.h"

/*
 * The following #define is here because we *know* its behaviour.
 * The behaviour of toupper tends to be undefined when it's given
 * a non lower case letter.
 * All the systems supported by IRCII should be ASCII
 */
#define	mkupper(c)	(((c) >= 'a' && (c) <= 'z') ? ((c) - 'a' + 'A') : c)

#if 0
int old_match(const char *pattern, const char *string)
{
	char	type = 0;

	while (*string && *pattern && *pattern != '*' && *pattern != '%')
	{
		if (*pattern == '\\' && pattern[1])
		{
			if (!*++pattern || !(mkupper(*pattern) == mkupper(*string)))
				return 0;
			else
				pattern++, string++, total_explicit++;
			continue;	/* Erf! try $match(\\* *) */
		}

		if (*pattern == '?')
			pattern++, string++;
		else if (mkupper(*pattern) == mkupper(*string))
			pattern++, string++, total_explicit++;
		else
			break;
	}
	if (*pattern == '*' || *pattern == '%')
	{
		type = (*pattern++);
		while (*string)
		{
			if (old_match(pattern, string))
				return 1;
			else if (type == '*' || *string != ' ')
				string++;
			else
				break;
		}
	}

	/* Slurp up any trailing *'s or %'s... */
	if (!*string && (type == '*' || type == '%'))
		while (*pattern && (*pattern == '*' || *pattern == '%'))
			pattern++;

	if (!*string && !*pattern)
		return 1;

	return 0;
}
#endif

int new_match (const char *pattern, const char *string)
{
	int		count = 1;
	int 		asterisk = 0;
	int		percent = 0;
	const char	*last_asterisk_point = NULL;
	const char	*last_percent_point = NULL;
	int		last_asterisk_count = 0;
	int		last_percent_count = 0;
	const char	*after_wildcard = NULL;
	int		sanity = 0;
	const char	*old_pattern = pattern, *old_string = string;
	if (x_debug & DEBUG_REGEX_DEBUG)
		yell("Matching [%s] against [%s]", pattern, string);

	for (;;)
	{
		if (sanity++ > 100000)
		{
			yell("Infinite loop in match! pattern = [%s] string = [%s]", old_pattern, old_string);
			return 0;
		}

		/*
		 * If the last character in the pattern was a *, then
		 * we walk the string until we find the next instance int
		 * string, of the character that was after the *.
		 * If we get to the end of string, then obviously there
		 * is no match.  A * at the end of the pattern is handled
		 * especially, so we dont need to consider that.
		 */
		if (asterisk)
		{
			/*
			 * More pattern, no source.  Obviously this
			 * asterisk isnt going to cut it.  Try again.
			 * This replaces an 'always failure' case. 
			 * In 99% of the cases, we will try again and it
			 * will fail anyhow, but 1% of the cases it would
			 * have succeeded, so we need that retry.
			 */
			if (!*string)
				return 0;

			/*
			 * XXXX Skip over any backslashes...
			 */
			if (*pattern == '\\')
			{
				pattern++;
				if (tolower((unsigned char)*string) != tolower((unsigned char)*pattern))
					continue;
			}

			/*
			 * If the character in the pattern immediately
			 * following the asterisk is a qmark, then we
			 * save where we're at and we allow the ? to be
			 * matched.  If we find it doesnt work later on,
			 * then we will come back to here and try again.
			 *     OR 
			 * We've found the character we're looking for!
			 * Save some state information about how to recover
			 * if we dont match
			 */
			else if (*pattern == '?' || 
				(tolower((unsigned char)*string) == tolower((unsigned char)*pattern)))
			{
				asterisk = 0;
				last_asterisk_point = string;
				last_asterisk_count = count;
			}

			/*
			 * This is not the character we're looking for.
			 */
			else
				string++;

			continue;
		}

		/*
		 * Ok.  If we're dealing with a percent, but not a asterisk,
		 * then we need to look for the character after the percent.
		 * BUT, if we find a space, then we stop anyways.
		 */
		if (percent)
		{
			/*
			 * Ran out of string.  If there is more to the 
			 * pattern, then we failed.  Otherwise if the %
			 * was at the end of the pattern, we havent found
			 * a space, so it succeeds!
			 */
			if (!*string)
			{
				if (*pattern)
					return 0;
				else
					return count;
			}

			/*
			 * XXXX Skip over any backslashes...
			 */
			if (*pattern == '\\')
			{
				pattern++;
				if (tolower((unsigned char)*string) != tolower((unsigned char)*pattern))
					continue;
			}

			/*
			 * If we find a space, then we stop looking at the
			 * percent.  We're definitely done with it.  We also
			 * go back to normal parsing mode, presumably with
			 * the space after the %.
			 */
			if (*string == ' ')
			{
				percent = 0;
				last_percent_point = NULL;
			}
   
			/*
			 * If this is not the char we're looking for, then
			 * keep looking.
			 */
			else if (tolower((unsigned char)*string) != tolower((unsigned char)*pattern))
				string++;

			/*
			 * We found it!  Huzzah!
			 */
			else
			{
				percent = 0;
				last_percent_point = string;
				last_percent_count = count;
			}

			continue;
		}


		/*
		 * Ok.  So at this point, we know we're not handling an
		 * outstanding asterisk or percent request.  So we look
		 * to see what the next char is in the pattern and deal
		 * with it.
		 */
		switch (*pattern)
		{

		/*
		 * If its an asterisk, then we just keep some info about
		 * where we're at. 
		 */
		case ('*') : case ('%') :
		{
			asterisk = 0, percent = 0;
			do
			{
				if (*pattern == '*')
					asterisk = 1;
				pattern++;
			}
			while (*pattern == '*' || *pattern == '%');

			after_wildcard = pattern;
			if (asterisk)
			{
				last_asterisk_point = string;
				last_asterisk_count = count;
			}
			else
			{
				percent = 1;
				last_percent_point = string;
				last_percent_count = count;
			}

			/*
			 * If there's nothing in the pattern after the
			 * asterisk, then it slurps up the rest of string,
			 * and we're definitely done!
			 */
			if (asterisk && !*pattern)
				return count;

			break;
		}

		/*
		 * If its a question mark, then we have to slurp up one 
		 * character from the pattern and the string.
		 */
		case ('?') :
		{
			pattern++;

			/*
			 * If there is nothing left in string, then we
			 * definitely fail.
			 */
			if (!*string)
				return 0;
			string++;
			break;
		}

		/*
		 * De-quote any \'s in the pattern.
		 */
		case ('\\') :
		{
			/*
			 * ircII says that a single \ at the end of a pattern
			 * is defined as a failure. (must quote SOMETHING)
			 */
			pattern++;
			if (!*pattern)
				return 0;

			/*
			 * Check to see if the dequoted character and
			 * the next string character are the same.
			 */
			if (tolower((unsigned char)*pattern) != tolower((unsigned char)*string))
				return 0;

			count++, string++, pattern++;
			break;
		}

		/*
		 * If there is nothing left in the pattern and string,
		 * then we've definitely succeeded.  Return the number of
		 * non-wildcard characters.
		 */
		default:
		{
			if (!*pattern && !*string)
				return count;

			/*
			 * There are regular characters next in the pattern 
			 * and string.  Are they the same?  If they are, walk 
			 * past them and go to the next character.
			 */
			if (tolower((unsigned char)*pattern) == tolower((unsigned char)*string))
			{
				count++, pattern++, string++;
			}

			/*
			 * The two characters are not the same.  If we're 
			 * currently trying to match a wildcard, go back to 
			 * where we started after the wildcard and try looking
			 * again from there.  If we are not currently matching
			 * a wildcard, then the entire match definitely fails.
			 */
			else if (last_asterisk_point)
			{
                                asterisk = 1;
                                string = last_asterisk_point + 1;
                                pattern = after_wildcard;
                                count = last_asterisk_count;
			}
			else if (last_percent_point)
			{
                                percent = 1;
                                string = last_percent_point + 1;
                                pattern = after_wildcard;
                                count = last_percent_count;
			}
			else
				return 0;

			break;
		}
		}
	}

	return 0;
}

/*
 * wild_match: calculate the "value" of str when matched against pattern.
 * The "value" of a string is always zero if it is not matched by the pattern.
 * In all cases where the string is matched by the pattern, then the "value"
 * of the match is 1 plus the number of non-wildcard characters in "str".
 *
 * \\[ and \\] handling done by Jeremy Nelson
 */
int BX_wild_match (const char *p, const char *str)
{
	/*
	 * Is there a \[ in the pattern to be expanded? 
	 * 
	 * This stuff here just reduces the \[ \] set into a series of
	 * one-simpler patterns and then recurses over the options.
	 */
	if (strstr(p, "\\["))
	{
		char *pattern, *ptr, *ptr2, *arg, *placeholder;
		int nest = 0;

		/*
		 * Only make the copy if we're going to be tearing it apart.
		 */
		pattern = LOCAL_COPY(p);

		/*
		 * We will have to null this out, but not until we've used it
		 */
		placeholder = ptr = ptr2 = strstr(pattern, "\\[");

		/*
		 * Look for the matching \].
		 */
		do
		{
			switch (ptr[1]) 
			{
					/* step over it and add to nest */
				case '[' :  ptr2 = ptr + 2 ;
					    nest++;
					    break;
					/* step over it and remove nest */
				case ']' :  ptr2 = ptr + 2;
					    nest--;
					    break;
				default:
					    ptr2 = ptr + 2;
					    break;
			}
		}
		while (nest && (ptr = strchr(ptr2, '\\')));

		/*
		 * Right now, we know that ptr points to a \] or to a NULL.
		 * Remember that '&&' short circuits and that ptr will
		 * not be set to NULL if (nest) is zero.
		 */
		if (ptr)
		{
			int best_total = 0;

			*ptr = 0;
			ptr += 2;
			*placeholder = 0;
			placeholder += 2;

			/* 
			 * grab words ("" sets or space words) one at a time
			 * and attempt to match all of them.  The best value
			 * matched is the one used.
			 */
			while ((arg = new_next_arg(placeholder, &placeholder)))
			{
				int tmpval;
				char my_buff[BIG_BUFFER_SIZE + 1];

				strlcpy(my_buff, pattern, BIG_BUFFER_SIZE);
				strlcat(my_buff, arg, BIG_BUFFER_SIZE);
				strlcat(my_buff, ptr, BIG_BUFFER_SIZE);

				/*
				 * The total_explicit we return is whatever
				 * sub-pattern has the highest total_explicit
				 */
				if ((tmpval = wild_match(my_buff, str)))
				{
					if (tmpval > best_total)
						best_total = tmpval;
				}
			}

			return best_total; /* end of expansion section */
		}

		/*
		 * Possibly an unmatched \[ \] set.  Just wing it.
		 */
		else
			return new_match(pattern, str);
	}

	/*
	 * Trivial case -- No \[ \] sets, just do the match.
	 */
	else
		return new_match(p, str);
}

