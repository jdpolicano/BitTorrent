/**
 * @file network.h
 * @brief Header file for network operations in C.
*/
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <stdbool.h>
#include <openssl/sha.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "bencode.h"
#include "bstring.h"
#include "url.h"

#define IP_V4_MAX_LENGTH 15 // 4 octets + 3 dots

typedef int socket_t;

typedef struct {
    BString *ip;
    int port;
} Peer;

typedef struct {
    int interval;
    Peer *peers;
    size_t peers_count;
} Tracker_Answer;

typedef struct
{
    BString *data;
    bool ok;
    Tracker_Answer parsed;
} Tracker_Response;

/**
 * @brief Get the tracker response from the announce URL in the torrent meta
 * @param torrent The torrent meta info
 * @return a pointer to The tracker response
 */
Tracker_Response *get_tracker_response(Bencoded *torrent);


/**
 * @brief Free the memory allocated for a tracker response
 * @return void
*/
void tracker_response_free(Tracker_Response *response);


/**
 * Hash a bencoded string
 * @param hash The hash to store the result in
 * @param bencoded The bencoded string to hash
 * @return bool true if hashing was successful, false otherwise
*/
bool hash_bencoded(unsigned char* hash, Bencoded *bencoded);


/**
 * @brief connect to a peer address and return the socket file descriptor
 * @param peer The peer to connect to
 * @return int The socket file descriptor, if successful, -1 otherwise
*/
int connect_to_peer(Peer *peer);


BString *get_info_hash(Bencoded *torrent);

socket_t connect_to_inet_cstr(const char *addr);