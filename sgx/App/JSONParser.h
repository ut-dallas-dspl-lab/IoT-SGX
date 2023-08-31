#include <iostream>
#include <string.h>
#ifndef IOTENCLAVE_JSONPARSER_H
#define IOTENCLAVE_JSONPARSER_H

int decode_message(char *buffer, char *decoded_msg);
int get_decoded_message_length(char *buffer);
std::string encode_message(char *text, int len);

std::string make_json_from_message(struct Message *ptr);

bool parse_data_with_tag(char *buffer, struct Message *ptr, bool isAllocated);
bool parse_data_length_with_tag(char *buffer, struct Message *ptr);



#endif //IOTENCLAVE_JSONPARSER_H
