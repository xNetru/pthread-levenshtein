#include "levenshtein.h"

#include "error.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>

#define MIN(x, y) (x < y ? x : y)

// general helpers
static leven_status_t leven_data_validate(leven_data_t *data)
{
    if (!data)
    {
        return null_parameters;
    }

    if (!data->row_string || !data->column_string || !data->dist_table)
    {
        return null_parameters;
    }

    auto thread_count = data->thread_count;
    if (0 == thread_count)
    {
        return invalid_parameter;
    }

    if (thread_count > 1 && !data->last_match)
    {
        return invalid_parameter;
    }

    return success;
}

static size_t leven_str_size_to_table_dim(size_t string_size)
{
    return string_size + 1;
}

static size_t leven_dist_table_size(size_t row_string_size, size_t column_string_size)
{
    return leven_str_size_to_table_dim(row_string_size) *
           leven_str_size_to_table_dim(column_string_size);
}

static size_t leven_2d_to_1d_index(size_t row, size_t column, size_t row_size)
{
    return row * row_size + column;
}

// leven data
leven_status_t leven_data_init(leven_data_t *data, const char *row_string, const char *column_string, uint8_t thread_count)
{
    if (!data || !row_string || !column_string)
    {
        return null_parameters;
    }

    size_t row_string_size = strlen(row_string);
    size_t column_string_size = strlen(column_string);

    size_t dist_table_count = leven_dist_table_size(row_string_size, column_string_size);
    uint32_t *dist_table = (uint32_t *)malloc(sizeof(uint32_t) * dist_table_count);

    if (!dist_table)
    {
        return malloc_failure;
    }

    size_t *last_match = NULL;
    if (0 == thread_count)
    {
        thread_count = PL_DEFAULT_THREAD_COUNT;
    }

    if (thread_count > 1)
    {
        last_match = (size_t *)malloc(sizeof(size_t) * dist_table_count);
        if (!last_match)
        {
            free(dist_table);
            return malloc_failure;
        }
    }

    data->row_string = row_string;
    data->column_string = column_string;
    data->dist_table = dist_table;
    data->last_match = last_match;
    data->row_string_size = row_string_size;
    data->column_string_size = column_string_size;
    data->thread_count = thread_count;

    return success;
}

void leven_data_destroy(leven_data_t *data)
{
    if (!data || !data->dist_table)
    {
        return;
    }

    free((void *)(data->dist_table));
}

// leven dist single threaded
static uint32_t leven_compute_for_index_single(const uint32_t *dist_table, const char *row_string, const char *column_string, size_t i, size_t j, size_t row_size)
{
    size_t ind01 = leven_2d_to_1d_index(i, j - 1, row_size);
    size_t ind10 = leven_2d_to_1d_index(i - 1, j, row_size);
    size_t ind11 = leven_2d_to_1d_index(i - 1, j - 1, row_size);

    uint32_t leven_dist = dist_table[ind11];
    if (row_string[i - 1] != column_string[j - 1])
    {
        leven_dist++;
    }

    return MIN(leven_dist, MIN(dist_table[ind01], dist_table[ind10]) + 1);
}

static leven_status_t leven_compute_dist_single(size_t *result, leven_data_t *data)
{
    const char *row_string = data->row_string;
    size_t row_size = leven_str_size_to_table_dim(data->row_string_size);
    const char *column_string = data->column_string;
    size_t column_size = leven_str_size_to_table_dim(data->column_string_size);
    uint32_t *dist_table = data->dist_table;

    for (size_t j = 0; j < row_size; j++)
    {
        size_t ind = leven_2d_to_1d_index(0, j, row_size);
        dist_table[ind] = j;
    }

    for (size_t i = 1; i < column_size; i++)
    {
        size_t ind = leven_2d_to_1d_index(i, 0, row_size);
        dist_table[ind] = i;
        for (size_t j = 1; j < row_size; j++)
        {
            ind = leven_2d_to_1d_index(i, j, row_size);
            dist_table[ind] = leven_compute_for_index_single(dist_table, row_string, column_string, i, j, row_size);
        }
    }

    size_t last_ind = row_size * column_size - 1;
    *result = dist_table[last_ind];
    return success;
}

// leven dist multi threaded
static void leven_cancel_threads(pthread_t *tid, uint8_t thread_count)
{
    for (int i = 0; i < thread_count; i++)
    {
        if (0 != pthread_cancel(tid[i]))
        {
            PL_ERROR("pthread_cancel: error");
        }
    }
}

