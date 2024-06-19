/**
 * @file bencode.c
 * @brief This file contains the implementation of a Bencode decoder in C.
 * Bencode is a data encoding format used in BitTorrent.
 * The code provides functions to decode Bencoded strings, integers, and lists.
 * It also includes a main function that takes a Bencoded string as input and decodes it.
 * The decoded value is then printed to the console.
 */
#include "bencode.h"

// the address of the end of the source input.
#define SOURCE_END(src) (src.origin + src.length)

// the number of bytes left in the source input.
#define SOURCE_LEFT(src) (SOURCE_END(src) - src.position)

// whether the Parser is in an ok state.
#define PARSER_OK(p) (p->state == PARSER_SUCCESS)


typedef struct {
    const char *origin;
    const char *position;
    size_t length;
} Source;

typedef struct Parser {
    // the source to parse.
    Source src;
    // the current state of the parser.
    int state;
} Parser;

//////////////////////////////////////////////////////////////////////////
////////////////////////// Private Declarations //////////////////////////
//////////////////////////////////////////////////////////////////////////
/**
 * Initializes a parser with the given source.
 * @param p - the parser to initialize.
 * @param src - the source to parse.
 * @param length - the length of the source.
 * @return void
*/
void parser_init(Parser *p, const char *src, size_t length);

/**
 * Advances the parser by the given number of bytes.
 * If out of bounds, the parser will be set to an error state.
 * @param p - the parser to advance.
 * @param nbytes - the number of bytes to advance by.
 * @return void
*/
void parser_advance(Parser *p, size_t nbytes);

/**
 * Skips the current character in the parser.
 * If out of bounds, the parser will be set to an error state.
 * @param p - the parser to skip.
 * @return void
*/
void parser_skip(Parser *p);

/**
 * Sets the parser's position to a new location, setting an error if out of bounds.
 * @param p - the parser to set the position on.
 * @param position - the new position to set.
 * @return void
*/
void parser_set_position(Parser *p, char *position);

/**
 * Sets the parser to an error state.
 * @param p - the parser to set the error on.
 * @param error - the error code to set.
 * @return void
*/
void parser_set_error(Parser *p, int error);

/**
 * Checks if a character is a digit.
 * @param c - the character to check.
 * @return true if the character is a digit, false otherwise.
*/
bool source_contains(Parser p, char c);


/**
 * Prints the parser state to a give fd
 * @param parser - the source to print.
 * @param fd - the file descriptor to print to.
*/
void print_source(Parser p, FILE* fd);

/**
 * Checks if a character is a digit.
 * @param c - the character to check.
 * @return true if the character is a digit, false otherwise.
*/
bool is_digit(char c);

/**
 * Checks if a character represents the start of a bencoded integer.
 * @param c - the character to check.
 * @return true if the character is 'i', false otherwise.
*/
bool is_bencoded_int(char c);

/**
 * Frees memory allocated for a bencoded string
 * @param b - the element to free.
 */
void free_bencoded_string(Bencoded b);

/**
 * Frees the memory allocated for the elements of a Bencoded list.
 *
 * @param elements The elements of the list to free.
 * @param length The number of elements in the list.
 */
void free_bencoded_list_elements(Bencoded *elements, size_t length);

/**
 * Frees the memory allocated for a bencoded dict.
 * @param elements The elements of the list to free.
 * @param length The number of elements in the list.
 */
void free_bencoded_dict_elements(BencodedDictElement *elements, size_t length);


/**
 * Decodes a bencoded string and stores it in the given container
 * @param p - the source parser struct to decode.
 * @param container - the container to store the decoded string after parsing.
*/
void decode_bencode_inner(Parser *p, Bencoded *container);

/**
 * Decodes a bencoded string and stores it in the given container.
 * @param p - the source to decode.
 * @param container - the container to decode the string into.
 * @return void
*/
void decode_bencoded_string(Parser *p, Bencoded *container);

