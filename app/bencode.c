/**
 * @file bencode.c
 * @brief This file contains the implementation of a Bencode decoder in C.
 * Bencode is a data encoding format used in BitTorrent.
 * The code provides functions to decode Bencoded strings, integers, and lists.
 * It also includes a main function that takes a Bencoded string as input and decodes it.
 * The decoded value is then printed to the console.
 */
#include "bencode.h"

/**
 * Prints the given Bencoded data structure.
 *
 * @param b The Bencoded data structure to print.
 * @param flush_output If true, flushes the output after printing.
 */
void print_bencoded(Bencoded b, bool flush_output)
{
    switch(b.type) {
        case INTEGER:  {
            printf("%ld", b.data.integer);
            break;
        }

        case STRING: {
            printf("\"%s\"", b.data.string);
            break;
        }

        case LIST: {
            printf("[");
            for (size_t i = 0; i < b.data.list.size; i++) {
                print_bencoded(b.data.list.elements[i], false);
                if (i != b.data.list.size - 1) {
                    printf(", ");
                }
            }
            printf("]");
            break;
        }

        case DICTIONARY: {
            printf("{");
            for (size_t i = 0; i < b.data.dictionary.size; i++) {
                BencodedDictElement el = b.data.dictionary.elements[i];
                printf("\"%s\": ", el.key);
                print_bencoded(*el.value, false);
                if (i != b.data.dictionary.size - 1) {
                    printf(", ");
                }
            }
            printf("}");
        }
    }

    if (flush_output) {
        printf("\n");
    }
}

/**
 * Checks if a character is a digit.
 *
 * @param c The character to check.
 * @return true if the character is a digit, false otherwise.
 */
bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

/**
 * Checks if a character represents the start of a bencoded integer.
 *
 * @param c The character to check.
 * @return true if the character is 'i', false otherwise.
 */
bool is_bencoded_int(char c) {
    return c == 'i'; 
}

/**
 * Allocates memory for a new Bencoded data structure.
 *
 * @return A pointer to the newly allocated Bencoded data structure.
 */
Bencoded* get_bencoded() {
    Bencoded *bcode = malloc(sizeof(Bencoded));
    if (bcode == NULL) {
        fprintf(stderr, "could not alloc memory for new bencode.");
        return NULL;
    };
    return bcode;
};

/**
 * Frees the memory allocated for the elements of a Bencoded list.
 *
 * @param elements The elements of the list to free.
 * @param length The number of elements in the list.
 */
void free_bencoded_list_elements(Bencoded* elements, size_t length) {
    for (size_t i = 0; i < length; i++) 
    {
        Bencoded el = elements[i];
        if (el.type == STRING) 
        {
            free(el.data.string);
        }

        if (el.type == LIST) 
        {
            free_bencoded_list_elements(el.data.list.elements, el.data.list.size);
        }

        if (el.type == DICTIONARY) 
        {
            for (size_t j = 0; j < el.data.dictionary.size; j++) 
            {
                free(el.data.dictionary.elements[j].key);
                free_bencoded(el.data.dictionary.elements[j].value);
            }
            free(el.data.dictionary.elements);
        }
    }

    free(elements);
};

/**
 * Frees the memory allocated for a Bencoded data structure. Only frees the inner data structures.
 * This is for stack allocated Bencoded structs or for Bencoded structs within an array.
 *
 * @param b The Bencoded data structure to free.
 */
void free_bencoded_inner(Bencoded* b) {
    if (b->type == STRING) {
        free(b->data.string);
    };

    if (b->type == LIST) {
        free_bencoded_list_elements(b->data.list.elements, b->data.list.size);
    };

    if (b->type == DICTIONARY) {
        for (size_t i = 0; i < b->data.dictionary.size; i++) {
            free(b->data.dictionary.elements[i].key);
            free_bencoded(b->data.dictionary.elements[i].value);
        }
        free(b->data.dictionary.elements);
    }
};

