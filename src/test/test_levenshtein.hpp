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
    std::string first;
    std::string second;

public:
    void set_first(const std::string s);
    void set_second(const std::string s);

    void assert_dist(size_t expected, leven_comp_mode_t mode);
};

#endif // _PTHREAD_LEVENSHTEIN_TEST_LEVENSHTEIN_HPP
