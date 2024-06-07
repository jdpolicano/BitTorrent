/**
 * @file main.c
 * @brief This file contains the implementation of a Bencode decoder in C.
 * Bencode is a data encoding format used in BitTorrent.
 * The code provides functions to decode Bencoded strings, integers, and lists.
 * It also includes a main function that takes a Bencoded string as input and decodes it.
 * The decoded value is then printed to the console.
 */
#include "bencode.h"
#include "url.h"
#include "bstring.h"
#include "bstring.h"
#include <openssl/sha.h>
#include <curl/curl.h>
#include <stdlib.h>

typedef struct {
    size_t size;
    char *content;
} FILE_CONTENT;

typedef struct {
    BString *data; 
} RESPONSE;

// print functions
void print_tracker_url(Bencoded announce);
void print_info(Bencoded info);
void print_hex(unsigned char *data, size_t size);
// misc functions
bool hash_bencoded(unsigned char* hash, Bencoded bencoded);
FILE_CONTENT read_file(const char *path);
// curl functions
static size_t handle_data(void *contents, size_t size, size_t nmemb, void *userp);
void get_tracker_response(Bencoded torrent);
void handle_bencoded_response(Bencoded response);

void get_tracker_response(Bencoded torrent)
{
    Bencoded *announce = get_dict_key("announce", torrent);
    if (announce == NULL)
    {
        fprintf(stderr, "ERR: 'announce' key not found in torrent meta");
        return;
    }

    const char *tracker_url = (const char *)announce->data.string.chars;
    size_t size = announce->data.string.size;

    URL *url = url_new(tracker_url, size);
    if (url == NULL)
    {
        fprintf(stderr, "ERR: failed to create URL object\n");
        return;
    }

    // set query param "info_hash" to the SHA1 hash of the info dictionary
    Bencoded *info = get_dict_key("info", torrent);
    if (info == NULL)
    {
        
        fprintf(stderr, "ERR: length key not found in info dict.");
        url_free(url);
        return;
    }

    unsigned char hash[SHA_DIGEST_LENGTH];
    if (!hash_bencoded(hash, *info))
    {
        fprintf(stderr, "ERR: failed to hash bencoded info");
        url_free(url);
        return;
    }  

    char *escaped_hash = curl_easy_escape(NULL, (char *)hash, SHA_DIGEST_LENGTH);
    if (escaped_hash == NULL)
    {
        fprintf(stderr, "ERR: failed to escape hash");
        url_free(url);
        return;
    }

    printf("Escaped Hash: %s\n", escaped_hash);
    // set query param "info_hash" to the SHA1 hash of the info dictionary
    if (url_append_query_param(url, "info_hash", escaped_hash) != URL_SUCCESS)
    {
        fprintf(stderr, "ERR: failed to append query param info_hash");
        url_free(url);
        return;
    }

    // set query param "peer_id" to a unique identifier
    if (url_append_query_param(url, "peer_id", "00112233445566778899") != URL_SUCCESS)
    {
        fprintf(stderr, "ERR: failed to append query param peer_id");
        url_free(url);
        return;
    }

    // set query param "port" to the port the client is listening on
    if (url_append_query_param(url, "port", "6881") != URL_SUCCESS)
    {
        fprintf(stderr, "ERR: failed to append query param port");
        url_free(url);
        return;
    }

    // set query param "uploaded" to the number of bytes uploaded
    if (url_append_query_param(url, "uploaded", "0") != URL_SUCCESS)
    {
        fprintf(stderr, "ERR: failed to append query param uploaded");
        url_free(url);
        return;
    }   

    // set query param "downloaded" to the number of bytes downloaded
    if (url_append_query_param(url, "downloaded", "0") != URL_SUCCESS)
    {
        fprintf(stderr, "ERR: failed to append query param downloaded");
        url_free(url);
        return;
    }

    Bencoded *length = get_dict_key("length", *info);
    if (length == NULL)
    {
        fprintf(stderr, "ERR: length key not found in info dict.");
        url_free(url);
        return;
    }

    char *length_str = malloc(sizeof(char) * length->encoded_length); // this should be guaranteed to be enough
    if (length_str == NULL)
    {
        
        fprintf(stderr, "ERR: could not allocate memory for length_str.");
        url_free(url);
        return;
    }

    sprintf(length_str, "%li", length->data.integer);

    if (url_append_query_param(url, "left", length_str) != URL_SUCCESS)
    {
        fprintf(stderr, "ERR: could not append query param left.");
        url_free(url);
        return;
    }

    if (url_append_query_param(url, "compact", "1") != URL_SUCCESS)
    {
        fprintf(stderr, "ERR: could not append query param compact.");
        url_free(url);
        return;
    }

    CURL *curl;
    CURLcode res;
    RESPONSE response_aggregator = {0};
    response_aggregator.data = bstring_new(1024);

    fprintf(stderr, "Response Aggregator: %p\n", response_aggregator.data);

    if (response_aggregator.data == NULL)
    {
        
        fprintf(stderr, "ERR: length key not found in info dict.");fprintf(stderr, "ERR: failed to allocate memory for response aggregator");
        url_free(url);
        return;
    }

    curl = curl_easy_init();

    if (curl)
    {
        char* url_str = bstring_to_cstr(url->data);

        curl_easy_setopt(curl, CURLOPT_URL, url_str);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handle_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_aggregator);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
    }
}


