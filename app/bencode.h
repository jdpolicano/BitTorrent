/**
 * @file bencode.h
 * @brief Header file for Bencode decoder in C.
 */
#ifndef BENCODE_H
#define BENCODE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "bstring.h"

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

// Separate struct for list data
typedef struct
{
    size_t size;
    Bencoded *elements;
} BencodedList;

// Separate struct for dictionary elements
typedef struct
{
    BString *key;
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
        BString *string;
        long integer;
        BencodedList list;
        BencodedDictionary dictionary;
    } data;
};

/**
 * Prints the given Bencoded data structure.
 * @param b The Bencoded data structure to print.
 * @param fd The file descriptor to print to.
 * @param flush_output If true, flushes the output after printing.
 */
void print_bencoded(Bencoded b, FILE* fd, bool flush_output);

/**
 * Allocates memory for a new Bencoded data structure.
 * @return A pointer to the newly allocated Bencoded data structure.
 */
Bencoded *get_bencoded();

/**
 * Frees the memory allocated for a Bencoded data structure.
 * @param b The Bencoded data structure to free.
 * @return void
*/
void free_bencoded(Bencoded *b);

/**
 * Frees the memory allocated for a Bencoded data structure's inner data.
 * Useful if the Bencoded data structure was stack allocated.
 * @param b The Bencoded data structure to free.
 * @return void
 */
void free_bencoded_inner(Bencoded b);

/**
 * Gets the Bencoded data structure at the given index in a list.
 * @param b The Bencoded data structure containing the list.
 * @param search_str the string to search for in the dictionary
*/
Bencoded *get_dict_key(Bencoded *b, const char *search_str);

/**
 * Decodes a Bencoded string into a Bencoded data structure.
 * @param container The Bencoded data structure to store the decoded data.
 * @param bencoded_value The Bencoded string to decode.
 * @param stream_length The length of the Bencoded string.
 * @return The size of the decoded data, or an error code less than 0; -1 for partial data, -2 for syntax error, -3 for memory error.
 */
int decode_bencode(Bencoded *container, const char *bencoded_value, size_t stream_length);

/**
 * Encodes a Bencoded data structure into a Bencoded string.
 * @param b The Bencoded data structure to encode.
 * @param target The target string to store the encoded data.
 * @return The size of the encoded data.
*/
size_t encode_bencode(Bencoded *b, char *target);


/**
 * Checks if a Bencoded data structure is of a certain type.
 * @param b The Bencoded data structure to check.
 * @param type The type to check for.
 * @return true if the Bencoded data structure is of the given type, false otherwise.
*/
bool typeis(Bencoded *b, BType type);


#endif // BENCODE_H
