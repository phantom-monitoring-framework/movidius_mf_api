#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <rtems.h>
#include <fcntl.h>
#include <assert.h>
#include "rtems/rtems_bsdnet.h" //rtems_bsdnet_initialize_network()
#include "sys/types.h"
#include "mv_types.h"
#include "time.h"
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
#include "mf_api.h"
#include "power_monitor.h"

#define MAX_STR_LEN 512

// Static Local Data
// ----------------------------------------------------------------------------
static char http_request[] = "POST /v1/phantom_mf/metrics HTTP/1.0\r\n";
static char http_headers[] = "Content-Type: application/json\r\nContent-Length: %d\r\n\r\n";
static tyBrd198Handle powerMonHandle;
static I2CM_Device *i2c2Handle;

// Functions implementation 
// ----------------------------------------------------------------------------

/* initialize hardware and driver interfaces for power metrics collection */
void PowerInit(void)
{
    s32 boardStatus = BoardInitialise(EXT_PLL_CFG_148_24_24MHZ);	//for board MV0182
    i2c2Handle=gAppDevHndls.i2c2Handle;
    assert(i2c2Handle != NULL && "I2C not initialized for MV0198");

    if (boardStatus != B_SUCCESS)
    {
    	printf("Error: board initialization failed with %ld status\n", boardStatus);
    	exit(0);
    }

    int returnValue = Brd198Init(&powerMonHandle, i2c2Handle, NULL);
    assert(returnValue == DRV_BRD198_DRV_SUCCESS && "Board 198 init error");
}

/* sample the current power measurements by calling the driver provided APIs; metrics are formatted into a JSON string */
void PowerSamples_Once(char *string)
{
	tyAdcResultAllRails powRes;
	fp32 ddrMw, ddrMa, coreMw;

    /*get current local_timestamp in millisecond */
    struct timespec time_now;
    clock_gettime(CLOCK_MONOTONIC, &time_now);
    double local_timestamp = time_now.tv_sec * 1000.0 + (time_now.tv_nsec) *1e-6;

	Brd198SampleAllRails(&powerMonHandle, &powRes);
	Brd198GetDdrPowerAndCurrent(&powerMonHandle, &powRes, &ddrMw, &ddrMa);
	coreMw = powRes.totalMilliWatts - ddrMw;

    memset(string, '\0', strlen(string));

    sprintf(string, "{\"WorkflowID\":\"%s\", \"ExperimentID\":\"%s\", \"TaskID\":\"%s\", \"type\":\"power_monitor\", \"local_timestamp\":\"%.1f\", \"power_core\":%f, \"power_ddr\":%f}", 
        APPLICATION_ID, experiment_id, TASK_ID, local_timestamp, coreMw, ddrMw);
}

/* the function is executed in a loop, while "running" is set to 1;
inside the loop: we create ethernet connection with the MF server; sample current power metrics; send the formatted json documents to the server */
void *PowerSamples(long sampling_interval, char *server)
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
        return NULL;
    }

    int ret, s;
    char *json_string = calloc(MAX_STR_LEN, sizeof(char));
    char *buffer = calloc(MAX_STR_LEN, sizeof(char));
    while(running) {
        s = socket(PF_INET, SOCK_STREAM, 0);
        if(s < 0) {
            printf("ERROR opening socket\n");
            return NULL;
        }

        ret = connect(s, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if( ret < 0) {
            printf("ERROR: Cannot connect to server %s: %s!\n", inet_ntoa(server_addr.sin_addr), strerror(errno));
            return NULL;
        }

        memset(buffer, '\0', MAX_STR_LEN);
        strcpy(buffer, http_request);
        PowerSamples_Once(json_string);

        usleep(sampling_interval * 1000);
        
        sprintf(buffer+strlen(buffer), http_headers, strlen(json_string)+2);
        sprintf(buffer+strlen(buffer), "[%s]", json_string);
        
        if(send(s, buffer, strlen(buffer), 0) < 0) {
            printf("HTTP request to %s failed: %s\n", server, strerror(errno));
            return NULL;
        }
        close(s);
    }
    return NULL; // just so the compiler thinks we returned something
}