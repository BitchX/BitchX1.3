/*
 * cdrom.c: This file handles all the CDROM routines, in BitchX
 *
 * Written by Tom Zickel
 * a.k.a. IceBreak on the irc
 *
 * Copyright(c) 1996
 * Modified Colten Edwards aka panasync.
 *
 */
#ifdef __linux__
#define CDROM_VERSION "0.02"

#include "irc.h"
#include "struct.h"
#include "ircaux.h"
#define MODULE_CDROM
#include "cdrom.h"
#include "output.h"
#include "misc.h"
#include "vars.h"

#include "module.h"
#define INIT_MODULE
#include "modval.h"

#define cparse(s) convert_output_format(s, NULL, NULL)

static int drive = 0;

static char cdrom_prompt[]="%gC%Gd%gROM%w";

#ifndef __FreeBSD__
static struct cdrom_tochdr hdr;
static struct cdrom_ti ti;
#else
static struct ioc_toc_header hdr;
#include <sys/disklabel.h>
#define MOUNT_CD9660 FS_ISO9660
#endif

static struct cdrom_etocentry TocEntry[101];

static char *cd_device = NULL;

static int cd_init(char *);

#ifndef __FreeBSD__
void play_chunk(int start, int end)
{
	struct cdrom_msf msf;

	end--;
	if (start >= end)
		start = end-1;

	msf.cdmsf_min0 = start / (60*75);
	msf.cdmsf_sec0 = (start % (60*75)) / 75;
	msf.cdmsf_frame0 = start % 75;
	msf.cdmsf_min1 = end / (60*75);
	msf.cdmsf_sec1 = (end % (60*75)) / 75;
	msf.cdmsf_frame1 = end % 75;

	if (ioctl(drive, CDROMSTART))
	{
		put_it("%s: Could not start the cdrom",cparse(cdrom_prompt));
		return;
	}
	if (ioctl(drive, CDROMPLAYMSF, &msf))
	{
		put_it("%s: Could not play the track",cparse(cdrom_prompt));
		return;
	}
}
#endif

static int check_cdrom_str(void)
{
	if (cd_device)
		return 1;
	put_it("%s: /CDDEVICE  - The name of the CDROM device",cparse(cdrom_prompt));
	return 0;
}

#if 0
static void lba2msf(int lba, unsigned char *msf)
{
#if !defined(CD_BLOCK_OFFSET)
#define CD_BLOCK_OFFSET CD_MSF_OFFSET
#endif
	lba += CD_BLOCK_OFFSET;
	msf[0] = lba / (CD_SECS*CD_FRAMES);
	lba %= CD_SECS*CD_FRAMES;
	msf[1] = lba / CD_FRAMES;
	msf[2] = lba % CD_FRAMES;
}
#endif

int cd_init(char *dev)
{
int i, rc, pos;

	if (!dev || ((drive = open(dev, O_RDONLY)) < 0)) 
		return (-1);

	if ((rc = ioctl(drive, CDROMREADTOCHDR, &hdr)) == -1)
	{
		put_it("%s: can't get TocHeader (error %d).",cparse(cdrom_prompt), rc);
		return (-2);
	}

#ifndef __FreeBSD__
	for (i=1;i<=hdr.cdth_trk1+1;i++)
	{
		if (i!=hdr.cdth_trk1+1) 
			TocEntry[i].cdte_track = i;
		else 
			TocEntry[i].cdte_track = CDROM_LEADOUT;
		TocEntry[i].cdte_format = CDROM_MSF;
		if (ioctl(drive,CDROMREADTOCENTRY,&TocEntry[i]))
			put_it("%s: Can't get TocEntry #%d",cparse(cdrom_prompt), i);
		else
		{
			TocEntry[i].avoid=TocEntry[i].cdte_ctrl & CDROM_DATA_TRACK ? 1 : 0;
			TocEntry[i].m_length = TocEntry[i].cdte_addr.msf.minute * 60 + TocEntry[i].cdte_addr.msf.second;
			TocEntry[i].m_start = TocEntry[i].m_length * 75 + TocEntry[i].cdte_addr.msf.frame;
		}
	}

	pos = TocEntry[1].m_length;

	for (i=1;i<=hdr.cdth_trk1+1;i++)
	{
		TocEntry[i].m_length = TocEntry[i+1].m_length - pos;
		pos = TocEntry[i+1].m_length;
		if (TocEntry[i].avoid)
			TocEntry[i].m_length = (TocEntry[i+1].m_start - TocEntry[i+1].m_start) *2;
	}
	return (hdr.cdth_trk1);
#else
	for (i = hdr.starting_track; i <= hdr.ending_track; i++)
	{
		TocEntry[i].avoid=0;
		TocEntry[i].m_start=1;
		TocEntry[i].m_length=1;
	}
	return (hdr.ending_track);
#endif
}

