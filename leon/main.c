#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <rtems.h>
#include <fcntl.h>
#include <assert.h>
#include "time.h"
#include "rtems/rtems_bsdnet.h" // rtems_bsdnet_initialize_network()
#include "sys/types.h"
#include <sys/socketvar.h>
#include "sys/socket.h"
#include <netinet/in.h>
#include "netdb.h"  // gethostbyname
#include <sys/ioctl.h>
#include "net/if.h"
#include "net/if_var.h"
#include "arpa/inet.h"
#include "sys/proc.h"
#include <bsp/greth_gbit.h>
#include <bsp.h>
#include "DrvGpio.h"
#include "DrvI2cMaster.h"
#include "DrvI2c.h"
#include "OsDrvCpr.h" // DEV_CSS_GETH, DEV_CSS_I2C0, ...
#include "rtems/dhcp.h"
#include "rtems_config.h"
#include "AppConfig.h"
#include "mf_api.h"

// 2:  Source Specific #defines and types  (typedef, enum, struct)
// ----------------------------------------------------------------------------

#define RTEMS_BSP_NETWORK_DRIVER_NAME	 				"gr_eth1"
#define RTEMS_BSP_NETWORK_DRIVER_ATTACH    				rtems_leon_greth_gbit_driver_attach

#define RDA_COUNT     									16 //number of Rx descriptors
#define TDA_COUNT     									16 //number of Tx descriptors

// The interrupt priority that the ETH will have, 1 (low) - 15 (high)
#define ETH_ISR_PRIORITY								5


#define IO_RXD_0										(69)
#define IO_RXD_1										(70)
#define IO_RXD_2										(71)
#define IO_RXD_3										(72)
#define IO_RXD_4										(73)
#define IO_RXD_5										(46)
#define IO_RXD_6										(47)
#define IO_RXD_7										(48)
							
#define IO_TXCLK										(1)
#define IO_TX_EN										(2)
#define IO_TXER											(3)
#define IO_RXCLK										(4)
#define IO_RXDV											(5)
#define IO_RXER											(6)
#define IO_RXCOL										(7)
#define IO_RXCRS										(8)
#define IO_GTXCK										(35)
#define IO_125CLK										(0)
							
#define IO_TXD_0										(11)
#define IO_TXD_1										(62)
#define IO_TXD_2										(63)
#define IO_TXD_3										(64)
#define IO_TXD_4										(65)
#define IO_TXD_5										(66)
#define IO_TXD_6										(67)
#define IO_TXD_7										(68)
							
#define IO_MDC											(9)
#define IO_MDIO											(10)
							
// I2C							
#define I2C3_SDA										80
#define I2C3_SCL										79
#define I2C3_SPEED_KHZ_DEFAULT  						(400)
#define I2C3_ADDR_SIZE_DEFAULT  						(ADDR_7BIT)
#define WM8325_I2C										((0x6C)>>1)

// This have a big impact in Gigabit mode (at least for loopback test), set to  1 or 0
#define INVERT_GTX_CLK_CFG								1

// Memory allocated for mbufs
#define APP_MBUF_ALLOCATION								(2*64*1024)
// Memory allocated for mbuf clusters
#define APP_MBUFCLUSTERALLOCATION						(2*128*1024)
// MAC ADDRESS
#define APP_MACADDRESS									"\x94\xDE\x80\x6B\x12\x07"

#define SET_BIT(reg, bit, val) 							(val?(reg |= 1<<bit):(reg &= ~(1<<bit)))

#define CLIENTHOSTNAME									"Myriad2"

// 3: Gloval Variables 
// ----------------------------------------------------------------------------

// 4: Static Local Data
// ----------------------------------------------------------------------------
struct rtems_bsdnet_ifconfig ifconfig =
{
    RTEMS_BSP_NETWORK_DRIVER_NAME, 	//name
    RTEMS_BSP_NETWORK_DRIVER_ATTACH,  // attach function
    0, // link to next interface 
    NULL, //IP address is resolved by DHCP server
    NULL, //IP net mask is resolved by DHCP server
    APP_MACADDRESS, //*hardware_address;
    0, //broadcast
    0, //mtu
    0, //rbuf_count
    0, //xbuf_count
    0, //port
    0, //irno
    0, //bpar
    NULL //Driver control block pointer
};

struct rtems_bsdnet_config rtems_bsdnet_config = 
{
    &ifconfig,
    rtems_bsdnet_do_dhcp,
    0, // Default network task priority 
    APP_MBUF_ALLOCATION,//Default mbuf capacity
    APP_MBUFCLUSTERALLOCATION,//Default mbuf cluster capacity
    CLIENTHOSTNAME,//Host name
    NULL,//Domain name
    0, //Gateway is resolved by DHCP server
    NULL,//Log host
    {NULL},//Name server(s)
    {NULL},//NTP server(s)
    0, //sb_efficiency
    0, //udp_tx_buf_size
    0, //udp_rx_buf_size
    0, //tcp_tx_buf_size
    0, //tcp_rx_buf_size
};

