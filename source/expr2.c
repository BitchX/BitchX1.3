/*
 * $Id: expr2.c 3 2008-02-25 09:49:14Z keaston $
 * math.c - mathematical expression evaluation
 * This file is based on 'math.c', which is part of zsh, the Z shell.
 *
 * Copyright (c) 1992-1997 Paul Falstad, All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright notice,
 *    this list of conditions and the two following paragraphs.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimers in the
 *    documentation and/or other materials provided with the distribution
 * 3. The names of the author(s) may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * In no event shall Paul Falstad or the Zsh Development Group be liable
 * to any party for direct, indirect, special, incidental, or consequential
 * damages arising out of the use of this software and its documentation,
 * even if Paul Falstad and the Zsh Development Group have been advised of
 * the possibility of such damage.
 *
 * Paul Falstad and the Zsh Development Group specifically disclaim any
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose.  The software
 * provided hereunder is on an "as is" basis, and Paul Falstad and the
 * Zsh Development Group have no obligation to provide maintenance,
 * support, updates, enhancements, or modifications.
 *
 */
/*
 * Significant modifications by Jeremy Nelson
 * Coypright 1998 EPIC Software Labs, All rights reserved.
 *
 * You may distribute this file under the same terms as the above, by 
 * including the parties "Jeremy Nelson" and "EPIC Software Labs" to the 
 * limitations of liability and the express disclaimer of all warranties.  
 * This software is provided "AS IS".
 *
 * $Id: expr2.c 3 2008-02-25 09:49:14Z keaston $
 */

#include "irc.h"

#include <math.h>
#include "debug.h"

/* XXX in expr.c */
#if 0
static char *alias_special_char(char **, char *, const char *, char *, int *);
#endif

#define STACKSZ 	100
#define TOKENCOUNT	256
#define MAGIC_TOKEN	-14

/*
 * One thing of note is that while the original did only ints, we really
 * only do strings.  We convert to and from ints as neccesary.  Icky,
 * but given the semantics we require its the only way.
 */
/*
 * All the information for each expression is stored in a struct.  This
 * is done so that there are no global variables in use (theyre all collected
 * making them easier to handle), and makes re-entrancy much easier since 
 * i dont have to ask myself "have i accounted for the old state of all the
 * global variables?"
 */

/*
 * When we want to refer symbolically to a token, we just sling around
 * the integer index to the token table.  This serves two purposes:  We
 * dont have to worry about whether something is malloced or not, or who
 * is resopnsible to free, or anything like that.  If you want to keep 
 * something around, you tokenize() it and that returns a "handle" to the
 * token and then you pass that handle around.  So the pair (context,handle)
 * refers to a unique, usable token. 
 */
typedef int	TOKEN;

/*
 * This sets up whether we do floating point math or integer math
 */
#ifdef FLOATING_POINT_MATH			/* XXXX This doesnt work yet */
  typedef	double		NUMBER;
  typedef 	long		BooL;
# define STON atof
# define NTOS ftoa
#else
  typedef	unsigned long	NUMBER;
  typedef 	long		BooL;
# define STON atol
# define NTOS ltoa
#endif

typedef struct
{
	/* POSITION AND STATE INFORMATION */
	/*
	 * This is the current position in the lexing.
	 */
	char	*ptr;

	/*
	 * When set, the expression is lexed, but nothing that may have a side
	 * effect (function calls, assignments, etc) are actually performed.
	 * Dummy values are instead substituted.
	 */
	int	noeval;

	/* 
	 * When set, this means the next token may either be a prefix operator
	 * or an operand.  When clear, it means the next operator must be a
	 * non-prefix operator.
	 */
	int	operand;


	/* TOKEN TABLE */
	/*
	 * Each registered 'token' is given a TOKEN id.  The idea is
	 * that we want TOKEN to be an opaque type to be used to refer
	 * to a token in a generic way, but in practice its just an integer
	 * offset into a char ** table.  We register all tokens sequentially,
	 * so this just gets incremented when we want to register a new token.
	 */
	TOKEN	token;

	/*
	 * This is the list of operand (string) tokens we have extracted
	 * so far from the expression.  Offsets into this array are stored
	 * into the parsing stack.
	 */
	char *	tokens[TOKENCOUNT + 1];


	/* OPERAND STACK */
	/*
	 * This is the operand shift stack.  These are the operands that
	 * are currently awaiting reduction.  Note that rather than keeping
	 * track of the lvals and rvals here, we simply keep track of offsets
	 * to the 'tokens' table that actually stores all the relevant data.
	 * Then we can just call the token-class functions to get that data.
	 * This is more efficient because it allows us to recycle tokens
	 * more reasonably without wasteful malloc-copies.
	 */
	TOKEN 	stack[STACKSZ + 1];

	/* Current index to the operand stack */
	int	sp;

	/* This is the last token that was lexed. */
	TOKEN	mtok;

	/* This is set when an error happens */
	int	errflag;

	TOKEN	last_token;

	const char	*args;
	int	*args_flag;
} expr_info;

__inline static	TOKEN	tokenize (expr_info *c, const char *t);
	 static char *	after_expando_special (expr_info *c);

void	setup_expr_info (expr_info *c)
{
	int	i;

	c->ptr = NULL;
	c->noeval = 0;
	c->operand = 1;
	c->token = 0;
	for (i = 0; i <= TOKENCOUNT; i++)
		c->tokens[i] = NULL;
	for (i = 0; i <= STACKSZ; i++)
		c->stack[i] = 0;
	c->sp = -1;
	c->mtok = 0;
	c->errflag = 0;
	c->last_token = 0;
	tokenize(c, empty_string);	/* Always token 0 */
}

void 	destroy_expr_info (expr_info *c)
{
	int	i;

	c->ptr = NULL;
	c->noeval = -1;
	c->operand = -1;
	for (i = 0; i < c->token; i++)
		new_free(&c->tokens[i]);
	c->token = -1;
	for (i = 0; i <= STACKSZ; i++)
		c->stack[i] = -1;
	c->sp = -1;
	c->mtok = -1;
	c->errflag = -1;
	c->last_token = -1;
}


/* 
 * LR = left-to-right associativity
 * RL = right-to-left associativity
 * BOO = short-circuiting boolean
 */
#define LR 		0
#define RL 		1
#define BOOL 		2

/*
 * These are all the token-types.  Each of the operators is represented,
 * as is the generic operand type
 */