/**
 * Decodes a bencoded integer and stores it in the given container.
 * @param p - the source to decode.
 * @param container - the container to decode the integer into.
 * @return void
*/
void decode_bencoded_integer(Parser *p, Bencoded *container);

/**
 * Decodes a bencoded list and stores it in the given container.
 * @param p - the source to decode.
 * @param container - the container to decode the list into.
 * @return void
*/
void decode_bencoded_list(Parser *p, Bencoded *container);

/**
 * Decodes a bencoded dictionary and stores it in the given container.
 * @param p - the source to decode.
 * @param container - the container to decode the dictionary into.
 * @return void
*/
void decode_bencoded_dictionary(Parser *p, Bencoded *container);


/**
 * Encodes a bencoded string and stores it in the given target buffer.
 * @param b - the bencoded string to encode.
 * @param target - the target buffer to store the encoded string.
 * @return the number of bytes written to the target buffer.
*/
size_t encode_string(Bencoded *b, char *target);

/**
 * Encodes a bencoded integer and stores it in the given target buffer.
 * @param b - the bencoded integer to encode.
 * @param target - the target buffer to store the encoded integer.
 * @return the number of bytes written to the target buffer.
*/
size_t encode_integer(Bencoded *b, char *target);

/**
 * Encodes a bencoded list and stores it in the given target buffer.
 * @param b - the bencoded list to encode.
 * @param target - the target buffer to store the encoded list.
 * @return the number of bytes written to the target buffer.
*/
size_t encode_list(Bencoded *b, char *target);

/**
 * Encodes a bencoded dictionary and stores it in the given target buffer.
 * @param b - the bencoded dictionary to encode.
 * @param target - the target buffer to store the encoded dictionary.
 * @return the number of bytes written to the target buffer.
*/
size_t encode_dict(Bencoded *b, char *target);


/**
 * Routes the bencode encoding to the correct function.
 * @param b - the bencoded value to encode.
 * @param target - the target buffer to store the encoded value.
 * @return the number of bytes written to the target buffer.
*/
size_t route_bencode(Bencoded *b, char *target);

/////////////////////////////////////////////////////////////////////////////
////////////////////////// Private Implementations //////////////////////////
/////////////////////////////////////////////////////////////////////////////
void parser_init(Parser *p, const char *src, size_t length) {
    p->src.origin = src;
    p->src.position = src;
    p->src.length = length;
    p->state = PARSER_SUCCESS;
}

void parser_advance(Parser *p, size_t nbytes) {
    if (p->src.position + nbytes > SOURCE_END(p->src)) {
        parser_set_error(p, PARSER_ERR_PARTIAL);
        return;
    }
    p->src.position += nbytes;
}

void parser_skip(Parser *p) {
    if (p->src.position + 1 > SOURCE_END(p->src)) {
        parser_set_error(p, PARSER_ERR_PARTIAL);
        return;
    }
    p->src.position++;
}

void parser_set_position(Parser *p, char *position) {
    if (position < p->src.origin || position > SOURCE_END(p->src)) {
        // todo add a state for internal errors.
        parser_set_error(p, PARSER_ERR_PARTIAL);
        return;
    }
    p->src.position = position;
}

void parser_set_error(Parser *p, int error) {
    p->state = error;
}

bool source_contains(Parser p, char c) {
    const char *tmp = p.src.position;
    const char *end = SOURCE_END(p.src);
    while (tmp < end) {
        if (*tmp == c) {
            return true;
        }
        tmp++;
    }
    return false;
}

void print_source(Parser p, FILE* fd) {
    fprintf(fd, "Source: %.*s\n", (int)SOURCE_LEFT(p.src), p.src.position);   
}

bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

bool is_bencoded_int(char c)
{
    return c == 'i';
}

void free_bencoded_string(Bencoded b)
{
    bstring_free(b.data.string);
}

