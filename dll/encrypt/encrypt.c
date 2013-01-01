#define ENCODE_VERSION "0.001"

/*
 *
 * Written by Colten Edwards. (C) August 97
 * Based on script by suicide for evolver script.
 */
#include "irc.h"
#include "struct.h"
#include "ircaux.h"
#include "vars.h" 
#include "misc.h"
#include "output.h"
#include "module.h"
#define INIT_MODULE
#include "modval.h"

#define cparse convert_output_format
char encode_version[] = "Encode 0.001";
unsigned char *encode_string = NULL;

BUILT_IN_FUNCTION(func_encode)
{
char *new;
	if (!input)
		return m_strdup(empty_string);
	new = m_strdup(input);
	my_encrypt(new, strlen(new), encode_string);
	return new;
}

BUILT_IN_FUNCTION(func_decode)
{
char *new;
	if (!input)
		return m_strdup(empty_string);
	new = m_strdup(input);
	my_decrypt(new, strlen(new), encode_string);
	return new;
}

char *Encode_Version(IrcCommandDll **intp)
{
	return ENCODE_VERSION;
}


int Encrypt_Init(IrcCommandDll **intp, Function_ptr *global_table)
{
int i, j;
char buffer[BIG_BUFFER_SIZE+1];
	initialize_module("encrypt");
	
	add_module_proc(ALIAS_PROC, "encrypt", "MENCODE", NULL, 0, 0, func_encode, NULL);
	add_module_proc(ALIAS_PROC, "encrypt", "MDECODE", NULL, 0, 0, func_decode, NULL);
	encode_string = (char *)new_malloc(512);
	for (i = 1, j = 255; i <= 255; i++, j--)
	{
		switch (i)
		{
			case 27:
			case 127:
			case 255:
				encode_string[i-1] = i;
				break;
			default:
				encode_string[i-1] = j;
				break;
		}
	}
	sprintf(buffer, "$0+%s by panasync - $2 $3", encode_version);
	fset_string_var(FORMAT_VERSION_FSET, buffer);
	put_it("%s", convert_output_format("$G $0 v$1 by panasync. Based on suicide's Abot script.", "%s %s", encode_version, ENCODE_VERSION));
	return 0;
}

