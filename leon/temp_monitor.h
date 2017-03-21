#ifndef _TEMP_MONITOR_H_
#define _TEMP_MONITOR_H_

void TempInit(void);
void TempSamples_Once(char *string);
void *TempSamples(long sampling_interval, char *server);

#endif