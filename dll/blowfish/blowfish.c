/*
		blowfish.c -- handles:
		encryption and decryption of passwords

		The first half of this is very lightly edited from public domain
		sourcecode.	For simplicity, this entire module will remain public
		domain.
 */

#include "blowfish.h"
#include "bf_tab.h"		/* P-box P-array, S-box	*/

#define BOXES	3

/* #define S(x,i) (bf_S[i][x.w.byte##i])	*/
#define S0(x) (bf_S[0][x.w.byte0])
#define S1(x) (bf_S[1][x.w.byte1])
#define S2(x) (bf_S[2][x.w.byte2])
#define S3(x) (bf_S[3][x.w.byte3])
#define bf_F(x) (((S0(x) + S1(x)) ^ S2(x)) + S3(x))
#define ROUND(a,b,n) (a.word ^= bf_F(b) ^ bf_P[n])

/* keep a set of rotating P & S boxes */
static struct box_t {
	UWORD_32bits *P;
	UWORD_32bits **S;
	char key[81];
	char keybytes;
	time_t lastuse;
} blowbox[BOXES];

/*
	static UWORD_32bits bf_P[bf_N+2];
	static UWORD_32bits bf_S[4][256];
 */

static UWORD_32bits *bf_P;
static UWORD_32bits **bf_S;

static char blowfish_version[] = "BitchX blowfish encryption module v1.0";

static void blowfish_encipher (UWORD_32bits * xl, UWORD_32bits * xr)
{
	union aword Xl;
	union aword Xr;

	Xl.word = *xl;
	Xr.word = *xr;

	Xl.word ^= bf_P[0];
	ROUND(Xr, Xl, 1);
	ROUND(Xl, Xr, 2);
	ROUND(Xr, Xl, 3);
	ROUND(Xl, Xr, 4);
	ROUND(Xr, Xl, 5);
	ROUND(Xl, Xr, 6);
	ROUND(Xr, Xl, 7);
	ROUND(Xl, Xr, 8);
	ROUND(Xr, Xl, 9);
	ROUND(Xl, Xr, 10);
	ROUND(Xr, Xl, 11);
	ROUND(Xl, Xr, 12);
	ROUND(Xr, Xl, 13);
	ROUND(Xl, Xr, 14);
	ROUND(Xr, Xl, 15);
	ROUND(Xl, Xr, 16);
	Xr.word ^= bf_P[17];

	*xr = Xl.word;
	*xl = Xr.word;
}

static void blowfish_decipher (UWORD_32bits * xl, UWORD_32bits * xr)
{
	union aword Xl;
	union aword Xr;

	Xl.word = *xl;
	Xr.word = *xr;

	Xl.word ^= bf_P[17];
	ROUND(Xr, Xl, 16);
	ROUND(Xl, Xr, 15);
	ROUND(Xr, Xl, 14);
	ROUND(Xl, Xr, 13);
	ROUND(Xr, Xl, 12);
	ROUND(Xl, Xr, 11);
	ROUND(Xr, Xl, 10);
	ROUND(Xl, Xr, 9);
	ROUND(Xr, Xl, 8);
	ROUND(Xl, Xr, 7);
	ROUND(Xr, Xl, 6);
	ROUND(Xl, Xr, 5);
	ROUND(Xr, Xl, 4);
	ROUND(Xl, Xr, 3);
	ROUND(Xr, Xl, 2);
	ROUND(Xl, Xr, 1);
	Xr.word ^= bf_P[0];

	*xl = Xr.word;
	*xr = Xl.word;
}