static size_t handle_data(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    RESPONSE *mem = (RESPONSE *)userp;
    // 
    if (bstring_append_bytes(mem->data, contents, realsize) != BSTRING_SUCCESS)
    {
        fprintf(stderr, "ERR: failed to append data to response aggregator");
        return 0;
    }

    Bencoded container;
    size_t nbytes = decode_bencode((const char*)mem->data->chars, mem->data->size, &container);

    if (nbytes == 0)
    {
        if (errno == PARSER_ERR_MEMORY || errno == PARSER_ERR_SYNTAX)
        {
            return 0;
        }

        return realsize;
    }

    handle_bencoded_response(container);
    return realsize;
}

void handle_bencoded_response(Bencoded b)
{
    if (b.type != DICTIONARY)
    {
        fprintf(stderr, "ERR: expected dictionary in response");
        return;
    }

    Bencoded *failure_reason = get_dict_key("failure reason", b);
    if (failure_reason != NULL)
    {
        if (failure_reason->type != STRING)
        {
            fprintf(stderr, "ERR: failure reason expected to be a string");
            return;
        }
        printf("Failure Reason: %.*s\n", (int)failure_reason->data.string.size, failure_reason->data.string.chars);
        return;
    }

    Bencoded *interval = get_dict_key("interval", b);
    if (interval == NULL)
    {
        fprintf(stderr, "ERR: interval key not found in response");
        return;
    }

    if (interval->type != INTEGER)
    {
        fprintf(stderr, "ERR: interval key expected to be an integer");
        return;
    }


    Bencoded *peers = get_dict_key("peers", b);
    if (peers == NULL)
    {
        fprintf(stderr, "ERR: peers key not found in response");
        return;
    }

    if (peers->type == STRING)
    {
        if (peers->data.string.size % 6 != 0)
        {
            fprintf(stderr, "ERR: invalid length for peers string");
            return;
        }

        for (size_t i = 0; i < peers->data.string.size; i += 6)
        {
            printf(
                "%u.%u.%u.%u:%u\n", 
                (unsigned char)peers->data.string.chars[i], 
                (unsigned char)peers->data.string.chars[i + 1],
                (unsigned char)peers->data.string.chars[i + 2], 
                (unsigned char)peers->data.string.chars[i + 3], 
                (unsigned char)peers->data.string.chars[i + 4] << 8 | (unsigned char)peers->data.string.chars[i + 5]);
        }
    }
}

void print_hex(unsigned char *data, size_t size)
{
    for (int i = 0; i < size; i++)
    {
        printf("%02x", data[i]);
    }
}

void print_torrent_meta(Bencoded torrent)
{
    if (torrent.type != DICTIONARY)
    {
        fprintf(stderr, "ERR: parse error, expected dictionary, but recevied something else");
        exit(1);
    }

    Bencoded *announce = get_dict_key("announce", torrent);
    if (announce == NULL)
    {
        fprintf(stderr, "ERR: 'announce' key not found in torrent meta");
        return;
    }
    print_tracker_url(*announce);

    Bencoded *info = get_dict_key("info", torrent);
    if (info == NULL)
    {
        fprintf(stderr, "ERR: 'info' key not found in torrent meta");
        return;
    }
    print_info(*info);
    return;
}

void print_tracker_url(Bencoded announce)
{
    if (announce.type != STRING)
    {
        fprintf(stderr, "ERR: announce key is expected to be a string.");
        return;
    }

    printf("Tracker URL: %.*s\n", (int)announce.data.string.size, announce.data.string.chars);
}

