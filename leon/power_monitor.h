#ifndef _POWER_MONITOR_H_
#define _POWER_MONITOR_H_

struct PowerSamplesArg
{
	int loops_num;
	int seconds;
};

void PowerInit(void);
void PowerSamples_Once(char *string);
void *PowerSamples(void *arg);

#endif