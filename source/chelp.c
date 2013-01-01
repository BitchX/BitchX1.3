/*
 * Copyright Colten Edwards (c) 1996
 * BitchX help file system. 
 * When Chelp is called the help file is loaded from 
 * BitchX.help and saved. This file is never loaded from disk after this.
 * Information from the help file is loaded into an array as 0-Topic.
 * $help() also calls the same routines except this information is loaded 
 * differantly as 1-Topic. this allows us to distingush between them 
 * internally. 
 */
 
#include "irc.h"
static char cvsrevision[] = "$Id: chelp.c 127 2011-05-02 11:43:38Z keaston $";
CVS_REVISION(chelp_c)
#include "struct.h"
#include "ircaux.h"
#include "chelp.h"
#include "output.h"
#include "hook.h"
#include "misc.h"
#include "vars.h"
#include "window.h"
#define MAIN_SOURCE
#include "modval.h"

#ifdef WANT_CHELP
int read_file (FILE *help_file, int helpfunc);
int in_chelp = 0;

struct chelp_entry {
	char *title;
	char **contents;
	char *relates;
};

struct chelp_index {
	int size;
	struct chelp_entry *entries;
};

struct chelp_index bitchx_help;
struct chelp_index script_help;

void get_help_topic(const char *args, int helpfunc)
{
	int found = 0, i;
	char *others = NULL;
	struct chelp_index *index = helpfunc ? &script_help : &bitchx_help;
	size_t arglen = strlen(args);

	for (i = 0; i < index->size; i++)
	{
		if (!my_strnicmp(index->entries[i].title, args, arglen))
		{
			int j;
			char *text = NULL;
			if (found++)
			{
				m_s3cat(&others, " , ", index->entries[i].title);
				continue;
			}
			if (do_hook(HELPTOPIC_LIST, "%s", index->entries[i].title))
				put_it("%s", convert_output_format("$G \002$0\002: Help on Topic: \002$1-\002",
					"%s %s", version, index->entries[i].title));
			for (j = 0; (text = index->entries[i].contents[j]) != NULL; j++)
			{
				if (do_hook(HELPSUBJECT_LIST, "%s , %s", index->entries[i].title, text))
				{
					in_chelp++;
					put_it("%s", convert_output_format(text, NULL));
					in_chelp--;
				}
			}		
			text = index->entries[i].relates;
			if (text && do_hook(HELPTOPIC_LIST, "%s", text))
				put_it("%s", convert_output_format(text, NULL));
		}
		else if (found)
			break;
	}
	if (!found)
	{
		if (do_hook(HELPTOPIC_LIST, "%s", args))
			bitchsay("No help on %s", args);
	}

	if (others && found)
	{
		if (do_hook(HELPTOPIC_LIST, "%d %s", found, others))
			put_it("Other %d subjects: %s", found - 1, others);
	}
	new_free(&others);
}

BUILT_IN_COMMAND(chelp)
{
	int reload = 0;

	reset_display_target();

	if (args && !my_strnicmp(args, "-dump", 4))
	{
		next_arg(args, &args);
		reload = 1;
	}

	if (reload || !bitchx_help.size)
	{
		char *help_dir = NULL;
		FILE *help_file;
#ifdef PUBLIC_ACCESS
		malloc_strcpy(&help_dir, DEFAULT_BITCHX_HELP_FILE);
#else
		malloc_strcpy(&help_dir, get_string_var(BITCHX_HELP_VAR));
#endif
		help_file = uzfopen(&help_dir, get_string_var(LOAD_PATH_VAR), 1);
		new_free(&help_dir);
		if (!help_file)
			return;

		read_file(help_file, 0);
		fclose(help_file);
	}	

	if (!args || !*args)
		userage(command, helparg);
	else
		get_help_topic(args, 0);
}

static void free_index(struct chelp_index *index)
{
	int i, j;

	for (i = 0; i < index->size; i++)
	{
		for (j = 0; index->entries[i].contents[j]; j++)
			new_free(&index->entries[i].contents[j]);
		new_free(&index->entries[i].contents);
		new_free(&index->entries[i].title);
		new_free(&index->entries[i].relates);
	}
	new_free(&index->entries);
	index->size = 0;
}

static int compare_chelp(const void *va, const void *vb)
{
    const struct chelp_entry *a = va, *b = vb;
    return my_stricmp(a->title, b->title);
}

int read_file(FILE *help_file, int helpfunc)
{
	char line[BIG_BUFFER_SIZE + 1];
	int item_number = 0;
	int topic = -1;
	struct chelp_index *index = helpfunc ? &script_help : &bitchx_help;

	free_index(index);

	while (fgets(line, sizeof line, help_file))
	{
		size_t len = strlen(line);
		if (line[len - 1] == '\n')
			line[len - 1] = '\0';

		if (!*line || *line == '#')
			continue;

		if (*line != ' ') /* we got a topic copy to topic */
		{
			if (!my_strnicmp(line, "-RELATED", 7))
			{
				if (topic > -1)
				{
					index->entries[topic].relates = m_strdup(line+8);
				}
			}
			else
			{	
				topic++;
				item_number = 0;
				RESIZE(index->entries, index->entries[0], topic + 1);

				index->entries[topic].title = m_strdup(line);
				index->entries[topic].contents = new_malloc(sizeof(char *));
				index->entries[topic].contents[0] = NULL;
				index->entries[topic].relates = NULL;
			}
		}
		else if (topic > -1)
		{ /* we found the subject material */
			item_number++;
			RESIZE(index->entries[topic].contents, char *, item_number + 1);

			index->entries[topic].contents[item_number-1] = m_strdup(line);
			index->entries[topic].contents[item_number] = NULL;
		}
	}

	index->size = topic + 1;

	qsort(index->entries, index->size, sizeof index->entries[0], compare_chelp);

	return 0;
}
#endif
