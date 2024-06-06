/**
 * @file bencode.c
 * @brief This file contains the implementation of a Bencode decoder in C.
 * Bencode is a data encoding format used in BitTorrent.
 * The code provides functions to decode Bencoded strings, integers, and lists.
 * It also includes a main function that takes a Bencoded string as input and decodes it.
 * The decoded value is then printed to the console.
 */
#include "bencode.h"


size_t route_bencode(Bencoded b, char *target);

/**
 * Compares a BencodedString to a C string.
 * Since BencodedStrings are binary safe, this function will compare the two strings byte by byte.
 * Bencoded strings do not have a null terminator, so the size of the BencodedString is used to determine the length of the string.
 * We will iterate until the end of the BencodedString or the last character of the C string, whichever comes first.
 * @param s - the BencodedString to compare.
 * @param cmp - the C string to compare.
 * @return 0 if the strings are equal, a negative number if s is less than cmp, a positive number if s is greater than cmp.
*/
int bencode_strcmp(BencodedString s, const char *cmp)
{
    size_t i = 0;
    while (i < s.size && cmp[i] != '\0') {
        if (s.chars[i] != cmp[i]) {
            return (unsigned char)s.chars[i] - (unsigned char)cmp[i];
        }
        i++;
    }

    // if we reached the end of the BencodedString
    if (i == s.size) {
        // if the C string is at the end also this will be 0
        // if not, the BencodedString is less than the C string
        // so we should return a negative number
        return -1 * (unsigned char)cmp[i];
    }
    
    // the only other case is that the C string is shorter than the BencodedString
    return (unsigned char)s.chars[i];
}


/**
 * given a str and length, checks for a specific character returning a boolean if found.
 * @param stream - the byte stream to check.
 * @param length - the length of the stream.
 * @param c - the byte to look for.
 * @return true if the character is a digit, false otherwise.
*/
bool stream_contains(const char* stream, size_t length, char c) {
    for (size_t i = 0; i < length; i++) {
        if (stream[i] == c) {
            return true;
        }
    }
    return false;
}

/**
 * Prints the given Bencoded data structure.
 *
 * @param b The Bencoded data structure to print.
 * @param flush_output If true, flushes the output after printing.
 */
void print_bencoded(Bencoded b, bool flush_output)
{
    switch (b.type)
    {
        case INTEGER:
        {
            printf("%ld", b.data.integer);
            break;
        }

        case STRING:
        {
            printf("\"%.*s\"", (int)b.data.string.size, b.data.string.chars);
            break;
        }

        case LIST:
        {
            printf("[");
            for (size_t i = 0; i < b.data.list.size; i++)
            {
                print_bencoded(b.data.list.elements[i], false);
                if (i != b.data.list.size - 1)
                {
                    printf(",");
                }
            }
            printf("]");
            break;
        }

        case DICTIONARY:
        {
            printf("{");
            for (size_t i = 0; i < b.data.dictionary.size; i++)
            {
                BencodedDictElement el = b.data.dictionary.elements[i];
                printf("\"%.*s\":", (int)el.key.size, el.key.chars);
                print_bencoded(*el.value, false);
                if (i != b.data.dictionary.size - 1)
                {
                    printf(",");
                }
            }
            printf("}");
        }
    }

    if (flush_output)
    {
        printf("\n");
    }
}

/**
 * Checks if a character is a digit.
 *
 * @param c The character to check.
 * @return true if the character is a digit, false otherwise.
 */
bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

/**
 * Checks if a character represents the start of a bencoded integer.
 *
 * @param c The character to check.
 * @return true if the character is 'i', false otherwise.
 */
bool is_bencoded_int(char c)
{
    return c == 'i';
}

/**
 * Allocates memory for a new Bencoded data structure.
 *
 * @return A pointer to the newly allocated Bencoded data structure.
 */
Bencoded *get_bencoded()
{
    Bencoded *bcode = malloc(sizeof(Bencoded));
    if (bcode == NULL)
    {
        fprintf(stderr, "could not alloc memory for new bencode.");
        return NULL;
    };
    return bcode;
};

/**
 * Frees memory allocated for a bencoded string
 * @param b - the element to free.
 */
void free_bencoded_string(BencodedString b)
{
    free(b.chars);
}

/**
 * Frees the memory allocated for the elements of a Bencoded list.
 *
 * @param elements The elements of the list to free.
 * @param length The number of elements in the list.
 */
void free_bencoded_list_elements(Bencoded *elements, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        free_bencoded_inner(elements[i]);
    }

    free(elements);
};

/**
 * Frees the memory allocated for a bencoded dict.
 * @param elements The elements of the list to free.
 * @param length The number of elements in the list.
 */
