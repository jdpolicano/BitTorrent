#include "torrent.h"

#define REMAINDER_OR_FULL(total_size, part_size) total_size % part_size == 0 ? part_size : total_size % part_size

/**
 * @brief Create a new torrent file object from a bencoded dictionary
 * @param info_dict - the bencoded dictionary
 * @return TorrentFile* A pointer to the newly created TorrentFile object or null
 */
TorrentFile *torrent_file_from_bcoded(Bencoded *info_dict);

/**
 * @brief attach the file length to the torrent file object
 * @param file - the torrent file object
 * @param info_dict - the bencoded dictionary
 * @return a pointer to the torrent file object or null
*/
TorrentFile *torrent_file_attach_file_size(TorrentFile *file, Bencoded *info_dict);

/**
 * @brief attach the file name to the torrent file object
 * @param file - the torrent file object
 * @param info_dict - the bencoded dictionary
 * @return a pointer to the torrent file object or null
*/
TorrentFile *torrent_file_attach_name(TorrentFile *file, Bencoded *info_dict);

/**
 * @brief attach the piece length to the torrent file object
 * @param file - the torrent file object
 * @param info_dict - the bencoded dictionary
 * @return a pointer to the torrent file object or null
*/
TorrentFile *torrent_file_attach_piece_length(TorrentFile *file, Bencoded *info_dict);

/**
 * @brief attach the pieces to the torrent file object
 * @param file - the torrent file object
 * @param info_dict - the bencoded dictionary
 * @return a pointer to the torrent file object or null
*/
TorrentFile *torrent_file_attach_pieces(TorrentFile *file, Bencoded *info_dict);

/**
 * @brief initialize the blocks for a piece
 * @param piece - the piece to initialize blocks for
 * @return a pointer to the blocks or null
*/
Block *piece_init_blocks(Piece *piece);

/**
 * @brief Free the memory allocated for a Piece object
 * @return void
*/
void piece_free(Piece *piece);

/**
 * @brief Free the memory allocated for a Block object
 * @return void
*/
void block_free(Block *block);

/**
 * @brief print the piece object
 * @param fd - the file descriptor to print to
 * @param piece - the piece object
 * @return void
*/
void print_piece(FILE *fd, Piece *piece);

/**
 * @brief print the block object
 * @param fd - the file descriptor to print to
 * @param block - the block object
 * @return void
*/
void print_block(FILE *fd, Block *block);


TorrentFile *torrent_file_new(char *torrent, size_t size)
{
    Bencoded *bencoded = get_bencoded();
    if (bencoded == NULL)
    {
        fprintf(stderr, "ERR: failed to parse torrent file\n");
        return NULL;
    }

    if (decode_bencode(bencoded, (const char *)torrent, size) != PARSER_SUCCESS)
    {
        fprintf(stderr, "ERR: failed to parse torrent file\n");
        free_bencoded(bencoded);
        return NULL;
    }

    Bencoded *info_dict = get_dict_key(bencoded, "info");
    if (info_dict == NULL || info_dict->type != DICTIONARY)
    
    {
        fprintf(stderr, "ERR: info key is expected to be a dict.");
        return NULL;
    }

    TorrentFile *file = torrent_file_from_bcoded(info_dict);

    return file;
}

TorrentFile *torrent_file_from_bcoded(Bencoded *info_dict)
{
    TorrentFile *file = malloc(sizeof(TorrentFile));
    if (file == NULL)
    {
        fprintf(stderr, "ERR: failed to allocate memory for torrent file\n");
        return NULL;
    }

    if (torrent_file_attach_file_size(file, info_dict) == NULL)
    {
        fprintf(stderr, "ERR: failed to attach length to torrent file\n");
        free(file);
        return NULL;
    }

    if (torrent_file_attach_name(file, info_dict) == NULL)
    {
        fprintf(stderr, "ERR: failed to attach name to torrent file\n");
        free(file);
        return NULL;
    }


    if (torrent_file_attach_piece_length(file, info_dict) == NULL)
    {
        fprintf(stderr, "ERR: failed to attach piece length to torrent file\n");
        free(file);
        return NULL;
    }

    if (torrent_file_attach_pieces(file, info_dict) == NULL)
    {
        fprintf(stderr, "ERR: failed to attach pieces to torrent file\n");
        free(file);
        return NULL;
    }


    return file;
}


