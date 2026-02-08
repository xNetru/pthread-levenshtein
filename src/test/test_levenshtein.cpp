#include <gtest/gtest.h>
#include "test_levenshtein.hpp"

void TestLevenshtein::set(const std::string first, const std::string second)
{
    this->first = first;
    this->second = second;
}

void TestLevenshtein::assert_dist(size_t expected, leven_comp_mode_t mode)
{
    leven_data_t ldata;
    leven_status_t status;

    status = leven_data_init(&ldata, first.c_str(), second.c_str());
    EXPECT_EQ(status, success);

    size_t result;
    status = leven_compute_dist(&result, &ldata, single_threaded);
    EXPECT_EQ(status, success);
    EXPECT_EQ(result, expected);

    leven_data_destroy(&ldata);
}

TEST_F(TestLevenshtein, EqualLengthSingleDistPositive)
{
    this->set("abba", "baba");
    this->assert_dist(2, single_threaded);
}

TEST_F(TestLevenshtein, DifferentLengthSingleDistPositive)
{
    this->set("abba", "abaca");
    this->assert_dist(2, single_threaded);
}

TEST_F(TestLevenshtein, EmptyWordSingleDistPositive)
{
    this->set("", "aaaaaa");
    this->assert_dist(6, single_threaded);
}