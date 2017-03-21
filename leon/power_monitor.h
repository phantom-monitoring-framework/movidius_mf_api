#ifndef _POWER_MONITOR_H_
#define _POWER_MONITOR_H_

void PowerInit(void);
void PowerSamples_Once(char *string);
void *PowerSamples(long sampling_interval, char *server);

#endif