/**
 * @file network.c
 * @brief Implementation file for network operations in C.
*/
#include "network.h"

void handle_bencoded_response(Bencoded *b, Tracker_Response *response_aggregator);
bool hash_bencoded(unsigned char *hash, Bencoded *b);
static size_t handle_data(void *contents, size_t size, size_t nmemb, void *userp);
BString *get_announce_url(Bencoded *torrent);
URL *initialize_url(const char *tracker_url, size_t size);
char *get_escaped_info_hash(Bencoded *torrent);
int append_query_params(URL *url, char *info_hash);
int append_length_param(URL *url, Bencoded *info);
socket_t initialize_socket(BString *ip, int port);
int parse_address(const char *address, char *ip, int *port);

bool hash_bencoded(unsigned char *hash, Bencoded *b)
{
    char *buf = malloc(b->encoded_length);
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

BString *get_info_hash(Bencoded *torrent)
{
    Bencoded *info = get_dict_key(torrent, "info");
    if (info == NULL)
    {
        fprintf(stderr, "ERR: 'info' key not found in torrent meta\n");
        return NULL;
    }

    BString *hash = bstring_new(SHA_DIGEST_LENGTH);
    if (hash == NULL)
    {
        fprintf(stderr, "ERR: failed to allocate memory for hash\n");
        return NULL;
    }

    if (!hash_bencoded(hash->chars, info))
    {
        fprintf(stderr, "ERR: failed to hash bencoded info\n");
        return NULL;
    }

    hash->size = SHA_DIGEST_LENGTH;

    return hash;
}

BString *get_announce_url(Bencoded *torrent) {
    Bencoded* announce = get_dict_key(torrent, "announce");
    if (announce == NULL) {
        fprintf(stderr, "ERR: 'announce' key not found in torrent meta");
        return NULL;
    }

    if (announce->type != STRING) {
        fprintf(stderr, "ERR: announce key is expected to be a string.");
        return NULL;
    }

    return announce->data.string;
}

URL* initialize_url(const char* tracker_url, size_t size) {
    URL* url = url_new(tracker_url, size);
    if (url == NULL) {
        fprintf(stderr, "ERR: failed to create URL object\n");
        return NULL;
    }
    return url;
}

char* get_escaped_info_hash(Bencoded *torrent) {
    Bencoded* info = get_dict_key(torrent, "info");
    if (info == NULL) {
        fprintf(stderr, "ERR: 'info' key not found in torrent meta\n");
        return NULL;
    }

    unsigned char hash[SHA_DIGEST_LENGTH];
    if (!hash_bencoded(hash, info)) {
        fprintf(stderr, "ERR: failed to hash bencoded info\n");
        return NULL;
    }

    char* escaped_hash = curl_easy_escape(NULL, (char*)hash, SHA_DIGEST_LENGTH);
    if (escaped_hash == NULL) {
        fprintf(stderr, "ERR: failed to escape hash\n");
        return NULL;
    }

    return escaped_hash;
}

int append_query_params(URL* url, char* info_hash) {
    if (url_append_query_param(url, "info_hash", info_hash) != URL_SUCCESS ||
        url_append_query_param(url, "peer_id", "00112233445566778899") != URL_SUCCESS ||
        url_append_query_param(url, "port", "6881") != URL_SUCCESS ||
        url_append_query_param(url, "uploaded", "0") != URL_SUCCESS ||
        url_append_query_param(url, "downloaded", "0") != URL_SUCCESS ||
        url_append_query_param(url, "compact", "1") != URL_SUCCESS) {
        fprintf(stderr, "ERR: failed to append query params\n");
        return -1;
    }
    return 0;
}

int append_length_param(URL* url, Bencoded* info) {
    Bencoded* length = get_dict_key(info, "length");
    if (length == NULL) {
        fprintf(stderr, "ERR: 'length' key not found in info dict\n");
        return -1;
    }

    char length_str[20];
    snprintf(length_str, sizeof(length_str), "%li", length->data.integer);

    if (url_append_query_param(url, "left", length_str) != URL_SUCCESS) {
        fprintf(stderr, "ERR: failed to append 'left' query param\n");
        return -1;
    }

    return 0;
}

Tracker_Response *get_tracker_response(Bencoded *torrent) {
    BString *tracker_url = get_announce_url(torrent);
    if (!tracker_url) 
        return NULL;

    URL *url = initialize_url((const char*)tracker_url->chars, tracker_url->size);
    if (!url) 
        return NULL;

    char *escaped_hash = get_escaped_info_hash(torrent);
    if (escaped_hash == NULL) {
        url_free(url);
        return NULL;
    }

    if (append_query_params(url, escaped_hash) != 0) {
        url_free(url);
        curl_free(escaped_hash);
        return NULL;
    }

    if (append_length_param(url, get_dict_key(torrent, "info")) != 0) {
        url_free(url);
        curl_free(escaped_hash);
        return NULL;
    }


    Tracker_Response *response_aggregator = malloc(sizeof(Tracker_Response));
    if (response_aggregator == NULL)
    {
        fprintf(stderr, "ERR: could not alloc memory for response_aggregator.");
        url_free(url);
        return NULL;
    }

    response_aggregator->data = bstring_new(1024);
    if (response_aggregator->data == NULL)
    {
        fprintf(stderr, "ERR: could not alloc memory for response_aggregator.data.");
        url_free(url);
        free(response_aggregator);
        return NULL;
    }

    CURL *curl = curl_easy_init();
    CURLcode res;

    if (curl)
    {
        char* url_str = bstring_to_cstr(*url->data);

        curl_easy_setopt(curl, CURLOPT_URL, url_str);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handle_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_aggregator);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK || !response_aggregator->ok)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            url_free(url);
            curl_easy_cleanup(curl);
            bstring_free(response_aggregator->data);
            free(response_aggregator);
            return NULL;
        }

        url_free(url);
        curl_easy_cleanup(curl);
        return response_aggregator;
    }

    return NULL;
}