void free_bencoded_dict_elements(BencodedDictElement *elements, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        free_bencoded_string(elements[i].key);
        free_bencoded(elements[i].value);
    }

    free(elements);
};

/**
 * Frees the memory allocated for a Bencoded data structure. Only frees the inner data structures.
 * This is for stack allocated Bencoded structs or for Bencoded structs within an array.
 *
 * @param b The Bencoded data structure to free.
 */
void free_bencoded_inner(Bencoded b)
{
    if (b.type == STRING)
    {
        free_bencoded_string(b.data.string);
    };

    if (b.type == LIST)
    {
        free_bencoded_list_elements(b.data.list.elements, b.data.list.size);
    };

    if (b.type == DICTIONARY)
    {
        free_bencoded_dict_elements(b.data.dictionary.elements, b.data.dictionary.size);
    }
};

/**
 * Frees the memory allocated for a Bencoded data structure.
 *
 * @param b The Bencoded data structure to free.
 */
void free_bencoded(Bencoded *b)
{
    free_bencoded_inner(*b);
    free(b);
};

/**
 * a decoding function takes a string to decode and the containter to decode it into. It will return a pointer to the new "head"
 * of the original stream it was parsing. for primitive types, if should end on a a null terminator.
 * It will return the number of bytes read. If 0 the caller should check for errors in errno.
 * The caller is repsonsible for freeing the memory allocated for the Bencoded string.
 * @param bencoded_string the string to decode.
 * @param stream_length - the length of the string to decode.
 * @param container the container to decode the string into.
 * @return the amount of bytes read. 
 */
size_t decode_bencoded_string(const char *bencoded_string, size_t stream_length, Bencoded *container)
{
    if (!stream_contains(bencoded_string, stream_length, ':')) {
        fprintf(stderr, "ERR: Invalid bencoded string, no colon found\n");
        errno = PARSER_ERR_PARTIAL;
        return 0;
    }

    char *endptr;
    long encoded_length = strtol(bencoded_string, &endptr, 10);

    if (*endptr != ':') {
        fprintf(stderr, "Invalid encoded value: %s\n", bencoded_string);
        errno = PARSER_ERR_SYNTAX;
        return 0;
    }

    endptr++;
    long bytes_read = endptr - bencoded_string;
    long bytes_remaining = stream_length - bytes_read;

    if (bytes_remaining < encoded_length) {
        fprintf(stderr, "ERR: String length will cause out of bounds read.\n");
        errno = PARSER_ERR_PARTIAL;
        return 0;
    }

    char *decoded_str = (char *)malloc(sizeof(char) * encoded_length);
    if (!decoded_str) {
        fprintf(stderr, "ERR: Program out of heap memory (decoding string)\n");
        errno = PARSER_ERR_MEMORY;
        return 0;
    }

    memcpy(decoded_str, endptr, encoded_length);
    container->data.string.chars = decoded_str;
    container->data.string.size = encoded_length;
    endptr += encoded_length;

    return endptr - bencoded_string;
}

/**
 * This decoding function takes a bencoded string and a container to decode it into. 
 * It returns the number of bytes read from the input string.
 * For primitive types, it should end on a null terminator.
 * 
 * @param bencoded_string The bencoded string to decode.
 * @param stream_length The length of the bencoded string.
 * @param container The container to decode the string into.
 * @return The number of bytes read from the input string. if 0 the caller should check for errors in errno.
 */
size_t decode_bencoded_integer(const char *bencoded_integer, size_t stream_length, Bencoded *container)
{
    if (!stream_contains(bencoded_integer, stream_length, 'e'))
    {
        fprintf(stderr, "ERR: invalid bencoded integer, no end found\n");
        errno = PARSER_ERR_PARTIAL;
        return 0;
    }

    char *endptr;
    // plus one to skip the 'i' at the start.
    long integer = strtol(bencoded_integer + 1, &endptr, 10);
    // this means that the integer was not a valid integer.
    // either because there was no integer to convert, the integer was too long,
    // or the integer was not followed by an 'e'.
    if (*endptr != 'e')
    {
        fprintf(stderr, "ERR conversion of integer failed. coverting %s\n", endptr);
        errno = PARSER_ERR_SYNTAX;
        return 0;
    };

    container->data.integer = integer;
    return (endptr + 1) - bencoded_integer; // the number of bytes read. 
}

/**
 * a decoding function takes a string to decode and the containter to decode it into.
 * This function is specifically for decoding lists. It will return the number of bytes read.
 * If 0 the caller should check for errors in errno.
 * @param bencoded_list the string to decode.
 * @param stream_length the length of the string to decode.
 * @param container the container to decode the string into.
 * @return the number of bytes read from the stream.
 */
