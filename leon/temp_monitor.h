#ifndef _TEMP_MONITOR_H_
#define _TEMP_MONITOR_H_

/* @brief Initialize hardware and driver interfaces for temperature metrics collection 
 *
 */
void TempInit(void);

/* @brief Sample the current temperature measurements by calling the driver provided APIs 
 *
 */
void TempSamples_Once(char *string);

/* @brief The function is executed in a loop, while "running" is set to 1;
 * inside the loop: we create ethernet connection with the MF server; sample current temperature metrics; and 
 * send the formatted json documents to the server 
 */
void *TempSamples(long sampling_interval, char *server);

#endif //_TEMP_MONITOR_H