

int save_file (CELL *c)
{
FILE *out = NULL;
	if ((out = fopen(current_path, "w")))
	{
		int i = 0;
		mvwaddstr(c->window, c->erow + 3, c->scol , "Saving file now.......");

		fprintf(out, "#define _USE_LOCAL_CONFIG\n");		
		fprintf(out, "\n\n\n/*\n * Compile Time options\n */\n");
		for (i = 0; compile_default[i].option; i++)
		{
			if (*(compile_default[i].help))
				fprintf(out, "/*\n * %s\n */\n", compile_default[i].help);
			if (compile_default[i].integer)
				fprintf(out, "#define %-50s1\n\n", compile_default[i].out);
			else
				fprintf(out, "/* #undef %s */\n\n", compile_default[i].out);
		}
		fprintf(out, "\n\n\n/*\n * Userlist options\n */\n");
		for (i = 0; userlist_default[i].option; i++)
		{
			if (*(userlist_default[i].help))
				fprintf(out, "/*\n * %s\n */\n", userlist_default[i].help);
			switch(userlist_default[i].type)
			{
				case STR_TYPE:
					break;
				default:
					fprintf(out, "#define %-50s %d\n\n", userlist_default[i].out, userlist_default[i].integer);
			}
		}
		fprintf(out, "\n\n\n/*\n * Flood options\n */\n");
		for (i = 0; flood_default[i].option; i++)
		{
			if (*(flood_default[i].help))
				fprintf(out, "/*\n * %s\n */\n", flood_default[i].help);
			switch(flood_default[i].type)
			{
				case STR_TYPE:
					break;
				default:
					fprintf(out, "#define %-50s %d\n\n", flood_default[i].out, flood_default[i].integer);
			}
		}
		fprintf(out, "\n\n\n/*\n * DCC options\n */\n");
		for (i = 0; dcc_default[i].option; i++)
		{
			if (*(dcc_default[i].help))
				fprintf(out, "/*\n * %s\n */\n", dcc_default[i].help);
			switch(dcc_default[i].type)
			{
				case STR_TYPE:
					break;
				default:
					fprintf(out, "#define %-50s %d\n\n", dcc_default[i].out, dcc_default[i].integer);
			}
		}
		fprintf(out, "\n\n\n/*\n * Server options\n */\n");
		for (i = 0; server_default[i].option; i++)
		{
			if (*(server_default[i].help))
				fprintf(out, "/*\n * %s\n */\n", server_default[i].help);
			switch(server_default[i].type)
			{
				case STR_TYPE:
					break;
				default:
					fprintf(out, "#define %-50s %d\n\n", server_default[i].out, server_default[i].integer);
			}
		}
		fprintf(out, "\n\n\n/*\n * Misc options\n */\n");
		for (i = 0; various_default[i].option; i++)
		{
			if (*(various_default[i].help))
				fprintf(out, "/*\n * %s\n */\n", various_default[i].help);
			switch(various_default[i].type)
			{
				case STR_TYPE:
					break;
				default:
					fprintf(out, "#define %-50s %d\n\n", various_default[i].out, various_default[i].integer);
			}
		}
		fprintf(out, "\n\n\n");
		fclose(out);
	}
	return TRUE;
}
