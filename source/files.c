/*
 * files.c -- allows you to read/write files. Wow.
 *
 * (C) 1995 Jeremy Nelson (ESL)
 * See the COPYRIGHT file for more information
 */

#include "irc.h"
static char cvsrevision[] = "$Id: files.c 3 2008-02-25 09:49:14Z keaston $";
CVS_REVISION(files_c)
#include "ircaux.h"
#define MAIN_SOURCE
#include "modval.h"

/* Here's the plan.
 *  You want to open a file.. you can READ it or you can WRITE it.
 *    unix files can be read/written, but its not the way you expect.
 *    so we will only alllow one or the other.  If you try to write to
 *    read only file, it punts, and if you try to read a writable file,
 *    you get a null.
 *
 * New functions: open(FILENAME <type>)
 *			<type> is 0 for read, 1 for write, 0 is default.
 *			Returns fd of opened file, -1 on error
 *		  read (fd)
 *			Returns line for given fd, as long as fd is
 *			opened via the open() call, -1 on error
 *		  write (fd text)
 *			Writes the text to the file pointed to by fd.
 *			Returns the number of bytes written, -1 on error
 *		  close (fd)
 *			closes file for given fd
 *			Returns 0 on OK, -1 on error
 *		  eof (fd)
 *			Returns 1 if fd is at EOF, 0 if not. -1 on error
 */

struct FILE___ {
	FILE *file;
	struct FILE___ *next;
};
typedef struct FILE___ File;

static File *FtopEntry = NULL;

File *new_file (void)
{
	File *tmp = FtopEntry;
	File *tmpfile = (File *)new_malloc(sizeof(File));

	if (FtopEntry == NULL)
		FtopEntry = tmpfile;
	else
	{
		while (tmp->next)
			tmp = tmp->next;
		tmp->next = tmpfile;
	}
	return tmpfile;
}

void remove_file (File *file)
{
	File *tmp = FtopEntry;

	if (file == FtopEntry)
		FtopEntry = file->next;
	else
	{
		while (tmp->next && tmp->next != file)
			tmp = tmp->next;
		if (tmp->next)
			tmp->next = tmp->next->next;
	}
	fclose(file->file);
	new_free((char **)&file);
}

	
int open_file_for_read (char *filename)
{
	char *dummy_filename = NULL;
	FILE *file;

	malloc_strcpy(&dummy_filename, filename);
	file = uzfopen(&dummy_filename, ".", 0);
	new_free(&dummy_filename);
	if (file)
	{
		File *nfs = new_file();
		nfs->file = file;
		nfs->next = NULL;
		return fileno(file);
	}
	else
		return -1;
}

int open_file_for_write (char *filename)
{
	/* patch by Scott H Kilau so expand_twiddle works */
	char *expand = NULL;
	FILE *file;

	if (!(expand = expand_twiddle(filename)))
		malloc_strcpy(&expand, filename);
	file = fopen(expand, "a");
	new_free(&expand);
	if (file)
	{
		File *nfs = new_file();
		nfs->file = file;
		nfs->next = NULL;
		return fileno(file);
	}
	else 
		return -1;
}

int open_file_for_bwrite (char *filename)
{
	/* patch by Scott H Kilau so expand_twiddle works */
	char *expand = NULL;
	FILE *file;

	if (!(expand = expand_twiddle(filename)))
		malloc_strcpy(&expand, filename);
	file = fopen(expand, "wb");
	new_free(&expand);
	if (file)
	{
		File *nfs = new_file();
		nfs->file = file;
		nfs->next = NULL;
		return fileno(file);
	}
	else 
		return -1;
}

File *lookup_file (int fd)
{
	File *ptr = FtopEntry;

	while (ptr)
	{
		if (fileno(ptr->file) == fd)
			return ptr;
		else
			ptr = ptr -> next;
	}
	return NULL;
}

int file_write (int fd, char *stuff)
{
	File *ptr = lookup_file(fd);
	int ret;
	if (!ptr)
		return -1;
	ret = fprintf(ptr->file, "%s\n", stuff);
	fflush(ptr->file);
	return ret;
}

int file_writeb (int fd, char *stuff)
{
	File *ptr = lookup_file(fd);
	int ret;
	if (!ptr)
		return -1;
	ret = fwrite(stuff, 1, strlen(stuff), ptr->file);
	fflush(ptr->file);
	return ret;
}

char *file_read (int fd)
{
	File *ptr = lookup_file(fd);
	if (!ptr)
		return m_strdup(empty_string);
	else
	{
		char blah[10240];
		if ((fgets(blah, 10239, ptr->file)))
			chop(blah, 1);
		else
			blah[0] = 0;
		return m_strdup(blah);
	}
}

char *file_readb (int fd, int numb)
{
	File *ptr = lookup_file(fd);
	if (!ptr)
		return m_strdup(empty_string);
	else
	{
		char *blah = (char *)new_malloc(numb+1);
		if ((fread(blah, 1, numb, ptr->file)))
			blah[numb] = 0;
		else
			blah[0] = 0;
		return blah;
	}
}


int	file_eof (int fd)
{
	File *ptr = lookup_file (fd);
	if (!ptr)
		return -1;
	else
		return feof(ptr->file);
}

int	file_close (int fd)
{
	File *ptr = lookup_file (fd);
	if (!ptr)
		return -1;
	else
		remove_file (ptr);
	return 0;
}

/* 
** by: Walter Bright via Usenet C newsgroup
**
** modified by: Bob Stout based on a recommendation by Ray Gardner
**
** There is no point in going to asm to get high speed file copies. Since it
** is inherently disk-bound, there is no sense (unless tiny code size is
** the goal). Here's a C version that you'll find is as fast as any asm code
** for files larger than a few bytes (the trick is to use large disk buffers):
*/

int file_copy(int from,int to)
{
	int bufsiz;
	if (from < 0)
		return 1;
	if (to < 0)
		return 1;
		
	if (!fork())
	{
	        /* Use the largest buffer we can get    */
		for (bufsiz = 0x4000; bufsiz >= 128; bufsiz >>= 1)
		{
			register char *buffer;
		
			buffer = (char *) malloc(bufsiz);
			if (buffer)
			{
				while (1)
				{
					register int n;

					n = read(from,buffer,bufsiz);
					if (n == -1)                /* if error             */
						break;
					if (n == 0)                 /* if end of file       */
					{   
						free(buffer);
						_exit(0);
					}
					if (n != write(to,buffer,(unsigned) n))
						break;
				}
				free(buffer);
				break;
			}
		}
		_exit(1);
	}
	return 0;
}

int	file_valid (int fd)
{
	if (lookup_file(fd))
		return 1;
	return 0;
}
  