enum LEX {
	M_INPAR,
	NOT, 		COMP, 		PREMINUS,	PREPLUS,
			UPLUS,		UMINUS,		STRLEN,
			WORDC,		DEREF,
	POWER,
	MUL,		DIV,		MOD,
	PLUS,		MINUS,		STRCAT,
	SHLEFT,		SHRIGHT,
	LES,		LEQ,		GRE,		GEQ,
	MATCH,		NOMATCH,
	DEQ,		NEQ,
	AND,
	XOR,
	OR,
	DAND,
	DXOR,
	DOR,
	QUEST,		COLON,
	EQ,		PLUSEQ,		MINUSEQ,	MULEQ,		DIVEQ,
			MODEQ,		ANDEQ,		XOREQ,		OREQ,
			SHLEFTEQ,	SHRIGHTEQ,	DANDEQ,		DOREQ,
			DXOREQ,		POWEREQ,	STRCATEQ,    STRPREEQ,
			SWAP,
	COMMA,
	POSTMINUS,	POSTPLUS,
	ID,
	M_OUTPAR,
	EERROR,
	EOI,
	TOKCOUNT
};


/*
 * Precedence table:  Operators with a lower precedence VALUE have a higher
 * precedence.  The theory behind infix notation (algebraic notation) is that
 * you have a sequence of operands seperated by (typically binary) operators.
 * The problem of precedence is that each operand is surrounded by two
 * operators, so it is ambiguous which operator the operand "binds" to.  This
 * is resolved by "precedence rules" which state that given two operators,
 * which one is allowed to "reduce" (operate on) the operand.  For a simple
 * explanation, take the expression  (3+4*5).  Now the middle operand is a
 * '4', but we dont know if it should be reduced via the plus, or via the
 * multiply.  If we look up both operators in the prec table, we see that
 * multiply has the lower value -- therefore the 4 is reduced via the multiply
 * and then the result of the multiply is reduced by the addition.
 */
static	int	prec[TOKCOUNT] = 
{
	1,
	2,		2,		2,		2,
			2,		2,		2,
			2,		2,
	3,
	4,		4,		4,
	5,		5,		5,
	6,		6,
	7,		7,		7,		7,
	8,		8,
	9,		9,
	10,
	11,
	12,
	13,
	14,
	15,
	16,		16,
	17,		17,		17,		17,		17,
			17,		17,		17,		17,
			17,		17,		17,		17,
			17,		17,		17,		17,
			17,
	18,
	2,		2,
	0,
	137,
	156,
	200
};
#define TOPPREC 21


/*
 * Associativity table: But precedence isnt enough.  What happens when you
 * have two identical operations to determine between?  Well, the easy way
 * is to say that the first operation is always done first.  But some
 * operators dont work that way (like the assignment operator) and always
 * reduce the LAST (or rightmost) operation first.  For example:
 *	(3+4+5)	    ((4+3)+5)    (7+5)    (12)
 *      (v1=v2=3)   (v1=(v2=3))  (v1=3)   (3)
 * So for each operator we need to specify how to determine the precedence
 * of the same operator against itself.  This is called "associativity".
 * Left-to-right associativity means that the operations occur left-to-right,
 * or first-operator, first-reduced.  Right-to-left associativity means
 * that the operations occur right-to-left, or last-operator, first-reduced.
 * 
 * We have a special case of associativity called BOOL, which is just a 
 * special type of left-to-right associtivity whereby the right hand side
 * of the operand is not automatically parsed. (not really, but its the 
 * theory that counts.)
 */
static 	int 	assoc[TOKCOUNT] =
{
	LR,
	RL,		RL,		RL,		RL,
			RL,		RL,		RL,
			RL,		RL,
	RL,
	LR,		LR,		LR,
	LR,		LR,		LR,
	LR,		LR,
	LR,		LR,		LR,		LR,
	LR,		LR,
	LR,		LR,
	LR,
	LR,
	LR,
	BOOL,
	BOOL,
	BOOL,
	RL,		RL,
	RL,		RL,		RL,		RL,		RL,
			RL,		RL,		RL,		RL,
			RL,		RL,		RL,		RL,
			RL,		RL,		RL,		RL,
			RL,
	LR,
	RL,		RL,
	LR,
	LR,
	LR,
	LR
};


/* ********************* OPERAND TOKEN REPOSITORY ********************** */
/*
 * This is where we store our lvalues, kind of.  What we really store here
 * are all of the string operands.  Actually, we store all of the operands
 * here.  When an operand is parsed, its converted to a string and put in
 * here and the index into the 'token' table is returned.
 */
/* THIS FUNCTION MAKES A NEW COPY OF 'T'.  YOU MUST DISPOSE OF 'T' YOURSELF */
__inline static	TOKEN		tokenize (expr_info *c, const char *t)
{
	if (c->token >= TOKENCOUNT)
	{
		error("Too many tokens for this expression");
		return -1;
	}
	c->tokens[c->token] = m_strdup(t);
	return c->token++;
}

/* get_token gets the ``lvalue'', or original text, of a token */
/* YOU MUST _NOT_ FREE THE RETURN VALUE FROM THIS FUNCTION! */
__inline static	const char *		get_token (expr_info *c, TOKEN v)
{
	if (v == MAGIC_TOKEN)	/* Magic token */
		return c->args;

	if (v < 0 || v >= c->token)
	{
		error("Token index [%d] is out of range", v);
		return get_token(c, 0);	/* The empty token */
	}
	return c->tokens[v];
}



/*
 * These three functions ``getsval'', ``getival'', and ``getbval'' take
 * as arguments token-indexes, and return the appropriate rvalue for those
 * tokens.  For literal text strings, they are simply expanded and returned.
 * For function calls, the function is called, and the return value is 
 * returned.  For variable references, the value is looked up and returned.
 * Furthermore, ``getival'' takes this value and converts it into a long
 * int and returns that.  ``getbval'' takes the value and checks it for its
 * truth or falseness (as determined by check_val()).
 */
/* YOU MUST FREE THE RETURN VALUE FROM THIS FUNCTION! */
static	char *	getsval2 (expr_info *c, TOKEN s);
static char * getsval (expr_info *c, TOKEN s)
{
	char *		retval;
	const char *	t;

	t = get_token(c, s);
	if (x_debug & DEBUG_NEW_MATH_DEBUG)
		debugyell(">>> Expanding token [%d]: [%s]", s, t);
	retval = getsval2(c, s);
	if (x_debug & DEBUG_NEW_MATH_DEBUG)
		debugyell("<<< Expanded token [%d]: [%s] to: [%s]", s, t, retval);
	return retval;
}

