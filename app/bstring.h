/**
 * @file bstring.h
 * @brief Header file for a binary safe string in C. handles string operations, resizing, and offers ability to free the string.
*/

#ifndef BSTRING_H
#define BSTRING_H

#include <string.h>
#include <stdlib.h>

// Define custom error codes
#define BSTRING_SUCCESS 0
#define BSTRING_ERR_MEMORY -1

// binary safe immutable string.
typedef struct
{
    size_t capacity;
    size_t size;
    unsigned char *chars;
} BString;

// a pop result indicating the success of the operation
typedef enum
{
    POP_SUCCESS,
    POP_EMPTY
} PopResult;

// a pop struct to return the character and the result of the operation
typedef struct
{
    int c;
    PopResult result;
} Pop;

/**
 * @brief Create a new BString object
 * 
 * @param capacity The initial capacity of the BString
 * @return BString* A pointer to the newly created BString object or NULL if memory allocation failed
 */
BString *bstring_new(size_t capacity);

/**
 * @brief Create a new BString object from a C string
 * 
 * @param cstr The C string to create the BString from
 * @return BString* A pointer to the newly created BString object
 */
BString *bstring_from_cstr(const char *cstr);

/**
 * @brief Free the memory allocated for a BString object
 * 
 * @param bstr The BString object to free
 */
void bstring_free(BString *bstr);

/**
 * @brief Append a character to the end of a BString
 * 
 * @param bstr The BString to append to
 * @param c The character to append
 * @return int 0 if successful, -1 if memory allocation failed
 */
int bstring_append_char(BString *bstr, char c);

/**
 * @brief Append a C string to the end of a BString
 * 
 * @param bstr The BString to append to
 * @param cstr The C string to append
 * @return int 0 if successful, -1 if memory allocation failed
 */
int bstring_append_cstr(BString *bstr, const char *cstr);

/**
 * @brief Append a BString to the end of a BString
 * 
 * @param bstr The BString to append to
 * @param bstr2 The BString to append
 * @return int 0 if successful, -1 if memory allocation failed
 */
int bstring_append_bstring(BString *bstr, BString *bstr2);


/**
 * @brief Append a byte array of size n to the end of a BString
 * 
 * @param bstr The BString to append to
 * @param bytes The byte array to append
 * @param n The number of bytes to append
 * @return int 0 if successful, -1 if memory allocation failed
*/
int bstring_append_bytes(BString *bstr, const unsigned char *bytes, size_t n);

/**
 * @brief Remove the last character of the BString
 * 
 * @param bstr The BString to get the length of
 * @return Pop The character that was removed, users should check the result field to see if the operation was successful
 */
Pop bstring_pop(BString *bstr);

/**
 * @brief compare two BStrings
 * 
 * @param bstr1 The first BString
 * @param bstr2 The second BString
 * @return int 0 if the strings are equal, a negative integer if bstr1 is less than bstr2, a positive integer if bstr1 is greater than bstr2
*/
int bstring_cmp(BString *bstr1, BString *bstr2);

/**
 * @brief compare a bstring to a c string
 * 
 * @param bstr The BString
 * @param cstr The C string
 * @return int 0 if the strings are equal, a negative integer if bstr is less than cstr, a positive integer if bstr is greater than cstr
*/
int bstring_cmp_cstr(BString *bstr, const char *cstr);

/**
 * @brief Get the BString as a cstring. The caller is responsible for ensuring the 
 * underlying char* is a valid cstring. Note this function allocates a new string,
 * in the future there may be implementations to simply add a null terminator to the underlying buffer and return that instead.
 * 
 * @param bstr The BString
 * @return char* The BString as a C string
*/
char *bstring_to_cstr(BString *bstr);


#endif