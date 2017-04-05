# Movidius application-level monitoring APIs
> In order to achieve monitoring of movidius application, we implemented functions and interfaces for programmers convenience. The monitoring functions are implemented based on available sensors and drivers included in the Movidius Software Development Kit (MDK). At run-time the sampled metrics are sent to the PHANTOM monitoring server via given URL through HTTP Ethernet interface.

## List of available APIs
Following two interfaces are implemented in the file `mf_api.c` and can be used by a generic Movidius application. Monitoring in controlled and operated by one of the Leon RSIC processors, where the operating system is running.
```
char *mf_start(char *server, char *platform_id, metrics *m);
void mf_end(void);
```

## List of metrics
The metrics supported by the above APIs are listed in the table as follows:

| Type            | Metrics          | Units  | Description                              |
|---------------- |----------------  |------  |----------------------------------------- |
| power_monitor   | power_core       | mW     | power usage of cores                     |
| power_monitor   | power_ddr        | mW     | power usage of DDR memory                |
| temp_monitor    | temperature_CSS  | °c     | temperature of CSS  (CPU Sub System)     |
| temp_monitor    | temperature_MSS  | °c     | temperature of MSS  (Media Sub System)   |
| temp_monitor    | temperature_UPA0 | °c     | temperature of UPA0 (6 SHAVES)           |
| temp_monitor    | temperature_UPA1 | °c     | temperature of UPA1 (the other 6 SHAVES) |


## Compiling and debugging
Before compiling and running this example, ensure that the Movidius board is powered on and connected through JTAG cabel with a hosting computer. Setup the Linux <MV_TOOLS_DIR> environment variable to the MDK tools directory, like "xxx/mdk_release_xx.xx.x_general_purpose/tools".

Check also the installation of the MDK package by typing:
```
$ cd xxx/mdk_release_xx.xx.x_general_purpose/mdk/common/utils
$ bash ./checkinstall.sh
```

Open a terminal where MDK was extrated and type the following command to start a debug server:
```
$ cd xxx/mdk_release_xx.xx.x_general_purpose/mdk/examples/HowTo/xxx/
$ make start_server
```

Open another terminal in the same directory and type the following command for compiling and running:
```
$ make clean all
$ make run
```

It should be noticed that the type of Movidius board should match that given in the Makefile, otherwise there would be errors during running and debugging.


## Results
This example shows the users of Movidius MA2150 about how to monitor power and temperature of various processors and cores using the standard MF APIs. Movidius MF APIs are particular implemented for this embedded machine, although similar interfaces are designed as common MF APIs. 

<mf_start> creates threads for monitoring power and temperature, which can be configured by setting the metrics name and sampling intervals. Each thread runs in a loop, samples the specialized metrics, and uses Movidius network stack to send power and temperature metrics to the monitoring server.

<mf_end> terminates the created threads and ends the sampling.

The APIs prints out also important variables (application ID, task ID, and experiment ID) for metrics query at last.