void free_bencoded_list_elements(Bencoded *elements, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        free_bencoded_inner(elements[i]);
    }

    free(elements);
};


void free_bencoded_dict_elements(BencodedDictElement *elements, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        bstring_free(elements[i].key);
        free_bencoded(elements[i].value);
    }

    free(elements);
};


void decode_bencoded_string(Parser *p, Bencoded *container)
{
    if (!source_contains(*p, ':')) {
        fprintf(stderr, "ERR: Invalid bencoded string, no colon found\n");
        parser_set_error(p, PARSER_ERR_PARTIAL);
        return;
    }

    char *endptr;
    long encoded_length = strtol(p->src.position, &endptr, 10);

    if (*endptr != ':') {
        fprintf(stderr, "ERR: Invalid bencoded string, found non-integer before colon\n");
        parser_set_error(p, PARSER_ERR_SYNTAX);
        return;
    }

    parser_set_position(p, endptr + 1);

    if (SOURCE_LEFT(p->src) < encoded_length) {
        fprintf(stderr, "ERR: Invalid bencoded string, read would fall out of bounds\n");
        parser_set_error(p, PARSER_ERR_PARTIAL);
        return;
    }

    BString *bstring = bstring_new(encoded_length);
    if (bstring == NULL) {
        fprintf(stderr, "ERR: Program out of heap memory (decoding string)\n");
        parser_set_error(p, PARSER_ERR_MEMORY);
        return;
    }

    bstring_append_bytes(bstring, (unsigned const char *)p->src.position, encoded_length);
    container->data.string = bstring;
    parser_advance(p, encoded_length);
    return;
}


void decode_bencoded_integer(Parser *p, Bencoded *container)
{
    if (!source_contains(*p, 'e'))
    {
        fprintf(stderr, "ERR: Invalid bencoded integer, no 'e' found\n");
        parser_set_error(p, PARSER_ERR_PARTIAL);
        return;
    }

    parser_skip(p); // skip the 'i' at the start.
    char *endptr;
    // plus one to skip the 'i' at the start.
    long integer = strtol(p->src.position, &endptr, 10);
    // this means that the integer was not a valid integer.
    // either because there was no integer to convert, the integer was too long,
    // or the integer was not followed by an 'e'.
    if (endptr == p->src.position || *endptr != 'e')
    {
        fprintf(stderr, "ERR conversion of integer failed. coverting\n");
        parser_set_error(p, PARSER_ERR_SYNTAX);
        return;
    };

    parser_set_position(p, endptr + 1); // skip the trailing 'e';
    container->data.integer = integer;
}

void decode_bencoded_list(Parser *p, Bencoded *container)
{
    size_t capacity = 64; // arbitrary default for now;
    Bencoded *elements = malloc(sizeof(Bencoded) * capacity);
    if (elements == NULL)
    {
        fprintf(stderr, "ERR heap out of memory, decoding list\n");
        parser_set_error(p, PARSER_ERR_MEMORY);
        return;
    }

    // setup the list for some pushing.
    parser_skip(p); // skip the 'l' at the start.
    size_t list_size = 0;

    while (*p->src.position != 'e')
    {
        // get the number of bytes read next.
        decode_bencode_inner(p, &elements[list_size++]);

        if (!PARSER_OK(p))
        {
            free_bencoded_list_elements(elements, list_size);
            return;
        }

        if (list_size == capacity)
        {
            capacity *= 2;
            Bencoded *tmp = realloc(elements, sizeof(Bencoded) * capacity);
            if (tmp == NULL)
            {
                free_bencoded_list_elements(elements, list_size);
                parser_set_error(p, PARSER_ERR_MEMORY);
                return;
            }
            elements = tmp;
        }
    }

    parser_skip(p); // skip the trailing 'e'
    container->data.list.size = list_size;
    container->data.list.elements = elements;
}

