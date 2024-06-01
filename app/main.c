#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

bool is_bencoded_int(char c) {
    return c == 'i'; 
}

char* decode_bencoded_string(const char* bencoded_string) {
        char *endptr;
        // this should not fail considering we checked if it has a digit first.
        long length = strtol(bencoded_string, &endptr, 10); 

        if (endptr[0] == ':')
        {
            const char* start = endptr + 1;
            char* decoded_str = malloc(length + 1);
            strncpy(decoded_str, start, length);
            decoded_str[length] = '\0';
            return decoded_str;
        }
        else
        {
            fprintf(stderr, "Invalid encoded value: %s\n", bencoded_string);
            exit(1);
        }

}

char* decode_bencoded_integer(const char* bencoded_integer) {
    const char* start = bencoded_integer + 1; // cut out the encoding character, this is already checked.
    const char* end = strchr(bencoded_integer, 'e');

    if (end == NULL)
    {
        fprintf(stderr, "Invalid encoded value %s\n", bencoded_integer);
        exit(1);
    }

    size_t length = end - start;
    char* ascii_integer = malloc(length + 1);
    if (ascii_integer == NULL)
    {
        fprintf(stderr, "ERR program out of heap memory");
        exit(1);
    }
    
    strncpy(ascii_integer, start, length);
    ascii_integer[length] = '\0';
    return ascii_integer; 
}

char* decode_bencode(const char* bencoded_value) {
    // string
    if (is_digit(bencoded_value[0])) 
    {
        return decode_bencoded_string(bencoded_value);
    }
    else if (is_bencoded_int(bencoded_value[0]))
    {
        return decode_bencoded_integer(bencoded_value);
    }
    else
    {
        fprintf(stderr, "Only strings are supported at the moment\n");
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
    	// You can use print statements as follows for debugging, they'll be visible when running tests.
        // printf("Logs from your program will appear here!\n");
            
        // Uncomment this block to pass the first stage
        const char* encoded_str = argv[2];
        char* decoded_str = decode_bencode(encoded_str);
        printf("\"%s\"\n", decoded_str);
        free(decoded_str);
    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}
