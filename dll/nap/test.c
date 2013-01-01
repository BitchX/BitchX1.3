
#include <stdio.h>
unsigned short len = 0;
unsigned char blah[3];

main()
{
int burp = 1;
	blah[2] = '\224';
	blah[3] = '\1';
	blah[0] = '\027';
	blah[1] = 0x0;
	memcpy(&len, blah, sizeof(len));
	printf("%d\n", len);
	memcpy(&len, blah+2, sizeof(len));
	printf("%d\n", len);
	printf("%d ", burp);	
	burp += 0xf0;
	printf("%d ", burp);	
	printf("and %d ", burp & ~0xf0);	
	printf("or %d \n", burp | 0xf0);	

}