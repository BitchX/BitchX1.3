/* The New routine for getting ips and interfaces for /hostname
   Phear MHacker ;)

 */

/* added rev dns support
   Phear sideshow ;)

 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#if defined(sun)
#include <sys/sockio.h>
#else
#include <sys/sysctl.h>
#endif
#include <sys/time.h>
#include <net/if.h>
#include <netinet/in.h>
#if !defined(linux)
#include <netinet/in_var.h>
#endif
#include <netdb.h>

void check_inter (char *, char **);
void usage (char **);

int main (int argc, char **argv)
{
  int s;
  char *buffer;
  struct ifconf ifc;
  char name[100];
  struct ifreq *ifptr, *end;
  struct ifreq ifr;
  int ifflags, selectflag = -1;
  int oldbufsize, bufsize = sizeof (struct ifreq);
  if(!argv[1])
{
  usage(argv);
  exit(0);
} 
 printf ("This program should print out valid hosts on all network devices\n");
  s = socket (AF_INET, SOCK_DGRAM, 0);
  if (s < 0)
    {
      perror ("ifconfig: socket");
      exit (1);
    }
  buffer = malloc (bufsize);
  ifc.ifc_len = bufsize;
  do
    {
      oldbufsize = ifc.ifc_len;
      bufsize += 1 + sizeof (struct ifreq);
      buffer = realloc ((void *) buffer, bufsize);
      ifc.ifc_len = bufsize;
      ifc.ifc_buf = buffer;
      if (ioctl (s, SIOCGIFCONF, (char *) &ifc) < 0)
	{
	  perror ("ifconfig (SIOCGIFCONF)");
	  exit (1);
	}
    }
  while (ifc.ifc_len > oldbufsize);
  ifflags = ifc.ifc_req->ifr_flags;
  end = (struct ifreq *) (ifc.ifc_buf + ifc.ifc_len);
  ifptr = ifc.ifc_req;
  while (ifptr < end)
    {
      sprintf (ifr.ifr_name, "%s", ifptr->ifr_name);
      sprintf (name, "%s", ifptr->ifr_name);
      close (s);
      check_inter (name, argv);
      ifptr++;
    }
return 0;
}

void 
check_inter (char *interface, char **argv)
{
  struct in_addr i;
  struct hostent *he;
  struct ifreq ifr;
  char rhost[256], fhost[30];
  int fd;
  char *ip;
  register int flags;
  ip = malloc (sizeof (ip));
  if ((fd = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
    return;
  strcpy (ifr.ifr_name, interface);

  if (ioctl (fd, SIOCGIFADDR, &ifr) < 0)
    {
      close (fd);
      return;
    }
  if (ioctl (fd, SIOCGIFFLAGS, &ifr) < 0)
    {
      close (fd);
      return;
    }
  ip = (char *) inet_ntoa (((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr);
  if (ifr.ifr_flags & IFF_UP)
    {
      i.s_addr = inet_addr (ip);
      he = gethostbyaddr ((char *) &i, sizeof (struct in_addr), AF_INET);
      bzero(rhost, sizeof(rhost));
      if (he != NULL)
	{
	  strncpy (rhost, he->h_name, 255);
	  he = gethostbyname (rhost);
	  if (he != NULL)
	    sprintf (fhost, "%u.%u.%u.%u", he->h_addr[0] & 0xff, he->h_addr[1] & 0xff, he->h_addr[2] & 0xff, he->h_addr[3] & 0xff);

	}
      if (rhost)
	{
	  if (strcasecmp (ip, fhost) == 0)
	    {
	      if ((strcmp ("-m", argv[1]) == 0) || (strcmp ("-a", argv[1]) == 0))
		{
		  printf ("Interface %s: %s %s (matching)\n", interface, ip, rhost);
		  bzero (rhost, sizeof (rhost));
		  bzero (fhost, sizeof (fhost));
		}
	    }
	  else if ((strcmp("-r", argv[1])==0) || (strcmp("-a", argv[1]) == 0))
	    {
	      printf ("Interface %s: %s %s\n", interface, ip, rhost);
	      bzero (rhost, sizeof (rhost));
	    }
	}
      else if ((strcmp("-i", argv[1]) ==0) || (strcmp("-a", argv[1]) == 0))
	printf ("Interface %s: %s\n", interface, ip);
    }
  close (fd);
}
 
void usage(char **argv)
{
printf("\nVirtual Host checking system by Warren Rees\n");
printf("Support for reverse dns and dns matching by Matt Watson\n\n");
printf("Usage: %s [-amri]\n\n", argv[0]);
printf("\t-a :  Show all ips\n");
printf("\t-m :  Show only ips & hostname with matching forward and reverse dns\n");
printf("\t-r :  Show only ips that have reverse dns but no forward dns\n");
printf("\t-i :  Show only non-resolving ips no forward or reverse dns\n\n\n");
}