void print_info(Bencoded info)
{
    if (info.type != DICTIONARY)
    {
        fprintf(stderr, "ERR: info key is expected to be a dict.");
        return;
    }

    Bencoded *length = get_dict_key("length", info);
    if (length == NULL)
    {
        fprintf(stderr, "ERR: length key not found in info dict.");
        return;
    }

    if (length->type != INTEGER)
    {
        fprintf(stderr, "ERR: length key inside info expected to be an integer.");
        return;
    }

    printf("Length: %li\n", length->data.integer);

    // print the info hash
    char hash[SHA_DIGEST_LENGTH];
    if (!hash_bencoded((unsigned char *)hash, info))
    {
        fprintf(stderr, "ERR: failed to hash bencoded info");
        return;
    }

    printf("Info Hash: ");
    print_hex((unsigned char *)hash, SHA_DIGEST_LENGTH);
    printf("\n");

    // print piece length
    Bencoded *piece_length = get_dict_key("piece length", info);
    if (piece_length == NULL)
    {
        fprintf(stderr, "ERR: piece length key not found in info dict.");
        return;
    }

    if (piece_length->type != INTEGER)
    {
        fprintf(stderr, "ERR: piece length key inside info expected to be an integer.");
        return;
    }

    printf("Piece Length: %li\n", piece_length->data.integer);

    // print the pieces
    Bencoded *pieces = get_dict_key("pieces", info);
    if (pieces == NULL)
    {
        fprintf(stderr, "ERR: pieces key not found in info dict.");
        return;
    }

    if (pieces->type != STRING)
    {
        fprintf(stderr, "ERR: pieces key inside info expected to be a string.");
        return;
    }

    if (pieces->data.string.size % SHA_DIGEST_LENGTH != 0)
    {
        fprintf(stderr, "ERR: pieces key inside info has invalid length.");
        return;
    }

    printf("Piece Hashes:\n");

    for(size_t i = 0; i < pieces->data.string.size; i += SHA_DIGEST_LENGTH)
    {
        print_hex((unsigned char *)pieces->data.string.chars + i, SHA_DIGEST_LENGTH);
        printf("\n");
    }
}

bool hash_bencoded(unsigned char* hash, Bencoded b)
{
    char *buf = malloc(b.encoded_length);
    if (buf == NULL)
    {
        fprintf(stderr, "ERR: failed to allocate memory for bencoded string");
        return false;
    }
    size_t size = encode_bencode(b, buf);
    SHA1((unsigned char *)buf, size, hash);
    free(buf);
    return true;
}

FILE_CONTENT read_file(const char *path)
{
    FILE_CONTENT content = {0};

    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
        fprintf(stderr, "unable to open file at: %s\n", path);
        return content;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *data = malloc(size + 1);
    if (data == NULL)
    {
        fprintf(stderr, "ERR: failed to alloc buffer for file contents");
        return content; // NULL;
    }

    long nbytes = fread(data, sizeof(char), size, file);
    if (nbytes != size)
    {
        fprintf(stderr, "ERR: number of bytes read differs from file size.");
        return content; // NULL;
    }

    content.size = size;
    content.content = data;
    fclose(file);

    return content;
}

/**
 * @brief Main function to decode a Bencoded string and print the decoded value.
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line arguments.
 * @return 0 if successful, 1 if there was an error.
 */
int main(int argc, char *argv[])
{
    // Disable output buffering
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    if (argc < 3)
    {
        fprintf(stderr, "Usage: your_bittorrent.sh <command> <args>\n");
        return 1;
    }

    const char *command = argv[1];

    if (strcmp(command, "decode") == 0)
    {
        const char *encoded_str = argv[2];
        size_t len = strlen(encoded_str);
        Bencoded container;
        size_t nbytes = decode_bencode(encoded_str, len, &container);
        if (nbytes == 0)
        {
            fprintf(stderr, "Failed to decode bencoded string\n");
            return 1;
        }
        print_bencoded(stdout, container, true);
        free_bencoded_inner(container);
    }

    else if (strcmp(command, "info") == 0)
    {
        const char *torrent_path = argv[2];
        FILE_CONTENT file_contents = read_file(torrent_path);
        if (file_contents.content == NULL || file_contents.size == 0)
        {
            return 1;
        }
        Bencoded container;
        size_t nbytes = decode_bencode(file_contents.content, file_contents.size, &container);
        print_torrent_meta(container);
        free_bencoded_inner(container);
    }

    else if (strcmp(command, "peers") == 0)
    {
        const char *torrent_path = argv[2];
        FILE_CONTENT file_contents = read_file(torrent_path);
        if (file_contents.content == NULL || file_contents.size == 0)
        {
            fprintf(stderr, "ERR: failed to read file contents\n");
            fprintf(stderr, "torrent path %s\n", torrent_path);
            return 1;
        }
        Bencoded container;
        size_t nbytes = decode_bencode(file_contents.content, file_contents.size, &container);
        get_tracker_response(container);
        free_bencoded_inner(container);
    }

    else
    {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}
