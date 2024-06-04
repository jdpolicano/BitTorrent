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
typedef enum
{
    INTEGER,
    STRING,
    LIST,
    DICTIONARY
} BType;

// Forward declaration of Bencoded
typedef struct Bencoded Bencoded;

// binary safe immutable string.
typedef struct
{
    size_t size;
    char *chars;
} BencodedString;

// Separate struct for list data
typedef struct
{
    size_t size;
    Bencoded *elements;
} BencodedList;

// Separate struct for dictionary elements
typedef struct
{
    BencodedString key;
    Bencoded *value;
} BencodedDictElement;

// Separate struct for dictionary data
typedef struct
{
    size_t size;
    BencodedDictElement *elements;
} BencodedDictionary;

// Bencoded struct using separate inner structs
struct Bencoded
{
    BType type;
    union
    {
        BencodedString string;
        long integer;
        BencodedList list;
        BencodedDictionary dictionary;
    } data;
};

// Function prototypes
void print_bencoded(Bencoded b, bool flush_output);
Bencoded *get_bencoded();
// frees a struct that was heap alloc'd
void free_bencoded(Bencoded *b); 
// frees just the inner allocations if the struct was stack alloc'd or part
// of an array.
void free_bencoded_inner(Bencoded b);
Bencoded *get_dict_key(const char *search_str, Bencoded b);
const char *decode_bencode(const char *bencoded_value, Bencoded *container);
size_t encode_bencode(Bencoded b, char *target);

#endif // BENCODE_H
