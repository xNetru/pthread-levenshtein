#ifndef _PTHREAD_LEVENSHTEIN_LEVENSHTEIN_H
#define _PTHREAD_LEVENSHTEIN_LEVENSHTEIN_H

#include <stddef.h>
#include <stdint.h>

#define PL_DEFAULT_THREAD_COUNT 16

typedef struct leven_data
{
    const char *row_string;
    const char *column_string;
    uint32_t *dist_table;
    size_t *last_match;
    size_t row_string_size;
    size_t column_string_size;
    uint8_t thread_count;
} leven_data_t;

typedef enum leven_status
{
    // on success
    success = 0,
    // on error
    null_parameters,
    invalid_parameter,
    malloc_failure,
    pthread_error,
    pthread_barrier_error,
    error,
} leven_status_t;

leven_status_t leven_data_init(leven_data_t *data, const char *row_string, const char *column_string, uint8_t thread_count);
void leven_data_destroy(leven_data_t *data);

leven_status_t leven_compute_dist(size_t *result, leven_data_t *data);

#endif // _PTHREAD_LEVENSHTEIN_LEVENSHTEIN_H
