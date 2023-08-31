#include "JSONParser.h"
#include "Enclave_u.h"
#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include <jsoncpp/json/json.h>
#include "../cppcodec/base32_crockford.hpp"
#include "../cppcodec/base64_rfc4648.hpp"
#include "../cppcodec/base64.h"

/* decode a buffer */
int decode_message(char *buffer, char *decoded_msg){
    /*base64.cpp*/
    std::string decoded = base64_decode(buffer);
    memcpy(decoded_msg, (char *)decoded.c_str(), decoded.length());
    decoded_msg[decoded.length()] = '\0';
    if (memcmp(decoded_msg, decoded.c_str(), decoded.length()) != 0){
        std::cout << "JSONParser::base64_decode Unsuccessful!" << std::endl;
        return 0;
    }
    return 1;
}

/* Get a decoded buffer length */
int get_decoded_message_length(char *buffer){
    /*base64.cpp*/
    std::string decoded = base64_decode(buffer);
    return decoded.length();
}

/* Encode a string */
std::string encode_message(char *text, int len){
    /*base64.cpp*/
    using base64 = cppcodec::base64_rfc4648;
    std::string encoded_text = base64::encode(text, len);
    return encoded_text;
}

/* Make a json string from ciphertext and tag/mac */
std::string make_json_from_message(struct Message *ptr)
{
    Json::Value root;
    using base64 = cppcodec::base64_rfc4648;

    std::string encoded_text = base64::encode(ptr->text, ptr->textLength);
    std::string encoded_tag = base64::encode(ptr->tag, ptr->tagLength);

    root["cp"] = encoded_text;
    root["tag"] = encoded_tag;

    Json::FastWriter fastwriter;
    std::string message = fastwriter.write(root);

    return message;
}

/* Parse a json string to retrieve ciphertext and tag/mac */
bool parse_data_with_tag(char *buffer, struct Message *ptr, bool isAllocated) {
    Json::Value jsonData;
    Json::Reader jsonReader;

    if (jsonReader.parse(buffer, jsonData)) {
        /*base64.cpp*/
        std::string decoded = base64_decode(jsonData["cp"].asString());
        //std::cout << "Decoding: " << decoded << std::endl;
        //std::cout << decoded.length() << std::endl;

        std::string decoded_tag = base64_decode(jsonData["tag"].asString());
        //std::cout << "Decoding: " << decoded_tag << std::endl;
        //std::cout << decoded_tag.length() << std::endl;

        if (!isAllocated){
            ptr->text = (char *) malloc((decoded.size()+1)*sizeof(char));
            ptr->textLength = decoded.length();
        }
        memcpy(ptr->text, (char *)decoded.c_str(), ptr->textLength);
        ptr->text[ptr->textLength] = '\0';
        if (memcmp(ptr->text, decoded.c_str(), ptr->textLength) != 0){
            std::cout << "JSONParser::Copy Unsuccessful. Parsing incomplete!" << std::endl;
            if (!isAllocated) free(ptr->text);
            return false;
        }

        if (!isAllocated) {
            ptr->tag = (char *) malloc((decoded_tag.size() + 1) * sizeof(char));
            ptr->tagLength = decoded_tag.length();
        }
        memcpy(ptr->tag, decoded_tag.c_str(), ptr->tagLength);
        ptr->tag[ptr->tagLength] = '\0';
        if (memcmp(ptr->tag, decoded_tag.c_str(), ptr->tagLength) != 0){
            std::cout << "JSONParser:: Copy Unsuccessful. Parsing incomplete!" << std::endl;
            if (!isAllocated) free(ptr->tag);
            return false;
        }

        //std::cout << "Done Parsing!" << std::endl;
        return true;

    } else{
        std::cout << "JSONParser:: Something wrong with the parsing." << std::endl;
        return false;
    }
}

/* Parse a json string to retrieve ciphertext length and tag/mac length */
bool parse_data_length_with_tag(char *buffer, struct Message *ptr){
    Json::Value jsonData;
    Json::Reader jsonReader;

    if (jsonReader.parse(buffer, jsonData)) {
        /*base64.cpp*/
        std::string decoded = base64_decode(jsonData["cp"].asString());
        //std::cout << "Decoding: " << decoded << std::endl;
        //std::cout << decoded.length() << std::endl;

        std::string decoded_tag = base64_decode(jsonData["tag"].asString());
        //std::cout << "Decoding: " << decoded_tag << std::endl;
        //std::cout << decoded_tag.length() << std::endl;

        ptr->textLength = decoded.length();
        ptr->tagLength = decoded_tag.length();

        //std::cout << "Done Parsing!" << std::endl;
        return true;

    } else{
        std::cout << "JSONParser:: Something wrong with the parsing." << std::endl;
        return false;
    }
}

