/*
 * timer.h: header for timer.c 
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifndef _TIMER_H_
#define _TIMER_H_

/* functions that may be called by others */
extern	void	timercmd (char *, char *, char *, char *);
extern	void	ExecuteTimers (void);
extern	char	*BX_add_timer (int, char *, double, long, int (*) (void *, char *), char *, char *, int, char *);
extern	int	BX_delete_timer (char *);
extern	int	get_delete_timer(char *);
extern	int	kill_timer(char *);
extern	void	BX_delete_all_timers (void);
extern	int	timer_exists (char *ref);


extern	time_t	TimerTimeout (void);
char		*tcl_add_timer (TimerList **, long, char *, unsigned long);
int 		tcl_remove_timer (TimerList **, unsigned long);
int		timer_callback_exists(void *);

time_t		tclTimerTimeout(time_t);

#define MAGIC_TIMEOUT 100000000

#endif /* _TIMER_H_ */
