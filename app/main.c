/**
 * @file main.c
 * @brief This file contains the implementation of a Bencode decoder in C.
 * Bencode is a data encoding format used in BitTorrent.
 * The code provides functions to decode Bencoded strings, integers, and lists.
 * It also includes a main function that takes a Bencoded string as input and decodes it.
 * The decoded value is then printed to the console.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

typedef enum 
{
    INTEGER,
    STRING,
    LIST
} BType;

typedef struct Bencoded Bencoded;

struct Bencoded {
    // a type flag.
    BType type;
    union {
        char* string; // if string type
        long integer; // if integer type.
        struct {
            size_t capacity; // how many items the list can fit.
            size_t size; /// how many items the data currently holds.
            Bencoded* elements; // the elements array. 
        } list;
    } data;
};

void print_bencoded(Bencoded b,  bool flush_output);
bool is_digit(char c);
bool is_bencoded_int(char c);
Bencoded* get_bencoded();
void free_bencoded(Bencoded* b);
const char* decode_bencoded_string(const char* bencoded_string, Bencoded* container);
const char* decode_bencoded_integer(const char* bencoded_integer, Bencoded* container);
const char* decode_bencoded_list(const char* bencoded_list, Bencoded* container);
const char* decode_bencode(const char* bencoded_value, Bencoded* container);

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
    }

    free(elements);
};

/**
 * Frees the memory allocated for a Bencoded data structure.
 *
 * @param b The Bencoded data structure to free.
 */
void free_bencoded(Bencoded* b) {
    if (b->type == STRING) {
        free(b->data.string);
    };

    if (b->type == LIST) {
        free_bencoded_list_elements(b->data.list.elements, b->data.list.size);
    };
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
    while(bencoded_list[0] != 'e') {
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

    container->data.list.size = size;
    container->data.list.capacity = capacity;
    container->data.list.elements = elements;
    return bencoded_list + 1;
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

    else
    {
        fprintf(stderr, "Only strings + integers + lists are supported at the moment\nReceived: %s\n", bencoded_value);
        exit(1);
    }
}

int main(int argc, char* argv[]) {
	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);

    if (argc < 3) {
        fprintf(stderr, "Usage: your_bittorrent.sh <command> <args>\n");
        return 1;
    }

    const char* command = argv[1];

    if (strcmp(command, "decode") == 0) {
        const char* encoded_str = argv[2];
        Bencoded* container = get_bencoded();
        if (container == NULL) {
            fprintf(stderr, "ERR could not allocate memory for bencoded container\n");
            return 1;
        }
        const char* endstr = decode_bencode(encoded_str, container);
        print_bencoded(*container, true);
        // todo handle freeing up this bish...
        free_bencoded(container);
    }
    else
    {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}
