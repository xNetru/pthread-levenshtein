#ifndef _PTHREAD_LEVENSHTEIN_LEVENSHTEIN_H
#define _PTHREAD_LEVENSHTEIN_LEVENSHTEIN_H

#include <stddef.h>

typedef struct leven_data
{
    const char *first;
    const char *second;
    const char *dyn_table;
    size_t first_size;
    size_t second_size;
} leven_data_t;

typedef struct leven_trans
{
    enum
    {
        ins,
        del,
        subs,
    } action;
    char c;
    size_t index;
} leven_trans_t;

typedef struct leven_result
{
    size_t distance;
    leven_trans *trans_table;
} leven_result_t;

int leven_data_init(leven_data_t *data, const char *first, const char *second);
void leven_data_destroy(leven_data_t *data);

int leven_compute(leven_data_t *data, leven_result_t *result);

void leven_result_free(leven_result_t *result);

#endif // _PTHREAD_LEVENSHTEIN_LEVENSHTEIN_H
