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

void print_torrent_meta(Bencoded torrent);
void print_tracker_url(Bencoded announce);
void print_info(Bencoded info);
void print_torrent_hash(Bencoded info);
char *read_file(const char *path);

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
    if (announce == NULL)
    {
        fprintf(stderr, "ERR: 'info' key not found in torrent meta");
        return;
    }
    print_info(*info);

    char buf[10000]; // known to be big enough for now.
    size_t size = encode_bencode(*info, buf);
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char *)buf, size, hash);
    printf("Info Hash: ");
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
        printf("%02x", hash[i]);
    }
    printf("\n");
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

    if (length->type != INTEGER)
    {
        fprintf(stderr, "ERR: length key inside info expected to be an integer.");
        return;
    }

    printf("Length: %li\n", length->data.integer);
}

char *read_file(const char *path)
{
    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
        fprintf(stderr, "unable to open file at: %s\n", path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *contents = malloc(size + 1);
    if (contents == NULL)
    {
        fprintf(stderr, "ERR: failed to alloc buffer for file contents");
        return contents; // NULL;
    }

    long nbytes = fread(contents, sizeof(char), size, file);
    if (nbytes != size)
    {
        fprintf(stderr, "ERR: number of bytes read differs from file size.");
    }

    fclose(file);
    return contents;
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
        Bencoded container;
        const char *endstr = decode_bencode(encoded_str, &container);
        print_bencoded(container, true);
        free_bencoded_inner(container);
    }

    else if (strcmp(command, "info") == 0)
    {
        const char *torrent_path = argv[2];
        const char *file_contents = read_file(torrent_path);
        Bencoded container;
        const char *endstr = decode_bencode(file_contents, &container);
        print_torrent_meta(container);
        free_bencoded_inner(container);
    }
    else
    {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}
