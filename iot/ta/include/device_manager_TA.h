#ifndef OPTEE_EXAMPLE_IOT_DEVICE_MANAGER_TA_H
#define OPTEE_EXAMPLE_IOT_DEVICE_MANAGER_TA_H


int prepare_device_info(char *info);
int get_num_subscriber_devices();
int get_subscriber_topic(int sub_topic_index, char** topic_name);

int prepare_trigger_event(char *sensor_value, int device_type, int mqtt_op_type, char **response, char** topic_name);
int prepare_action_command(char *action, int *device_type, int *command_type, int *argument_type, char **argument, int *argument_size);

void destroy_device_network_info();

#endif //OPTEE_EXAMPLE_IOT_DEVICE_MANAGER_TA_H
