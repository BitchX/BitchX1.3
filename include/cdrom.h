#ifndef __cdrom_h_
#define __cdrom_h_

#ifndef __FreeBSD__
#include <mntent.h>
#else
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#include <sys/file.h>
#include <sys/cdio.h>
#endif /* __FreeBSD__ */


#include <sys/ioctl.h>
#include <signal.h>
#include <fcntl.h>

#ifndef __FreeBSD__
#include <linux/cdrom.h>
#include <linux/errno.h>
#endif

#ifndef MODULE_CDROM
extern void	set_cd_device (Window *, char *, int);
extern void cd_stop (char *, char *, char *, char *);
extern void cd_eject (char *, char *, char *, char *);
extern void cd_play (char *, char *, char *, char *);
extern void cd_list (char *, char *, char *, char *);
extern void cd_volume (char *, char *, char *, char *);
extern void cd_pause (char *, char *, char *, char *);
extern void cd_help (char *, char *, char *, char *);
#endif

#ifndef __FreeBSD__
struct cdrom_etocentry 
{
	u_char	cdte_track;
	u_char	cdte_adr	:4;
	u_char	cdte_ctrl	:4;
	u_char	cdte_format;
	union cdrom_addr cdte_addr;
	u_char	cdte_datamode;
	int avoid;
	int length;
	int m_length;
	int m_start;
};
#else

struct cdrom_etocentry
{
	u_char m_length;
	u_char m_start;
	int avoid;
};
	
#define CDROMSTOP CDIOCSTOP
#define CDROMEJECT CDIOCEJECT
#define CDROMREADTOCHDR CDIOREADTOCHEADER
#define CDROMVOLCTRL CDIOCSETVOL
#define CDROMPAUSE CDIOCPAUSE
#define CDROMRESUME CDIOCRESUME
#define CDROMVOLREAD CDIOCGETVOL
#endif /* __FreeBSD__ */

#endif /* cdrom.h */