TorrentFile *torrent_file_attach_file_size(TorrentFile *file, Bencoded *info_dict)
{
    Bencoded *length = get_dict_key(info_dict, "length");
    if (length == NULL || length->type != INTEGER)
    {
        fprintf(stderr, "ERR: length key not found in info dict.");
        return NULL;
    }

    file->file_size = length->data.integer;

    return file;
}

TorrentFile *torrent_file_attach_name(TorrentFile *file, Bencoded *info_dict)
{
    Bencoded *name = get_dict_key(info_dict, "name");
    if (name == NULL || name->type != STRING)
    {
        fprintf(stderr, "ERR: name key not found in info dict.");
        return NULL;
    }

    char *name_str = bstring_to_cstr(name->data.string);
    if (name_str == NULL)
    {
        fprintf(stderr, "ERR: failed to convert name to string");
        return NULL;
    }

    file->name = name_str;
    return file;
}

TorrentFile *torrent_file_attach_piece_length(TorrentFile *file, Bencoded *info_dict)
{
    Bencoded *piece_length = get_dict_key(info_dict, "piece length");
    if (piece_length == NULL || piece_length->type != INTEGER)
    {
        fprintf(stderr, "ERR: piece length key not found in info dict.");
        return NULL;
    }

    file->piece_length = piece_length->data.integer;
    return file;
}

TorrentFile *torrent_file_attach_pieces(TorrentFile *file, Bencoded *info_dict)
{

    Bencoded *pieces_hashes = get_dict_key(info_dict, "pieces");
    if (pieces_hashes == NULL || pieces_hashes->type != STRING)
    {
        fprintf(stderr, "ERR: pieces key not found in info dict.");
        return NULL;
    }

    // sanity check that the pieces hashes are a multiple of the SHA_DIGEST_LENGTH
    if (pieces_hashes->data.string->size % SHA_DIGEST_LENGTH != 0)
    {
        fprintf(stderr, "ERR: pieces key inside info expected to be a string.");
        return NULL;
    }

    file->num_pieces = pieces_hashes->data.string->size / SHA_DIGEST_LENGTH;

    file->pieces = malloc(sizeof(Piece) * file->num_pieces);
    if (file->pieces == NULL)
    {
        fprintf(stderr, "ERR: failed to allocate memory for pieces");
        return NULL;
    }

    for (int i = 0; i < file->num_pieces - 1; i++)
    {
        Piece *piece = &file->pieces[i];
        piece->index = i;
        // need to figure out how large this exact piece will be, accounting for the last piece
        piece->size = file->piece_length;
        // this is the beginning of the hash tht corresponds to this piece
        const unsigned char *hash = pieces_hashes->data.string->chars + (i * SHA_DIGEST_LENGTH);
        // copy in the hash.
        memcpy(piece->hash, hash, SHA_DIGEST_LENGTH);
        // piece_init_blocks will allocate memory for the blocks and break these pieces into blocks.
        if (piece_init_blocks(piece) == NULL)
        {
            fprintf(stderr, "ERR: failed to initialize blocks for piece %d\n", i);
            return NULL;
        }
    }

    // the last piece is a special case, it may not be the same size as the other pieces.
    Piece *last_piece = &file->pieces[file->num_pieces - 1];
    last_piece->index = file->num_pieces - 1;
    last_piece->size = REMAINDER_OR_FULL(file->file_size, file->piece_length);
    const unsigned char *hash = pieces_hashes->data.string->chars + ((file->num_pieces - 1) * SHA_DIGEST_LENGTH);
    memcpy(last_piece->hash, hash, SHA_DIGEST_LENGTH);

    if (piece_init_blocks(last_piece) == NULL)
    {
        fprintf(stderr, "ERR: failed to initialize blocks for last piece\n");
        return NULL;
    }

    return file;
}

