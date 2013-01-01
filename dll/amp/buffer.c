/* this file is a part of amp software

   buffer.c: written by Andrew Richards  <A.Richards@phys.canterbury.ac.nz>
             (except printout())

   Last modified by:
   Karl Anders Oygard added flushing of audio buffers, 13 May 1997

*/
#include "amp.h"
#include "transform.h"

#include <sys/types.h>
#include <sys/signal.h>
#ifdef HAVE_MLOCK
#ifdef OS_Linux
#include <sys/mman.h>
#endif
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audioIO.h"
#include "audio.h"

#if !defined(OS_Linux) && !defined(AUSIZ)
#define AUSIZ 32768
#endif

struct ringBuffer {		/* A ring buffer to store the data in */
	char *bufferPtr;	/* buffer pointer */
	int inPos, outPos;	/* positions for reading and writing */
};


static int buffer_fd;
static int control_fd;

/* little endian systems do not require any byte swapping whatsoever. 
 * big endian systems require byte swapping when writing .wav files, 
 * but not when playing directly
 */

/* This is a separate (but inlined) function to make it easier to implement */
/* a threaded buffer */

inline void audioBufferWrite(char *buf,int bytes)
{
        write(buffer_fd, buf, bytes);
}

void printout(void)
{
int j;

        if (nch==2)
                j=32 * 18 * 2;
        else
                j=32 * 18;

       if (AUDIO_BUFFER_SIZE==0)
               audioWrite((char*)sample_buffer, j * sizeof(short));
       else
               audioBufferWrite((char*)sample_buffer, j * sizeof(short));
}

int AUDIO_BUFFER_SIZE;

#define bufferSize(A) (((A)->inPos+AUDIO_BUFFER_SIZE-(A)->outPos) % AUDIO_BUFFER_SIZE)
#define bufferFree(A) (AUDIO_BUFFER_SIZE-1-bufferSize(A))

void 
initBuffer(struct ringBuffer *buffer)
{
	buffer->bufferPtr=malloc(AUDIO_BUFFER_SIZE);
	if (buffer->bufferPtr==NULL) 
		_exit(-1);
#ifdef HAVE_MLOCK
	mlock(buffer->bufferPtr,AUDIO_BUFFER_SIZE);
#endif
	buffer->inPos = 0;
	buffer->outPos = 0;
}

void
freeBuffer(struct ringBuffer *buffer)
{
#ifdef HAVE_MLOCK
  munlock(buffer->bufferPtr,AUDIO_BUFFER_SIZE);
#endif
	free(buffer->bufferPtr);
}

/* This just sends some bogus data on the control pipe, which will cause */
/* the reader to flush its buffer. */

void
audioBufferFlush()
{
	if (AUDIO_BUFFER_SIZE!=0) 
	{
		int dummy;

		/* We could use the control pipe for passing commands to the */
		/* audio player process, but so far we haven't bothered. */

        	write(control_fd, &dummy, sizeof dummy);
	} else
		audioFlush();
}


/* The decoded data are stored in a the ring buffer until they can be sent */
/* to the audio device. Variables are named in relation to the buffer */
/* ie writes are writes from the codec to the buffer and reads are reads */
/* from the buffer to the audio device */

