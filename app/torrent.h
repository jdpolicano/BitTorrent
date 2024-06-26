#ifndef TORRENT_H
#define TORRENT_H

/**
 * @file torrent.h
 * @brief Header file for torrenting files.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <openssl/sha.h>
#include <sys/socket.h>
#include "bencode.h"
#include "bstring.h"
#include "url.h"
#include "network.h"

// 16KB || 2^14 - see https://wiki.theory.org/BitTorrentSpecification
#define DEFAULT_BLOCK_SIZE 16384 

typedef struct {
    Peer *peer_info; // the ip port etc...
    bool connected;
    bool am_choking;
    bool am_interested;
    bool peer_choking;
    bool peer_interested;
} Peer_State;

typedef struct {
    size_t size;         // the number of bytes in this block. should default to 16KB, exept for the last block.
    size_t offset;       // the offset of this block in the piece.
    unsigned char *data; // the data in this block.
} Block;

typedef struct {
    size_t index;    // the index of this piece.
    size_t size;    // the acutal size of this this piece.
    unsigned char hash[20]; // the hash of this piece.
    size_t block_count;     // the number of blocks in this piece.
    size_t blocks_received; // the total number of blocks received.
    Block *blocks;          // the blocks in this piece. Will maintain a sorted order.
} Piece;

typedef struct {
    size_t num_pieces; // the number of pieces in the file.
    size_t file_size; // the size of the file.
    size_t piece_length; // the length of each piece. all except the last piece are guarranteed to be this length.
    char *name; // the name of the file.
    Piece *pieces; // the pieces in the file.
} TorrentFile;

typedef struct {
    size_t connected_peers; // the number of connected peers.
    size_t total_peers; // the total number of peers.
    unsigned char *bit_map; // the bit map of the pieces the client has.
    Peer *peers; // the peers for this file.
    TorrentFile *file; // the file to download.
} ClientState;

typedef struct __attribute((packed)) {
    unsigned char pstrlen; // length of the protocol identifier
    unsigned char proto_name[19]; // "BitTorrent protocol"
    unsigned char reserved[8]; // reserved bytes, all 0's
    unsigned char info_hash[20]; // info hash.
    unsigned char peer_id[20]; // peer id.
} Peer_Header_BitTorrent;

/**
 * @brief perform the handshake with a peer, sending the handshake message and receiving the response.
 * @param socket The socket file descriptor to send the handshake message to
 * @param info_hash The info hash of the torrent
 * @param peer_id The peer id of the client
 * @return a pointer to the Peer_Header_BitTorrent struct, or NULL if an error occurred
*/
Peer_Header_BitTorrent *handshake(socket_t socket, unsigned char *info_hash, unsigned char *peer_id);

/**
 * @brief Create a new torrent file object
 * @param torrent - the berncoded torrent file array.
 * @return TorrentFile* A pointer to the newly created TorrentFile object or null
 */
TorrentFile *torrent_file_new(char *torrent, size_t size);


/**
 * @brief print the torrent file object
 * @param fd - the file descriptor to print to
 * @param file - the torrent file object
 * @return void
*/
void print_torrent_file(FILE *fd, TorrentFile *file);

/**
 * @brief Free the memory allocated for a TorrentFile object
 * @return void
*/
void torrent_file_free(TorrentFile *file);

#endif