static void blowfish_init (UBYTE_08bits * key, short keybytes)
{
	int i, j, bx;
	time_t lowest;
	UWORD_32bits data;
	UWORD_32bits datal;
	UWORD_32bits datar;
	union aword temp;

	/* is buffer already allocated for this? */
	for (i = 0; i < BOXES; i++)
		if (blowbox[i].P != NULL) 
		{
			if ((blowbox[i].keybytes == keybytes) &&
				(strncmp((char *) (blowbox[i].key), (char *) key, keybytes) == 0)) 
			{
				blowbox[i].lastuse = now;
				bf_P = blowbox[i].P;
				bf_S = blowbox[i].S;
				return;
			}
		}
		/* no pre-allocated buffer: make new one */
		/* set 'bx' to empty buffer */
	bx = (-1);
	for (i = 0; i < BOXES; i++) 
	{
		if (blowbox[i].P == NULL) 
		{
			bx = i;
			i = BOXES + 1;
		}
	}
	if (bx < 0) 
	{
		/* find oldest */
		lowest = now;
		for (i = 0; i < BOXES; i++)
			if (blowbox[i].lastuse <= lowest) 
			{
				lowest = blowbox[i].lastuse;
				bx = i;
			}
		new_free(&blowbox[bx].P);
		for (i = 0; i < 4; i++)
			new_free(&blowbox[bx].S[i]);
		new_free(&blowbox[bx].S);
	}
	/* initialize new buffer */
	/* uh... this is over 4k */
	blowbox[bx].P = (UWORD_32bits *) new_malloc((bf_N + 2) * sizeof(UWORD_32bits));
	blowbox[bx].S = (UWORD_32bits **) new_malloc(4 * sizeof(UWORD_32bits *));
	for (i = 0; i < 4; i++)
		blowbox[bx].S[i] = (UWORD_32bits *) new_malloc(256 * sizeof(UWORD_32bits));
	bf_P = blowbox[bx].P;
	bf_S = blowbox[bx].S;
	blowbox[bx].keybytes = keybytes;
	strncpy(blowbox[bx].key, key, keybytes);
	blowbox[bx].lastuse = now;

	/* robey: reset blowfish boxes to initial state */
	/* (i guess normally it just keeps scrambling them, but here it's */
	/* important to get the same encrypted result each time) */
	for (i = 0; i < bf_N + 2; i++)
		bf_P[i] = initbf_P[i];
	for (i = 0; i < 4; i++)
		for (j = 0; j < 256; j++)
	bf_S[i][j] = initbf_S[i][j];

	j = 0;
	for (i = 0; i < bf_N + 2; ++i) 
	{
		temp.word = 0;
		temp.w.byte0 = key[j];
		temp.w.byte1 = key[(j + 1) % keybytes];
		temp.w.byte2 = key[(j + 2) % keybytes];
		temp.w.byte3 = key[(j + 3) % keybytes];
		data = temp.word;
		bf_P[i] = bf_P[i] ^ data;
		j = (j + 4) % keybytes;
	}

	datal = 0x00000000;
	datar = 0x00000000;

	for (i = 0; i < bf_N + 2; i += 2) 
	{
		blowfish_encipher(&datal, &datar);
		bf_P[i] = datal;
		bf_P[i + 1] = datar;
	}

	for (i = 0; i < 4; ++i) 
	{
		for (j = 0; j < 256; j += 2) 
		{
			blowfish_encipher(&datal, &datar);
			bf_S[i][j] = datal;
			bf_S[i][j + 1] = datar;
		}
	}
}


/* stuff below this line was written by robey for eggdrop use */

/* convert 64-bit encrypted password to text for userfile */
static char *base64 = "./0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

static int base64dec (char c)
{
	int i;
	for (i = 0; i < 64; i++)
		if (base64[i] == c)
			return i;
	return 0;
}

/* returned string must be freed when done with it! */
static char *encrypt_string (char * key, char * str)
{
	UWORD_32bits left, right;
	char *p, *s, *dest, *d;
	int i;
	dest = (char *) new_malloc((strlen(str) + 9) * 2);
	/* pad fake string with 8 bytes to make sure there's enough */
	s = (char *) new_malloc(strlen(str) + 9);
	strcpy(s, str);
	p = s;
	while (*p)
		p++;
	for (i = 0; i < 8; i++)
		*p++ = 0;
	blowfish_init(key, strlen(key));
	p = s;
	d = dest;
	while (*p) 
	{
		left = ((*p++) << 24);
		left += ((*p++) << 16);
		left += ((*p++) << 8);
		left += (*p++);
		right = ((*p++) << 24);
		right += ((*p++) << 16);
		right += ((*p++) << 8);
		right += (*p++);
		blowfish_encipher(&left, &right);
		for (i = 0; i < 6; i++) 
		{
			*d++ = base64[right & 0x3f];
			right = (right >> 6);
		}
		for (i = 0; i < 6; i++) 
		{
			*d++ = base64[left & 0x3f];
			left = (left >> 6);
		}
	}
	*d = 0;
	new_free(&s);
	return dest;
}

