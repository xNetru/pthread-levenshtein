#include <gtest/gtest.h>
#include "test_levenshtein.hpp"

void TestLevenshtein::set(const std::string row_string, const std::string column_string)
{
    this->row_string = row_string;
    this->column_string = column_string;
}

void TestLevenshtein::assert_dist(size_t expected, uint8_t thread_count)
{
    leven_data_t ldata;
    leven_status_t status;

    status = leven_data_init(&ldata, row_string.c_str(), column_string.c_str(), thread_count);
    EXPECT_EQ(status, success);

    size_t result;
    status = leven_compute_dist(&result, &ldata);
    EXPECT_EQ(status, success);
    EXPECT_EQ(result, expected);

    leven_data_destroy(&ldata);
}

TEST_F(TestLevenshtein, EqualLengthSingleDistPositive)
{
    this->set("abba", "baba");
    this->assert_dist(2, 1);
}

TEST_F(TestLevenshtein, DifferentLengthSingleDistPositive)
{
    this->set("abba", "abaca");
    this->assert_dist(2, 1);
}

TEST_F(TestLevenshtein, EmptyWordSingleDistPositive)
{
    this->set("", "aaaaaa");
    this->assert_dist(6, 1);
}