static size_t handle_data(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    Tracker_Response *mem = (Tracker_Response *)userp;
    if (bstring_append_bytes(mem->data, contents, realsize) != BSTRING_SUCCESS)
    {
        fprintf(stderr, "ERR: failed to append data to response aggregator");
        return 0;
    }

    Bencoded container;
    int result = decode_bencode(&container, (const char*)mem->data->chars, mem->data->size);
    if (result != PARSER_SUCCESS)
    {
        if (result == PARSER_ERR_MEMORY || result == PARSER_ERR_SYNTAX)
        {
            return 0;
        }

        return realsize;
    }

    handle_bencoded_response(&container, mem);
    free_bencoded_inner(container);
    return realsize;
}

void handle_bencoded_response(Bencoded *b, Tracker_Response *res)
{
    if (b->type != DICTIONARY)
    {
        fprintf(stderr, "ERR: expected dictionary in response");
        return;
    }

    Bencoded *failure_reason = get_dict_key(b, "failure reason");
    if (failure_reason != NULL)
    {
        if (failure_reason->type != STRING)
        {
            fprintf(stderr, "ERR: failure reason expected to be a string");
            res->ok = false;
            return;
        }
        printf("Failure Reason: %.*s\n", (int)failure_reason->data.string->size, failure_reason->data.string->chars);
        res->ok = false;
        return;
    }

    Bencoded *interval = get_dict_key(b, "interval");
    if (interval == NULL)
    {
        fprintf(stderr, "ERR: interval key not found in response");
        res->ok = false;
        return;
    }

    if (interval->type != INTEGER)
    {
        fprintf(stderr, "ERR: interval key expected to be an integer");
        res->ok = false;
        return;
    }

    res->parsed.interval = interval->data.integer;

    Bencoded *peers = get_dict_key(b, "peers");
    if (peers == NULL)
    {
        fprintf(stderr, "ERR: peers key not found in response");
        res->ok = false;
        return;
    }

    if (peers->type != STRING)
    {
        fprintf(stderr, "ERR: peers key expected to be a string");
        res->ok = false;
        return;
    }

    size_t start_size = 10;
    Peer *peers_list = malloc(sizeof(Peer) * start_size);
    if (peers_list == NULL)
    {
        fprintf(stderr, "ERR: could not allocate memory for peers list");
        res->ok = false;
        return;
    }

    if (peers->data.string->size % 6 != 0)
    {
        fprintf(stderr, "ERR: invalid length for peers string");
        free(peers_list);
        res->ok = false;
        return;
    }

    size_t count = 0;
    for (size_t i = 0; i < peers->data.string->size; i += 6)
    {
        size_t index = i / 6;
        if (index >= start_size)
        {
            start_size *= 2;
            Peer *new_peers_list = realloc(peers_list, sizeof(Peer) * start_size);
            if (new_peers_list == NULL)
            {
                fprintf(stderr, "ERR: could not reallocate memory for peers list");
                free(peers_list);
                res->ok = false;
                return;
            }
            peers_list = new_peers_list;
        }

        Peer *peer = &peers_list[index];
        peer->ip = bstring_new(IP_V4_MAX_LENGTH);
        if (peers_list[index].ip == NULL)
        {
            fprintf(stderr, "ERR: could not allocate memory for peer ip");
            for (size_t j = 0; j < index; j++)
                bstring_free(peers_list[j].ip);
            free(peers_list);
            res->ok = false;
            return;
        }

        char ip[IP_V4_MAX_LENGTH];
        int bytes_written = snprintf(ip, IP_V4_MAX_LENGTH, "%u.%u.%u.%u",
                                     (unsigned char)peers->data.string->chars[i],
                                     (unsigned char)peers->data.string->chars[i + 1],
                                     (unsigned char)peers->data.string->chars[i + 2],
                                     (unsigned char)peers->data.string->chars[i + 3]);

        if (bytes_written < 0)
        {
            fprintf(stderr, "ERR: could not write to peer ip");
            bstring_free(peers_list[index].ip);
            for (size_t j = 0; j < index; j++)
                bstring_free(peers_list[j].ip);
            free(peers_list);
            res->ok = false;
            return;
        }

        bstring_append_bytes(peer->ip, (unsigned char*)ip, bytes_written);
        peer->port = (unsigned char)peers->data.string->chars[i + 4] << 8 | (unsigned char)peers->data.string->chars[i + 5];
        count++;
    }
    
    res->parsed.peers = peers_list;
    res->parsed.peers_count = count;
    res->ok = true;
}