/* XXX Ick. :-D */
static	char *	getsval2 (expr_info *c, TOKEN s)
{
	const	char	*t;

	if (c->noeval || s == 0)
		return m_strdup(get_token(c, 0));

	/* XXXX Bleh */
	if (s == MAGIC_TOKEN)
	{
		*c->args_flag = 1;
		return m_strdup(c->args);
	}

	t = get_token(c, s);

	/*
	 * Handle [string] token types.
	 */
	if (*t == '[')
	{
		t++;

		/*
	 	 * Attempt to handle $0 and friends here.  Also handle
		 * Things like $1-, and also $*.
		 */
		if (*t == '$')
		{
		   if (t[1] == '*' && !t[2])
			return m_strdup(c->args);
		   else
		   {
			char *	end = NULL;
			long 	j = strtol(t + 1, &end, 10);

			/* Handle [$X] */
			if (end && !*end)
			{
				*c->args_flag = 1;

				if (j < 0)
					return extract2(c->args, SOS, -j);
				else
					return extract2(c->args, j, j);
			}

			/* Gracefully handle [$X-] as well */
			else if (*end == '-' && !end[1])
			{
				*c->args_flag = 1;
				return extract2(c->args, j, EOS);
			}

			/* Anything else we dont grok */
			else
				return expand_alias(t, c->args, 
							c->args_flag, NULL);
		   }
		}

		/* Handle [plain text] */
		else if (!strchr(t, '$') && !strchr(t, '\\'))
			return m_strdup(t);

		/* Everything else gets expanded per normal */
		else
			return expand_alias(t, c->args, c->args_flag, NULL);
	}

	/* Do this first, its cheap */
	else if (is_number(t))
		return m_strdup(t);

	/* Figure out if its a variable reference or a function call */
	else
	{
		char	*after,
			*ptr,
			*w,
			saver = 0;
		int	func = 0;

		w = LOCAL_COPY(t);
		after = after_expando(w, 0, &func);
		if (after)
		{
			saver = *after;
			*after = 0;
		}

		if (func)
			ptr = call_function(w, c->args, c->args_flag);
		else
			ptr = get_variable_with_args(w, c->args, c->args_flag);

		if (!ptr)
			return m_strdup(empty_string);
		if (after)
			*after = saver;
		return ptr;
	}

	return NULL /* <whatever> */;
}

__inline static  NUMBER	getnval (expr_info *c, TOKEN s)
{
	char	*t;
	NUMBER	retval;

	if (c->noeval)
		return 0;

	if (!(t = getsval(c, s)))
		return 0;

	retval = STON(t);
	new_free(&t);
	return retval; 
}

#ifdef notused
__inline static  BooL	getbval (expr_info *c, TOKEN s)
{
	char	*t;
	long	retval;

	if (c->noeval)
		return 0;

	if (!(t = getsval(c, s)))
		return 0;

	retval = check_val(t);
	new_free(&t);
	return retval;
}
#endif


/* ******************** ASSIGNING TO VARIABLES ************************** */
/*
 * When you have an lvalue (left hand side of an assignment) that needs to
 * be assigned to, then you can call these functions to assign to it the
 * appropriate type.  The basic operation is to assign and rvalue token
 * to an lvalue token.  But some times you dont always have a tokenized
 * rvalue, so you can just pass in a raw value and we will tokenize it for
 * you and go from there.  Note that the "result" of an assignment is the
 * rvalue token.  This is then pushed back onto the stack.
 */
__inline static	TOKEN	setvar (expr_info *c, TOKEN l, TOKEN r)
{
	char *t = expand_alias(get_token(c, l), c->args, c->args_flag, NULL);
	char *u = getsval(c, r);
	char *s;

	if (!c->noeval)
	{
		int 	old_window_display = window_display;
		window_display = 0;
		add_var_alias(t, u);
		window_display = old_window_display;
	}

	s = alloca(strlen(u) + 3);
	s[0] = '[';
	strcpy(s+1, u);

	new_free(&t);
	new_free(&u);
	return tokenize(c, s);
}

__inline static TOKEN 	setnvar (expr_info *c, TOKEN l, NUMBER v) 
{
	 return setvar(c, l, tokenize(c, NTOS(v)));
}

__inline static	TOKEN	setsvar (expr_info *c, TOKEN l, char *v)
{
	char	*s;

	s = alloca(strlen(v) + 3);
	s[0] = '[';
	strcpy(s+1, v);
	return setvar(c, l, tokenize(c, s));
}




/* *************************** OPERAND STACK ************************** */


/*
 * Adding (shifting) and Removing (reducing) operands from the stack is a 
 * fairly straightforward process.  The general way to add an token to
 * the stack is to pass in its TOKEN index.  However, there are some times
 * when you want to shift a value that has not been tokenized.  So you call
 * one of the other functions that will do this for you.
 */
__inline static	TOKEN	pusht (expr_info *c, TOKEN t)
{
	if (c->sp == STACKSZ - 1)
	{
		error("Expressions may not have more than 99 operands");
		return -1;
	}
	else
		c->sp++;

	if (x_debug & DEBUG_NEW_MATH_DEBUG)
		debugyell("Pushing token [%d] [%s]", t, get_token(c, t));
	return ((c->stack[c->sp] = t));
}

__inline static	TOKEN	pushn (expr_info *c, NUMBER val)
{ 
	return pusht(c, tokenize(c, NTOS(val)));
}

__inline static	TOKEN	pushs (expr_info *c, char *val)
{
	char	*blah;
	blah = alloca(strlen(val) + 2);
	sprintf(blah, "[%s", val);
	return pusht(c, tokenize(c, blah)); 
}

__inline static TOKEN	top (expr_info *c)
{
	if (c->sp < 0)
	{
		error("No operands.");
		return -1;
	}
	else
		return c->stack[c->sp];
}

__inline static	TOKEN	pop (expr_info *c)
{
	if (c->sp < 0)
	{
		/* 
		 * Attempting to pop more operands than are available
		 * Yeilds empty values.  Thats probably the most reasonable
		 * course of action.
		 */
		error("Cannot pop operand: no more operands");
		return 0;
	}
	else
		return c->stack[c->sp--];
}

__inline static	double	popn (expr_info *c)
{
	char *	x = getsval(c, pop(c));
	NUMBER	i = atof(x);

	new_free(&x);
	return i;
}

/* YOU MUST FREE THE RETURN VALUE FROM THIS FUNCTION */
__inline static	char *	pops (expr_info *c)
{
	return getsval(c, pop(c));
}

