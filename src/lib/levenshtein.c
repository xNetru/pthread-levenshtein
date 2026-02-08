#include "levenshtein.h"

#include <stdlib.h>
#include <string.h>

#define MIN(x, y) (x < y ? x : y)

// general helpers
static leven_status_t leven_data_validate(leven_data_t *data)
{
    if (!data)
    {
        return null_parameters;
    }

    if (!data->row_string || !data->column_string || !data->dyn_table)
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

static size_t leven_dyn_table_size(size_t row_string_size, size_t column_string_size)
{
    return (row_string_size + 1) * (column_string_size + 1);
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

    size_t dyn_table_count = leven_dyn_table_size(row_string_size, column_string_size);
    uint32_t *dyn_table = (uint32_t *)malloc(sizeof(uint32_t) * dyn_table_count);

    if (!dyn_table)
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
        last_match = (size_t *)malloc(sizeof(size_t) * dyn_table_count);
        if (!last_match)
        {
            free(dyn_table);
            return malloc_failure;
        }
    }

    data->row_string = row_string;
    data->column_string = column_string;
    data->dyn_table = dyn_table;
    data->last_match = last_match;
    data->row_string_size = row_string_size;
    data->column_string_size = column_string_size;
    data->thread_count = thread_count;

    return success;
}

void leven_data_destroy(leven_data_t *data)
{
    if (!data || !data->dyn_table)
    {
        return;
    }

    free((void *)(data->dyn_table));
}

// leven dist single threaded
static uint32_t leven_compute_for_index_single(const uint32_t *dyn_table, const char *row_string, const char *column_string, size_t i, size_t j, size_t row_size)
{
    size_t ind01 = leven_2d_to_1d_index(i, j - 1, row_size);
    size_t ind10 = leven_2d_to_1d_index(i - 1, j, row_size);
    size_t ind11 = leven_2d_to_1d_index(i - 1, j - 1, row_size);

    uint32_t leven_dist = dyn_table[ind11];
    if (row_string[i - 1] != column_string[j - 1])
    {
        leven_dist++;
    }

    return MIN(leven_dist, MIN(dyn_table[ind01], dyn_table[ind10]) + 1);
}

static leven_status_t leven_compute_dist_single(size_t *result, leven_data_t *data)
{
    const char *row_string = data->row_string;
    size_t row_size = data->row_string_size + 1;
    const char *column_string = data->column_string;
    size_t column_size = data->column_string_size + 1;
    uint32_t *dyn_table = data->dyn_table;

    for (size_t j = 0; j < row_size; j++)
    {
        size_t ind = leven_2d_to_1d_index(0, j, row_size);
        dyn_table[ind] = j;
    }

    for (size_t i = 1; i < column_size; i++)
    {
        size_t ind = leven_2d_to_1d_index(i, 0, row_size);
        dyn_table[ind] = i;
        for (size_t j = 1; j < row_size; j++)
        {
            ind = leven_2d_to_1d_index(i, j, row_size);
            dyn_table[ind] = leven_compute_for_index_single(dyn_table, row_string, column_string, i, j, row_size);
        }
    }

    size_t last_ind = row_size * column_size - 1;
    *result = dyn_table[last_ind];
    return success;
}

// leven compute
leven_status_t leven_compute_dist(size_t *result, leven_data_t *data)
{
    if(!result)
    {
        return null_parameters;
    }

    leven_status_t valid_status = leven_data_validate(data);
    if (success != valid_status)
    {
        return valid_status;
    }

    leven_status_t comp_status = success;

    if (1 == data->thread_count)
    {
        comp_status = leven_compute_dist_single(result, data);
    }
    else
    {
        // TODO multithread
    }

    return comp_status;
}
