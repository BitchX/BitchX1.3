/*
 * words.c -- right now it just holds the stuff i wrote to replace
 * that beastie arg_number().  Eventually, i may move all of the
 * word functions out of ircaux and into here.  Now wouldnt that
 * be a beastie of a patch! Beastie! Beastie!
 *
 * Oh yea.  This file is beastierighted (C) 1994 by the beastie author.
 * Right now the only author is Jeremy "Beastie" Nelson.  See the
 * beastieright file for beastie info.
 */

#include "irc.h"
static char cvsrevision[] = "$Id: words.c 26 2008-04-30 13:57:56Z keaston $";
CVS_REVISION(words_c)
#include "ircaux.h"
#include "modval.h"

/*
 * search() looks for a character forward or backward from mark 
 */
extern char	*BX_strsearch(register char *start, char *mark, char *chars, int how)
{
        if (!mark)
                mark = start;

        if (how > 0)   /* forward search */
        {
		mark = sindex(mark, chars);
		how--;
		for (; how > 0 && mark && *mark; how--)
			mark = sindex(mark + 1, chars);
	}

	else if (how == 0)
		return NULL;

	else  /* how < 0 */
	{
		mark = rsindex(mark, start, chars, -how);
#if 0
		how++;
		for (;(how < 0) && *mark && **mark;how++)
			*mark = rsindex(*mark-1, start, chars);
#endif
	}

	return mark;
}

/* Move to an absolute word number from start */
/* First word is always numbered zero. */
extern char	*BX_move_to_abs_word (const register char *start, char **mark, int word)
{
	register char *pointer = (char *)start;
	register int counter = word;

	/* This fixes a bug that counted leading spaces as
	 * a word, when theyre really not a word.... 
	 * (found by Genesis K.)
	 *
	 * The stock client strips leading spaces on both
	 * the cases $0 and $-0.  I personally think this
	 * is not the best choice, but im not going to stick
	 * my foot in this one... im just going to go with
	 * what the stock client does...
	 */
	 while (pointer && *pointer && my_isspace(*pointer))
		pointer++;

	for (;counter > 0 && *pointer;counter--)
	{
		while (*pointer && !my_isspace(*pointer))
			pointer++;
		while (*pointer && my_isspace(*pointer))
			pointer++;
	}

	if (mark)
		*mark = pointer;
	return pointer;
}

/* Move a relative number of words from the present mark */
extern char	*BX_move_word_rel (const register char *start, char **mark, int word)
{
	register char *pointer = *mark;
	register int counter = word;
	char *end = (char *)start + strlen((char *)start);

	if (end == start) 	/* null string, return it */
		return (char *)start;

	/* 
	 * XXXX - this is utterly pointless at best, and
	 * totaly wrong at worst.
 	 */

	if (counter > 0)
	{
		for (;counter > 0 && pointer;counter--)
		{
			while (*pointer && !my_isspace(*pointer))
				pointer++;
			while (*pointer && my_isspace(*pointer)) 
				pointer++;
		}
	}
	else if (counter == 0)
		pointer = *mark;
	else /* counter < 0 */
	{
		for (;counter < 0 && pointer > start;counter++)
		{
			while (pointer >= start && my_isspace(*pointer))
				pointer--;
			while (pointer >= start && !my_isspace(*pointer))
				pointer--;
		}
		pointer++; /* bump up to the word we just passed */
	}

	if (mark)
		*mark = pointer;
	return pointer;
}

/*
 * extract2 is the word extractor that is used when its important to us
 * that 'firstword' get special treatment if it is negative (specifically,
 * that it refer to the "firstword"th word from the END).  This is used
 * basically by the ${n}{-m} expandos and by function_rightw(). 
 *
 * Note that because of a lot of flak, if you do an expando that is
 * a "range" of words, unless you #define STRIP_EXTRANEOUS_SPACES,
 * the "n"th word will be backed up to the first character after the
 * first space after the "n-1"th word.  That apparantly is what everyone
 * wants, so thats whatll be the default.  Those of us who may not like
 * that behavior or are at ambivelent can just #define it.
 */