/* returned string must be freed when done with it! */
static char *decrypt_string (char * key, char * str)
{
	UWORD_32bits left, right;
	char *p, *s, *dest, *d;
	int i;
	dest = (char *) new_malloc(strlen(str) + 12);
	/* pad encoded string with 0 bits in case it's bogus */
	s = (char *) new_malloc(strlen(str) + 12);
	strcpy(s, str);
	p = s;
	while (*p)
		p++;
	for (i = 0; i < 12; i++)
		*p++ = 0;
	blowfish_init(key, strlen(key));
	p = s;
	d = dest;
	while (*p) {
		right = 0L;
		left = 0L;
		for (i = 0; i < 6; i++)
			right |= (base64dec(*p++)) << (i * 6);
		for (i = 0; i < 6; i++)
			left |= (base64dec(*p++)) << (i * 6);
		blowfish_decipher(&left, &right);
		for (i = 0; i < 4; i++)
			*d++ = (left & (0xff << ((3 - i) * 8))) >> ((3 - i) * 8);
		for (i = 0; i < 4; i++)
			*d++ = (right & (0xff << ((3 - i) * 8))) >> ((3 - i) * 8);
	}
	*d = 0;
	new_free(&s);
	return dest;
}

#ifdef WANT_TCL
static int tcl_encrypt STDVAR
{
	char *p;
	BADARGS(3, 3, " key string");
	p = encrypt_string(argv[1], argv[2]);
	Tcl_AppendResult(irp, p, NULL);
	new_free(&p);
	return TCL_OK;
}

static int tcl_decrypt STDVAR
{
	char *p;
	BADARGS(3, 3, " key string");
	p = decrypt_string(argv[1], argv[2]);
	Tcl_AppendResult(irp, p, NULL);
	new_free(&p);
	return TCL_OK;
}

#endif /* WANT_TCL */

BUILT_IN_FUNCTION(ircii_encrypt)
{
	char *p, *q, *r;
	if (!input) 
		return m_strdup("1");
	p = input;
	if ((q = strchr(input, ' '))) 
	{
		*q++ = 0; 
		r = encrypt_string(p, q);
		return r;
	} 
	return m_strdup(empty_string);
}

BUILT_IN_FUNCTION(ircii_decrypt)
{
	char *p, *q, *r;
	if (!input) 
		return m_strdup("1");
	p = input;
	if ((q = strchr(input, ' '))) 
	{
		*q++ = 0; 
		r = decrypt_string(p, q);
		return r;
	} 
	return m_strdup(empty_string);
}

int Blowfish_Init(IrcCommandDll **intp, Function_ptr *global_table)
{
	int i;
	initialize_module("Blowfish");
	                
	for (i = 0; i < BOXES; i++) {
		blowbox[i].P = NULL;
		blowbox[i].S = NULL;
		blowbox[i].key[0] = 0;
		blowbox[i].lastuse = 0L;
	}
#ifdef WANT_TCL
	Tcl_CreateCommand(tcl_interp, "encrypt", tcl_encrypt, NULL, NULL);
	Tcl_CreateCommand(tcl_interp, "decrypt", tcl_decrypt, NULL, NULL);
	Tcl_SetVar(tcl_interp, "blowfish_version", blowfish_version, TCL_GLOBAL_ONLY);
#endif
	add_module_proc(ALIAS_PROC, "blowfish", "encrypt", "Blowfish Encryption", 0, 0, ircii_encrypt, NULL);
	add_module_proc(ALIAS_PROC, "blowfish", "decrypt", "Blowfish Decryption", 0, 0, ircii_decrypt, NULL);
  put_it("%s loaded.", blowfish_version);
  put_it("Adapted from eggdrop by By-Tor");
  return 0;
}

int Blowfish_Cleanup(IrcCommandDll **intp)
{
#ifdef WANT_TCL
	Tcl_DeleteCommand(tcl_interp, "encrypt");
	Tcl_DeleteCommand(tcl_interp, "decrypt");
	Tcl_UnsetVar(tcl_interp, "blowfish_version", TCL_GLOBAL_ONLY);
#endif
	return 1;
}
