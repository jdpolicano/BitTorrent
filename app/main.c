/**
 * @file main.c
 * @brief This file contains the implementation of a Bencode decoder in C.
 * Bencode is a data encoding format used in BitTorrent.
 * The code provides functions to decode Bencoded strings, integers, and lists.
 * It also includes a main function that takes a Bencoded string as input and decodes it.
 * The decoded value is then printed to the console.
 */
#include "bencode.h"
#include <openssl/sha.h>
#include <curl/curl.h>

typedef struct {
    size_t size;
    char *content;
} FILE_CONTENT;

typedef struct {
    size_t size;
    size_t capacity;
    unsigned char *response;
} RESPONSE;

typedef struct {
    size_t size;
    size_t capacity;
    bool contains_query;
    char *data;
} URL;

// url handling functions
URL *get_url();
void free_url(URL *url);
bool set_url(const char *new_url, URL *url);
bool append_query_param(const char *key, size_t key_size, const char *value, size_t value_size, URL *url);
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
    URL *url_struct = get_url();

    if (url_struct == NULL)
    {
        return;
    }
    
    Bencoded *announce = get_dict_key("announce", torrent);
    if (announce == NULL)
    {
        fprintf(stderr, "ERR: 'announce' key not found in torrent meta");
        return;
    }

    const char *url = (const char *)announce->data.string.chars;

    if (!set_url(url, url_struct))
    {
        free_url(url_struct);
        return;
    }

    // set query param "info_hash" to the SHA1 hash of the info dictionary
    Bencoded *info = get_dict_key("info", torrent);
    if (info == NULL)
    {
        fprintf(stderr, "ERR: 'info' key not found in torrent meta");
        free_url(url_struct);
        return;
    }

    unsigned char hash[SHA_DIGEST_LENGTH];
    if (!hash_bencoded(hash, *info))
    {
        fprintf(stderr, "ERR: failed to hash bencoded info");
        free_url(url_struct);
        return;
    }  

    char *escaped_hash = curl_easy_escape(NULL, (char *)hash, SHA_DIGEST_LENGTH);
    if (escaped_hash == NULL)
    {
        fprintf(stderr, "ERR: failed to escape hash");
        free_url(url_struct);
        return;
    }

    char *hash_key = "info_hash";
    size_t hash_key_size = strlen(hash_key);
    size_t escaped_hash_size = strlen(escaped_hash);

    if (!append_query_param(hash_key, hash_key_size, escaped_hash, escaped_hash_size, url_struct))
    {
        free_url(url_struct);
        return;
    }

    // set query param "peer_id" to a unique identifier
    char *peer_id_key = "peer_id";
    size_t peer_id_key_size = strlen(peer_id_key);
    char *peer_id = "00112233445566778899";
    size_t peer_id_size = strlen(peer_id);

    if (!append_query_param(peer_id_key, peer_id_key_size, peer_id, peer_id_size, url_struct))
    {
        free_url(url_struct);
        return;
    }

    // set query param "port" to the port the client is listening on
    char *port_key = "port";
    size_t port_key_size = strlen(port_key);
    char *port = "6881";
    size_t port_size = strlen(port);

    if (!append_query_param(port_key, port_key_size, port, port_size, url_struct))
    {
        free_url(url_struct);
        return;
    }

    // set query param "uploaded" to the number of bytes uploaded
    char *uploaded_key = "uploaded";
    size_t uploaded_key_size = strlen(uploaded_key);
    char *uploaded = "0";
    size_t uploaded_size = strlen(uploaded);

    if (!append_query_param(uploaded_key, uploaded_key_size, uploaded, uploaded_size, url_struct))
    {
        free_url(url_struct);
        return;
    }

    // set query param "downloaded" to the number of bytes downloaded
    char *downloaded_key = "downloaded";
    size_t downloaded_key_size = strlen(downloaded_key);
    char *downloaded = "0";
    size_t downloaded_size = strlen(downloaded);

    if (!append_query_param(downloaded_key, downloaded_key_size, downloaded, downloaded_size, url_struct))
    {
        free_url(url_struct);
        return;
    }

    Bencoded *length = get_dict_key("length", *info);
    if (length == NULL)
    {
        fprintf(stderr, "ERR: length key not found in info dict.");
        free_url(url_struct);
        return;
    }

    char *length_str = malloc(sizeof(char) * length->encoded_length); // this should be guaranteed to be enough
    if (length_str == NULL)
    {
        fprintf(stderr, "ERR: failed to allocate memory for length string");
        free_url(url_struct);
        return;
    }

    sprintf(length_str, "%li", length->data.integer);

    // set query param "left" to the number of bytes left to download
    char *left_key = "left";
    size_t left_key_size = strlen(left_key);
    size_t length_str_size = strlen(length_str);

    if (!append_query_param(left_key, left_key_size, length_str, length_str_size, url_struct))
    {
        free_url(url_struct);
        return;
    }

    char *compact_key = "compact";
    size_t compact_key_size = strlen(compact_key);
    char *compact = "1";
    size_t compact_size = strlen(compact);

    if (!append_query_param(compact_key, compact_key_size, compact, compact_size, url_struct))
    {
        free_url(url_struct);
        return;
    }

    CURL *curl;
    CURLcode res;
    unsigned char *response_data = malloc(sizeof(char) * 1024); // arbitrary size for now.
    RESPONSE response_aggregator = {0};
    response_aggregator.response = response_data;
    response_aggregator.size = 0;
    response_aggregator.capacity = 1024;

    curl = curl_easy_init();

    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url_struct->data);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handle_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_aggregator);

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

    if (mem->size + realsize > mem->capacity)
    {
        unsigned char *ptr = realloc(mem->response, mem->size + realsize);
        if (ptr == NULL)
        {
            fprintf(stderr, "ERR: not enough memory (realloc returned NULL)\n");
            return 0;
        }

        mem->response = ptr;
        mem->capacity = mem->size + realsize;
    }

    memcpy(&(mem->response[mem->size]), contents, realsize);
    mem->size += realsize;

    Bencoded container;
    size_t nbytes = decode_bencode((const char*)mem->response, mem->size, &container);

    if (nbytes == 0 && (errno == PARSER_ERR_MEMORY || errno == PARSER_ERR_SYNTAX))
    {
        return 0;
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

/**
 * @brief alloc a new url struct.
 * @return a new URL struct or NULL if an error occurred.
*/
URL *get_url()
{
    URL *url = malloc(sizeof(URL));
    if (url == NULL)
    {
        fprintf(stderr, "ERR: failed to allocate memory for URL");
        return NULL;
    }
    url->size = 0;
    url->capacity = 1000;
    url->data = malloc(url->capacity);
    if (url->data == NULL)
    {
        fprintf(stderr, "ERR: failed to allocate memory for URL data");
        free(url);
        return NULL;
    }
    return url;
}

/**
 * @brief free a URL struct.
 * @param url The URL struct to free.
*/
void free_url(URL *url)
{
    free(url->data);
    free(url);
}

/**
 * @brief set the URL struct to a new URL.
 * Note: urls are not binary safe, so no size is passed here.
 * @param url The URL struct to set.
 * @param new_url The new URL to set.
 * @param new_url_size The size of the new URL.
 * @return true if successful, false if an error occurred.
*/

bool set_url(const char *new_url, URL *url)
{
    size_t new_url_size = strlen(new_url);
    if (new_url_size > url->capacity)
    {
        char *new_data = realloc(url->data, new_url_size);
        if (new_data == NULL)
        {
            fprintf(stderr, "ERR: failed to allocate memory for new URL");
            return false;
        }
        url->data = new_data;
        url->capacity = new_url_size;
    }
    memcpy(url->data, new_url, new_url_size);
    url->size = new_url_size;
    return true;
}

/**
 * binary safe appender for a url with query parameters.
 * @brief Append a query parameter to a URL.
 * @param key The key of the query parameter.
 * @param key_size The size of the key.
 * @param value The value of the query parameter.
 * @param value_size The size of the value.
 * @param url The URL struct to append the query parameter to.
 * @return true if successful, false if an error occurred.
*/
bool append_query_param(const char* key, size_t key_size, const char* value, size_t value_size, URL *url)
{
    if (!url->contains_query)
    {
        url->data[url->size++] = '?';
        url->contains_query = true;
    }

    size_t new_size = url->size + key_size + value_size + 2; // 2 for '=' and '?'
    if (new_size > url->capacity)
    {
        size_t new_capacity = url->capacity * 2 > new_size ? url->capacity * 2 : new_size;
        char *new_data = realloc(url->data, url->capacity * 2);
        if (new_data == NULL)
        {
            fprintf(stderr, "ERR: failed to allocate memory for query params");
            return false;
        }
        url->data = new_data;
        url->capacity = new_capacity;
    }

    memcpy(url->data + url->size, key, key_size);
    url->size += key_size;
    url->data[url->size++] = '=';
    memcpy(url->data + url->size, value, value_size);
    url->size += value_size;
    url->data[url->size++] = '&';
    return true;
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
        print_bencoded(container, true);
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
        fprintf(stderr, "torrent path %s\n", torrent_path);
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
