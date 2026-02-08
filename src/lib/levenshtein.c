#include "levenshtein.h"

#include <string.h>

static size_t leven_dyn_table_size(size_t first_size, size_t second_size)
{
    return (first_size + 1) * (second_size + 1);
}

leven_status_t leven_data_init(leven_data_t *data, const char *first, const char *second)
{
    if (!data || !first || !second)
    {
        return null_parameters;
    }

    size_t first_size = strlen_s(first, PL_MAX_WORD_SIZE);
    size_t second_size = strlen_s(second, PL_MAX_WORD_SIZE);

    if (first_size == PL_MAX_WORD_SIZE || second == PL_MAX_WORD_SIZE)
    {
        return max_word_size_exceeded;
    }

    size_t dyn_table_count = leven_dyn_table_size(first_size, second_size);
    uint32_t *dyn_table = (uint32_t *)malloc(sizeof(uint32_t) * dyn_table_count);

    if (!dyn_table)
    {
        return malloc_failure;
    }

    data->first = first;
    data->first_size = first_size;
    data->second = second;
    data->second_size = second_size;
    data->dyn_table = dyn_table;

    return success;
}

void leven_data_destroy(leven_data_t *data)
{
    if (!data || !data->dyn_table)
    {
        return;
    }

    free(data->dyn_table);
}

void leven_result_free(leven_result_t *result)
{
    if (!result || !result->trans_table)
    {
        return;
    }

    free(result->trans_table);
}
