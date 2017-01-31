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
#include "power_monitor.h"

#define WORKFLOWID                                      "Movidius"
#define EXPERIMENTID                                    "123456789"
#define TASKID                                          "power_monitor"
#define MAX_STR_LEN                                     512

static char http_host_name[] = "141.58.0.8";
static char http_request[] = "POST /v1/dreamcloud/mf/metrics HTTP/1.0\r\n";
static char http_headers[] = "Content-Type: application/json\r\nContent-Length: %d\r\n\r\n";

static tyBrd198Handle powerMonHandle;
static I2CM_Device *i2c2Handle;

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

void PowerSamples_Once(char *string)
{
	tyAdcResultAllRails powRes;
	fp32 ddrMw, ddrMa, coreMw;

	Brd198SampleAllRails(&powerMonHandle, &powRes);
	Brd198GetDdrPowerAndCurrent(&powerMonHandle, &powRes, &ddrMw, &ddrMa);
	coreMw = powRes.totalMilliWatts - ddrMw;

    memset(string, '\0', strlen(string));

    sprintf(string, "{\"WorkflowID\":\"%s\", \"ExperimentID\":\"%s\", \"TaskID\":\"%s\", \"power_core\":%f, \"power_ddr\":%f}", 
        WORKFLOWID, EXPERIMENTID, TASKID, coreMw, ddrMw);
}

void *PowerSamples(void *arg)
{
    struct PowerSamplesArg *v = arg;
    struct sockaddr_in server_addr;
    struct in_addr **addr_list;
    struct hostent* host;

    /*get server_addr */
    memset(&server_addr, 0, sizeof(server_addr));
    host = gethostbyname(http_host_name);
    if(host != NULL) {
        addr_list = (struct in_addr**)host->h_addr_list;
        server_addr.sin_family  = AF_INET;
        server_addr.sin_addr    = *addr_list[0];
        server_addr.sin_port    = htons(3030);
    }
    else {
        printf("\nERROR: Cannot resolve hostname!\n");
        pthread_exit(0);
    }

    int i, ret, s;
    char *json_string = calloc(MAX_STR_LEN, sizeof(char));
    char *buffer = calloc(MAX_STR_LEN, sizeof(char));

    for (i = 0; i < v->loops_num; i++) {
        s = socket(PF_INET, SOCK_STREAM, 0);
        if(s < 0) {
            printf("ERROR opening socket\n");
            pthread_exit(0); 
        }

        ret = connect(s, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if( ret < 0) {
            printf("ERROR: Cannot connect to server %s: %s!\n", inet_ntoa(server_addr.sin_addr), strerror(errno));
            pthread_exit(0);
        }

        memset(buffer, '\0', MAX_STR_LEN);
        strcpy(buffer, http_request);

        PowerSamples_Once(json_string);
        sleep(v->seconds);
        
        sprintf(buffer+strlen(buffer), http_headers, strlen(json_string)+2);
        sprintf(buffer+strlen(buffer), "[%s]", json_string);
        
        if(send(s, buffer, strlen(buffer), 0) < 0) {
            printf("HTTP request to %s failed: %s\n", http_host_name, strerror(errno));
            pthread_exit(0); 
        }
        close(s);
    }
    pthread_exit(0); 
    return NULL; // just so the compiler thinks we returned something
}