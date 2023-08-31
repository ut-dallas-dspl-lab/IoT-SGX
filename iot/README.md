# About #
This repo contains the implementation of host and trusted application designed for the edge/IoT device with TrustZone.
It operates as the edge/IoT device application of framework SecIoTT from the Paper (insert link/title),
that collects trigger events from sensors and securely shares with cloud, as well as receive action commands from cloud to control actuator devices. We use **OP-TEE** as the TEE of TrustZone. See the paper for more details.


## Devices used for test cases
- Raspberry Pi 3B (Rpi3)
- Motion sensors, Temperature sensors (Trigger device)
- LEDs (Actuator)


## Required libraries/packages
- Mosquitto
- GPIOD
- cJSON

You can install these libraries with **OP-TEE** by enabling a _BR_ flag. For example, to add *Mosquitto* for Rpi3 device setting:
1. Find the name of the _BR_ flag in `<optee_project_dir>/buildroot/package/mosquitto/Config.in`
2. Include the _BR_ flag in the `<optee_project_dir>/build/rpi3.mk`
3. Package will be installed when you Build the project.

### Required App Specific Files:
You need to provide a json file that contains your basic device information. We have provided a sample file `sample_device_info.json` that includes:
- user information (e.g., user id) 
- necessary device information for each device (e.g., device id, capability, attribute, value unit, topic for pub/sub)


# Installation #
1. Install OP-TEE (Rpi3/Qemu). For more information, see [OP-TEE documentation](https://optee.readthedocs.io/en/latest).
2. Install required packages mentioned above.
2. Clone this repo in the `<optee_project_dir>/optee_examples` folder.


# Build #
- Build following the OP-TEE documentation:
    - [Rpi3 OP-TEE documentation](https://optee.readthedocs.io/en/latest/building/devices/rpi3.html).
    - [Qemu OP-TEE documentation](https://optee.readthedocs.io/en/latest/building/devices/qemu.html#qemu-v8).
- Copy any necessary files, that you need to access from your app (e.g., *sample_device_info.json*), to `<optee_project_dir>/out-br/target/root/` folder. You may need to build again after this.


# Run #
1. Login with password `root`
2. To run the app, use command: `optee_example_iot <device info file path>`
      - For example: `optee_example_iot sample_device_info.json`
      - See the `main.c` for more information.


## Add new Device spicific code:
```
Folder structure:
|- iot
|	|- host
|	|	|- include 						/* Header files */
|	|	|- main.c 						/* Main file */
|	|	|- utils.c
|	|	|- device_manager.c 			/* Manages the IoT devices */
|	|	|- device_switch.c 				/* Implementation of a switch/LED */
|	|	|- <new_device>.c 				/* Add code for new device */
|	|- ta
|	|	|- include 						/* Header files */
|	|	|- main_TA.c 					/* Main entry point to TA */
|	|	|- utils_TA.c
|	|	|- parser_TA.c
|	|	|- cJSON_TA.c
|	|	|- device_manager_TA.c 			/* Manages the IoT devices in TA */
|	|	|- device_motion_sensor_TA.c 	/* Implementation of a motion sensor in TA */
|	|	|- device_temp_sensor_TA.c 		/* Implementation of a temperature sensor in TA */
|	|	|- <new_device>_TA.c 			/* Add code for new device in TA */
```
- Any code for new device implementation should fall into `<new_device>.c` in the NW and `<new_device>_TA.c` in the SW. 
- To control the new devices, `device_manager.c` in NW and `device_manager_TA.c` in SW should be updated accordingly. 
- To get familiar with the protocol, please follow the code files for existing devices: `device_switch.c`, `device_motion_sensor_TA.c`; and device managers: `device_manager.c`, `device_manager_TA.c`.


