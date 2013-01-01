/*
napster code base by Drago (drago@0x00.org)
released: 11-30-99
*/

#include "napi.h"

void motdhook(char *s) {
	printf("MOTD: %s\n", s);
}

void statshook(_N_STATS *s) {
	printf("Stats: Libraries==%d Songs==%d Gigs==%d\n", s->libraries, s->songs, s->gigs);
}

int main(void) {
	_N_SERVER *s;
	_N_AUTH a={"dragotest", "nap"};
	n_SetMotdHook(motdhook);
	n_SetStatsHook(statshook);
	s=n_GetServer();
	if (!n_Connect(s, &a)) {
		printf("Error connecting\n");	
		return 1;
	}
	while(1) n_Loop();
}