static int check_mount(char *device)
{
#ifndef __FreeBSD__
FILE *fp;
struct mntent *mnt;

	if ((fp = setmntent(MOUNTED, "r")) == NULL)
		return 0;
	
	while ((mnt = getmntent (fp)) != NULL)
	{
		if (!strcmp (mnt->mnt_type, "iso9660") && !strcmp (mnt->mnt_fsname, device))
		{
			endmntent(fp);
			return 0;
		}
	}
	endmntent (fp);
#else
struct statfs *mntinfo;
int i,count;

	if (!(count = getmntinfo(&mntinfo,MNT_WAIT|MOUNT_CD9660)))
		return 0;
	
	for(i = 0; i < count; i++)
		if (strstr(mntinfo[i].f_mntfromname,device) && !stricmp(mntinfo[i].f_dstypename, "iso9660"))
			return 0;
#endif
	return 1;
}

BUILT_IN_DLL(set_cd_device)
{
char *str;
int code;
	if (!(str = next_arg(args , &args)))
	{
		return;
	}
	if (drive) 
  		close(drive);
	if (!str || !check_mount(str))
	{
		put_it("%s: ERROR: CDROM is already mounted, please unmount, and try again",cparse(cdrom_prompt));
		new_free(&cd_device);
		return;
	}

	if ((code = cd_init(str)) < 0)
	{
		put_it("%s: ERROR(%d): Could not initalize the CDROM, check if a disk is inside",cparse(cdrom_prompt), code);
		new_free(&cd_device);
		return;
	}
	put_it("%s: CDROM device is now set to - %s",cparse(cdrom_prompt),str);
	malloc_strcpy(&cd_device, str);
}

BUILT_IN_DLL(cd_stop)
{
	if (!check_cdrom_str())
		return;
	if (!ioctl(drive, CDROMSTOP))
		put_it("%s: Stopped playing cdrom",cparse(cdrom_prompt));
	else
		put_it("%s: Stopped playing cdrom",cparse(cdrom_prompt));
}

BUILT_IN_DLL(cd_eject)
{
	if (!check_cdrom_str() || !drive)
		return;

	if (!ioctl(drive,CDROMEJECT))
		put_it("%s: ejected cdrom tray",cparse(cdrom_prompt));
	else
		put_it("%s: Stopped playing cdrom",cparse(cdrom_prompt));
	close(drive);
	drive=0;
}

BUILT_IN_DLL(cd_play)
{

int tn;
char *trackn;
#ifndef __FreeBSD__
unsigned char first, last;
struct cdrom_tochdr tocHdr;
#else
struct ioc_play_track cdrom_play_args;
int result;
#endif
	
	if (!check_cdrom_str() || !drive)
		return;
	
	if (args && *args)
	{
		trackn=next_arg(args, &args);
		tn=atoi(trackn);

#ifndef __FreeBSD__
		if ((ioctl(drive,CDROMREADTOCHDR,&tocHdr)))
		{
	        	put_it("%s: Couldnt get cdrom heder",cparse(cdrom_prompt));
	        	return;
		}

	        first = tocHdr.cdth_trk0;
	        last = tocHdr.cdth_trk1;
	        ti.cdti_trk0=tn;

	        if (ti.cdti_trk0<first) 
	        	ti.cdti_trk0=first;
	        if (ti.cdti_trk0>last) 
	        	ti.cdti_trk0=last;

	        ti.cdti_ind0=0;
	        ti.cdti_trk1=last;
	        ti.cdti_ind1=0;
#else
		if (tn < hdr.starting_track)
			tn=hdr.starting_track;
		if (tn > hdr.ending_track)
 			tn=hdr.ending_track;
#endif
	        if (TocEntry[tn].avoid==0)
	        {
#ifndef __FreeBSD__
			play_chunk(TocEntry[tn].m_start,TocEntry[last+1].m_start - 1);
#else
			cdrom_play_args.start_track=tn;
			cdrom_play_args.start_index=1;
			cdrom_play_args.end_track=hdr.ending_track;
			cdrom_play_args.end_index=1;
			(void)ioctl(drive,CDIOCPLAYTRACKS,&cdrom_play_args);
#endif
		        put_it("%s: Playing track number #%d",cparse(cdrom_prompt),tn);
	        }
	        else
	        	put_it("%s: Cannot play track #%d (Might be data track)",cparse(cdrom_prompt),tn);
	}
        else
	        put_it("%s: Usage: /cdplay <track number>",cparse(cdrom_prompt));

}

