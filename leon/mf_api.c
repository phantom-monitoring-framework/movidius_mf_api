
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <rtems.h>
#include <fcntl.h>
#include <assert.h>
#include "rtems/rtems_bsdnet.h" //rtems_bsdnet_initialize_network()
#include "mv_types.h"
#include <sys/socketvar.h>
#include "sys/socket.h"
#include <netinet/in.h>
#include "netdb.h" //gethostbyname
#include <sys/ioctl.h>
#include "net/if.h"
#include "net/if_var.h"
#include "arpa/inet.h"
#include "sys/proc.h"
#include <bsp/greth_gbit.h>
#include <bsp.h>
#include "DrvI2cMaster.h"
#include "DrvI2c.h"
#include "OsDrvCpr.h" // DEV_CSS_GETH, DEV_CSS_I2C0, ...
#include "rtems/dhcp.h"
#include "Board182Api.h"
#include "DrvCDCEL.h"
#include "brdMv0198.h"
#include "temp_monitor.h"
#include "power_monitor.h"
#include "mf_api.h"

// Variables declaration  
// ----------------------------------------------------------------------------
typedef struct each_metric_t
{
	long sampling_interval;
	char metric_name[NAME_LENGTH];
} each_metric;

int running;
int keep_local_data_flag = 0;
pthread_t threads[MAX_NUM_METRICS];
int num_threads;
char *experiment_id;
char *server_name;

// Functions definition
// ----------------------------------------------------------------------------
int create_new_experiment(char *server, char *message, char *experiment_id);
void *MonitorStart(void *arg);

// Functions implementation  
// ----------------------------------------------------------------------------
/* @brief Start monitoring with given MF server URL, current platform ID, and specific metrics desired for monitoring 
 *
 * @return Unique generated experiment ID is returned on success; otherwise a NULL pointer is returned
 */
char *mf_start(char *server, char *platform_id, metrics *m)
{
	/*create experiment by sending the MF server a msg and get the returned experiment ID */
	char *msg = calloc(1024, sizeof(char));
	char *buffer = calloc(256, sizeof(char));

	server_name = calloc(128, sizeof(char));
	experiment_id = calloc(64, sizeof(char));

	strcpy(server_name, server);

	sprintf(buffer, "{\"application\":\"%s\", \"task\": \"%s\", \"host\": \"%s\"}", APPLICATION_ID, TASK_ID, platform_id);
	sprintf(msg, "POST /v1/phantom_mf/experiments/%s HTTP/1.0\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s", 
					APPLICATION_ID, strlen(buffer), buffer);
	printf("application_id > %s\n", APPLICATION_ID);
	printf("task_id > %s\n", TASK_ID);

	create_new_experiment(server, msg, experiment_id);
	if(experiment_id[0] == '\0') {
		printf("ERROR: Cannot create new experiment for application %s\n", APPLICATION_ID);
		return NULL;
	}

	/* setup attributes for threads */
	pthread_attr_t attr;
    if(pthread_attr_init(&attr) !=0)
	   printk("pthread_attr_init error\n");
    if(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) != 0)
	   printk("pthread_attr_setinheritsched error\n");
    if(pthread_attr_setschedpolicy(&attr, SCHED_RR) != 0)
	   printk("pthread_attr_setschedpolicy error\n");

	/* number of threads == number of plugins; For Movidius platform, there are two plugins available --> power & 
	                                                                                                  --> temperature */
	num_threads = m->num_metrics;
	int t, iret[num_threads];
	each_metric *each_m = malloc(num_threads * sizeof(each_metric));
	running = 1;
	keep_local_data_flag = m->local_data_storage;

	for(t = 0; t < num_threads; t++) {
		each_m[t].sampling_interval = m->sampling_interval[t];
		strcpy(each_m[t].metric_name, m->metrics_names[t]);
		iret[t] = pthread_create(&threads[t], &attr, MonitorStart, &(each_m[t]));
		if(iret[t]) {
			printf("ERROR: pthread_create failed for %s\n", strerror(iret[t]));
			return NULL;
		}
	}
	return experiment_id;
}

/* @brief End monitoring by stopping all the created threads for plugins
 *
 */
void mf_end(void)
{
	int t;

	running = 0;
	for (t = 0; t < num_threads; t++) {
		pthread_join(threads[t], NULL);
	}
}

/* register new experiment by the MF server; the unique generated experiment_id is returned on success */
int create_new_experiment(char *server, char *message, char *experiment_id)
{
	struct sockaddr_in server_addr;
    struct in_addr **addr_list;
    struct hostent* host;

    /*get server_addr */
    memset(&server_addr, 0, sizeof(server_addr));
    host = gethostbyname(server);
    if(host != NULL) {
        addr_list = (struct in_addr**)host->h_addr_list;
        server_addr.sin_family  = AF_INET;
        server_addr.sin_addr    = *addr_list[0];
        server_addr.sin_port    = htons(3033);
    }
    else {
        printf("\nERROR: Cannot resolve hostname!\n");
        return 0;
    }
    int s = socket(PF_INET, SOCK_STREAM, 0);
    if(s < 0) {
    	printf("ERROR opening socket\n");
    	return 0;
    }
    int ret = connect(s, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(ret < 0) {
        printf("ERROR: Cannot connect to server %s: %s!\n", inet_ntoa(server_addr.sin_addr), strerror(errno));
        return 0;
    }
    if(send(s, message, strlen(message), 0) < 0) {
        printf("HTTP request to %s failed: %s\n", server, strerror(errno));
        return 0;
    }
	sleep(1);
    char *buffer = calloc(256, sizeof(char));
    int bytes_read = recv(s, buffer, 255, MSG_WAITALL); //last byte is null termination
    if (bytes_read > 0) {
    	strncpy(experiment_id, buffer+strlen(buffer)-20, 20);
    	printf("experiment_id > %s\n", experiment_id);
    }
    
    close(s);
    return 1;
}

/* start different threads for different plugins and metrics */
void *MonitorStart(void *arg) {
	each_metric *metric = (each_metric*) arg;
	if(strcmp(metric->metric_name, "temp_monitor") == 0) {
		TempInit();
		/* sample temperature related metrics within a loop */
		TempSamples(metric->sampling_interval, server_name);
	}
	else if(strcmp(metric->metric_name, "power_monitor") == 0) {
		PowerInit();
		/* sample power related metrics within a loop */
		PowerSamples(metric->sampling_interval, server_name);
	}
	else {
		printf("ERROR: it is not possible to monitor %s\n", metric->metric_name);
		return NULL;
	}
	return NULL;
}