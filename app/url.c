/**
 * @file url.c
 * @brief Implementation file for URL builder in C.
 */

#include "url.h"

URL *url_new(const char* url, size_t size) {
    URL *new_url = malloc(sizeof(URL));
    if (new_url == NULL) {
        return NULL;
    }
    new_url->data = bstring_new(size);
    if (new_url->data == NULL)
    {
        free(new_url);
        return NULL;
    }

    if (bstring_append_bytes(new_url->data, (const unsigned char *)url, size) != BSTRING_SUCCESS) {
        bstring_free(new_url->data);
        free(new_url);
        return NULL;
    }

    new_url->contains_query = false;
    return new_url;
}


void url_free(URL *url) {
    bstring_free(url->data);
    free(url);
}


int url_append_query_param(URL *url, char *key, char *value) {
    if (!url->contains_query) {
        if (bstring_append_char(url->data, '?') != BSTRING_SUCCESS) {
            return URL_ERR_MEMORY;
        }
        url->contains_query = true;
    } else {
        if (bstring_append_char(url->data, '&') != BSTRING_SUCCESS) {
            return URL_ERR_MEMORY;
        }
    }

    if (bstring_append_cstr(url->data, key) != BSTRING_SUCCESS) {
        return URL_ERR_MEMORY;
    }

    if (bstring_append_char(url->data, '=') != BSTRING_SUCCESS) {
        return URL_ERR_MEMORY;
    }

    if (bstring_append_cstr(url->data, value) != BSTRING_SUCCESS) {
        return URL_ERR_MEMORY;
    }

    return 0;
}