static void leven_join_threads(pthread_t *tid, uint8_t thread_count)
{
    int err;
    for (uint8_t i = 0; i < thread_count; i--)
    {
        if (0 != (err = pthread_join(tid[i], NULL)))
        {
            if (EDEADLK == err)
            {
                PL_ERROR("pthread_join: deadlock detected");
            }
            PL_ERROR("pthread_join: error");
        }
    }
}

typedef struct leven_thread_data
{
    uint8_t thread_index;
    leven_data_t *data;
    pthread_barrier_t *barrier;
} leven_thread_data_t;

static leven_status_t leven_dispatch_threads(leven_data_t *data, typeof(void *(void *)) *routine, leven_thread_data_t *tdata)
{
    uint8_t thread_count = data->thread_count;

    pthread_t *tid = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);

    leven_status_t status = success;
    if (!tid)
    {
        status = malloc_failure;
        goto cleanup;
    }

    for (uint8_t i = 0; i < thread_count; i++)
    {
        if (0 != (pthread_create(&tid[i], NULL, routine, (void *)(&tdata[i]))))
        {
            leven_cancel_threads(tid, i);
            leven_join_threads(tid, i);
            status = pthread_error;
            goto cleanup;
        }
    }

    leven_join_threads(tid, thread_count);

cleanup:
    free((void *)tid);

    return status;
}

static void leven_fill_last_match_row(int i, size_t row_size, size_t *last_match,
                                      const char *row_string, const char *column_string)
{
    if (0 == i)
    {
        for (size_t j = 0; j < row_size; j++)
        {
            size_t ind = leven_2d_to_1d_index(i, j, row_size);
            last_match[ind] = 0;
        }
    }
    else
    {
        size_t ind = leven_2d_to_1d_index(i, 0, row_size);
        last_match[ind] = 0;
        char column_char = column_string[i - 1];
        for (size_t j = 1; j < row_size; j++)
        {
            ind = leven_2d_to_1d_index(i, j, row_size);
            size_t ind01 = leven_2d_to_1d_index(i, j - 1, row_size);
            char row_char = row_string[j - 1];
            last_match[ind] = column_char == row_char ? j : last_match[ind01];
        }
    }
}

static void *leven_fill_last_match_routine(void *arg)
{
    leven_thread_data_t *tdata = (leven_thread_data_t *)arg;
    if (!tdata || success != leven_data_validate(tdata->data))
    {
        PL_ERROR("invalid thread data");
    }

    leven_data_t *data = tdata->data;
    const char *row_string = data->row_string;
    const char *column_string = data->column_string;
    size_t row_size = leven_str_size_to_table_dim(data->row_string_size);
    size_t column_size = leven_str_size_to_table_dim(data->column_string_size);
    size_t *last_match = data->last_match;
    uint8_t thread_count = data->thread_count;

    for (size_t i = tdata->thread_index; i < column_size; i += thread_count)
    {
        leven_fill_last_match_row(i, row_size, last_match, row_string, column_string);
    }

    return NULL;
}

static leven_status_t leven_fill_last_match(leven_data_t *data)
{
    uint8_t thread_count = data->thread_count;

    leven_thread_data_t *tdata =
        (leven_thread_data_t *)malloc(sizeof(leven_thread_data_t) * thread_count);

    leven_status_t status;
    if (!tdata)
    {
        status = malloc_failure;
        goto cleanup;
    }

    for (uint8_t i = 0; i < thread_count; i++)
    {
        leven_thread_data_t *td = &(tdata[i]);
        td->data = data;
        td->thread_index = i;
    }

    status =
        leven_dispatch_threads(data, leven_fill_last_match_routine, tdata);

cleanup:
    free((void *)tdata);

    return status;
}

void leven_fill_dist_table_row(size_t i, const char *row_string, const char *column_string,
                               size_t row_size, uint32_t *dist_table, size_t *last_match,
                               pthread_barrier_t *barrier, size_t start_index, size_t end_index)
{
    if (0 == i)
    {
        for (size_t j = start_index; j < end_index; j++)
        {
            size_t ind = leven_2d_to_1d_index(0, j, row_size);
            dist_table[ind] = j;
        }
    }
    else
    {
        for (size_t j = start_index; j < end_index; j++)
        {
            size_t ind = leven_2d_to_1d_index(i, j, row_size);
            size_t ind10 = leven_2d_to_1d_index(i - 1, j, row_size);
            size_t ind11 = leven_2d_to_1d_index(i - 1, j - 1, row_size);
            if (j == 0)
            {
                dist_table[ind] = i;
            }
            else if (row_string[j - 1] == column_string[i - 1])
            {
                dist_table[ind] = MIN(dist_table[ind11], dist_table[ind10] + 1);
            }
            else
            {
                size_t min_dist = MIN(dist_table[ind11], dist_table[ind10]) + 1;
                size_t k = j - last_match[ind];
                size_t ind1k1 = leven_2d_to_1d_index(i - 1, j - k - 1, row_size);
                dist_table[ind] = MIN(dist_table[ind1k1] + k, min_dist);
            }
        }
    }

    pthread_barrier_wait(barrier);
}