void decode_bencoded_dictionary(Parser *p, Bencoded *container)
{
    size_t capacity = 64; // arbitrary default for now;
    BencodedDictElement *elements = malloc(sizeof(BencodedDictElement) * capacity);
    if (elements == NULL)
    {
        fprintf(stderr, "ERR heap out of memory, decoding dictionary\n");
        parser_set_error(p, PARSER_ERR_MEMORY);
        return;
    }

    parser_skip(p); // skip the 'd' at the start.
    size_t size = 0;
    while (*p->src.position != 'e')
    {
        // the key container will be copied into the key because it is a known size.
        Bencoded key_container;
        decode_bencode_inner(p, &key_container);

        if (!PARSER_OK(p))
        {
            free_bencoded_dict_elements(elements, size);
            return;
        }

        else if (key_container.type != STRING)
        {
            fprintf(stderr, "dictionary key is not a string");
            parser_set_error(p, PARSER_ERR_SYNTAX);
            free_bencoded_inner(key_container);
            free_bencoded_dict_elements(elements, size);
            return;
        }

        // this, however, is not known yet, so it must be heap alloc'd
        Bencoded *value_container = get_bencoded();
        if (value_container == NULL)
        {
            fprintf(stderr, "ERR could not allocate memory for bencoded container\n");
            parser_set_error(p, PARSER_ERR_MEMORY);
            free_bencoded_inner(key_container);
            free_bencoded_dict_elements(elements, size);
            return;
        }

        decode_bencode_inner(p, value_container); 
    
        if (!PARSER_OK(p))
        {
            // errno should be set elsewhere.
            free_bencoded_inner(key_container);
            free_bencoded(value_container);
            free_bencoded_dict_elements(elements, size);
            return;
        }

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
                parser_set_error(p, PARSER_ERR_MEMORY);
                free_bencoded_inner(key_container);
                free_bencoded(value_container);
                free_bencoded_dict_elements(elements, size);
                return;
            }
            elements = tmp;
        }
    }

    parser_skip(p); // skip the trailing 'e'
    container->data.dictionary.size = size;
    container->data.dictionary.elements = elements;
}


void decode_bencode_inner(Parser *p, Bencoded *container)
{
    if (!PARSER_OK(p))
    {
        return;
    }

    // string
    if (is_digit(p->src.position[0]))
    {
        container->type = STRING;
        const char* start = p->src.position;
        decode_bencoded_string(p, container);
        container->encoded_length = p->src.position - start;
        return;
    }

    else if (is_bencoded_int(p->src.position[0]))
    {
        container->type = INTEGER;
        const char* start = p->src.position;
        decode_bencoded_integer(p, container);
        container->encoded_length = p->src.position - start;
        return;
    }

    else if (p->src.position[0] == 'l')
    {
        container->type = LIST;
        const char* start = p->src.position;
        decode_bencoded_list(p, container);
        container->encoded_length = p->src.position - start;
        return;
    }

    else if (p->src.position[0] == 'd')
    {
        container->type = DICTIONARY;
        const char* start = p->src.position;
        decode_bencoded_dictionary(p, container);
        container->encoded_length = p->src.position - start;
        return;
    }

    else
    {
        // todo add a state for invalid bencode type.
        p->state = PARSER_ERR_SYNTAX;
        return;
    }
}


size_t encode_string(Bencoded *b, char *target)
{
    size_t nbytes = 0;
    size_t length = b->data.string->size;
    nbytes += sprintf(target, "%li:", length);

    for (size_t i = 0; i < length; i++)
    {
        target[nbytes++] = b->data.string->chars[i];
    }

    return nbytes;
}

size_t encode_integer(Bencoded *b, char *target)
{
    size_t nbytes = 0;
    nbytes += sprintf(target, "i%lie", b->data.integer);
    return nbytes;
}

