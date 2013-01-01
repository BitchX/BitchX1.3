/*
 * hebrew.c 
 * by crisk
 * 
 * MS-Windows like Hebrew process routine
 *
 * This first started as an ircII script.. the function was 
 * ported almost 1 to 1 from the script, so you can excuse
 * the extreme mess.
 * 
 */

#include "irc.h"
#include "ircaux.h"
#define MAIN_SOURCE
#include "modval.h"

#ifdef WANT_HEBREW
#include <stdio.h>
#include <string.h>

unsigned char *heb_reverse(char translate, unsigned char *str)
{
   unsigned char tmp1,tmp2;
   int ind,len,transind;
   unsigned char *transloc;
   
   unsigned char *transfrom = "()[]{}";
   unsigned char *transto   = ")(][}{";
   
   len = strlen(str)-1; 
   
   for (ind=0;ind<((len+1)/2);ind++)
     {
	tmp1 = *(str+ind);
	tmp2 = *(str+len-ind);
	
	if (translate)
	  {
	     transloc = index(transfrom,tmp1);
	     if (transloc != NULL) 
	       {
		  transind = transloc-transfrom;
		  tmp1 = *(transto+transind);
	       }
	   
	     transloc = index(transfrom,tmp2);
	     if (transloc != NULL) 
	       {
		  transind = transloc-transfrom;
		  tmp2 = *(transto+transind);
	       }
	  }
	*(str+ind) = tmp2;
	*(str+len-ind) = tmp1;
     }
   
   if (translate)
   if (len % 2) 
     {
	ind = (len % 2)+1;
	transloc = index(transfrom,*(str+ind));
	if (transloc != NULL) 
	  {
	     transind = transloc-transfrom;
	     *(str+ind) = *(transto+transind);
	  }
     }	
   
   return str;
}

unsigned char *hebrew_process(unsigned char *str)
{
   
#define hebrew_isheb(x) ((x >= 224) && (x <= 256))
#define hebrew_iseng(x) (((x >= 'a') && (x <= 'z')) || ((x >= 'A') && (x <= 'Z')))   
#define hebrew_isnum(x) ((x >= '0') && (x <= '9'))   
  
   unsigned char *engspecial = "";
   unsigned char *numrelated = "+-/.,";
   
   int len;
   
   unsigned char *result;
   unsigned char let;
   unsigned char *tmpstr; 
   unsigned char *tmpbuf;
   int pos;
   int p;
   int mode;
   int pnum;
   int pnumplus;
   int ppp;
   int j,j2;
   
   if (!str || !*str)
   	return empty_string;
   len = strlen(str);
   
   result = (unsigned char *)new_malloc(len+2);
   tmpstr = (unsigned char *)new_malloc(len*2);
   tmpbuf = (unsigned char *)new_malloc(len*2);
   
   pos = 0;
   p   = 0;
   pnum = 0;
   mode = 0;
   
   while (pos < len)
     {
	let = str[pos];
	pos++;
	tmpstr[0] = let;
	tmpstr[1] = '\0';

	if ((hebrew_iseng(let)) || (index(engspecial,let) != NULL))
	  {
	     
	     strcat(tmpstr,tmpbuf);
	     
	     strcat(tmpstr,result);
	     strcpy(result,tmpstr);
	     tmpbuf[0] = '\0';
	     tmpstr[1] = '\0';
	     
	     p = 0;
	     mode = 1;
	     pnum = 0;
	  } else if (hebrew_isheb(let))
	  {
	     if (mode)
	       {
		  strcat(tmpbuf,result);
		  strcpy(result,tmpbuf);
	       }
	     else
	       {
		  tmpbuf = heb_reverse(1,tmpbuf);
		  strcat(tmpbuf,tmpstr);
		  strcpy(tmpstr,tmpbuf);
	       }
	     
	     mode = 0;
	     
	     for (j=0;j<p;j++)
	       *(tmpbuf+j) = *(result+j);
	     
	     for (j2=0;j2<strlen(tmpstr);j2++)
	       *(tmpbuf+j+j2) = *(tmpstr+j2);
	     
	     for (j=j;j<=strlen(result);j++)
	       *(tmpbuf+j+j2) = *(result+j);
	     
	     *(tmpbuf+j+j2) = '\0';
	     strcpy(result,tmpbuf);
	     tmpbuf[0] = '\0';
	     
	     p += strlen(tmpstr);
	     pnum = 0;
	  }
	else if (hebrew_isnum(let))
	  {
	     if (!mode)
	       {
		  tmpbuf = heb_reverse(1,tmpbuf);
		  pnumplus = 0;
		  
		  if (tmpbuf[0] != '\0')
		    {
		       if ((strlen(tmpbuf) == 1) && (index(numrelated,tmpbuf[0])))
			 {
			    strcat(tmpstr,tmpbuf);
			    tmpbuf[0] = '\0';
			    pnumplus = 1;
			 }
		       else
			 {
			    pnum = 0;
			 }
		    }
		  
		  strcat(tmpbuf,tmpstr);
		  strcpy(tmpstr,tmpbuf);

		  ppp = p-pnum;
		  
		  for (j=0;j<ppp;j++)
		    *(tmpbuf+j) = *(result+j);
		  
		  for (j2=0;j2<strlen(tmpstr);j2++)
		    *(tmpbuf+j+j2) = *(tmpstr+j2);
	     
		  for (j=j;j<=strlen(result);j++)
		    *(tmpbuf+j+j2) = *(result+j);
	
		  *(tmpbuf+j+j2) = '\0';
		  
		  strcpy(result,tmpbuf);
		  
		  tmpbuf[0] = '\0';
		  
		  p += strlen(tmpstr);
		  pnum++;
		  pnum += pnumplus; 
	       }
	     else
	       {
		  strcat(tmpstr,tmpbuf);
		  strcat(tmpstr,result);
		  strcpy(result,tmpstr);
		  tmpbuf[0]='\0';
		  p = 0;
	       }
	  }
	else
	  {
	     strcat(tmpstr,tmpbuf);
	     strcpy(tmpbuf,tmpstr);
	  }
     }

   mode = 1;                    /* MS-Hebrew behavior thing */
   
   if (mode)
     {
	strcat(tmpbuf,result);
	strcpy(result,tmpbuf);
     }
   else
     {
	tmpbuf = heb_reverse(1,tmpbuf);
	strcpy(tmpstr,tmpbuf);
	
	for (j=0;j<p;j++)
	  *(tmpbuf+j) = *(result+j);
	
	for (j2=0;j2<strlen(tmpstr);j2++)
	  *(tmpbuf+j+j2) = *(tmpstr+j2);
	
	for (j=j;j<=strlen(result);j++)
	  *(tmpbuf+j+j2) = *(result+j);
	
        strcpy(result,tmpbuf);
     }
   
   result = heb_reverse(0,result);		  

   strcpy(str,result);
   
   new_free(&result);
   new_free(&tmpstr);
   new_free(&tmpbuf);
   
   return str;
   
}
#endif

