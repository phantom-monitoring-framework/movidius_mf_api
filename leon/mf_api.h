#ifndef _MF_API_H
#define _MF_API_H

#define NAME_LENGTH	    32
#define MAX_NUM_METRICS 2
#define APPLICATION_ID  "movidius"	//default application ID, which should be registered by the MF server before start monitoring
#define TASK_ID         "monitor"	//default task ID, which should be included in the application during application's registration

/* data type for metrics configuration */
typedef struct metrics_t {
	long sampling_interval[MAX_NUM_METRICS];	//in milliseconds
	char metrics_names[MAX_NUM_METRICS][NAME_LENGTH];		//user defined metrics
	int num_metrics;
	int local_data_storage;
} metrics;

extern int running;
extern int keep_local_data_flag;
extern char *experiment_id;
extern char *server_name;

/* @brief Start monitoring with given MF server URL, current platform ID, and specific metrics desired for monitoring 
 *
 * @return Unique generated experiment ID is returned on success; otherwise a NULL pointer is returned
 */
char *mf_start(char *server, char *platform_id, metrics *m);


/* @brief End monitoring by stopping all the created threads for plugins
 *
 */
void mf_end(void);

#endif //_MF_API_H