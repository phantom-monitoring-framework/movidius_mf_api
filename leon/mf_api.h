#ifndef _MF_API_H
#define _MF_API_H

#define NAME_LENGTH	    32
#define MAX_NUM_METRICS 2
#define APPLICATION_ID  "movidius"
#define TASK_ID         "monitor"

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

char *mf_start(char *server, char *platform_id, metrics *m);

void mf_end(void);

#endif