size_t encode_list(Bencoded *b, char *target)
{
    size_t nbytes = 0;
    target[nbytes++] = 'l';

    for (size_t i = 0; i < b->data.list.size; i++)
    {
        nbytes += route_bencode(&(b->data.list.elements[i]), target + nbytes);
    }

    target[nbytes++] = 'e';
    return nbytes;
}

size_t encode_dict(Bencoded *b, char *target)
{
    size_t nbytes = 0;
    target[nbytes++] = 'd';

    for (size_t i = 0; i < b->data.dictionary.size; i++)
    {
        BencodedDictElement el = b->data.dictionary.elements[i];
        Bencoded key;
        key.type = STRING;
        key.data.string = el.key;
        nbytes += route_bencode(&key, target + nbytes);
        nbytes += route_bencode(el.value, target + nbytes);
    }

    target[nbytes++] = 'e';
    return nbytes;
}

size_t route_bencode(Bencoded *b, char *target)
{
    if (b->type == STRING)
    {
        return encode_string(b, target);
    }

    if (b->type == INTEGER)
    {
        return encode_integer(b, target);
    }

    if (b->type == LIST)
    {
        return encode_list(b, target);
    }

    if (b->type == DICTIONARY)
    {
        return encode_dict(b, target);
    }

    return 0;
}
////////////////////////////////////////////////////////////////
////////////////////////// Public API //////////////////////////
////////////////////////////////////////////////////////////////
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

void free_bencoded_inner(Bencoded b)
{
    if (b.type == STRING)
    {
        free_bencoded_string(b);
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

void free_bencoded(Bencoded *b)
{
    free_bencoded_inner(*b);
    free(b);
};

int decode_bencode(Bencoded *container, const char *bencoded_value, size_t stream_length)
{
    Parser p; 
    parser_init(&p, bencoded_value, stream_length);
    decode_bencode_inner(&p, container);
    return p.state;
}

Bencoded *get_dict_key(Bencoded *b, const char *search_str)
{
    if (b->type != DICTIONARY)
    {
        fprintf(stderr, "ERR: attempt to read key from none dictionary bencoded value");
        return NULL;
    }

    BencodedDictionary dict = b->data.dictionary;
    // for now perform a linear search through the keys until we find one matching;
    for (size_t i = 0; i < dict.size; i++)
    {
        BencodedDictElement el = dict.elements[i];
        if (bstring_cmp_cstr(*el.key, search_str) == 0)
        {
            return el.value;
        }
    }

    // not found
    return NULL;
}

size_t encode_bencode(Bencoded *b, char *target)
{
    size_t nbytes = route_bencode(b, target);
    target[nbytes] = 0;
    return nbytes;
}


void print_bencoded(Bencoded b, FILE* fd, bool flush_output)
{
    switch (b.type)
    {
        case INTEGER:
        {
            fprintf(fd, "%ld", b.data.integer);
            break;
        }

        case STRING:
        {
            fprintf(fd, "\"%.*s\"", (int)b.data.string->size, b.data.string->chars);
            break;
        }

        case LIST:
        {
            fprintf(fd, "[");
            for (size_t i = 0; i < b.data.list.size; i++)
            {
                Bencoded el = b.data.list.elements[i];
                print_bencoded(b.data.list.elements[i], fd, false);
                if (i != b.data.list.size - 1)
                {
                    fprintf(fd, ",");
                }
            }
            fprintf(fd, "]");
            break;
        }

        case DICTIONARY:
        {
            fprintf(fd, "{");
            for (size_t i = 0; i < b.data.dictionary.size; i++)
            {
                fprintf(fd, "\"%.*s\":", 
                (int)b.data.dictionary.elements[i].key->size, 
                b.data.dictionary.elements[i].key->chars);

                print_bencoded(*b.data.dictionary.elements[i].value, fd, false);
                if (i != b.data.dictionary.size - 1)
                {
                    fprintf(fd, ",");
                }
            }
            fprintf(fd, "}");
        }
    }

    if (flush_output)
    {
        fprintf(fd, "\n");
    }
}

