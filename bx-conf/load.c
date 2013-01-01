

void init_default()
{
int i;
	for (i = 0; compile_default[i].option; i++)
		compile_default[i].integer = 0;
	for (i = 0; dcc_default[i].option; i++)
		dcc_default[i].integer = 0;
	for (i = 0; server_default[i].option; i++)
		server_default[i].integer = 0;
	for (i = 0; userlist_default[i].option; i++)
		userlist_default[i].integer = 0;
	for (i = 0; flood_default[i].option; i++)
		flood_default[i].integer = 0;
	for (i = 0; various_default[i].option; i++)
		various_default[i].integer = 0;

}

char def[] = "#define ";
int load_file(char *filename)
{
FILE *out = NULL;
char buf[200];
int dlen = strlen(def);
char *p, *value, *define;
int i, found = 0;
int init = 0;
	if (!(out = fopen(filename, "r")))
		return 0;
	while(!feof(out))
	{
		found = 0;
		if (!(fgets(buf, 100, out)))
			break;
		if ((p = strchr(buf, '\n')))
			*p = 0;
		if ((p = strchr(buf, '\r')))
			*p = 0;
		if (!buf[0])
			continue;
		if (!init)
		{
			init_default();
			init++;
		}
		if (strncmp(buf, def, dlen))
			continue;
		/* got a #define lets break it down. */
		define = buf;
		if ((p = strchr(buf, ' ')))
			*p++ = 0;		
		define = p;
		if ((p = strchr(define, ' ')))
			*p++ = 0;		
		while (p && strchr(" \t", *p))
			p++;
		value = p;
		if (!value || !define || !*define || !*value)
			continue;
		for (i = 0; compile_default[i].option; i++)
		{
			if (!strcmp(compile_default[i].out, define))
			{
				if (isdigit(*value))
					compile_default[i].integer = strtoul(value, NULL, 10);
				found++;
				break;
			}
		}
		if (found) continue;
		for (i = 0; dcc_default[i].option; i++)
		{
			if (!strcmp(dcc_default[i].out, define))
			{
				if (isdigit(*value))
					dcc_default[i].integer = strtoul(value, NULL, 10);
				found++;
				break;
			}
		}
		if (found) continue;
		for (i = 0; server_default[i].option; i++)
		{
			if (!strcmp(server_default[i].out, define))
			{
				if (isdigit(*value))
					server_default[i].integer = strtoul(value, NULL, 10);
				found++;
				break;
			}
		}
		if (found) continue;
		for (i = 0; userlist_default[i].option; i++)
		{
			if (!strcmp(userlist_default[i].out, define))
			{
				if (isdigit(*value))
					userlist_default[i].integer = strtoul(value, NULL, 10);
				found++;
				break;
			}
		}
		if (found) continue;
		for (i = 0; flood_default[i].option; i++)
		{
			if (!strcmp(flood_default[i].out, define))
			{
				if (isdigit(*value))
					flood_default[i].integer = strtoul(value, NULL, 10);
				found++;
				break;
			}
		}
		if (found) continue;
		for (i = 0; various_default[i].option; i++)
		{
			if (!strcmp(various_default[i].out, define))
			{
				if (isdigit(*value))
					various_default[i].integer = strtoul(value, NULL, 10);
				found++;
				break;
			}
		}
	}
	fclose(out);
	return TRUE;
}
