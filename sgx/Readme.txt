# About #
This repo contains the implementation of the rule-based trigger-action automation platform for IoT with Intel SGX in the cloud.
At first, the client registers rules in the cloud server's SGX, where The rules are encrypted, stored in the cloud filesystem/database, processed against incoming data stream from the IoT devices to carry out the defined automation.
Later, the SGX enclave receives encrypted trigger events from IoT devices, process the events in enclave after decryption against the automation rules, and sends encrypted action commands when required to the particular IoT device.


## Required libraries/packages
- Mosquitto
- cJSON
- MongoDB, mongocxx driver


## Software versions used for test cases
- Ubuntu 18.04.6 LTS
- Intel SGX SDK version X


# Installation #
1. Install Intel SGX SDK. For more information, see [SGX SDK linux documentation](https://github.com/intel/linux-sgx).
2. Install required packages mentioned above.
2. Clone this repo in the project's source folder.


# Build and Run #
1. Make sure your environment is set: $ source ${sgx-sdk-install-path}/environment
2. Build the project with the prepared Makefile:
    a. Hardware Mode, Debug build: $ make
    b. Hardware Mode, Pre-release build: $ make SGX_PRERELEASE=1 SGX_DEBUG=0
    c. Hardware Mode, Release build: $ make SGX_DEBUG=0
    d. Simulation Mode, Debug build: $ make SGX_MODE=SIM
    e. Simulation Mode, Pre-release build: $ make SGX_MODE=SIM SGX_PRERELEASE=1 SGX_DEBUG=0
    f. Simulation Mode, Release build: $ make SGX_MODE=SIM SGX_DEBUG=0
3. Execute the binary directly: $ ./app <argument lists>
4. Remember to "make clean" before switching build mode or after updating Enclave.edl file.
5. Argument list: