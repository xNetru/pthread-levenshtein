#ifndef _PTHREAD_LEVENSHTEIN_LEVENSHTEIN_H
#define _PTHREAD_LEVENSHTEIN_LEVENSHTEIN_H

#include <stddef.h>
#include <stdint.h>

#ifndef PL_MAX_WORD_SIZE
#define PL_MAX_WORD_SIZE 64 * 1024
#endif

typedef struct leven_data
{
    const char *first;
    const char *second;
    const uint32_t *dyn_table;
    size_t first_size;
    size_t second_size;
} leven_data_t;

typedef enum leven_status
{
    // on success
    success = 0,
    // on error
    null_parameters,
    max_word_size_exceeded,
    malloc_failure,
    error,
} leven_status_t;

typedef enum leven_comp_mode
{
    single_threaded,
    multi_threaded
} leven_comp_mode_t;

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
    leven_trans_t *trans_table;
} leven_result_t;

leven_status_t leven_data_init(leven_data_t *data, const char *first, const char *second);
void leven_data_destroy(leven_data_t *data);

leven_status_t leven_compute_dist(leven_result_t *result, leven_data_t *data, leven_comp_mode_t mode);
leven_status_t leven_compute_trans(size_t *result, leven_data_t *data, leven_comp_mode_t mode);

void leven_result_free(leven_result_t *result);

#endif // _PTHREAD_LEVENSHTEIN_LEVENSHTEIN_H
