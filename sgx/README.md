# About #
This repo contains the implementation of the rule-based trigger-action automation platform for IoT with Intel SGX in the cloud.
At first, the client registers rules in the cloud server's SGX, where The rules are encrypted, stored in the cloud filesystem/database, processed against incoming data stream from the IoT devices to carry out the defined automation.
Later, the SGX enclave receives encrypted trigger events from IoT devices, process the events in enclave after decryption against the automation rules, and sends encrypted action commands when required to the particular IoT device.


## Required libraries/packages
- Mosquitto
- JsonCpp, cJSON (included), cppcodec (included)
- MongoDB, mongocxx driver, bsoncxx driver


## Software versions used for test cases
- Ubuntu 18.04.6 LTS
- Intel SGX SDK version X


# Installation #
1. Install Intel SGX SDK. For more information, see [SGX SDK linux documentation](https://github.com/intel/linux-sgx).
2. Install required packages mentioned above.
3. Clone this repo in the project's source folder.
4. You may need to update the **Makefile** to include necessary library information/path.


# Build and Run #
1. Make sure your environment is set: *$ source ${sgx-sdk-install-path}/environment*
2. Build the project with the prepared Makefile:
    - Hardware Mode, Debug build: *$ make*
    - Simulation Mode, Debug build: *$ make SGX_MODE=SIM*
3. Execute the binary directly: *$ ./app <argument lists>*
4. Remember to `make clean` before switching build mode or after updating `Enclave.edl` file.
5. Argument list:
   1. MongoDB collection name (required)
   2. json file path containing device information (required)
   3. Optional arguments: see the `App.c` for more information


## Example Run of the Sample Code:

1.  First, follow the `sample_device_info.json` in the *datafiles* folder that contains necessary IoT device information.
    - user information (e.g., user id) 
	- necessary device information for each device (e.g., device id, capability, attribute, value unit, topic for pub/sub)
3. Second, you need to register the rules in SGX.
    - Follow the `sample_ruleset_v1.0.json` file. It contains a list of rules in trigger-action format in json.
    - Load the sample encrypted ruleset from file `sample_ruleset_v1_enc.json` into the MongoDB using *mongoimport* command:
      `mongoimport --db=IOT --collection=sample_ruleset_v1 --file=sample_ruleset_v1_enc.json`
4. Lastly, run the SGX app that awaits for incoming trigger-events from registered IoT devices:
    - Run using command: `./app sample_ruleset_v1 datafiles/sample_device_info.json -r 0 -d 1`
