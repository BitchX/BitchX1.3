
#include "irc.h"
#include "struct.h"
#include "dcc.h"
#include "ircaux.h"
#include "ctcp.h"
#include "cdcc.h"
#include "input.h"
#include "status.h"
#include "lastlog.h"
#include "screen.h"
#include "vars.h" 
#include "misc.h"
#include "output.h"
#include "module.h"
#include "hook.h"
#include "hash2.h"
#include "napster.h"
#include "bsdglob.h"
#include "modval.h"

#include <sys/stat.h>
#include <sys/ioctl.h>
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif

GetFile *getfile_struct = NULL;
ResumeFile *resume_struct = NULL;


char *napster_status(void)
{
int upload = 0;
int download = 0;
GetFile *tmp;
char buffer[NAP_BUFFER_SIZE+1];
char tmpbuff[80];
double perc = 0.0;
	if (!get_dllint_var("napster_window"))
		return m_strdup(empty_string);
	sprintf(buffer, statistics.shared_files ? "%s [Sh:%lu/%3.2f%s] ": "%s ", 
		nap_current_channel ? nap_current_channel : empty_string, 
		statistics.shared_files, _GMKv(statistics.shared_filesize), 
		_GMKs(statistics.shared_filesize));
	for (tmp = getfile_struct; tmp; tmp = tmp->next, download++)
	{
		if (!tmp->filesize)
			continue;
                perc = (100.0 * (((double)(tmp->received + tmp->resume)) / (double)tmp->filesize));
		sprintf(tmpbuff, "%4.1f%%%%", perc);
		if (download)
			strcat(buffer, ",");
		else
			strcat(buffer, " [G:");
		strcat(buffer, tmpbuff);
	}
	if (download)
		strcat(buffer, "]");
	for (tmp = napster_sendqueue; tmp; tmp = tmp->next, upload++)
	{
		if (!tmp->filesize)
			continue;
                perc = (100.0 * (((double)(tmp->received + tmp->resume)) / (double)tmp->filesize));
		sprintf(tmpbuff, "%4.1f%%%%", perc);
		if (upload)
			strcat(buffer, ",");
		else
			strcat(buffer, " [S:");
		strcat(buffer, tmpbuff);
	}
	if (upload)
		strcat(buffer, "]");
	sprintf(tmpbuff, " [U:%d/D:%d]", upload, download);
	strcat(buffer, tmpbuff);
	return m_strdup(buffer);
}

BUILT_IN_DLL(nap_del)
{
int count = 0;
GetFile *tmp, *last = NULL;
int num;
char *t;
	if (!args && !*args)
		return;
	if (*args == '*')
	{
		if ((do_hook(MODULE_LIST, "NAP DEL ALL")))
			nap_say("%s", cparse("Removing ALL file send/upload", NULL));
		while (getfile_struct)
		{
			count++;
			tmp = getfile_struct;
			last = tmp->next;
			if ((do_hook(MODULE_LIST, "NAP DEL GET %s %s", tmp->nick, tmp->filename)))
				nap_say("%s", cparse("Removing $0 [$1-]", "%s %s", tmp->nick, base_name(tmp->filename)));
			nap_finished_file(tmp->socket, tmp);
			getfile_struct = last;
			send_ncommand(CMDS_UPDATE_GET, NULL);
		}
		while (napster_sendqueue)
		{
			count++;
			tmp = napster_sendqueue;
			last = tmp->next;
			if ((do_hook(MODULE_LIST, "NAP DEL SEND %s %s", tmp->nick, tmp->filename)))
				nap_say("%s", cparse("Removing $0 [$1-]", "%s %s", tmp->nick, base_name(tmp->filename)));
			nap_finished_file(tmp->socket, tmp);
			napster_sendqueue = last;
			send_ncommand(CMDS_UPDATE_SEND, NULL);
		}
		build_napster_status(NULL);
		return;
	}
	while ((t = next_arg(args, &args)))
	{	
		char *name = NULL;
		count = 1;
		if (!(num = my_atol(t)))
			name = t;
		for (tmp = getfile_struct; tmp; tmp = tmp->next, count++)
		{
			if ((count == num) || (name && !my_stricmp(name, tmp->nick)))
			{
				if (last)
					last->next = tmp->next;
				else	
					getfile_struct = tmp->next;
				if ((do_hook(MODULE_LIST, "NAP DEL GET %s %s", tmp->nick, tmp->filename)))
					nap_say("%s", cparse("Removing $0 [$1-]", "%s %s", tmp->nick, base_name(tmp->filename)));
				nap_finished_file(tmp->socket, tmp);
				build_napster_status(NULL);
				send_ncommand(CMDS_UPDATE_GET, NULL);
				return;
			}
			last = tmp;
		}
		last = NULL;
		for (tmp = napster_sendqueue; tmp; tmp = tmp->next, count++)
		{
			if ((count == num) || (name && !my_stricmp(name, tmp->nick)))
			{
				if (last)
					last->next = tmp->next;
				else	
					napster_sendqueue = tmp->next;
				if ((do_hook(MODULE_LIST, "NAP DEL SEND %s %s", tmp->nick, tmp->filename)))
					nap_say("%s", cparse("Removing $0 [$1-]", "%s %s", tmp->nick, base_name(tmp->filename)));
				nap_finished_file(tmp->socket, tmp);
				build_napster_status(NULL);
				send_ncommand(CMDS_UPDATE_SEND, NULL);
				return;
			}
			last = tmp;
		}
	}
}

BUILT_IN_DLL(nap_glist)
{
int count = 1;
GetFile *sg = getfile_struct;
time_t snow = now;
char	*dformat = "%W#$[3]0%n %Y$4%n $[14]1 $[-6]2$3 $5/$6 $7-";

	for(sg = getfile_struct; sg; sg = sg->next, count++)
	{
		char buff[80];
		char buff1[80];
		char buff2[80];
		char ack[4];
		double perc = 0.0;
		if (count == 1)
		{
			nap_put("%s", cparse("ÚÄÄÄÄÄ%GD%gownloads", NULL));
			nap_put("%s", cparse("%KÀÄÄ%nÄ%WÄ%nÄ%KÄÄÄÄÄÄÄÄÄÄÄÄÄÄ%nÄ%WÄ%nÄ%KÄÄÄÄÄÄ%nÄ%WÄ%nÄ%KÄÄÄÄÄÄÄÄ%nÄ%WÄ%nÄ%KÄÄÄÄÄ%nÄ%WÄ%nÄ%KÄÄÄÄÄ%nÄ%WÄ%nÄ%KÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ", NULL, NULL));
		}
		if (sg->starttime)
			sprintf(buff, "%2.3f", sg->received / 1024.0 / (snow - sg->starttime));
		else
			strcpy(buff, "N/A");
		if (sg->filesize)
			perc = (100.0 * (((double)(sg->received + sg->resume)) / (double)sg->filesize));
		sprintf(buff1, "%4.1f%%", perc);
		sprintf(buff2, "%4.2f", _GMKv(sg->filesize));
		*ack = 0;
		if (sg->up & NAP_QUEUED)
			strcpy(ack, "Q");
		strcat(ack, sg->starttime ? "D" : "W");
		nap_put("%s", cparse(dformat, "%d %s %s %s %s %s %s %s", 
			count, sg->nick, buff2, _GMKs(sg->filesize),
			ack, buff, buff1, base_name(sg->filename)));
	}
	for (sg = napster_sendqueue; sg; sg = sg->next, count++)
	{
		char buff[80];
		char buff1[80];
		char buff2[80];
		char ack[10];
		double perc = 0.0;
		if (count == 1)
		{
			nap_put("%s", cparse("ÚÄÄÄÄÄ%GU%gploads", NULL));
			nap_put("%s", cparse("%KÀÄÄ%nÄ%WÄ%nÄ%KÄÄÄÄÄÄÄÄÄÄÄÄÄÄ%nÄ%WÄ%nÄ%KÄÄÄÄÄÄ%nÄ%WÄ%nÄ%KÄÄÄÄÄÄÄÄ%nÄ%WÄ%nÄ%KÄÄÄÄÄ%nÄ%WÄ%nÄ%KÄÄÄÄÄ%nÄ%WÄ%nÄ%KÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ", NULL, NULL));
		}
		if (sg->starttime)
			sprintf(buff, "%2.3f", sg->received / 1024.0 / (snow - sg->starttime));
		else
			strcpy(buff, "N/A");
		if (sg->filesize)
			perc = (100.0 * (((double)(sg->received + sg->resume)) / (double)sg->filesize));
		sprintf(buff1, "%4.1f%%", perc);
		sprintf(buff2, "%4.2f", _GMKv(sg->filesize));
		*ack = 0;
		if (sg->up & NAP_QUEUED)
			strcpy(ack, "Q");
		strcat(ack, sg->starttime ? "U" : "W");
		nap_put("%s", cparse(dformat, "%d %s %s %s %s %s %s %s", 
			count, sg->nick, buff2, _GMKs(sg->filesize), 
			ack, buff, buff1, base_name(sg->filename)));
	}
}