__inline static BooL	popb (expr_info *c)
{
	char *	x = getsval(c, pop(c));
	BooL	i = check_val(x);

	new_free(&x);
	return i;
}

__inline static void	pop2 (expr_info *c, TOKEN *t1, TOKEN *t2)
{
	*t2 = pop(c);
	*t1 = pop(c);
}

__inline static	void	pop2n (expr_info *c, NUMBER *a, NUMBER *b)
{
	TOKEN	t1, t2;
	char	*x, *y;

	pop2(c, &t1, &t2);
	x = getsval(c, t1);
	y = getsval(c, t2);
	*a = STON(x);
	*b = STON(y);
	new_free(&x);
	new_free(&y);
}

__inline static void	pop2s (expr_info *c, char **s, char **t)
{
	TOKEN	t1, t2;
	char	*x, *y;

	pop2(c, &t1, &t2);
	x = getsval(c, t1);
	y = getsval(c, t2);
	*s = x;
	*t = y;
}

__inline static void	pop2b (expr_info *c, BooL *a, BooL *b)
{
	TOKEN	t1, t2;
	char	*x, *y;

	pop2(c, &t1, &t2);
	x = getsval(c, t1);
	y = getsval(c, t2);
	*a = check_val(x);
	*b = check_val(y);
	new_free(&x);
	new_free(&y);
}

__inline static	void	pop2n_a (expr_info *c, NUMBER *a, NUMBER *b, TOKEN *v)
{
	TOKEN	t1, t2;
	char	*x, *y;

	pop2(c, &t1, &t2);
	x = getsval(c, t1);
	y = getsval(c, t2);
	*a = STON(x);
	*b = STON(y);
	*v = t1;
	new_free(&x);
	new_free(&y);
}

__inline static	void	pop2s_a (expr_info *c, char **s, char **t, TOKEN *v)
{
	TOKEN	t1, t2;
	char	*x, *y;

	pop2(c, &t1, &t2);
	x = getsval(c, t1);
	y = getsval(c, t2);
	*s = x;
	*t = y;
	*v = t1;
}

#if notused
__inline static void	pop2b_a (expr_info *c, BooL *a, BooL *b, TOKEN *v)
{
	TOKEN	t1, t2;
	char	*x, *y;

	pop2(c, &t1, &t2);
	x = getsval(c, t1);
	y = getsval(c, t2);
	*a = check_val(x);
	*b = check_val(y);
	*v = t1;
	new_free(&x);
	new_free(&y);
}
#endif

__inline static	void	pop3 (expr_info *c, NUMBER *a, TOKEN *v, TOKEN *w)
{
	TOKEN	t1, t2, t3;
	char	*x;

	t3 = pop(c);
	t2 = pop(c);
	t1 = pop(c);

	x = getsval(c, t1);
	*a = STON(x);
	*v = t2;
	*w = t3;
	new_free(&x);
}



/*
 * This is the reducer.  It takes the relevant arguments off the argument
 * stack and then performs the neccesary operation on them.
 */
