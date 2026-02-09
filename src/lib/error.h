#ifndef _PTHREAD_LEVENSHTEIN_ERROR_H
#define _PTHREAD_LEVENSHTEIN_ERROR_H

#include <stdio.h>
#include <stdlib.h>

#define PL_ERROR(msg)                                              \
    perror(msg),                                                   \
        fprintf(stderr, "file: %s, line: %d", __FILE__, __LINE__), \
        exit(EXIT_FAILURE)

#endif // _PTHREAD_LEVENSHTEIN_ERROR_H