size_t decode_bencoded_list(const char *bencoded_list, size_t stream_length, Bencoded *container)
{
    size_t capacity = 64; // arbitrary default for now;
    Bencoded *elements = malloc(sizeof(Bencoded) * capacity);
    if (elements == NULL)
    {
        fprintf(stderr, "ERR heap out of memory, decoding list: %s\n", bencoded_list);
        errno = PARSER_ERR_MEMORY;
        return 0;
    }

    // setup the list for some pushing.
    const char *endptr = bencoded_list + 1; // skip the 'l' at the start.
    size_t list_size = 0;

    while (*endptr != 'e')
    {
        // get the number of bytes read next.
        size_t nbytes = decode_bencode(
            endptr, 
            stream_length - (endptr - bencoded_list), 
            &elements[list_size++]
        );

        if (nbytes == 0)
        {
            // errno should be set elsewhere.
            free_bencoded_list_elements(elements, list_size);
            return 0;
        }

        // move the pointer to the next element.
        endptr += nbytes;

        if (list_size == capacity)
        {
            capacity *= 2;
            Bencoded *tmp = realloc(elements, sizeof(Bencoded) * capacity);
            if (tmp == NULL)
            {
                free_bencoded_list_elements(elements, list_size);
                errno = PARSER_ERR_MEMORY;
                return 0;
            }
            elements = tmp;
        }
    }

    endptr++; // include the trailing 'e' in the count.
    container->data.list.size = list_size;
    container->data.list.elements = elements;
    return endptr - bencoded_list;
}

/**
 * a decoding function takes a string to decode and the containter to decode it into.
 * This function is specifically for decoding dictonaries. It will return the number of bytes read.
 * If 0 the caller should check for errors in errno.
 * @param bencoded_list the string to decode.
 * @param stream_length the length of the string to decode.
 * @param container the container to decode the string into.
 * @return the number of bytes read from the stream.
 */
size_t decode_bencoded_dictionary(const char *bencoded_dict, long stream_length, Bencoded *container)
{
    size_t capacity = 64; // arbitrary default for now;
    BencodedDictElement *elements = malloc(sizeof(BencodedDictElement) * capacity);
    if (elements == NULL)
    {
        fprintf(stderr, "ERR heap out of memory, decoding dictionary: %s\n", bencoded_dict);
        errno = PARSER_ERR_MEMORY;
        return 0;
    }

    const char *endptr = bencoded_dict + 1; // skip the 'd' at the start.
    size_t size = 0;
    while (*endptr != 'e')
    {
        // the key container will be copied into the key because it is a known size.
        Bencoded key_container;
        size_t nbytes_key = decode_bencode(
            endptr, 
            stream_length - (endptr - bencoded_dict), 
            &key_container);

        if (nbytes_key == 0)
        {
            free_bencoded_dict_elements(elements, size);
            return nbytes_key;
        }
        else if (key_container.type != STRING)
        {
            fprintf(stderr, "dictionary key is not a string");
            errno = PARSER_ERR_SYNTAX;
            free_bencoded_inner(key_container);
            free_bencoded_dict_elements(elements, size);
            return -1; // unrecoverable.
        }

        endptr += nbytes_key;

        // this, however, is not known yet, so it must be heap alloc'd
        Bencoded *value_container = get_bencoded();
        if (value_container == NULL)
        {
            fprintf(stderr, "ERR could not allocate memory for bencoded container\n");
            errno = PARSER_ERR_MEMORY;
            free_bencoded_inner(key_container);
            free_bencoded_dict_elements(elements, size);
            return -1;
        }

        size_t nbytes_value = decode_bencode(
            endptr, 
            stream_length - (endptr - bencoded_dict), 
            value_container);

        if (nbytes_value == 0)
        {
            // errno should be set elsewhere.
            free_bencoded_inner(key_container);
            free_bencoded(value_container);
            free_bencoded_dict_elements(elements, size);
            return 0;
        }
   
        endptr += nbytes_value;

        // push the key and value into the dictionary.
        elements[size].key = key_container.data.string;
        // push the value into the dict.
        elements[size].value = value_container;
        size++;
        // resize the dictionary if necessary.
        if (size == capacity)
        {
            capacity *= 2;
            BencodedDictElement *tmp = realloc(elements, sizeof(BencodedDictElement) * capacity);
            if (tmp == NULL)
            {
                fprintf(stderr, "failed to resize the bencoded dictionary\n");
                errno = PARSER_ERR_MEMORY;
                free_bencoded_inner(key_container);
                free_bencoded(value_container);
                free_bencoded_dict_elements(elements, size);
                return 0;
            }
            elements = tmp;
        }
    }

    endptr++; // include the trailing 'e' in the count.
    container->data.dictionary.size = size;
    container->data.dictionary.elements = elements;

    return endptr - bencoded_dict;
}