#undef STRIP_EXTRANEOUS_SPACES
extern char	*BX_extract2(const char *start, int firstword, int lastword)
{
	/* If firstword or lastword is negative, then
	   we take those values from the end of the string */
	char *mark;
	char *mark2;
	char *booya = NULL;

	/* If firstword is EOS, then the user wants the last word */
	if (firstword == EOS)
	{
		mark = (char *)start + strlen(start);
		mark = move_word_rel(start, &mark, -1);
#ifndef NO_CHEATING
		/* 
		 * Really. the only case where firstword == EOS is
		 * when the user wants $~, in which case we really
		 * dont need to do all the following crud.  Of
		 * course, if there ever comes a time that the
		 * user would want to start from the EOS (when??)
		 * we couldnt make this assumption.
		 */
		return m_strdup(mark);
#endif
	}

	/* SOS is used when the user does $-n, all leading spaces 
	 * are retained  
	 */
	else if (firstword == SOS)
		mark = (char *)start;

	/* If the firstword is positive, move to that word */
	else if (firstword >= 0)
	{
		move_to_abs_word(start, &mark, firstword);
		if (!*mark)
			return m_strdup(empty_string);
	}
	/* Otherwise, move to the firstwords from the end */
	else
	{
		mark = (char *)start + strlen((char *)start);
		move_word_rel(start, &mark, firstword);
	}

#ifndef STRIP_EXTRANEOUS_SPACES
	/* IF the user did something like this:
	 *	$n-  $n-m
	 * then include any leading spaces on the 'n'th word.
	 * this is the "old" behavior that we are attempting
	 * to emulate here.
	 */
#ifndef NO_CHEATING
	if (lastword == EOS || (lastword > firstword))
#else
	if (((lastword == EOS) && (firstword != EOS)) || (lastword > firstword))
#endif
	{
		while (mark > start && my_isspace(mark[-1]))
			mark--;
		if (mark > start)
			mark++;
	}
#endif

	/* 
	 * When we find the last word, we need to move to the 
         * END of the word, so that word 3 to 3, would include
	 * all of word 3, so we sindex to the space after the word
	 */
	if (lastword == EOS)
		mark2 = mark + strlen(mark);

	else 
	{
		if (lastword >= 0)
			move_to_abs_word(start, &mark2, lastword+1);
		else
		{
			mark2 = (char *)start + strlen(start);
			move_word_rel(start, &mark2, lastword);
		}

		while (mark2 > start && my_isspace(mark2[-1]))
			mark2--;
	}

	/* 
	 * If the end is before the string, then there is nothing
	 * to extract (this is perfectly legal, btw)
         */
	if (mark2 < mark)
		booya = m_strdup(empty_string);

	else
	{
#if 0
		/* Otherwise, copy off the string we just isolated */ 
		char tmp;
		tmp = *mark2;
		*mark2 = '\0';
		booya = m_strdup(mark);
		*mark2 = tmp;
#endif
		booya = new_malloc(mark2 - mark + 1);
		strmcpy(booya, mark, (mark2 - mark));
	}

	return booya;
}

/*
 * extract is a simpler version of extract2, it is used when we dont
 * want special treatment of "firstword" if it is negative.  This is
 * typically used by the word/list functions, which also dont care if
 * we strip out or leave in any whitespace, we just do what is the
 * fastest.
 */
extern char	*BX_extract(char *start, int firstword, int lastword)
{
	/* 
	 * firstword and lastword must be zero.  If they are not,
	 * then they are assumed to be invalid  However, please note
	 * that taking word set (-1,3) is valid and contains the
	 * words 0, 1, 2, 3.  But word set (-1, -1) is an empty_string.
	 */
	char *mark;
	char *mark2;
	char *booya = NULL;

	/* 
	 * before we do anything, we strip off leading and trailing
	 * spaces. 
	 *
	 * ITS OK TO TAKE OUT SPACES HERE, AS THE USER SHOULDNT EXPECT
	 * THAT THE WORD FUNCTIONS WOULD RETAIN ANY SPACES. (That is
	 * to say that since the word/list functions dont pay attention
	 * to the whitespace anyhow, noone should have any problem with
	 * those ops removing bothersome whitespace when needed.)
	 */
	while (my_isspace(*start))
		start++;
	remove_trailing_spaces(start);

	if (firstword == EOS)
	{
		mark = start + strlen(start);
		mark = move_word_rel(start, &mark, -1);
	}

	/* If the firstword is positive, move to that word */
	else if (firstword >= 0)
		move_to_abs_word(start, &mark, firstword);

	/* Its negative.  Hold off right now. */
	else
		mark = start;


	/* When we find the last word, we need to move to the 
           END of the word, so that word 3 to 3, would include
	   all of word 3, so we sindex to the space after the word
 	 */
	/* EOS is a #define meaning "end of string" */
	if (lastword == EOS)
		mark2 = start + strlen(start);
	else 
	{
		if (lastword >= 0)
			move_to_abs_word(start, &mark2, lastword+1);
		else
			/* its negative -- thats not valid */
			return m_strdup(empty_string);

		while (mark2 > start && my_isspace(mark2[-1]))
			mark2--;
	}

	/* Ok.. now if we get to here, then lastword is positive, so
	 * we sanity check firstword.
	 */
	if (firstword < 0)
		firstword = 0;
	if (firstword > lastword)	/* this works even if fw was < 0 */
		return m_strdup(empty_string);

	/* If the end is before the string, then there is nothing
	 * to extract (this is perfectly legal, btw)
         */
#if 0
	booya = NULL;
#endif
	if (mark2 < mark)
		return m_strdup(empty_string);
	
	booya = new_malloc(mark2 - mark + 1);
	strmcpy(booya, mark, (mark2 - mark));
#if 0
		malloc_strcpy(&booya, empty_string);
	else
	{
		/* Otherwise, copy off the string we just isolated */ 
		char tmp;
		tmp = *mark2;
		*mark2 = '\0';
		malloc_strcpy(&booya, mark);
		*mark2 = tmp;
	}
#endif
	return booya;
}
