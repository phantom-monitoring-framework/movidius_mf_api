#ifndef _POWER_MONITOR_H_
#define _POWER_MONITOR_H_

/* @brief Initialize hardware and driver interfaces for power metrics collection 
 *
 */
void PowerInit(void);

/* @brief Sample the current power measurements by calling the driver provided APIs; metrics are formatted into a JSON string 
 *
 */
void PowerSamples_Once(char *string);

/* @brief The function is executed in a loop, while "running" is set to 1;
 * inside the loop: we create ethernet connection with the MF server; sample current power metrics; and 
 * send the formatted json documents to the server 
 */
void *PowerSamples(long sampling_interval, char *server);

#endif //_POWER_MONITOR_H_