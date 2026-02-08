#ifndef _PTHREAD_LEVENSHTEIN_TEST_LEVENSHTEIN_HPP
#define _PTHREAD_LEVENSHTEIN_TEST_LEVENSHTEIN_HPP

extern "C"
{
#include "levenshtein.h"
};

#include <string>

#include <gtest/gtest.h>

class TestLevenshtein : public testing::Test
{
private:
    std::string row_string;
    std::string column_string;

public:
    void set(const std::string row_string, const std::string column_string);

    void assert_dist(size_t expected, uint8_t thread_count);
};

#endif // _PTHREAD_LEVENSHTEIN_TEST_LEVENSHTEIN_HPP