NAP_COMM(cmd_resumerequest)
{
char *nick, *file, *checksum;
int port;
unsigned long filesize;
unsigned long ip;
int speed;
int count = 0;
ResumeFile *sf;

	nick = next_arg(args, &args);
	ip = my_atol(next_arg(args, &args));
	port = my_atol(next_arg(args, &args));
	file = new_next_arg(args, &args);
	checksum = next_arg(args, &args);
	filesize = my_atol(next_arg(args, &args));
	speed = my_atol(next_arg(args, &args));

	for (sf = resume_struct; sf; sf = sf->next)
	{
		if (!strcmp(checksum, sf->checksum) && (filesize == sf->filesize))
		{
			FileStruct *new;
			new = new_malloc(sizeof(FileStruct));
			new->nick = m_strdup(nick);
			new->ip = ip;
			new->name = m_strdup(file);
			new->checksum = m_strdup(checksum);
			new->port = port;
			new->filesize = filesize;
			new->speed = speed;
			new->next = sf->results;
			sf->results = new;
			count++;
		}
	}
	if (!count)
		nap_say("error in resume request. no match");
	return 0;
}

NAP_COMM(cmd_resumerequestend)
{
char *checksum;
unsigned long filesize;
ResumeFile *sf;
	checksum = next_arg(args, &args);
	filesize = my_atol(next_arg(args, &args));
	for (sf = resume_struct; sf; sf = sf->next)
	{
		if (!strcmp(checksum, sf->checksum) && (filesize == sf->filesize))
		{
			FileStruct *new;
			int count = 1;
			for (new = sf->results; new; new = new->next)
				print_file(new, count++);
		}
	}
	return 0;
}