void	op (expr_info *cx, int what)
{
	NUMBER	a, b;
	BooL	c, d;
	char	*s, *t;
	TOKEN	v, w;

	if (x_debug & DEBUG_NEW_MATH_DEBUG)
		debugyell("Reducing last operation...");

	if (cx->sp < 0) {
		error("An operator is missing a required operand");
		return;
	}

	if (cx->errflag)
		return;		/* Dont parse on an error */

#define BINARY(x) \
	{ \
		pop2n(cx, &a, &b); \
		pushn(cx, (x)); \
		if (x_debug & DEBUG_NEW_MATH_DEBUG) \
			debugyell("O: %s (%ld %ld) -> %ld", #x, a, b, (long)x); \
		break; \
	}
#define BINARY_BOOLEAN(x) \
	{ \
		pop2b(cx, &c, &d); \
		pushn(cx, (x)); \
		if (x_debug & DEBUG_NEW_MATH_DEBUG) \
			debugyell("O: %s (%ld %ld) -> %ld", #x, c, d, (long)x); \
		break; \
	}

#define BINARY_NOZERO(x) \
	{ \
		pop2n(cx, &a, &b); \
		if (b == 0) { \
			if (x_debug & DEBUG_NEW_MATH_DEBUG) \
				debugyell("O: %s (%ld %ld) -> 0", #x, a, b); \
			error("Division by zero"); \
			pushn(cx, 0); \
		} \
		else \
		{ \
			if (x_debug & DEBUG_NEW_MATH_DEBUG) \
				debugyell("O: %s (%ld %ld) -> %ld", #x, a, b, (long)x); \
			pushn(cx, (x)); \
		} \
		break; \
	}
#define IMPLIED(x) \
	{ \
		pop2n_a(cx, &a, &b, &v); \
		if (x_debug & DEBUG_NEW_MATH_DEBUG) \
			debugyell("O: %s = %s (%ld %ld) -> %ld",  \
				get_token(cx, v), #x, a, b, x); \
		pushn(cx, setnvar(cx, v, (x))); \
		break; \
	}
#define IMPLIED_NOZERO(x) \
	{ \
		pop2n_a(cx, &a, &b, &v); \
		if (b == 0) { \
			if (x_debug & DEBUG_NEW_MATH_DEBUG) \
				debugyell("O: %s = %s (%ld %ld) -> 0",  \
					get_token(cx, v), #x, a, b); \
			error("Division by zero"); \
			pushn(cx, setnvar(cx, v, 0)); \
		} \
		if (x_debug & DEBUG_NEW_MATH_DEBUG) \
			debugyell("O: %s =  %s (%ld %ld) -> %ld",  \
				get_token(cx, v), #x, a, b, x); \
		pushn(cx, setnvar(cx, v, (x))); \
		break; \
	}
#define AUTO_UNARY(x, y) \
	{ \
		v = pop(cx); \
		b = getnval(cx, v); \
		if (x_debug & DEBUG_NEW_MATH_DEBUG) \
			debugyell("O: %s (%s %ld) -> %ld", \
				#x, get_token(cx, v), b, (x)); \
		setnvar(cx, v, (x)); \
		pushn(cx, (y)); \
		break; \
	}

#define dpushn(x1,x2,y1) \
	{ \
		if (x_debug & DEBUG_NEW_MATH_DEBUG) \
		{ \
			debugyell("O: COMPARE"); \
			debugyell("O: %s -> %d", #x2, (x2)); \
		} \
		pushn(x1,y1);  \
	} 
#define COMPARE(x, y) \
	{ \
		pop2s(cx, &s, &t); \
		if ((a = STON(s)) && (b = STON(t))) \
		{ \
			if (x_debug & DEBUG_NEW_MATH_DEBUG) \
				debugyell("O: %s (%ld %ld) -> %d", #x, a, b, (x)); \
			if ((x))		dpushn(cx, x, 1) \
			else			dpushn(cx, x, 0) \
		} \
		else \
		{ \
			if (x_debug & DEBUG_NEW_MATH_DEBUG) \
				debugyell("O: %s (%s %s) -> %d", #x, s, t, (y)); \
			if ((y))			dpushn(cx, y, 1) \
			else				dpushn(cx, y, 0) \
		} \
		new_free(&s); \
		new_free(&t); \
		break; \
	}

	switch (what) 
	{
		/* Simple unary prefix operators */
		case NOT:
			c = popb(cx);
			if (x_debug & DEBUG_NEW_MATH_DEBUG)
				debugyell("O: !%ld -> %d", c, !c);
			pushn(cx, !c);
			break;
		case COMP:
			a = popn(cx);
			if (x_debug & DEBUG_NEW_MATH_DEBUG)
				debugyell(": ~%ld -> %ld", a, ~a);
			pushn(cx, ~a);
			break;
		case UPLUS:
			a = popn(cx);
			if (x_debug & DEBUG_NEW_MATH_DEBUG)
				debugyell("O: +%ld -> %ld", a, a);
			pushn(cx, a);
			break;
		case UMINUS:
			a = popn(cx);
			if (x_debug & DEBUG_NEW_MATH_DEBUG)
				debugyell("O: -%ld -> %ld", a, -a);
			pushn(cx, -a);
			break;
		case STRLEN:
			s = pops(cx);
			a = strlen(s);
			if (x_debug & DEBUG_NEW_MATH_DEBUG)
				debugyell("O: @(%s) -> %ld", s, a);
			pushn(cx, a);
			new_free(&s);
			break;
		case WORDC:
			s = pops(cx);
			a = word_count(s);
			if (x_debug & DEBUG_NEW_MATH_DEBUG)
				debugyell("O: #(%s) -> %ld", s, a);
			pushn(cx, a);
			new_free(&s);
			break;
		case DEREF:
		{
			char	*buffer = NULL,
				*tmp;

			if (top(cx) == MAGIC_TOKEN)
				break;		/* Dont do anything */

			s = pops(cx);
			tmp = expand_alias(s, cx->args, cx->args_flag, NULL);
			alias_special_char(&buffer, tmp, cx->args, 
						NULL, cx->args_flag);
			if (buffer == NULL)
				buffer = m_strdup(empty_string);
			*cx->args_flag = 1;
			pushs(cx, buffer);

			new_free(&buffer);
			new_free(&tmp);
			break;
		}

		/* (pre|post)(in|de)crement operators. */
		case PREPLUS:   AUTO_UNARY(b + 1, b + 1)
		case PREMINUS:  AUTO_UNARY(b - 1, b - 1)
		case POSTPLUS:	AUTO_UNARY(b + 1, b)
		case POSTMINUS: AUTO_UNARY(b - 1, b)
		
		/* Simple binary operators */
		case AND:	BINARY(a & b)
		case XOR:	BINARY(a ^ b)
		case OR:	BINARY(a | b)
		case PLUS:	BINARY(a + b)
		case MINUS:	BINARY(a - b)
		case MUL:	BINARY(a * b)
		case POWER:	BINARY(pow(a, b))
		case SHLEFT:	BINARY(a << b)
		case SHRIGHT:	BINARY(a >> b)
		case DIV:	BINARY_NOZERO(a / b)
		case MOD:	BINARY_NOZERO(a % b)
		case DAND:	BINARY_BOOLEAN((long)(c && d))
		case DOR:	BINARY_BOOLEAN((long)(c || d))
		case DXOR:	BINARY_BOOLEAN((long)((c && !d) || (!c && d)))
		case STRCAT:	
			pop2s(cx, &s, &t);
			if (x_debug & DEBUG_NEW_MATH_DEBUG)
				debugyell("O: (%s) ## (%s) -> %s%s", s, t, s, t);
			malloc_strcat(&s, t);
			pushs(cx, s);
			new_free(&s);
			new_free(&t);
			break;

		/* Assignment operators */
		case PLUSEQ:	IMPLIED(a + b)
		case MINUSEQ:	IMPLIED(a - b)
		case MULEQ:	IMPLIED(a * b)
		case POWEREQ:	IMPLIED((long)pow(a, b))
		case DIVEQ:	IMPLIED_NOZERO(a / b)
		case MODEQ:	IMPLIED_NOZERO(a % b)
		case ANDEQ:	IMPLIED(a & b)
		case XOREQ:	IMPLIED(a ^ b)
		case OREQ:	IMPLIED(a | b)
		case SHLEFTEQ:	IMPLIED(a << b)
		case SHRIGHTEQ: IMPLIED(a >> b)
		case DANDEQ:	IMPLIED((long)(c && d))
		case DOREQ:	IMPLIED((long)(c || d))
		case DXOREQ:	IMPLIED((long)((c && !d) || (!c && d)))
		case STRCATEQ:
			pop2s_a(cx, &s, &t, &v);
			if (x_debug & DEBUG_NEW_MATH_DEBUG) 
				debugyell("O: %s = (%s ## %s) -> %s%s", 
					get_token(cx, v), s, t, s, t);
			malloc_strcat(&s, t);
			pusht(cx, setsvar(cx, v, s));
			new_free(&s);
			new_free(&t);
			break;
		case STRPREEQ:
			pop2s_a(cx, &s, &t, &v);
			if (x_debug & DEBUG_NEW_MATH_DEBUG) 
				debugyell("O: %s = (%s ## %s) -> %s%s", 
					get_token(cx, v), t, s, t, s);
			malloc_strcat(&t, s);
			pusht(cx, setsvar(cx, v, t));
			new_free(&s);
			new_free(&t);
			break;
		case EQ:
			pop2(cx, &v, &w);
			if (x_debug & DEBUG_NEW_MATH_DEBUG)
				debugyell("O: %s = (%s)",
					get_token(cx, v), get_token(cx, w));
			pusht(cx, setvar(cx, v, w));
			break;
		case SWAP:
		{
			char *vval, *wval;

			pop2(cx, &v, &w);
			if (x_debug & DEBUG_NEW_MATH_DEBUG)
				debugyell("O: %s <=> %s",
					get_token(cx, v), get_token(cx, w));
			vval = getsval(cx, v);	/* lhs variable */
			wval = getsval(cx, w);	/* rhs variable */
			setsvar(cx, w, vval);
			pusht(cx, setsvar(cx, v, wval));
			new_free(&vval);
			new_free(&wval);
			break;
		}
		/* Comparison operators */
		case DEQ:
			pop2s(cx, &s, &t);
			if (x_debug & DEBUG_NEW_MATH_DEBUG)
				debugyell("O: %s == %s -> %d", s, t,
					!!my_stricmp(s, t));
			if (my_stricmp(s, t) == 0)	pushn(cx, 1);
			else				pushn(cx, 0);
			new_free(&s);
			new_free(&t);
			break;
		case NEQ:
			pop2s(cx, &s, &t);
			if (x_debug & DEBUG_NEW_MATH_DEBUG)
				debugyell("O: %s != %s -> %d", s, t,
					!my_stricmp(s, t));
			if (my_stricmp(s, t) != 0)	pushn(cx, 1);
			else				pushn(cx, 0);
			new_free(&s);
			new_free(&t);
			break;
		case MATCH:
			pop2s(cx, &s, &t);
			a = !!wild_match(t, s);
			if (x_debug & DEBUG_NEW_MATH_DEBUG)
				debugyell("O: %s =~ %s -> %ld", s, t, a);
			pushn(cx, a);
			new_free(&s);
			new_free(&t);
			break;
		case NOMATCH:
			pop2s(cx, &s, &t);
			a = !wild_match(t, s);
			if (x_debug & DEBUG_NEW_MATH_DEBUG)
				debugyell("O: %s !~ %s -> %ld", s, t, a);
			pushn(cx, a);
			new_free(&s);
			new_free(&t);
			break;
	

		case LES:	COMPARE(a < b,  my_stricmp(s, t) < 0)
		case LEQ:	COMPARE(a <= b, my_stricmp(s, t) <= 0)
		case GRE:	COMPARE(a > b,  my_stricmp(s, t) > 0)
		case GEQ:	COMPARE(a >= b, my_stricmp(s, t) >= 0)


		/* Miscelaneous operators */
		case QUEST:
			pop3(cx, &a, &v, &w);
			if (x_debug & DEBUG_NEW_MATH_DEBUG)
				debugyell("O: %ld ? %s : %s -> %s", a,
					get_token(cx, v), get_token(cx, w),
					(a) ? get_token(cx, v) : 
						get_token(cx, w));
			pusht(cx, (a) ? v : w);
			break;

		case COLON:
			break;

		case COMMA:
			v = pop(cx);
			w = pop(cx);
			if (x_debug & DEBUG_NEW_MATH_DEBUG)
				debugyell("O: %s , %s -> %s", 
					get_token(cx, w),
					get_token(cx, v), 
					get_token(cx, v));
			pusht(cx, v);
			break;

		default:
			error("Unknown operator or out of operators");
			return;
	}
}


/***************************************************************************/
/* 
 * THE LEXER
 */
static	int	dummy = 1;

int	lexerr (expr_info *c, char *format, ...)
{
	char 	buffer[BIG_BUFFER_SIZE + 1];
	va_list	a;

	va_start(a, format);
	vsnprintf(buffer, BIG_BUFFER_SIZE, format, a);
	va_end(a);

	error("%s", buffer);
	c->errflag = 1;
	return EOI;
}

/*
 * 'operand' is state information that tells us about what the next token
 * is expected to be.  When a binary operator is lexed, then the next token
 * is expected to be either a unary operator or an operand.  So in this
 * case 'operand' is set to 1.  When an operand is lexed, then the next token
 * is expected to be a binary operator, so 'operand' is set to 0. 
 */
__inline int	check_implied_arg (expr_info *c)
{
	if (c->operand == 2)
	{
		pusht(c, MAGIC_TOKEN);	/* XXXX Bleh */
		c->operand = 0;
		*c->args_flag = 1;
		return 0;
	}

	return c->operand;
}

__inline TOKEN 	operator (expr_info *c, char *x, int y, TOKEN z)
{
	check_implied_arg(c);
	if (c->operand)
		return lexerr(c, "A binary operator (%s) was found "
				 "where an operand was expected", x);
	c->ptr += y;
	c->operand = 1;
	return z;
}

__inline TOKEN 	unary (expr_info *c, char *x, int y, TOKEN z)
{
	if (!c->operand)
		return lexerr(c, "An operator (%s) was found where "
				 "an operand was expected", x);
	c->ptr += y;
	c->operand = dummy;
	return z;
}


/*
 * This finds and extracts the next token in the expression
 */
static int	zzlex (expr_info *c)
{
	char	*start = c->ptr;

#define OPERATOR(x, y, z) return operator(c, x, y, z);
#define UNARY(x, y, z) return unary(c, x, y, z);

	dummy = 1;
	if (x_debug & DEBUG_NEW_MATH_DEBUG)
		debugyell("Parsing next token from: [%s]", c->ptr);

	for (;;)
	{
	    switch (*(c->ptr++)) 
	    {
		case '(':
			c->operand = 1;
			return M_INPAR;
		case ')':
			/*
			 * If we get a close paren and the lexer is expecting
			 * an operand, then obviously thats a syntax error.
			 * But we gently just insert the empty value as the
			 * rhs for the last operand and hope it all works out.
			 */
			if (check_implied_arg(c))
				pusht(c, 0);
			c->operand = 0;
			return M_OUTPAR;

		case '+':
		{
			/*
			 * Note:  In general, any operand that depends on 
			 * whether it is a unary or binary operator based
			 * upon the context is required to call the func
			 * 'check_implied_arg' to solidify the context.
			 * That is because some operators are ambiguous,
			 * And if you see   (# + 4), it can only be determined
			 * on the fly how to lex that.
			 */
			check_implied_arg(c);
			if (*c->ptr == '+' && (c->operand || !isalnum((unsigned char)*c->ptr)))
			{
				c->ptr++;
				return c->operand ? PREPLUS : POSTPLUS;
			}
			else if (*c->ptr == '=') 
				OPERATOR("+=", 1, PLUSEQ)
			else if (c->operand)
				UNARY("+", 0, UPLUS)
			else
				OPERATOR("+", 0, PLUS)
		}
		case '-':
		{
			check_implied_arg(c);
			if (*c->ptr == '-' && (c->operand || !isalnum((unsigned char)*c->ptr)))
			{
				c->ptr++;
				return (c->operand) ? PREMINUS : POSTMINUS;
			}
			else if (*c->ptr == '=') 
				OPERATOR("-=", 1, MINUSEQ)
			else if (c->operand)
				UNARY("-", 0, UMINUS)
			else
				OPERATOR("-", 0, MINUS)
		}
		case '*':
		{
			if (*c->ptr == '*') 
			{
				c->ptr++;
				if (*c->ptr == '=') 
					OPERATOR("**=", 1, POWEREQ)
				else
					OPERATOR("**", 0, POWER)
			}
			else if (*c->ptr == '=') 
				OPERATOR("*=", 1, MULEQ)
			else if (c->operand)
			{
				dummy = 2;
				UNARY("*", 0, DEREF)
			}
			else
				OPERATOR("*", 0, MUL)
		}
		case '/':
		{
			if (*c->ptr == '=') 
				OPERATOR("/=", 1, DIVEQ)
			else
				OPERATOR("/", 0, DIV)
		}
		case '%':
		{
			if (*c->ptr == '=')
				OPERATOR("%=", 1, MODEQ)
			else
				OPERATOR("%", 0, MOD)
		}

		case '!':
		{
			if (*c->ptr == '=')
				OPERATOR("!=", 1, NEQ)
			else if (*c->ptr == '~')
				OPERATOR("!~", 1, NOMATCH)
			else
				UNARY("!", 0, NOT)
		}
		case '~':
			UNARY("~", 0, COMP)

		case '&':
		{
			if (*c->ptr == '&') 
			{
				c->ptr++;
				if (*c->ptr == '=')
					OPERATOR("&&=", 1, DANDEQ)
				else
					OPERATOR("&&", 0, DAND)
			} 
			else if (*c->ptr == '=') 
				OPERATOR("&=", 1, ANDEQ)
			else
				OPERATOR("&", 0, AND)
		}
		case '|':
		{
			if (*c->ptr == '|') 
			{
				c->ptr++;
				if (*c->ptr == '=') 
					OPERATOR("||=", 1, DOREQ)
				else
					OPERATOR("||", 0, DOR)
			} 
			else if (*c->ptr == '=')
				OPERATOR("|=", 1, OREQ)
			else
				OPERATOR("|", 0, OR)
		}
		case '^':
		{
			if (*c->ptr == '^')
			{
				c->ptr++;
				if (*c->ptr == '=')
					OPERATOR("^^=", 1, DXOREQ)
				else
					OPERATOR("^^", 0, DXOR)
			}
			else if (*c->ptr == '=')
				OPERATOR("^=", 1, XOREQ)
			else
				OPERATOR("^", 0, XOR)
		}
		case '#':
		{
			check_implied_arg(c);
			if (*c->ptr == '#') 
			{
				c->ptr++;
				if (*c->ptr == '=')
					OPERATOR("##=", 1, STRCATEQ)
				else
					OPERATOR("##", 0, STRCAT)
			}
			else if (*c->ptr == '=') 
				OPERATOR("#=", 1, STRCATEQ)
			else if (*c->ptr == '~')
				OPERATOR("#~", 1, STRPREEQ)
			else if (c->operand)
			{
				dummy = 2;
				UNARY("#", 0, WORDC)
			}
			else
				OPERATOR("#", 0, STRCAT)
		}

		case '@':
			dummy = 2;
			UNARY("@", 0, STRLEN)

		case '<':
		{
			if (*c->ptr == '<') 
			{
				c->ptr++;
				if (*c->ptr == '=')
					OPERATOR("<<=", 1, SHLEFTEQ)
				else
					OPERATOR("<<", 0, SHLEFT)
			}
			else if (*c->ptr == '=')
			{
				c->ptr++;
				if (*c->ptr == '>')
					OPERATOR("<=>", 1, SWAP)
				else
					OPERATOR("<=", 0, LEQ)
			}
			else
				OPERATOR("<", 0, LES)
		}
		case '>':
		{
			if (*c->ptr == '>') 
			{
				c->ptr++;
				if (*c->ptr == '=')
					OPERATOR(">>=", 1, SHRIGHTEQ)
				else
					OPERATOR(">>", 0, SHRIGHT)
			} 
			else if (*c->ptr == '=') 
				OPERATOR(">=", 1, GEQ)
			else
				OPERATOR(">", 0, GRE)
		}

		case '=':
			if (*c->ptr == '=') 
				OPERATOR("==", 1, DEQ)
			else if (*c->ptr == '~')
				OPERATOR("=~", 1, MATCH)
			else
				OPERATOR("=", 0, EQ)

		case '?':
			c->operand = 1;
			return QUEST;
		case ':':
			/*
			 * I dont want to hear anything from you anti-goto
			 * bigots out there. ;-)  If you can't figure out
			 * what this does, you ought to give up programming.
			 * And a big old :p to everyone who insisted that
			 * i support this horrid hack.
			 */
			if (c->operand)
				goto handle_expando;

			c->operand = 1;
			return COLON;

		case ',':
			/* Same song, second verse. */
			if (c->operand)
				goto handle_expando;

			c->operand = 1;
			return COMMA;

		case '\0':
			check_implied_arg(c);
			c->operand = 1;
			c->ptr--;
			return EOI;

		case '[':
		{
			char *p = c->ptr - 1;
			char oc = 0;

			if (!c->operand)
				return lexerr(c, "Misplaced [ token");

			if ((c->ptr = MatchingBracket(p + 1, '[', ']')))
			{
				oc = *c->ptr;
				*c->ptr = 0;
			}
			else
				c->ptr = empty_string;

			c->last_token = tokenize(c, p);
			if (oc)
				*c->ptr++ = oc;
			c->operand = 0;
			return ID;
		}
		case ' ':
		case '\t':
		case '\n':
			start++;
			break;

		/*
		 * Handle literal numbers
		 */
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		{
			char 	*end;
			char 	endc;

			c->operand = 0;
			c->ptr--;
			strtod(c->ptr, &end);
			endc = *end;
			*end = 0;
			c->last_token = tokenize(c, c->ptr);
			*end = endc;
			c->ptr = end;
			return ID;
		}

		/*
		 * Handle those weirdo $-values
		 */
		case '$':
			continue;

		/*
		 * Handle generic lvalue operands
		 */
		default:
handle_expando:
		{
			char 	*end;
			char	endc;

			c->operand = 0;
			c->ptr--;
			if ((end = after_expando_special(c)))
			{
				endc = *end;
				*end = 0;
				c->last_token = tokenize(c, start);
				*end = endc;
				c->ptr = end;
			}
			else
			{
				c->last_token = 0; /* Empty token */
				c->ptr = empty_string;
			}

			if (x_debug & DEBUG_NEW_MATH_DEBUG)
				debugyell("After token: [%s]", c->ptr);
			return ID;
		}
	    }
	}
}

/*
 * mathparse -- this is the state machine that actually parses the
 * expression.   The parsing is done through a shift-reduce mechanism,
 * and all the precedence levels lower than 'pc' are evaluated.
 */
static void	mathparse (expr_info *c, int pc)
{
	int	otok, 
		onoeval;

	/*
	 * Drop out of parsing if an error has occured
	 */
	if (c->errflag)
		return;

	/*
	 * Get the next token in the expression
	 */
	c->mtok = zzlex(c);

	/*
	 * For as long as the next operator indicates a shift operation...
	 */
	while (prec[c->mtok] <= pc) 
	{
		/* Drop out if an error has occured */
		if (c->errflag)
			return;

		/*
		 * Figure out what to do with this token that needs
		 * to be shifted.
		 */
		switch (c->mtok) 
		{
			case ID:
				if (x_debug & DEBUG_NEW_MATH_DEBUG)
					debugyell("Parsed identifier token [%s]", get_token(c, c->last_token));
				if (c->noeval)
					pusht(c, 0);
				else
					pusht(c, c->last_token);
				break;

			/*
			 * An open-parenthesis indicates that we should
			 * recursively evaluate the inside of the paren-set.
			 */
			case M_INPAR:
			{
				if (x_debug & DEBUG_NEW_MATH_DEBUG)
					debugyell("Parsed open paren");
				mathparse(c, TOPPREC);

				/*
				 * Of course if the expression ends without
				 * a matching rparen, then we whine about it.
				 */
				if (c->mtok != M_OUTPAR) 
				{
					if (!c->errflag)
					    error("')' expected");
					return;
				}
				break;
			}

			/*
			 * A question mark requires that we check for short
			 * circuiting.  We check the lhs, and if it is true,
			 * then we evaluate the lhs of the colon.  If it is
			 * false then we just parse the lhs of the colon and
			 * evaluate the rhs of the colon.
			 */
			case QUEST:
			{
				long u = popb(c);

				pushn(c, u);
				if (!u)
					c->noeval++;
				mathparse(c, prec[QUEST] - 1);
				if (!u)
					c->noeval--;
				else
					c->noeval++;
				mathparse(c, prec[QUEST]);
				if (u)
					c->noeval--;
				op(c, QUEST);

				continue;
			}

			/*
			 * All other operators handle normally
			 */
			default:
			{
				/* Save state */
				otok = c->mtok;
				onoeval = c->noeval;

				/*
				 * Check for short circuiting.
				 */
				if (assoc[otok] == BOOL)
				{
				    if (x_debug & DEBUG_NEW_MATH_DEBUG)
					debugyell("Parsed short circuit operator");
				    switch (otok)
				    {
					case DAND:
					case DANDEQ:
					{
						long u = popb(c);
						pushn(c, u);
						if (!u)
							c->noeval++;
						break;
					}
					case DOR:
					case DOREQ:
					{
						long u = popb(c);
						pushn(c, u);
						if (u)
							c->noeval++;
						break;
					}
				    }
				}

			 	if (x_debug & DEBUG_NEW_MATH_DEBUG)
				    debugyell("Parsed operator of type [%d]", otok);

				/*
				 * Parse the right hand side through
				 * recursion if we're doing things R->L.
				 */
				mathparse(c, prec[otok] - (assoc[otok] != RL));

				/*
				 * Then reduce this operation.
				 */
				c->noeval = onoeval;
				op(c, otok);
				continue;
			}
		}

		/*
		 * Grab the next token
		 */
		c->mtok = zzlex(c);
	}
}

/*
 * This is the new math parser.  It sets up an execution context, which
 * contains sundry information like all the extracted tokens, intermediate
 * tokens, shifted tokens, and the like.  The expression context is passed
 * around from function to function, each function is totaly independant
 * of state information stored in global variables.  Therefore, this math
 * parser is re-entrant safe.
 */
static char *	matheval (char *s, const char *args, int *args_flag)
{
	expr_info	context;
	char *		ret;

	/* Sanity check */
	if (!s || !*s)
		return m_strdup(empty_string);

	/* Create new state */
	setup_expr_info(&context);
	context.ptr = s;
	context.args = args;
	context.args_flag = args_flag;

	/* Actually do the parsing */
	mathparse(&context, TOPPREC);

	/* Check for error */
	if (context.errflag)
	{
		ret = m_strdup(empty_string);
		goto cleanup;
	}

	/* Check for leftover operands */
	if (context.sp)
		error("The expression has too many operands");

	if (x_debug & DEBUG_NEW_MATH_DEBUG)
	{
		int i;
		debugyell("Terms left: %d", context.sp);
		for (i = 0; i <= context.sp; i++)
			debugyell("Term [%d]: [%s]", i, 
				get_token(&context, context.stack[i]));
	}

	/* Get the return value */
	ret = getsval(&context, pop(&context));

cleanup:
	/* Clean up and restore order */
	destroy_expr_info(&context);

	if (internal_debug & DEBUG_EXPANSIONS && !in_debug_yell)
		debugyell("Returning [%s]", ret);

	/* Return the result */
	return ret;
}


/*
 * after_expando_special: This is a special version of after_expando that
 * can handle parsing out lvalues in expressions.  Due to the eclectic nature
 * of lvalues in expressions, this is quite a bit different than the normal
 * after_expando, requiring a different function. Ugh.
 *
 * This replaces some much more complicated logic strewn
 * here and there that attempted to figure out just how long an expando 
 * name was supposed to be.  Well, now this changes that.  This will slurp
 * up everything in 'start' that could possibly be put after a $ that could
 * result in a syntactically valid expando.  All you need to do is tell it
 * if the expando is an rvalue or an lvalue (it *does* make a difference)
 */
static  char *	after_expando_special (expr_info *c)
{
	char	*start;
	char	*rest;
	int	call;

	if (!(start = c->ptr))
		return c->ptr;

	for (;;)
	{
		rest = after_expando(start, 0, &call);
		if (*rest != '$')
			break;
		start = rest + 1;
	}

	/*
	 * All done!
	 */
	return rest;
}

