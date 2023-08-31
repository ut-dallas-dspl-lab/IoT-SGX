# About #
This repo contains the implementation of host and trusted application designed for the edge/IoT device with TrustZone.
It operates as the edge/IoT device application of framework SecIoTT from the Paper (insert link/title),
that collects trigger events from sensors and securely shares with cloud, as well as receive action commands from cloud to control actuator devices. See the paper for more details.


## Devices used for test cases
- Raspberry Pi 3B
- motion sensors (Trigger device)
- LEDs (Actuator)


## Required libraries/packages
- Mosquitto
- GPIOD
- cJSON

# Installation #
1. Install OP-TEE (Rpi3/Qemu). For more information, see [OP-TEE documentation](https://optee.readthedocs.io/en/latest).
2. Install required packages mentioned above.
2. Clone this repo in the optee_example folder.

# Build and Run #
1. Build following OP-TEE documentation:
    - [Rpi3 OP-TEE documentation](https://optee.readthedocs.io/en/latest/building/devices/rpi3.html).
    - [Qemu OP-TEE documentation](https://optee.readthedocs.io/en/latest/building/devices/qemu.html#qemu-v8).
2. Execute the binary: optee_example_iot