/**
 * Frees the memory allocated for a Bencoded data structure.
 *
 * @param b The Bencoded data structure to free.
 */
void free_bencoded(Bencoded* b) {
    free_bencoded_inner(b);
    free(b);
};

/**
 * a decoding function takes a string to decode and the containter to decode it into. It will return a pointer to the new "head"
 * of the original stream it was parsing. for primitive types, if should end on a a null terminator.  
 * @param bencoded_string the string to decode.
 * @param container the container to decode the string into.
 * @return a pointer to the new head of the stream.
*/
const char* decode_bencoded_string(const char* bencoded_string, Bencoded* container) {
        char *midptr;
        // this should not fail considering we checked if it has a digit first.
        long length = strtol(bencoded_string, &midptr, 10); 
        if (midptr[0] == ':')
        {
            const char* start_body = midptr + 1;
            char* decoded_str = malloc(length + 1);
            if (decoded_str == NULL) 
            {
                fprintf(stderr, "ERR program out of heap memory (decoding string)");
                exit(1);
            }
            strncpy(decoded_str, start_body, length);
            decoded_str[length] = '\0';
            container->data.string = decoded_str;
            return start_body + length;
        }
        else
        {
            fprintf(stderr, "Invalid encoded value: %s\n", bencoded_string);
            exit(1);
        }

}

/**
 * a decoding function takes a string to decode and the containter to decode it into. It will return a pointer to the new "head"
 * of the original stream it was parsing. for primitive types, if should end on a a null terminator.  
 * @param bencoded_integer the string to decode.
 * @param container the container to decode the string into.
 * @return a pointer to the new head of the stream.
*/
const char* decode_bencoded_integer(const char* bencoded_integer, Bencoded* container) {
    char *endptr;
    long integer = strtol(bencoded_integer, &endptr, 10);

    // to-do: this needs to be more robust.
    if (endptr == bencoded_integer || endptr[0] != 'e') {
        fprintf(stderr, "ERR conversion of integer failed. coverting %s\n", endptr);
        exit(1);
    };

    container->data.integer = integer;
    return endptr + 1; 
}

/**
 * a decoding function takes a string to decode and the containter to decode it into. It will return a pointer to the new "head"
 * of the original stream it was parsing. This function is specifically for decoding lists. It will return a pointer to the new "head"
 * It parses the list by recursively calling decode_bencode on each element in the list.  
 * @param bencoded_list the string to decode.
 * @param container the container to decode the string into.
 * @return a pointer to the new head of the stream.
*/
const char* decode_bencoded_list(const char* bencoded_list, Bencoded* container) {
    size_t capacity = 1024; // arbitrary default for now;
    Bencoded* elements = malloc(sizeof(Bencoded) * capacity);
    if (elements == NULL) 
    {
        fprintf(stderr, "ERR heap out of memory, decoding list -> %s\n", bencoded_list);
        exit(1);
    }
    // setup the list for some pushing.
    size_t size = 0;
    // this feels really flimsy...
    while(bencoded_list[0] != 'e' && bencoded_list[0] != '\0') {
        bencoded_list = decode_bencode(bencoded_list, &elements[size++]);
        if (size == capacity) {
            capacity *= 2;
            Bencoded *tmp = realloc(elements, capacity);
            if (tmp == NULL) 
            {
                fprintf(stderr, "failed to resize the bencoded list\n");
                exit(1);
            }
            elements = tmp;
        }
    }

    if (bencoded_list[0] != 'e') 
    {
        fprintf(stderr, "bencoded list terminated unexpectedly");
        exit(1);
    }

    container->data.list.size = size;
    container->data.list.capacity = capacity;
    container->data.list.elements = elements;

    return bencoded_list + 1;
}