static u32 commonI2CErrorHandler(I2CM_StatusType i2cCommsError, u32 slaveAddr, u32 regAddr);

// i2C data
static tyI2cConfig i2c2Config =
{
		.device                = IIC3_DEVICE,
		.sclPin                = I2C3_SCL,
		.sdaPin                = I2C3_SDA,
		.speedKhz              = I2C3_SPEED_KHZ_DEFAULT,
		.addressSize           = I2C3_ADDR_SIZE_DEFAULT,
		.errorHandler          = &commonI2CErrorHandler,
};

static u8 WM8325protocolWrite[] =
{
		S_ADDR_WR,
		R_ADDR_H,
		R_ADDR_L,
		DATAW,
		LOOP_MINUS_1
};

static u8 WM8325protocolRead[] =
{
		S_ADDR_WR,
		R_ADDR_H,
		R_ADDR_L,
		S_ADDR_RD,
		DATAR,
		LOOP_MINUS_1
};

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------


// 6: Functions Implementation
// ----------------------------------------------------------------------------


void Fatal_extension(
  Internal_errors_Source  the_source,
  bool                    is_internal,
  uint32_t                the_error
)
{
	if (the_source != RTEMS_FATAL_SOURCE_EXIT)
		printk ("\nSource %d Internal %d Error %d\n", the_source, is_internal, the_error);
	if (the_source == RTEMS_FATAL_SOURCE_EXCEPTION)
		rtems_exception_frame_print((void *) the_error);
}


void InitGpioEth(u8 invertGtxClk)
{
    DrvGpioSetMode(IO_TX_EN , D_GPIO_MODE_4);
    DrvGpioSetMode(IO_TXER  , D_GPIO_MODE_4);
    DrvGpioSetMode(IO_TXD_0 , D_GPIO_MODE_4);
    DrvGpioSetMode(IO_TXD_1 , D_GPIO_MODE_1);
    DrvGpioSetMode(IO_TXD_2 , D_GPIO_MODE_1);
    DrvGpioSetMode(IO_TXD_3 , D_GPIO_MODE_1);
    DrvGpioSetMode(IO_TXD_4 , D_GPIO_MODE_1);
    DrvGpioSetMode(IO_TXD_5 , D_GPIO_MODE_1);
    DrvGpioSetMode(IO_TXD_6 , D_GPIO_MODE_1);
    DrvGpioSetMode(IO_TXD_7 , D_GPIO_MODE_1);
    DrvGpioSetMode(IO_125CLK, D_GPIO_MODE_4);

    if(invertGtxClk){
        DrvGpioSetMode(IO_GTXCK , D_GPIO_MODE_1 | D_GPIO_DATA_INV_ON); // best with this INV;
        DrvGpioPadSet(IO_GTXCK, D_GPIO_PAD_DRIVE_2mA);//Drive: 2ma or 6mA allowed only, using 2mA
    }
    else{
        DrvGpioSetMode(IO_GTXCK , D_GPIO_MODE_1);
        DrvGpioPadSet(IO_GTXCK, D_GPIO_PAD_DRIVE_2mA);
    }

    DrvGpioSetMode(IO_TXCLK , D_GPIO_DIR_IN | D_GPIO_MODE_4 );
    DrvGpioSetMode(IO_RXCLK , D_GPIO_DIR_IN | D_GPIO_MODE_4 );
    DrvGpioSetMode(IO_RXDV  , D_GPIO_DIR_IN | D_GPIO_MODE_4);
    DrvGpioSetMode(IO_RXER  , D_GPIO_DIR_IN | D_GPIO_MODE_4);
    DrvGpioSetMode(IO_RXCOL , D_GPIO_DIR_IN | D_GPIO_MODE_4);
    DrvGpioSetMode(IO_RXCRS , D_GPIO_DIR_IN | D_GPIO_MODE_4);
    DrvGpioSetMode(IO_RXD_0 , D_GPIO_DIR_IN | D_GPIO_MODE_1);
    DrvGpioSetMode(IO_RXD_1 , D_GPIO_DIR_IN | D_GPIO_MODE_1);
    DrvGpioSetMode(IO_RXD_2 , D_GPIO_DIR_IN | D_GPIO_MODE_1);
    DrvGpioSetMode(IO_RXD_3 , D_GPIO_DIR_IN | D_GPIO_MODE_1);
    DrvGpioSetMode(IO_RXD_4 , D_GPIO_DIR_IN | D_GPIO_MODE_1);
    DrvGpioSetMode(IO_RXD_5 , D_GPIO_DIR_IN | D_GPIO_MODE_3);
    DrvGpioSetMode(IO_RXD_6 , D_GPIO_DIR_IN | D_GPIO_MODE_3);
    DrvGpioSetMode(IO_RXD_7 , D_GPIO_DIR_IN | D_GPIO_MODE_3);
    
    DrvGpioSetMode(IO_MDC, D_GPIO_MODE_4);
    DrvGpioSetMode(IO_MDIO, D_GPIO_MODE_4);
}

