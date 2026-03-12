/* test_harness.h - Minimal test framework for NPU runtime tests */

#ifndef _TEST_HARNESS_H
#define _TEST_HARNESS_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

static int g_test_pass;
static int g_test_fail;
static int g_test_total;

#define TEST_BEGIN(name)                    \
    do                                      \
    {                                       \
        ++g_test_total;                     \
        printf("  [TEST] %-50s ", name);    \
        fflush(stdout);                     \
    }   while (0)

#define TEST_PASS()         \
    do                      \
    {                       \
        ++g_test_pass;      \
        printf("PASS\n");   \
    }   while (0)

#define TEST_FAIL(msg)              \
    do                              \
    {                               \
        ++g_test_fail;              \
        printf("FAIL (%s)\n", msg); \
    }   while (0)

#define ASSERT_EQ(a, b)                 \
    do                                  \
    {                                   \
        if ((a) != (b))                 \
        {                               \
            TEST_FAIL(#a " != " #b);    \
            return;                     \
        }                               \
    }   while (0)

#define ASSERT_NE(a, b)                 \
    do                                  \
    {                                   \
        if ((a) == (b))                 \
        {                               \
            TEST_FAIL(#a " == " #b);    \
            return;                     \
        }                               \
    }   while (0)

#define ASSERT_TRUE(cond)                   \
    do                                      \
    {                                       \
        if (!(cond))                        \
        {                                   \
            TEST_FAIL(#cond " is false");   \
            return;                         \
        }                                   \
    }   while (0)

#define ASSERT_MEM_EQ(a, b, n)                              \
    do                                                      \
    {                                                       \
        if (memcmp((a), (b), (n)) != 0)                     \
        {                                                   \
            TEST_FAIL("memory mismatch: " #a " vs " #b);    \
            return;                                         \
        }                                                   \
    }   while (0)

#define SUITE_BEGIN(name)               \
    do                                  \
    {                                   \
        printf("\n=== %s ===\n", name); \
        g_test_pass = 0;                \
        g_test_fail = 0;                \
        g_test_total = 0;               \
    }   while (0)

#define SUITE_END()                                                         \
    do                                                                      \
    {                                                                       \
        printf("\n--- Results: %d/%d passed", g_test_pass, g_test_total);   \
        if (g_test_fail)                                                    \
            printf(" (%d FAILED)", g_test_fail);                            \
        printf(" ---\n\n");                                                 \
    }   while (0)

#define SUITE_RESULT() (!g_test_fail ? 0 : 1)

#endif /* _TEST_HARNESS_H */