/**
 * a decoding function takes a string to decode and the containter to decode it into. It will return a pointer to the new "head"
 * of the original stream it was parsing. This function is specifically for decoding dictionaries. It will return a pointer to the new "head".  
 * @param bencoded_dict the string to decode.
 * @param container the container to decode the string into.
 * @return a pointer to the new head of the stream.
*/
const char* decode_bencoded_dictionary(const char* bencoded_dict, Bencoded* container) {
    size_t capacity = 1024;
    BencodedDictElement* elements = malloc(sizeof(BencodedDictElement) * capacity);
    if (elements == NULL) 
    {
        fprintf(stderr, "ERR heap out of memory, decoding dictionary -> %s\n", bencoded_dict);
        exit(1);
    }
    size_t size = 0;


    while (bencoded_dict[0] != 'e' && bencoded_dict[0] != '\0') {
        // the key container will be discarded after this loop.
        // so no need to allocate dynamic memory for it.
        Bencoded key_container;
        bencoded_dict = decode_bencode(bencoded_dict, &key_container);
        if (key_container.type != STRING) 
        {
            fprintf(stderr, "dictionary key is not a string");
            exit(1);
        }

        // this, however, will be kept.
        Bencoded *value_container = get_bencoded();
        if (value_container == NULL) 
        {
            fprintf(stderr, "ERR could not allocate memory for bencoded container\n");
            exit(1);
        }
        bencoded_dict = decode_bencode(bencoded_dict, value_container);

        // push the key and value into the dictionary.
        elements[size].key = key_container.data.string;
        elements[size].value = value_container;
        size++;
        // resize the dictionary if necessary.
        if (size == capacity) {
            capacity *= 2;
            BencodedDictElement *tmp = realloc(elements, capacity);
            if (tmp == NULL) 
            {
                fprintf(stderr, "failed to resize the bencoded dictionary\n");
                exit(1);
            }
            elements = tmp;
        }
    }

    if (bencoded_dict[0] != 'e') 
    {
        fprintf(stderr, "bencoded dictionary terminated unexpectedly");
        exit(1);
    }

    container->data.dictionary.size = size;
    container->data.dictionary.capacity = capacity;
    container->data.dictionary.elements = elements;
    return bencoded_dict + 1;
}

/**
 * This is the top level decoding function. It will take a string to decode and the container to decode it into. It will return a pointer to the new "head"
 * of the original stream it was parsing. It will call the appropriate decoding function based on the type of the bencoded value. 
 * @param bencoded_value the string to decode.
 * @param container the container to decode the string into.
 * @return a pointer to the new head of the stream.
*/
const char* decode_bencode(const char* bencoded_value, Bencoded* container) {
    // string
    if (is_digit(bencoded_value[0])) 
    {
        // todo: run some checks on the char* returned here.
        container->type = STRING;
        return decode_bencoded_string(bencoded_value, container);
    }

    else if (is_bencoded_int(bencoded_value[0]))
    {
        // todo: run some checks on the char* returned here.
        container->type = INTEGER;
        return decode_bencoded_integer(bencoded_value + 1, container);
    }

    else if (bencoded_value[0] == 'l')
    {
        // todo: run some checks on the char* returned here.
        container->type = LIST;
        return decode_bencoded_list(bencoded_value + 1, container);
    }

    else if (bencoded_value[0] == 'd')
    {
        container->type = DICTIONARY;
        return decode_bencoded_dictionary(bencoded_value + 1, container);
    }

    else
    {
        fprintf(stderr, "Only strings + integers + lists are supported at the moment\nReceived: %s\n", bencoded_value);
        exit(1);
    }
}


/**
 * Gets a key from a bencoded dictionary.
 * @param search_str - the key to look for.
 * @return a pointer to the value if the key is found, else null.
*/
Bencoded* get_dict_key(const char* search_str, Bencoded b)
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
        if (strcmp(search_str, el.key) == 0)
        {
            return el.value;
        }
    }

    // not found
    return NULL;
}