static u32 commonI2CErrorHandler(I2CM_StatusType i2cCommsError, u32 slaveAddr, u32 regAddr)
{
    printf("ERROR HANDLER\n");
    
    slaveAddr = slaveAddr;
    regAddr = regAddr;

    return i2cCommsError; // Because we haven't really handled the error, pass it back to the caller
}

void EthPHYHWReset(void)
{
    unsigned int myVal;
    u8 bytes[2];
    I2CM_Device i2c3Handle;
    
    DrvI2cMInitFromConfig(&i2c3Handle, &i2c2Config);    // setup i2c module

    //Setting output mode for GPIO pin 8 (Ethernet PHY reset)
    // and set MODE1+2 ( WM8325 Pin 4 ) to 0 for wakeup
    DrvI2cMTransaction(&i2c3Handle, WM8325_I2C, 0x403F, WM8325protocolRead, bytes, 2);
    myVal=(bytes[0]<<8) | bytes[1];
    myVal=0x0480; // output mode ...
    bytes[0]=(myVal & 0xFF00) >> 8;
    bytes[1]=(myVal & 0xFF);
    DrvI2cMTransaction(&i2c3Handle, WM8325_I2C, 0x403F, WM8325protocolWrite, bytes, 2); // ... for gpio 8
    DrvI2cMTransaction(&i2c3Handle, WM8325_I2C, 0x403B, WM8325protocolWrite, bytes, 2); // ... for gpio 4

    //Make sure ETH is in reset
    DrvI2cMTransaction(&i2c3Handle, WM8325_I2C, 0x400C,WM8325protocolRead, bytes, 2);
    myVal=(bytes[0]<<8) | bytes[1];
    SET_BIT(myVal, 3, 0); // GPIO 4 = 0
    SET_BIT(myVal, 7, 0); // GPIO 8 = 0
    bytes[0]=(myVal & 0xFF00) >> 8;
    bytes[1]=(myVal & 0xFF);
    DrvI2cMTransaction(&i2c3Handle, WM8325_I2C, 0x400C, WM8325protocolWrite, bytes, 2);

    // Wait for reset..
    usleep(500 * 1000);

    //Deassert HW reset, but keep ETH_BOOT low
    DrvI2cMTransaction(&i2c3Handle, WM8325_I2C, 0x400C, WM8325protocolRead, bytes, 2);
    myVal=(bytes[0]<<8) | bytes[1];
    SET_BIT(myVal, 7, 1); // GPIO 8 = 1
    bytes[0]=(myVal & 0xFF00) >> 8;
    bytes[1]=(myVal & 0xFF);
    DrvI2cMTransaction(&i2c3Handle, WM8325_I2C, 0x400C, WM8325protocolWrite, bytes, 2);

    // Wait for init..
    // TODO: put back the ETH_BOOT GPIO in input mode (shared with ETH RX pins, but through a resistor)
	usleep(100 * 1000);    
}

void initGrethAndNet(void)
{
    int res;
    rtems_greth_gbit_hw_params params;
    // Setup Greth Gbit parameters
    params.priority = ETH_ISR_PRIORITY;
    params.txcount = TDA_COUNT;
    params.rxcount = RDA_COUNT;
    params.gbit_check = FALSE;
    params.read_function = NULL;
    params.write_function = NULL;
    
    // Call setup function
    res = rtems_leon_greth_gbit_driver_setup(&params);
    printf("\nrtems_leon_greth_gbit_driver_setup %s \n", rtems_status_text(res));
    assert(res == RTEMS_SUCCESSFUL);    

    res = rtems_bsdnet_initialize_network();
    assert(res == RTEMS_SUCCESSFUL);
}


void POSIX_Init(void *args)
{
    UNUSED(args);

    /* NEED FOR USING ETHERNET */
    initClocksAndMemory();
    EthPHYHWReset();
	InitGpioEth(INVERT_GTX_CLK_CFG);
    initGrethAndNet();

    /* SETUP METRICS */
    metrics m_resources;
    m_resources.num_metrics = 2;
    m_resources.local_data_storage = 0;      // TOD: local data storage
    m_resources.sampling_interval[0] = 1000; // 1s
    strcpy(m_resources.metrics_names[0], "power_monitor");
    m_resources.sampling_interval[1] = 1500; // 1.5s
    strcpy(m_resources.metrics_names[1], "temp_monitor");

    /* START MONITORING */
    mf_start("141.58.0.8", "movidius", &m_resources);

    /* DO THE WORK */
    sleep(15);

    /* STOP MONITORING */
    mf_end();

    exit(0);
}
