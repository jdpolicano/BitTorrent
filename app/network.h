#ifndef NETWORK_H
#define NETWORK_H

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
#define MAX_PORT_RANGE 65535 // 2^16 - 1
#define DEFAULT_BLOCK_SIZE 16384 // 16KB

typedef int socket_t;

typedef enum
{
    IPV4,
    IPV6
} IPType;

typedef enum
{
    CHOKE,          // 0 choke: this client is choking the peer
    UNCHOKE,        // 1 unchoke: this client is no longer choking the peer
    INTERESTED,     // 2 interested: this client is interested in the peer
    NOT_INTERESTED, // 3 not interested: this client is not interested in the peer
    HAVE,           // 4 have: the client has downloaded a piece
    BITFIELD,       // 5 bitfield: a bitfield of the pieces the client has
    REQUEST,        // 6 request: request a piece
    PIECE,          // 7 piece: a piece of the file
    CANCEL,         // 8 cancel: cancel a request
} PeerMessageId;

typedef struct {
    IPType type;
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
Tracker_Response *
get_tracker_response(Bencoded *torrent);

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
int tcp_connect_peer(Peer *peer);


/**
 * @brief Get the info hash from a torrent meta info
 * @param torrent The torrent meta info
 * @return BString* The info hash, or NULL if the info hash is not found
*/
BString *get_info_hash(Bencoded *torrent);


/**
 * @brief establish a tcp connection to a peer (IPV4) using the given ip and port in a cstring. 
 * The format is "ip:port" like a human would normally read it.
 * @param addr The address to connect to
 * @return socket_t The socket file descriptor, if successful, -1 otherwise
*/
socket_t tcp_connect_inet_cstr(const char *addr);


/**
 * @brief establish a tcp connection to a peer (IPV4) using the given ip and port in a human readable format.
 * @param ip The ip address to connect to
 * @param port The port to connect to
 * @return socket_t The socket file descriptor, if successful, -1 otherwise
*/
socket_t tcp_connect_inet_hp(const char *ip, int port);


/**
 * @brief read a specified number of bytes from a socket using recv. 
 * @param socket The socket file descriptor to read from
 * @param buffer The buffer to store the read data
 * @param n The number of bytes to read
 * @return int The number of bytes read, or -1 if an error occurred
*/
int read_socket_exact(socket_t socket, char *buffer, size_t n);

#endif