BUILT_IN_DLL(nap_request)
{
char *nick, *filen, *comm;

	if (!my_stricmp(command, "nrequest"))
	{
		nick = next_arg(args, &args);
		filen = new_next_arg(args, &args);
		if (nick && filen && *filen)
		{
			GetFile *new;
			do_hook(MODULE_LIST, "NAP REQUESTFILE %s %s", nick, filen);
			send_ncommand(CMDS_REQUESTFILE, "%s \"%s\"", nick, filen);
			new = new_malloc(sizeof(GetFile));
			new->nick = m_strdup(nick);
			new->filename = m_strdup(filen);
			new->next = getfile_struct;
			getfile_struct = new;
		}
	}
	else if (!my_stricmp(command, "nget") || !my_stricmp(command, "nresume"))
	{
		int	i = 0, count = 1, resume = 0;
		FileStruct *sf = NULL;
		ResumeFile *rf = NULL;

		resume = !my_stricmp(command, "nresume") ? 1 : 0;
		while (args && *args)
		{
			int req, browse;

			sf = NULL;
			req = browse = 0;
			count = 1;
			comm = next_arg(args, &args);
			if (!my_strnicmp(comm, "-request", 3))
			{
				req = 1;
				comm = next_arg(args, &args);
			}
			else if (!my_strnicmp(comm, "-browse", 3))
			{
				browse = 1;
				comm = next_arg(args, &args);
			}

			if (comm && *comm)
				i = strtoul(comm, NULL, 10);
			if (!req && !browse)
			{
				if (file_search)
					sf = file_search;
				else
					sf = file_browse;
			} 
			else if (req)
				sf = file_search;
			else
				sf = file_browse;

			if (sf && i)
			{
				for (; sf; sf = sf->next, count++)
				{
					if (i == count)
					{
						GetFile *new;
						if (resume)
						{
							for (rf = resume_struct; rf; rf= rf->next)
							{
								if (!strcmp(rf->checksum, sf->checksum) && (sf->filesize == rf->filesize))
								{
									nap_say("Already a Resume request for %s", base_name(sf->name));
									return;
								}
							}
							rf = new_malloc(sizeof(ResumeFile));
							rf->checksum = m_strdup(sf->checksum);
							rf->filename = m_strdup(sf->name);
							rf->filesize = sf->filesize;
							rf->next = resume_struct;
							resume_struct = rf;
							send_ncommand(CMDS_REQUESTRESUME, "%s %lu", rf->checksum, rf->filesize);
							do_hook(MODULE_LIST, "NAP RESUMEREQUEST %s %lu %s", sf->checksum, rf->filesize, rf->filename);
							return;
						}
						do_hook(MODULE_LIST, "NAP REQUESTFILE %s %s", sf->nick, sf->name);
						send_ncommand(CMDS_REQUESTFILE, "%s \"%s\"", sf->nick, sf->name);
						new = new_malloc(sizeof(GetFile));
						new->nick = m_strdup(sf->nick);
						new->filename = m_strdup(sf->name);
						new->filesize = sf->filesize;
						new->checksum = m_strdup(sf->checksum);
						new->next = getfile_struct;
						getfile_struct = new;
						return;
					}
				}
			} 
			else if (sf)
			{
				for (; sf; sf = sf->next)
					print_file(sf, count++);
				return;
			}
		}
		if (file_search)
		{
			for (sf = file_search; sf; sf = sf->next)
				print_file(sf, count++);
		} else if (file_browse)
			for (sf = file_browse; sf; sf = sf->next)
				print_file(sf, count++);
	}
}

GetFile *find_in_getfile(GetFile **fn, int remove, char *nick, char *check, char *file, int speed, int up)
{
GetFile *last, *tmp;
	last = NULL;
	if (!nick)
		return NULL;
	for (tmp = *fn; tmp; tmp = tmp->next)
	{
		if (!my_stricmp(tmp->nick, nick))
		{
			if (check && my_stricmp(tmp->checksum, check))
			{
				last = tmp;
				continue;
			}
			if (file && my_stricmp(tmp->filename, file))
			{
				last = tmp;
				continue;
			}
			if ((speed != -1) && (tmp->speed != speed))
			{
				last = tmp;
				continue;
			}
			if ((up != -1) && ((tmp->up & ~NAP_QUEUED) != up))
			{
				last = tmp;
				continue;
			}
			if (remove)
			{
				if (last)
					last->next = tmp->next;
				else
					*fn = tmp->next;
			}
			return tmp;
		}
		last = tmp;
	}
	return NULL;
}