int
audioBufferOpen(int frequency, int stereo, int volume)
{
	struct ringBuffer audioBuffer;
	
	int inFd,outFd,ctlFd,cnt,pid;
	int inputFinished=FALSE;
	int percentFull;
	fd_set inFdSet,outFdSet;
	fd_set *outFdPtr; 
	struct timeval timeout;
	int filedes[2];
	int controldes[2];
	
	
	if (pipe(filedes) || pipe(controldes)) 
	{
		perror("pipe");
		exit(-1);
	}
	if ((pid=fork())!=0) 
	{  
		/* if we are the parent */
		control_fd=controldes[1];
		close(filedes[0]);
		buffer_fd=filedes[1];
		close(controldes[0]);
		return(pid);	        /* return the pid */
	}
	
	
	/* we are the child */
	close(filedes[1]);
	inFd=filedes[0];
	close(controldes[1]);
	ctlFd=controldes[0];
	audioOpen(frequency,stereo,volume);
	outFd=getAudioFd();
	initBuffer(&audioBuffer);
	
	while(1) 
	{
		timeout.tv_sec=0;
		timeout.tv_usec=0;
		FD_ZERO(&inFdSet);
		FD_ZERO(&outFdSet);
		FD_SET(ctlFd,&inFdSet);
		FD_SET(outFd,&outFdSet);
		
		if (bufferSize(&audioBuffer)<AUSIZ) 
		{					/* is the buffer too empty */
			outFdPtr = NULL;		/* yes, don't try to write */
			if (inputFinished)		/* no more input, buffer exhausted -> exit */
				break;
		} else
			outFdPtr=&outFdSet;															/* no, select on write */
		
		/* check we have at least AUSIZ bytes left (don't want <1k bits) */
		if ((bufferFree(&audioBuffer)>=AUSIZ) && !inputFinished)
			FD_SET(inFd,&inFdSet);

/* The following selects() are basically all that is left of the system
   dependent code outside the audioIO_*&c files. These selects really
   need to be moved into the audioIO_*.c files and replaced with a
   function like audioIOReady(inFd, &checkIn, &checkAudio, wait) where
   it checks the status of the input or audio output if checkIn or
   checkAudio are set and returns with checkIn or checkAudio set to TRUE
   or FALSE depending on whether or not data is available. If wait is
   FALSE the function should return immediately, if wait is TRUE the
   process should BLOCK until the required condition is met. NB: The
   process MUST relinquish the CPU during this check or it will gobble
   up all the available CPU which sort of defeats the purpose of the
   buffer.

   This is tricky for people who don't have file descriptors (and
   select) to do the job. In that case a buffer implemented using
   threads should work. The way things are set up now a threaded version
   shouldn't be to hard to implement. When I get some time... */

		/* check if we can read or write */
		if (select(MAX3(inFd,outFd,ctlFd)+1,&inFdSet,outFdPtr,NULL,NULL) > -1) 
		{
			if (outFdPtr && FD_ISSET(outFd,outFdPtr)) 
			{							/* need to write */
				int bytesToEnd = AUDIO_BUFFER_SIZE - audioBuffer.outPos;

				percentFull=100*bufferSize(&audioBuffer)/AUDIO_BUFFER_SIZE;
				if (AUSIZ>bytesToEnd) 
				{
					cnt = audioWrite(audioBuffer.bufferPtr + audioBuffer.outPos, bytesToEnd);
					cnt += audioWrite(audioBuffer.bufferPtr, AUSIZ - bytesToEnd);
					audioBuffer.outPos = AUSIZ - bytesToEnd;
				} 
				else 
				{
					cnt = audioWrite(audioBuffer.bufferPtr + audioBuffer.outPos, AUSIZ);
					audioBuffer.outPos += AUSIZ;
				}

			}
			if (FD_ISSET(inFd,&inFdSet)) 
			{								 /* need to read */
			        cnt = read(inFd, audioBuffer.bufferPtr + audioBuffer.inPos, MIN(AUSIZ, AUDIO_BUFFER_SIZE - audioBuffer.inPos));
				if (cnt >= 0) 
				{
					audioBuffer.inPos = (audioBuffer.inPos + cnt) % AUDIO_BUFFER_SIZE;

					if (cnt==0)
						inputFinished=TRUE;
				} 
				else 
					_exit(-1);
			}
			if (FD_ISSET(ctlFd,&inFdSet)) 
			{
				int dummy;

			        cnt = read(ctlFd, &dummy, sizeof dummy);
				if (cnt >= 0) 
				{
					audioBuffer.inPos = audioBuffer.outPos = 0;
					audioFlush();
				} 
				else 
					_exit(-1);
			}
		} 
		else 
			_exit(-1);
	}
	close(inFd);
	audioClose();
	exit(0);
	return 0; /* just to get rid of warnings */
}

void
audioBufferClose()
{
	/* Close the buffer pipe and wait for the buffer process to close */
	close(buffer_fd);
	waitpid(0,0,0);
}
