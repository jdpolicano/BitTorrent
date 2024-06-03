/**
 * @file bencode.h
 * @brief Header file for Bencode decoder in C.
 */
#ifndef BENCODE_H
#define BENCODE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

// Enumeration for Bencode types
typedef enum {
    INTEGER,
    STRING,
    LIST,
    DICTIONARY
} BType;

// Forward declaration of Bencoded
typedef struct Bencoded Bencoded;

// Separate struct for list data
typedef struct {
    size_t capacity;
    size_t size;
    Bencoded* elements;
} BencodedList;

// Separate struct for dictionary elements
typedef struct {
    char* key;
    Bencoded* value;
} BencodedDictElement;

// Separate struct for dictionary data
typedef struct {
    size_t capacity;
    size_t size;
    BencodedDictElement* elements;
} BencodedDictionary;

// Bencoded struct using separate inner structs
struct Bencoded {
    BType type;
    union {
        char* string;
        long integer;
        BencodedList list;
        BencodedDictionary dictionary;
    } data;
};

// Function prototypes
void print_bencoded(Bencoded b, bool flush_output);
Bencoded* get_bencoded();
void free_bencoded(Bencoded* b);
void free_bencoded_inner(Bencoded* b);
Bencoded* get_dict_key(const char* search_str, Bencoded b);
const char *decode_bencode(const char *bencoded_value, Bencoded *container);
size_t encode_bencode(Bencoded b, char *target);

#endif // BENCODE_H
