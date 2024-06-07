/**
 * @file bstring.c
 * @brief Implementation file for a binary safe string in C. handles string operations, resizing, and offers ability to free the string.
 * It will also contain facilities  for pushing, popping, comparing, copying, and concatenating strings. It should also provide helpers for 
 * converting to and from C strings (and generally interacting with C strings). 
 */

#include "bstring.h"

int bstring_resize(BString *bstr, size_t new_capacity);

int bstring_resize(BString *bstr, size_t new_capacity) {
    while(bstr->capacity < new_capacity) {
        bstr->capacity *= 2;
    }

    unsigned char *new_chars = realloc(bstr->chars, bstr->capacity);
    if (new_chars == NULL) {
        bstring_free(bstr);
        return BSTRING_ERR_MEMORY;
    }

    bstr->chars = new_chars;
    return BSTRING_SUCCESS;
}

BString *bstring_new(size_t capacity) {
    BString *bstr = malloc(sizeof(BString));
    if (bstr == NULL) {
        return NULL;
    }
    bstr->capacity = capacity;
    bstr->size = 0;
    bstr->chars = malloc(sizeof(unsigned char) * capacity);
    if (bstr->chars == NULL) {
        free(bstr);
        return NULL;
    }

    return bstr;
}

void bstring_free(BString *bstr) {
    free(bstr->chars);
    free(bstr);
}

BString *bstring_from_cstr(const char *cstr) {
    BString *bstr = bstring_new(strlen(cstr) + 1);
    if (bstr == NULL) {
        return NULL;
    }
    strcpy((char *)bstr->chars, cstr);
    bstr->size = strlen(cstr);
    return bstr;
}

int bstring_append_char(BString *bstr, char c) {
    if (bstr->size == bstr->capacity) {
        int resize_result = bstring_resize(bstr, bstr->size + 1);
        if (resize_result != BSTRING_SUCCESS) {
            return resize_result;
        }
    }
    bstr->chars[bstr->size] = c;
    bstr->size++;
    return BSTRING_SUCCESS;
}

int bstring_append_cstr(BString *bstr, const char *cstr) {
    size_t cstr_len = strlen(cstr);
    if (bstr->size + cstr_len >= bstr->capacity) {
        // + 1 for null terminator that strcpy will add.
        int resize_result = bstring_resize(bstr, bstr->size + cstr_len + 1);
        if (resize_result != BSTRING_SUCCESS) {
            return resize_result;
        }
    }
    strcpy((char *)(bstr->chars + bstr->size), cstr);
    bstr->size += cstr_len;
    return BSTRING_SUCCESS;
}

int bstring_append_bstring(BString *bstr, BString *other) {
    if (bstr->size + other->size >= bstr->capacity) {
        int resize_result = bstring_resize(bstr, bstr->size + other->size);
        if (resize_result != BSTRING_SUCCESS) {
            return resize_result;
        }
    }
    memcpy(bstr->chars + bstr->size, other->chars, other->size);
    bstr->size += other->size;
    return BSTRING_SUCCESS;
}

int bstring_append_bytes(BString *bstr, const unsigned char *bytes, size_t num_bytes) {
    if (bstr->size + num_bytes >= bstr->capacity) {
        int resize_result = bstring_resize(bstr, bstr->size + num_bytes);
        if (resize_result != BSTRING_SUCCESS) {
            return resize_result;
        }
    }
    memcpy(bstr->chars + bstr->size, bytes, num_bytes);
    bstr->size += num_bytes;
    return BSTRING_SUCCESS;
}

Pop bstring_pop(BString *bstr) {
    Pop pop;
    if (bstr->size == 0) {
        pop.result = POP_EMPTY;
        return pop;
    }
    pop.c = bstr->chars[bstr->size--];
    pop.result = POP_SUCCESS;
    return pop;
}

int bstring_cmp(BString *bstr1, BString *bstr2) {
    if (bstr1->size != bstr2->size) {
        return bstr1->size - bstr2->size;
    }
    return memcmp(bstr1->chars, bstr2->chars, bstr1->size);
}

int bstring_cmp_cstr(BString *bstr, const char *cstr) {
    size_t cstr_len = strlen(cstr);
    if (bstr->size != cstr_len) {
        return bstr->size - cstr_len;
    }
    return memcmp(bstr->chars, cstr, bstr->size);
}

char* bstring_to_cstr(BString *bstr) {
    char *cstr = malloc(bstr->size + 1);
    if (cstr == NULL) {
        return NULL;
    }
    memcpy(cstr, bstr->chars, bstr->size);
    cstr[bstr->size] = '\0';
    return cstr;
}