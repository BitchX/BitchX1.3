/*
napster code base by Drago (drago@0x00.org)
released: 11-30-99
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(void) {
	char **argv;

printf("%d\n", getargs(strdup("buddhempseed 1916592664 6699 \"E:\\New
Folder\\mp3\\Korn - Twist Chi.mp3\" 7cf872d13eed1822b840144d77dc5329 7"),
&argv));

}

int getargs(char *s, char ***sargs) {
	char *orig;
	int escape=0;
	char *beginword;
	char *endword;
	char **args=NULL;
	int argc=0;
	int myidx=0;

	beginword=s;
	endword=NULL;

	while (1) {
		switch(*s) {
			case '"':
				escape^=1;
			break;

			case ' ':
				if (!escape)
					endword=s;
			break;
		}
		s++;
		if (*s=='\0') endword=s;
		if (endword) {
			int len=endword-beginword;
			char tmp[len+1];
			strncpy(tmp, beginword, len);
			tmp[len]=0;
			endword=NULL;
			beginword=s;
			myidx=argc;
			argc++;
			args=(char **) realloc(args, sizeof(char *) * argc);
			assert(args!=NULL);
			args[myidx]=strdup(tmp);
		}
		if (!*s) break;
	}
	myidx=argc;
	argc++;
	args=(char **) realloc(args, sizeof(char *) * argc);
	assert(args!=NULL);
	args[myidx]=NULL;
	*sargs=args;
	return argc-1;
}