/**
 * This is the top level decoding function. It will take a string to decode and the container to decode it into. It will return a pointer to the new "head"
 * of the original stream it was parsing. It will call the appropriate decoding function based on the type of the bencoded value.
 * @param bencoded_value the string to decode.
 * @param container the container to decode the string into.
 * @return a pointer to the new head of the stream.
 */
size_t decode_bencode(const char *bencoded_value, size_t stream_length, Bencoded *container)
{
    if (stream_length == 0)
    {
        fprintf(stderr, "ERR: invalid bencoded value, no end found\n");
        errno = PARSER_ERR_PARTIAL;
        return 0;
    };

    // string
    if (is_digit(bencoded_value[0]))
    {
        container->type = STRING;
        size_t nbytes = decode_bencoded_string(bencoded_value, stream_length, container);
        container->encoded_length = nbytes;
        return nbytes;
    }

    else if (is_bencoded_int(bencoded_value[0]))
    {
        container->type = INTEGER;
        size_t nbytes = decode_bencoded_integer(bencoded_value, stream_length, container);
        container->encoded_length = nbytes;
        return nbytes; // this as a trailing 'e' that we need to account for.
    }

    else if (bencoded_value[0] == 'l')
    {
        // todo: run some checks on the char* returned here.
        container->type = LIST;
        size_t nbytes = decode_bencoded_list(bencoded_value, stream_length, container);
        container->encoded_length = nbytes;
        return nbytes; // this as a trailing 'e' that we need to account for.
    }

    else if (bencoded_value[0] == 'd')
    {
        container->type = DICTIONARY;
        size_t nbytes = decode_bencoded_dictionary(bencoded_value, stream_length, container);
        container->encoded_length = nbytes;
        return nbytes; // this as a trailing 'e' that we need to account for.
    }

    else
    {
        fprintf(stderr, "Only strings + integers + lists + dicts are supported at the moment\n");
        fprintf(stderr, "Received: %.*s\n", (int)stream_length, bencoded_value);
        errno = PARSER_ERR_SYNTAX;
        return 0;
    }
}

size_t encode_string(Bencoded b, char *target)
{
    size_t nbytes = 0;
    size_t length = b.data.string.size;
    nbytes += sprintf(target, "%li:", length);

    for (size_t i = 0; i < length; i++)
    {
        target[nbytes++] = b.data.string.chars[i];
    }

    return nbytes;
}

size_t encode_integer(Bencoded b, char *target)
{
    size_t nbytes = 0;
    nbytes += sprintf(target, "i%lie", b.data.integer);
    return nbytes;
}

size_t encode_list(Bencoded b, char *target)
{
    size_t nbytes = 0;
    target[nbytes++] = 'l';

    for (size_t i = 0; i < b.data.list.size; i++)
    {
        nbytes += route_bencode(b.data.list.elements[i], target + nbytes);
    }

    target[nbytes++] = 'e';
    return nbytes;
}

size_t encode_dict(Bencoded b, char *target)
{
    size_t nbytes = 0;
    target[nbytes++] = 'd';

    for (size_t i = 0; i < b.data.dictionary.size; i++)
    {
        BencodedDictElement el = b.data.dictionary.elements[i];
        Bencoded key;
        key.type = STRING;
        key.data.string = el.key;
        nbytes += route_bencode(key, target + nbytes);
        nbytes += route_bencode(*el.value, target + nbytes);
    }

    target[nbytes++] = 'e';
    return nbytes;
}

size_t route_bencode(Bencoded b, char *target)
{
    if (b.type == STRING)
    {
        return encode_string(b, target);
    }

    if (b.type == INTEGER)
    {
        return encode_integer(b, target);
    }

    if (b.type == LIST)
    {
        return encode_list(b, target);
    }

    if (b.type == DICTIONARY)
    {
        return encode_dict(b, target);
    }

    return 0;
}

size_t encode_bencode(Bencoded b, char *target)
{
    size_t nbytes = route_bencode(b, target);
    target[nbytes] = 0;
    return nbytes;
}

/**
 * Gets a key from a bencoded dictionary.
 * @param search_str - the key to look for.
 * @return a pointer to the value if the key is found, else null.
 */
Bencoded *get_dict_key(const char *search_str, Bencoded b)
{
    if (b.type != DICTIONARY)
    {
        fprintf(stderr, "ERR: attempt to read key from none dictionary bencoded value");
        return NULL;
    }

    BencodedDictionary dict = b.data.dictionary;
    // for now perform a linear search through the keys until we find one matching;
    for (size_t i = 0; i < dict.size; i++)
    {
        BencodedDictElement el = dict.elements[i];
        if (bencode_strcmp(el.key, search_str) == 0)
        {
            return el.value;
        }
    }

    // not found
    return NULL;
}