BUILT_IN_DLL(cd_list)
{
int i;

	if (!check_cdrom_str())
		return;
#ifndef __FreeBSD__
	for (i=1;i<=hdr.cdth_trk1;i++)
#else
	for (i = hdr.starting_track; i < hdr.ending_track; i++)
#endif
	{
		put_it("%s: Track #%02d: %02d:%02d:%02d %02d:%02d:%02d",
			cparse(cdrom_prompt),
			i,
			TocEntry[i].m_length / (60*75),
			(TocEntry[i].m_length % (60*75)) / 75,
			TocEntry[i].m_length % 75,
			TocEntry[i].m_start / (60*75),
			(TocEntry[i].m_start % (60*75)) /75,
			TocEntry[i].m_start % 75
			);
	}
}

BUILT_IN_DLL(cd_volume)
{
char *left, *right;
#ifndef __FreeBSD__
struct cdrom_volctrl volctrl;
#else
struct ioc_vol volctrl;
#endif
	if (!check_cdrom_str())
		return;

	if (args && *args)
	{
		left=next_arg(args, &args);
		right=next_arg(args, &args);
		ioctl(drive, CDROMVOLREAD, &volctrl);
		if (left && *left)
#ifndef __FreeBSD__
			volctrl.channel0 = atoi(left);
#else
			volctrl.vol[0] = atoi(left);
#endif
		if (right && *right)
#ifndef __FreeBSD__
			volctrl.channel1 = atoi(right);
#else
			volctrl.vol[1] = atoi(right);
#endif
		if (ioctl(drive,CDROMVOLCTRL,&volctrl))
			put_it("%s: Couldnt set cdrom volume",cparse(cdrom_prompt));
		else
			put_it("%s: CDROM Volume is now <%d> <%d>",cparse(cdrom_prompt),
#ifndef __FreeBSD__			
				volctrl.channel0,volctrl.channel1);
#else
				volctrl.vol[0],volctrl.vol[1]);
#endif
	}
	else
		put_it("%s: Usage: /cdvol <left> <right>",cparse(cdrom_prompt));
}

BUILT_IN_DLL(cd_pause)
{
static int cpause = 0;
	if (!check_cdrom_str())
		return;
	if (ioctl(drive, !cpause?CDROMPAUSE:CDROMRESUME))
		put_it("%s: Couldnt pause/resume your cdrom",cparse(cdrom_prompt));
	else
		put_it("%s: %s",cparse(cdrom_prompt),!cpause?"Your cdrom has been paused":"Your cdrom has been resumed");
	cpause ^= 1;
}

BUILT_IN_DLL(cd_help)
{
	put_it("%s: CDPLAY            - Play a CDROM Track Number",cparse(cdrom_prompt));
	put_it("%s: CDSTOP            - Make the CDROM Stop playing",cparse(cdrom_prompt));
	put_it("%s: CDEJECT           - Eject the CDROM Tray",cparse(cdrom_prompt));
	put_it("%s: CDVOL             - Set's the CDROM Volume",cparse(cdrom_prompt));
	put_it("%s: CDLIST            - List of CDROM tracks",cparse(cdrom_prompt));
	put_it("%s: CDPAUSE           - Pause/resume the CDROM",cparse(cdrom_prompt));
}

char *Cdrom_Version (IrcCommandDll **interp)
{
	return CDROM_VERSION;
}

int Cdrom_Init(IrcCommandDll **interp, Function_ptr *global_table)
{
char *name = "cdrom";
	initialize_module(name);
	add_module_proc(COMMAND_PROC, name, "cdstop", NULL, 0, 0, cd_stop, NULL);
	add_module_proc(COMMAND_PROC, name, "cdplay", NULL, 0, 0, cd_play, NULL);
	add_module_proc(COMMAND_PROC, name, "cdeject", NULL, 0, 0, cd_eject, NULL);
	add_module_proc(COMMAND_PROC, name, "cdlist", NULL, 0, 0, cd_list, NULL);
	add_module_proc(COMMAND_PROC, name, "cdhelp", NULL, 0, 0, cd_help, NULL);
	add_module_proc(COMMAND_PROC, name, "cdvolume", NULL, 0, 0, cd_volume, NULL);
	add_module_proc(COMMAND_PROC, name, "cdpause", NULL, 0, 0, cd_pause, NULL);
	add_module_proc(COMMAND_PROC, name, "cddevice", NULL, 0, 0, set_cd_device, NULL);
	put_it("%s: Module loaded and ready. /cddevice <dev> to start", cparse(cdrom_prompt));
	put_it("%s: /cdhelp for list of new commands.", cparse(cdrom_prompt));
	return 0;
}
#endif