void getfile_cleanup(int snum)
{
SocketList *s;
	if ((s = get_socket(snum)) && s->info)
	{
		GetFile *f = (GetFile *)s->info;
		if ((f = find_in_getfile(&getfile_struct, 1, f->nick, f->checksum, f->filename, -1, NAP_DOWNLOAD)))
		{
			new_free(&f->nick);
			new_free(&f->filename);
			new_free(&f->realfile);
			new_free(&f->ip);
			new_free(&f->checksum);
			if (f->write > 0)
				close(f->write);
			new_free(&f);
		}
		s->info = NULL;
	}
	close_socketread(snum);
	build_napster_status(NULL);
	return;
}

void nap_getfile(int snum)
{
char indata[2*NAP_BUFFER_SIZE+1];
SocketList *s;
int rc;
int count = sizeof(indata) - 1;
GetFile *gf;
unsigned long nbytes = 0;

	s = get_socket(snum);
	gf = (GetFile *)get_socketinfo(snum);
	if (gf && gf->count)
	{
		int flags = O_WRONLY;
		memset(&indata, 0, 200);
		if ((rc = read(snum, &indata, gf->count)) != gf->count)
			return;
		if (!isdigit(*indata) || !*(indata+1) || !isdigit(*(indata+1)))
		{
			rc += read(snum, &indata[gf->count], sizeof(indata)-1);
			indata[rc] = 0;
			nap_say("Request from %s is %s", gf->nick, indata);
			gf = find_in_getfile(&getfile_struct, 1, gf->nick, gf->checksum, gf->filename, -1, NAP_DOWNLOAD);
			nap_finished_file(snum, gf);
			return;
		}
		gf->count = 0;
		set_non_blocking(snum);
		gf->starttime = time(NULL);
		if (!gf->resume)
			flags |= O_CREAT;
		if (!gf->realfile || ((gf->write = open(gf->realfile, flags, 0666)) == -1))
		{
			nap_say("Error opening output file %s: %s\n", base_name(gf->realfile), strerror(errno));
			gf = find_in_getfile(&getfile_struct, 1, gf->nick, gf->checksum, gf->filename, -1, NAP_DOWNLOAD);
			nap_finished_file(snum, gf);
			return;
		}
		if (gf->resume)
			lseek(gf->write, gf->resume, SEEK_SET);	
		if (do_hook(MODULE_LIST, "NAP GETFILE %sING %s %lu %s",gf->resume ? "RESUM": "GETT", gf->nick, gf->filesize, gf->filename)) 
		{
			sprintf(indata, "%4.2g%s %4.2g%s", _GMKv(gf->resume), _GMKs(gf->resume), _GMKv(gf->filesize), _GMKs(gf->filesize));
			nap_say("%s", cparse(gf->resume ?"$0ing from $1 $2/$3 [$4-]":"$0ing from $1 $3 [$4-]", "%s %s %s %s", gf->resume?"Resum":"Gett", gf->nick, indata, base_name(gf->filename)));
		}
		add_sockettimeout(snum, 0, NULL);
		send_ncommand(CMDS_UPDATE_GET1, NULL);
		build_napster_status(NULL);
		return;
	} 
	else if (!gf)
	{
		s->is_write = 0;
		close_socketread(snum);
		send_ncommand(CMDS_UPDATE_GET, NULL);
		return;
	}
        if ((rc = ioctl(snum, FIONREAD, &nbytes) != -1))
	{
		if (nbytes)
		{
			count = (nbytes > count) ? count : nbytes;
			rc = read(snum, indata, count);
		} else
			rc = 0;
	}
	switch (rc)
	{
		case -1:
			nap_say("ERROR reading file [%s]", strerror(errno));
		case 0:
		{
			if (gf && (gf = find_in_getfile(&getfile_struct, 1, gf->nick, gf->checksum, gf->filename, -1, NAP_DOWNLOAD)))
			{
				char speed1[80];
				double speed;
				speed = gf->received / 1024.0 / (now - gf->starttime);
				sprintf(speed1, "%4.2fK/s", speed);
				if ((gf->received + gf->resume) >= gf->filesize)
				{
					char rs[60];
					sprintf(rs, "%4.2g%s", _GMKv(gf->filesize), _GMKs(gf->filesize));
					if (rc != -1 && do_hook(MODULE_LIST, "NAP GETFILE FINISH %sING %s %s %lu %s", gf->resume ? "RESUM":"GETT", gf->nick, speed1, gf->received + gf->resume, gf->filename))
						nap_say("%s", cparse("Finished $2ing [$3-] from $0 $1", "%s %s %s %s %s", gf->nick, speed1, gf->resume ? "Resum":"Gett", rs, base_name(gf->filename)));
					if (speed > statistics.max_downloadspeed)
						statistics.max_downloadspeed = speed;
					statistics.files_received++;
					statistics.filesize_received += gf->received;
				}
				else if (do_hook(MODULE_LIST, "NAP GETFILE ERROR %sING %s %s %lu %lu %s", gf->resume? "RESUM":"GETT", gf->nick, speed1, gf->filesize, gf->received + gf->resume, gf->filename))
				{
					char fs[60];
					char rs[60];
					sprintf(fs, "%4.2g%s", _GMKv(gf->filesize), _GMKs(gf->filesize));
					sprintf(rs, "%4.2g%s", _GMKv(gf->received), _GMKs(gf->received));
					nap_say("%s", cparse("Error $2ing [$3-] from $0 $1", "%s %s %s %s %s %s", gf->nick, speed1, gf->resume ? "Resum":"Gett", fs, rs, base_name(gf->filename)));
				}
			}
			send_ncommand(CMDS_UPDATE_GET, NULL);
			nap_finished_file(snum, gf);
			build_napster_status(NULL);
			return;
		}
		default:
			break;
	}
	write(gf->write, indata, rc);
	gf->received += rc;
	if ((gf->received+gf->resume) >= gf->filesize)
	{
		if ((gf = find_in_getfile(&getfile_struct, 1, 
			gf->nick, gf->checksum, gf->filename, -1, NAP_DOWNLOAD)))
		{
			char speed1[80];
			double speed;
			speed = gf->received / 1024.0 / (now - gf->starttime);
			sprintf(speed1, "%4.2fK/s", speed);
			if (speed > statistics.max_downloadspeed)
				statistics.max_downloadspeed = speed;
			if (do_hook(MODULE_LIST, "NAP GETFILE FINISH %sING %s %s %lu %s", gf->resume ? "RESUM":"GETT", gf->nick, speed1, gf->filesize, gf->filename))
			{
				char rs[60];
				sprintf(rs, "%4.2g%s", _GMKv(gf->filesize), _GMKs(gf->filesize));
				nap_say("%s", cparse("Finished $2ing [$3-] from $0 at $1", "%s %s %s %s %s", gf->nick, speed1, gf->resume?"Resum":"Gett", rs, base_name(gf->filename)));
			}
			statistics.files_received++;
			statistics.filesize_received += gf->received;
		}
		send_ncommand(CMDS_UPDATE_GET, NULL);
		nap_finished_file(snum, gf);
		build_napster_status(NULL);
	}
}