void tracker_response_free(Tracker_Response *response)
{
    bstring_free(response->data);
    for (size_t i = 0; i < response->parsed.peers_count; i++)
    {
        bstring_free(response->parsed.peers[i].ip);
    }
    free(response->parsed.peers);
    free(response);
}

socket_t initialize_socket(BString *ip, int port)
{
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return -1;

    bstring_append_char(ip, '\0');

    struct sockaddr_in server_addr;
    server_addr.sin_family = PF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr((const char *)ip->chars);

    fprintf(stderr, "Connecting to %s:%d\n", ip->chars, port);
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        close(sock);
        return -1;
    }

    return sock;
}

socket_t connect_to_peer(Peer *peer)
{
    char addr[32];
    sprintf(addr, "%.*s:%u", (int)peer->ip->size, peer->ip->chars, peer->port);
    return connect_to_inet_cstr(addr);
}


socket_t connect_to_inet_cstr(const char *addr)
{
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return -1;

    int port;
    char ip[IP_V4_MAX_LENGTH];

    if (parse_address(addr, ip, &port) < 0)
    {
        close(sock);
        return -1;
    }

    fprintf(stderr, "Connecting to %s:%d\n", ip, port);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        close(sock);
        return -1;
    }

    return sock;
}

// Function to parse the address string and extract IP and port
int parse_address(const char *address, char *ip, int *port) {
    char *colon_pos = strchr(address, ':');
    if (colon_pos == NULL) {
        return -1; // Invalid address format
    }

    size_t ip_len = colon_pos - address;
    strncpy(ip, address, ip_len);
    ip[ip_len] = '\0';

    *port = atoi(colon_pos + 1);
    if (*port <= 0 || *port > 65535) {
        return -1; // Invalid port
    }

    return 0; // Success
}