# movidius_mf_api

## List of available APIs

```
char *mf_start(char *server, char *platform_id, metrics *m);
void mf_end(void);
```

## List of metrics

| Type            | Metrics          | Units  | Description   |
|---------------- |----------------  |------  |-------------- |
| power_monitor   | power_core       | mW     | power usage of cores |
| power_monitor   | power_ddr        | mW     | power usage of DDR memory |
| temp_monitor    | temperature_CSS  | °c     | temperature of CSS  |
| temp_monitor    | temperature_MSS  | °c     | temperature of MSS  |
| temp_monitor    | temperature_UPA0 | °c     | temperature of UPA0 |
| temp_monitor    | temperature_UPA1 | °c     | temperature of UPA1 |


## Compiling and debugging 

Before compiling and running this example, ensure that Movidius board is powered on and connected through JTAG cabel with a hosting computer. Setup the Linux <MV_TOOLS_DIR> environment variable to the MDK tools directory, like "xxx/mdk_release_xx.xx.x_general_purpose/tools".

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