void nap_getfilestart(int snum)
{
SocketList *s;
int rc;
char c;
GetFile *gf;
	s = get_socket(snum);
	gf = (GetFile *)get_socketinfo(snum);
	if (gf)
	{
		set_blocking(snum);
		if ((rc = read(snum, &c, 1)) != 1)
			return;
		s->func_read = nap_getfile;
		return;
	}
	close_socketread(snum);
}


void nap_firewall_get(int snum)
{
char indata[2*NAP_BUFFER_SIZE+1];
int rc;
	memset(indata, 0, sizeof(indata));
	alarm(15);
	rc = recv(snum, indata, sizeof(indata)-1, 0);
	alarm(0);
	switch(rc)
	{
		case -1:
			close_socketread(snum);
			nap_say("ERROR in nap_firewall_get %s", strerror(errno));
		case 0:
			break;
		default:
		{
			char *args, *nick, *filename;
			unsigned long filesize;
			GetFile *gf;
			SocketList *s;

			s = get_socket(snum);
			if (!strncmp(indata, "FILE NOT", 8) || !strncmp(indata, "INVALID DATA", 10))
			{
				close_socketread(snum);
				return;
			}
			args = &indata[0];
			if (!(nick = next_arg(args, &args)))
			{
				close_socketread(snum);
				return;
			}
			filename = new_next_arg(args, &args);
			filesize = my_atol(next_arg(args, &args));
			if (!filename || !*filename || !filesize)
			{
				close_socketread(snum);
				return;
			}
			if ((gf = find_in_getfile(&getfile_struct, 0, nick, NULL, filename, -1, NAP_DOWNLOAD)))
			{
				int flags = O_WRONLY;
#ifndef NO_STRUCT_LINGER
				int len;
				struct linger	lin;
				len = sizeof(lin);
				lin.l_onoff = lin.l_linger = 1;
#endif

				gf->count = 0;
				set_non_blocking(snum);
				gf->starttime = time(NULL);
				gf->socket = snum;
				gf->filesize = filesize;
				if (!gf->resume)
					flags |= O_CREAT;
				if (!gf->realfile || ((gf->write = open(gf->realfile, flags, 0666)) == -1))
				{
					nap_say("Error opening output file %s: %s\n", base_name(gf->realfile), strerror(errno));
					gf = find_in_getfile(&getfile_struct, 1, gf->nick, gf->checksum, gf->filename, -1, NAP_DOWNLOAD);
					nap_finished_file(snum, gf);
					return;
				}
				if (gf->resume)
					lseek(gf->write, gf->resume, SEEK_SET);	
				sprintf(indata, "%lu", gf->resume);
				write(snum, indata, strlen(indata));
				if (do_hook(MODULE_LIST, "NAP GETFILE %sING %s %lu %s",gf->resume ? "RESUM": "GETT", gf->nick, gf->filesize, gf->filename)) 
				{
					sprintf(indata, "%4.2g%s %4.2g%s", _GMKv(gf->resume), _GMKs(gf->resume), _GMKv(gf->filesize), _GMKs(gf->filesize));
					nap_say("%s", cparse("$0ing from $1 $3 [$4-]", "%s %s %s %s", gf->resume?"Resum":"Gett", gf->nick, indata, base_name(gf->filename)));
				}
				add_sockettimeout(snum, 0, NULL);
				send_ncommand(CMDS_UPDATE_GET1, NULL);
				build_napster_status(NULL);
				s->func_read = nap_getfile;
				set_socketinfo(snum, gf);
#ifndef NO_STRUCT_LINGER
				setsockopt(snum, SOL_SOCKET, SO_LINGER, (char *)&lin, len);
#endif
			}
		}
	}
}

