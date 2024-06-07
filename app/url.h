/**
 * @file url.h
 * @brief Header file for URL builder in C.
 */

#ifndef URL_H
#define URL_H

#include "bstring.h"
#include <stdbool.h>
#include <stdlib.h>

#define URL_SUCCESS 0
#define URL_ERR_MEMORY -1

typedef struct {
    BString *data;
    bool contains_query;
} URL;

/**
 * @brief Create a new URL object
 * 
 * @param url The url endpoint to create the URL object from
 * @param size The size of the url endpoint
 * @return URL* A pointer to the newly created URL object or NULL if memory allocation failed
 */
URL *url_new(const char* url, size_t size);

/**
 * @brief Free the memory allocated for a URL object
 * 
 * @param url The URL object to free
 */
void url_free(URL *url);

/**
 * @brief append a query parameter to the URL. Note that the query parameter will be URL encoded.
 * Urls will not contain arbitrary characters, so no need to pass length info for the value or key.
 * 
 * @param url The URL object to append to
 * @param key The key of the query parameter
 * @param value The value of the query parameter
 * @return int 0 if successful, -1 if memory allocation failed
*/
int url_append_query_param(URL *url, char *key, char *value);

#endif