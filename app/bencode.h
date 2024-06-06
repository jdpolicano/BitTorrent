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

// Define custom error codes
#define PARSER_SUCCESS 0
#define PARSER_ERR_PARTIAL -1
#define PARSER_ERR_SYNTAX -2
#define PARSER_ERR_MEMORY -3

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

// Bencoded struct from parsing a Bencoded string. The structs for creating a Bencoded string are separate.
struct Bencoded
{
    BType type;
    long encoded_length;
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
size_t decode_bencode(const char *bencoded_value, size_t stream_length, Bencoded *container);
size_t encode_bencode(Bencoded b, char *target);

#endif // BENCODE_H
