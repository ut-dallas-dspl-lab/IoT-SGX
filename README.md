# IoT-SGX

## Overview
We develop a secure end-to-end encrypted IoT rule-based trigger-action platform for smart home automation in the untrusted cloud with state-of-the-art trusted execution environments (TEE), i.e. Intel SGX and ARM TrustZone, and proper cryptographic techniques to alleviate data security and privacy issues in IoT. The principal idea is to utilize TrustZone and SGX to provide security and protection to sensitive IoT data and confidential automation rules at the edge and in the untrusted cloud, respectively.

Here, we briefly describe the overall architecture, which is demonstrated in Figure X. We consider rule-based trigger-action platform, one of the most widely used IoT programming platforms in the world of IoT automation, where rules contain the automation logic that interconnects multiple devices to enable customized tasks. As a case study, we consider a smart home environment as a motivating application where various interconnected smart IoT devices, such as multipurpose sensors, smart bulbs, smart outlets, security cameras, and so on, are stationed to assist their users with disparate functionalities including home automation.  We assume that the smart IoT devices are equipped with TrustZone and communicate with untrusted cloud services containing SGX to send and receive data. We utilize MQTT (Message Queuing Telemetry Transport) for sending data from the IoT to the cloud and vice versa, which is a standard publish/subscribe messaging protocol specifically designed for the IoT because of its low network bandwidth and minimal code footprint. 

Note that, our proposed system is agnostic to the underlying IoT platform, and can be employed in any smart platform, such as smart industry, where trigger-action-rule-based automation is deployed in the cloud.

The overall flow of the process is outlined below:

1. IoT Device Setup: The client setups necessary account information (e.g., id), IoT-server communication details (e.g., mqtt), and their basic IoT device information (e.g., id, capability) in the SGX enclave and the respective device's TrustZone.
2. Rule Registration: The client registers rules (in trigger-action format) in the cloud server's SGX via secure channel after remote attestation. The rules are encrypted, stored in the cloud filesystem/database, processed against incoming data stream from the IoT devices to carry out the defined automation.
3. IoT to Server Data Transmission: IoT devices capture data from the environment and send it to the untrusted cloud server after encrypting the payload in the TrustZone. 
4. Rule Automation in Server: In the cloud, SGX securely processes the received data after decryption of the payload in the enclave against the registered rules. SGX sends encrypted responses, when required for automation purposes according to the rules, to the particular IoT device. 
5. IoT Device Automation: The IoT devices process the response after decryption in the TrustZone.

For more information, please visit our website.

## Implementation
- `sgx` directory contains the SGX code for the cloud.
- `iot` directory contains the device-specific code that runs in IoT devices with TrustZone.
