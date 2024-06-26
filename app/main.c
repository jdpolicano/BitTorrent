/**
 * @file main.c
 * @brief This file contains the implementation of a Bencode decoder in C.
 * Bencode is a data encoding format used in BitTorrent.
 * The code provides functions to decode Bencoded strings, integers, and lists.
 * It also includes a main function that takes a Bencoded string as input and decodes it.
 * The decoded value is then printed to the console.
 */
#include "bencode.h"
#include "bstring.h"
#include "network.h"
#include "torrent.h"
#include <stdlib.h>

typedef struct {
    size_t size;
    char *content;
} FILE_CONTENT;

// print functions
void print_tracker_url(Bencoded announce);
void print_info(Bencoded info);
void print_hex(unsigned char *data, size_t size);
FILE_CONTENT read_file(const char *path);

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

    Bencoded *announce = get_dict_key(&torrent, "announce");
    if (announce == NULL)
    {
        fprintf(stderr, "ERR: 'announce' key not found in torrent meta");
        return;
    }
    print_tracker_url(*announce);

    Bencoded *info = get_dict_key(&torrent, "info");
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

    printf("Tracker URL: %.*s\n", (int)announce.data.string->size, announce.data.string->chars);
}

void print_info(Bencoded info)
{
    if (info.type != DICTIONARY)
    {
        fprintf(stderr, "ERR: info key is expected to be a dict.");
        return;
    }

    Bencoded *length = get_dict_key(&info, "length");
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
    if (!hash_bencoded((unsigned char *)hash, &info))
    {
        fprintf(stderr, "ERR: failed to hash bencoded info");
        return;
    }

    printf("Info Hash: ");
    print_hex((unsigned char *)hash, SHA_DIGEST_LENGTH);
    printf("\n");

    // print piece length
    Bencoded *piece_length = get_dict_key(&info, "piece length");
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
    Bencoded *pieces = get_dict_key(&info, "pieces");
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

    if (pieces->data.string->size % SHA_DIGEST_LENGTH != 0)
    {
        fprintf(stderr, "ERR: pieces key inside info has invalid length.");
        return;
    }

    printf("Piece Hashes:\n");

    for(size_t i = 0; i < pieces->data.string->size; i += SHA_DIGEST_LENGTH)
    {
        print_hex((unsigned char *)pieces->data.string->chars + i, SHA_DIGEST_LENGTH);
        printf("\n");
    }
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
        int result = decode_bencode(&container, encoded_str, len);
        if (result != PARSER_SUCCESS)
        {
            fprintf(stderr, "ERR: failed to decode bencoded value\n");
            return 1;
        }
        print_bencoded(container, stdout, true);
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
        int result = decode_bencode(&container, file_contents.content, file_contents.size);
        if (result != PARSER_SUCCESS)
        {
            fprintf(stderr, "ERR: failed to decode bencoded string\n");
            return 1;
        }
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
        int result = decode_bencode(&container, file_contents.content, file_contents.size);
        if (result != PARSER_SUCCESS)
        {
            fprintf(stderr, "ERR: failed to decode bencoded string\n");
            return 1;
        }

        Tracker_Response *res = get_tracker_response(&container);

        if (!res->ok)
        {
            fprintf(stderr, "ERR: failed to get tracker response\n");
            free_bencoded_inner(container);
            tracker_response_free(res);
            return 1;
        }

        for (int i = 0; i < res->parsed.peers_count; i++)
        {
            printf("%.*s:%d\n", (int)res->parsed.peers[i].ip->size, res->parsed.peers[i].ip->chars, res->parsed.peers[i].port);
        }
    }

    else if (strcmp(command, "handshake") == 0)
    {
        if (argc < 4)
        {
            fprintf(stderr, "Usage: your_bittorrent.sh handshake <torrent_path> <peer_addr>\n");
            return 1;
        }

        const char *torrent_path = argv[2];
        const char *peer_addr = argv[3];

        FILE_CONTENT file_contents = read_file(torrent_path);
        if (file_contents.content == NULL || file_contents.size == 0)
        {
            fprintf(stderr, "ERR: failed to read file contents\n");
            fprintf(stderr, "torrent path %s\n", torrent_path);
            return 1;
        }

        Bencoded container;
        int result = decode_bencode(&container, file_contents.content, file_contents.size);
        if (result != PARSER_SUCCESS)
        {
            fprintf(stderr, "ERR: failed to decode bencoded string\n");
            return 1;
        }

        int socket = tcp_connect_inet_cstr(peer_addr);

        if (socket < 0)
        {
            fprintf(stderr, "ERR: failed to connect to peer\n");
            free_bencoded_inner(container);
            return 1;
        }

        fprintf(stderr, "Connected to peer\n");

        BString *msg = bstring_new(1024);
        // todo this should likely be put in a struct so we can reuse it later in the protocol.
        BString *info_hash = get_info_hash(&container);

        Peer_Header_BitTorrent *header = handshake(socket, info_hash->chars, (unsigned char *)"00112233445566778899");

        if (header == NULL)
        {
            fprintf(stderr, "ERR: failed to connect with client at %s\n", peer_addr);
            free_bencoded_inner(container);
            bstring_free(msg);
            return 1;
        }

        printf("Peer ID: ");
        print_hex((unsigned char *)header->peer_id, 20);
        printf("\n");

        free_bencoded_inner(container);
    }

    else if (strcmp(command, "download_piece") == 0)
    {
        if (argc < 5)
        {
            fprintf(stderr, "Usage: your_bittorrent.sh download_piece -o <output_path> <torrent_path> <piece_index>\n");
            return 1;
        }

        const char *torrent_path = argv[4];
        const char *piece_index_str = argv[5];

        fprintf(stderr, "Downloading piece %s from torrent %s\n", piece_index_str, torrent_path);

        FILE_CONTENT file_contents = read_file(torrent_path);
        if (file_contents.content == NULL || file_contents.size == 0)
        {
            fprintf(stderr, "ERR: failed to read file contents\n");
            fprintf(stderr, "torrent path %s\n", torrent_path);
            return 1;
        }

        TorrentFile *torrent = torrent_file_new(file_contents.content, file_contents.size);
        if (torrent == NULL)
        {
            fprintf(stderr, "ERR: failed to parse torrent file\n");
            return 1;
        }

        print_torrent_file(stdout, torrent);
        torrent_file_free(torrent);
    }

    else
    {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}
