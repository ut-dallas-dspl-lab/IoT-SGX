project (optee_example_iot C)

set (SRC host/main.c host/utils.c host/device_manager.c host/device_switch.c ta/include/cJSON_TA.h ta/cJSON_TA.c)

add_executable (${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME}
		PRIVATE ta/include
		PRIVATE include
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../host/aarch64-buildroot-linux-gnu/sysroot/usr/include
		)

target_link_libraries (${PROJECT_NAME} PRIVATE teec
		${CMAKE_CURRENT_SOURCE_DIR}/../../../host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/libmosquitto.so
		${CMAKE_CURRENT_SOURCE_DIR}/../../../host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/libcrypto.so
		${CMAKE_CURRENT_SOURCE_DIR}/../../../host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/libssl.so
		${CMAKE_CURRENT_SOURCE_DIR}/../../../host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/libgpiod.so
		)


install (TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
