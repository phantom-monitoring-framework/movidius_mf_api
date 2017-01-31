#ifndef _TEMP_MONITOR_H_
#define _TEMP_MONITOR_H_

struct TempSamplesArg
{
	int loops_num;
	int seconds;
};

void TempInit(void);
void TempSamples_Once(char *string);
void *TempSamples(void *arg);

#endif