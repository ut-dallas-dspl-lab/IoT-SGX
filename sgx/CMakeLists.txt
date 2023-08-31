cmake_minimum_required(VERSION 3.12)
project(IOT_SGX_new)

set(CMAKE_CXX_STANDARD 11)

add_executable(IOT_SGX_new main.cpp Enclave/RuleConflictDetectionManager.cpp Enclave/RuleConflictDetectionManager.h Enclave/EnclaveDatabaseManager.cpp Enclave/EnclaveDatabaseManager.h Enclave/TimerQueueManager.cpp Enclave/TimerQueueManager.h)