static void leven_compute_thread_subrange(size_t total_range, uint8_t thread_index,
                                          uint8_t thread_count, size_t *start,
                                          size_t *end)
{
    size_t subrange_size = total_range / thread_count;
    size_t subrange_remainder = total_range % thread_count;

    size_t start_tmp;
    size_t end_tmp;
    if (thread_index < subrange_remainder)
    {
        start_tmp = thread_index * (subrange_size + 1);
        end_tmp = start_tmp + subrange_size + 1;
    }
    else
    {
        start_tmp = subrange_remainder * (subrange_size + 1) +
                    (thread_index - subrange_remainder) * subrange_size;
        end_tmp = start_tmp + subrange_size;
    }

    *start = start_tmp;
    *end = end_tmp;
}

static void *leven_fill_dist_table_routine(void *arg)
{
    leven_thread_data_t *tdata = (leven_thread_data_t *)arg;
    leven_data_t *data = tdata->data;
    pthread_barrier_t *barrier = tdata->barrier;

    if (!tdata || !barrier || success != leven_data_validate(tdata->data))
    {
        PL_ERROR("invalid thread data");
    }

    const char *row_string = data->row_string;
    const char *column_string = data->column_string;
    size_t row_size = leven_str_size_to_table_dim(data->row_string_size);
    size_t column_size = leven_str_size_to_table_dim(data->column_string_size);
    size_t *last_match = data->last_match;
    uint32_t *dist_table = data->dist_table;
    uint8_t thread_count = data->thread_count;
    uint8_t thread_index = tdata->thread_index;

    size_t start_index;
    size_t end_index;
    leven_compute_thread_subrange(row_size, thread_index, thread_count, &start_index, &end_index);

    for (size_t i = 0; i < column_size; i++)
    {
        leven_fill_dist_table_row(i, row_string, column_string,
                                  row_size, dist_table, last_match,
                                  barrier, start_index, end_index);
    }

    return NULL;
}

static leven_status_t leven_fill_dist_table(leven_data_t *data)
{
    // truncate number of threads to at most row size
    uint8_t thread_count_backup = data->thread_count;
    size_t row_size = leven_str_size_to_table_dim(data->row_string_size);
    uint8_t thread_count = MIN(row_size, thread_count_backup);
    data->thread_count = thread_count;

    leven_thread_data_t *tdata =
        (leven_thread_data_t *)malloc(sizeof(leven_thread_data_t) * thread_count);

    leven_status_t status;
    if (!tdata)
    {
        status = malloc_failure;
        goto cleanup;
    }

    pthread_barrier_t barrier;
    if (0 != pthread_barrier_init(&barrier, NULL, thread_count))
    {
        status = pthread_barrier_error;
        goto cleanup;
    }

    for (uint8_t i = 0; i < thread_count; i++)
    {
        leven_thread_data_t *td = &(tdata[i]);
        td->data = data;
        td->thread_index = i;
        td->barrier = &barrier;
    }

    status =
        leven_dispatch_threads(data, leven_fill_dist_table_routine, tdata);

    if (0 != pthread_barrier_destroy(&barrier))
    {
        status = pthread_barrier_error;
    }

cleanup:
    data->thread_count = thread_count_backup;
    free((void *)tdata);

    return status;
}

static leven_status_t leven_compute_dist_multi(size_t *result, leven_data_t *data)
{
    leven_status_t status;
    if (success != (status = leven_fill_last_match(data)))
    {
        return status;
    }

    if (success != (status = leven_fill_dist_table(data)))
    {
        return status;
    }

    uint32_t *dist_table = data->dist_table;
    size_t last_ind =
        leven_dist_table_size(data->row_string_size, data->column_string_size) - 1;
    *result = dist_table[last_ind];

    return success;
}

// leven compute
leven_status_t leven_compute_dist(size_t *result, leven_data_t *data)
{
    if (!result)
    {
        return null_parameters;
    }

    leven_status_t status = leven_data_validate(data);
    if (success != status)
    {
        return status;
    }

    if (1 == data->thread_count)
    {
        return leven_compute_dist_single(result, data);
    }

    return leven_compute_dist_multi(result, data);
}