Block *piece_init_blocks(Piece *piece)
{
    piece->block_count = piece->size % DEFAULT_BLOCK_SIZE == 0 
        ? piece->size / DEFAULT_BLOCK_SIZE 
        : (piece->size / DEFAULT_BLOCK_SIZE) + 1;

    piece->blocks_received = 0;

    piece->blocks = malloc(sizeof(Block) * piece->block_count);
    if (piece->blocks == NULL)
    {
        fprintf(stderr, "ERR: failed to allocate memory for blocks\n");
        return NULL;
    }

    for (int i = 0; i < piece->block_count - 1; i++)
    {
        Block *block = &piece->blocks[i];
        block->offset = i * DEFAULT_BLOCK_SIZE;
        block->size = DEFAULT_BLOCK_SIZE;
    }

    Block *last_block = &piece->blocks[piece->block_count - 1];
    last_block->size = REMAINDER_OR_FULL(piece->size, DEFAULT_BLOCK_SIZE);
    last_block->offset = (piece->block_count - 1) * DEFAULT_BLOCK_SIZE;
    return piece->blocks;
}

void torrent_file_free(TorrentFile *file)
{
    for (int i = 0; i < file->num_pieces; i++)
    {
        piece_free(&file->pieces[i]);
    }
    free(file->pieces);
    free(file->name);
    free(file);
}

void piece_free(Piece *piece)
{
    for (int i = 0; i < piece->block_count; i++)
    {
        block_free(&piece->blocks[i]);
    }

    free(piece->blocks);
}

void block_free(Block *block)
{
    free(block->data);
}

void print_torrent_file(FILE *fd, TorrentFile *file)
{
    fprintf(fd, "File Name: %s\n", file->name);
    fprintf(fd, "File Size: %li\n", file->file_size);
    fprintf(fd, "Piece Length: %li\n", file->piece_length);
    fprintf(fd, "Number of Pieces: %li\n", file->num_pieces);
    fprintf(fd, "Pieces:\n\n");
    for (int i = 0; i < file->num_pieces; i++)
    {
        print_piece(fd, &file->pieces[i]);
        fprintf(fd, "\n");
    }
}

void print_piece(FILE *fd, Piece *piece)
{
    fprintf(fd, "Piece Index: %li\n", piece->index);
    fprintf(fd, "Piece Size: %li\n", piece->size);
    fprintf(fd, "Piece Hash: TBD\n");
    fprintf(fd, "Number of Blocks: %li\n", piece->block_count);
    fprintf(fd, "Blocks:\n\n");
    for (int i = 0; i < piece->block_count; i++)
    {
        fprintf(fd, "Block %d\n", i);
        print_block(fd, &piece->blocks[i]); 
    }
}

void print_block(FILE *fd, Block *block)
{
    fprintf(fd, "Block Offset: %li\n", block->offset);
    fprintf(fd, "Block Size: %li\n", block->size);
}


Peer_Header_BitTorrent *handshake(socket_t socket, unsigned char *info_hash, unsigned char *peer_id)
{
    Peer_Header_BitTorrent *header = malloc(sizeof(Peer_Header_BitTorrent));
    if (header == NULL)
    {
        fprintf(stderr, "ERR: failed to allocate memory for header\n");
        return NULL;
    }

    header->pstrlen = 19;
    memcpy(header->proto_name, "BitTorrent protocol", 19);
    memset(header->reserved, 0, 8);
    memcpy(header->info_hash, info_hash, 20);
    memcpy(header->peer_id, peer_id, 20);

    if (send(socket, (unsigned char *)header, sizeof(Peer_Header_BitTorrent), 0) < 0)
    {
        fprintf(stderr, "ERR: failed to send handshake\n");
        free(header);
        return NULL;
    }

    // read the first byte to get the length of the protocol string
    if (read_socket_exact(socket, (char *)header, sizeof(Peer_Header_BitTorrent)) < 0)
    {
        fprintf(stderr, "ERR: failed to complete handshake\n");
        free(header);
        return NULL;
    }

    // the header struct should now bit filled with the response.
    if (memcmp(header->proto_name, "BitTorrent protocol", 19) != 0)
    {
        fprintf(stderr, "ERR: invalid protocol name\n");
        free(header);
        return NULL;
    }

    return header;
}