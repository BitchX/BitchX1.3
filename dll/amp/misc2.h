/* this file is a part of amp software, (C) tomislav uzelac 1996,1997
*/
 
/* misc2.h  
 *
 * Created by: tomislav uzelac  May 1996
 * Last modified by: tomislav uzelac Jan  8 1997
 */
#ifndef _MISC2_H_
#define _MISC2_H_
void requantize_mono(int, int, struct SIDE_INFO *, struct AUDIO_HEADER *);
void requantize_ms(int, struct SIDE_INFO *, struct AUDIO_HEADER *);
void requantize_downmix(int, struct SIDE_INFO *, struct AUDIO_HEADER *);

void alias_reduction(int);
void calculate_t43(void);

extern int no_of_imdcts[];

#ifdef MISC2

#define i_sq2   0.707106781188
#define IS_ILLEGAL 0xfeed

static inline float fras_l(int,int,int,int,int);
static inline float fras_s(int,int,int,int);
static inline float fras2(int,float);
static int find_isbound(int isbound[3],int gr,struct SIDE_INFO *info,struct AUDIO_HEADER *header);
static inline void stereo_s(int l,float a[2],int pos,int ms_flag,int is_pos,struct AUDIO_HEADER *header);
static inline void stereo_l(int l,float a[2],int ms_flag,int is_pos,struct AUDIO_HEADER *header);

#endif
#endif