NAP_COMM(cmd_getfileinfo)
{
char *nick;
GetFile *gf;
char indata[2*NAP_BUFFER_SIZE+1];
int count;
	nick = next_arg(args, &args);
	count = my_atol(args);
	if ((gf = find_in_getfile(&getfile_struct, 0, nick, NULL, NULL, count, NAP_DOWNLOAD)))
	{
		sprintf(indata, "%lu", gf->filesize);
		gf->count = strlen(indata);
		write(gf->socket, "GET", 3);
	        snprintf(indata, sizeof(indata), "%s \"%s\" %lu", get_dllstring_var("napster_user"), gf->filename, gf->resume);
		write(gf->socket, indata, strlen(indata));
		add_socketread(gf->socket, gf->port, gf->write, gf->nick, nap_getfilestart, NULL);
		set_socketinfo(gf->socket, gf);
		add_sockettimeout(gf->socket, 180, getfile_cleanup);
	}
	return 0;	
}

NAP_COMM(cmd_getfile)
{
/*
gato242 3068149784 6699 "d:\mp3\Hackers_-_07_-_Orbital_-_Halcyon_&_On_&_On.mp3"
8b451240c17fec98ea4f63e26bd42c60 7
*/
unsigned short port;
int getfd = -1;
int speed;
char *nick, *file, *checksum, *ip, *dir = NULL;
char *realfile = NULL;
char indata[2*NAP_BUFFER_SIZE+1];
struct sockaddr_in socka;
GetFile *gf = NULL;
struct stat st;
                
	nick = next_arg(args, &args);
	ip = next_arg(args, &args);
	port = my_atol(next_arg(args, &args));
	file = new_next_arg(args, &args);
	checksum = next_arg(args, &args);
	speed = my_atol(args);

	if (!(gf = find_in_getfile(&getfile_struct, 1, nick, checksum, file, -1, NAP_DOWNLOAD)))
	{
		nap_say("%s", "request not in getfile");
		return 0;
	}
	gf->ip = m_strdup(ip);	
	gf->checksum = m_strdup(checksum);
	gf->speed = atol(args);
	gf->port = port;

	if (!(dir = get_dllstring_var("napster_download_dir")))
		if (!(dir = get_string_var(DCC_DLDIR_VAR)))
			dir = "~";
	snprintf(indata, sizeof(indata), "%s/%s", dir, base_name(file));

	realfile = expand_twiddle(indata);

	gf->realfile = realfile;
	if (!(stat(realfile, &st)) && get_dllint_var("napster_resume_download"))
		gf->resume = st.st_size;

	gf->write = -1;

	if (!port) 
	{
		/* this is a firewalled host. make a listen socket instead */
		send_ncommand(CMDS_REQUESTFILEFIRE, "%s \"%s\"", nick, file);
		nap_say("Attempting to get from a firewalled host");
	}
	else
	{
#ifndef NO_STRUCT_LINGER
		int len;
		struct linger	lin;
		len = sizeof(lin);
		lin.l_onoff = lin.l_linger = 1;
#endif
		
		getfd = socket(AF_INET, SOCK_STREAM, 0);
		socka.sin_addr.s_addr = strtoul(ip, NULL, 10);
		socka.sin_family = AF_INET;
		socka.sin_port = htons(port);
		alarm(get_int_var(CONNECT_TIMEOUT_VAR));
		if (connect(getfd, (struct sockaddr *)&socka, sizeof(struct sockaddr)) != 0) 
		{
			nap_say("ERROR connecting [%s]", strerror(errno));
			send_ncommand(CMDR_DATAPORTERROR, gf->nick);
			new_free(&gf->nick);
			new_free(&gf->filename);
			new_free(&gf->ip);
			new_free(&gf->checksum);
			new_free(&gf->realfile);
			new_free(&gf);
			return 0;
		}
		alarm(0);
#ifndef NO_STRUCT_LINGER
		setsockopt(getfd, SOL_SOCKET, SO_LINGER, (char *)&lin, len);
#endif
		send_ncommand(CMDS_REQUESTINFO, nick);
	}

	gf->socket = getfd;
	gf->next = getfile_struct;
	gf->up = NAP_DOWNLOAD;
	getfile_struct = gf;
	return 0;
}

NAP_COMM(cmd_send_limit_msg)
{
char *nick, *filename, *limit, *filesize;
GetFile *gf;
	nick = next_arg(args, &args);
	filename = new_next_arg(args, &args);
	filesize = next_arg(args, &args);
	limit = args;
	if (!(gf = find_in_getfile(&getfile_struct, 1, nick, NULL, filename, -1, NAP_DOWNLOAD)))
	{
		nap_say("%s %s[%s]", "request not in getfile", nick, filename);
		return 0;
	}
/*	nap_finished_file(gf->socket, gf);*/
	gf->up &= NAP_QUEUED;
	if (do_hook(MODULE_LIST, "NAP QUEUE FULL %s %s %s %s", nick, filesize, limit, filename))
		nap_say("%s", cparse("$0 send queue[$1] is full.", "%s %s %s", nick, limit, filename));
	return 0;
	
}
