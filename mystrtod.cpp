// Copyright (c) 2020, Ben Wiederhake <BenWiederhake.GitHub@gmx.de>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef char assert_size_t_is_int[sizeof(size_t) == 4 ? 1 : -1];

#if 1
static const char* TEXT_ERROR = "\x1b[01;35m";
static const char* TEXT_WRONG = "\x1b[01;31m";
static const char* TEXT_RESET = "\x1b[0m";
#else
static const char* TEXT_ERROR = "";
static const char* TEXT_WRONG = "";
static const char* TEXT_RESET = "";
#endif

size_t serenity_strlen(const char* str)
{
    // Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
    size_t len = 0;
    while (*(str++))
        ++len;
    return len;
}

int serenity_atoi(const char* str)
{
    // Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
    size_t len = serenity_strlen(str);
    int value = 0;
    bool isNegative = false;
    for (size_t i = 0; i < len; ++i) {
        if (i == 0 && str[0] == '-') {
            isNegative = true;
            continue;
        }
        if (str[i] < '0' || str[i] > '9')
            return value;
        value = value * 10;
        value += str[i] - '0';
    }
    return isNegative ? -value : value;
}

double old_strtod(const char* str, char** endptr)
{
    // Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
    size_t len = serenity_strlen(str);
    size_t weight = 1;
    int exp_val = 0;
    double value = 0.0f;
    double fraction = 0.0f;
    bool has_sign = false;
    bool is_negative = false;
    bool is_fractional = false;
    bool is_scientific = false;

    if (str[0] == '-') {
        is_negative = true;
        has_sign = true;
    }
    if (str[0] == '+') {
        has_sign = true;
    }
    size_t i;
    for (i = has_sign; i < len; i++) {

        // Looks like we're about to start working on the fractional part
        if (str[i] == '.') {
            is_fractional = true;
            continue;
        }

        if (str[i] == 'e' || str[i] == 'E') {
            if (str[i + 1] == '-')
                exp_val = -serenity_atoi(str + i + 2);
            else if (str[i + 1] == '+')
                exp_val = serenity_atoi(str + i + 2);
            else
                exp_val = serenity_atoi(str + i + 1);

            is_scientific = true;
            continue;
        }

        if (str[i] < '0' || str[i] > '9' || exp_val != 0)
            continue;

        if (is_fractional) {
            fraction *= 10;
            fraction += str[i] - '0';
            weight *= 10;
        } else {
            value = value * 10;
            value += str[i] - '0';
        }
    }

    fraction /= weight;
    value += fraction;

    if (is_scientific) {
        bool divide = exp_val < 0;
        if (divide)
            exp_val *= -1;

        for (int i = 0; i < exp_val; i++) {
            if (divide)
                value /= 10;
            else
                value *= 10;
        }
    }
    //FIXME: Not entirely sure if this is correct, but seems to work.
    if (endptr)
        *endptr = const_cast<char*>(str + i);
    return is_negative ? -value : value;
}

double new_strtod(const char* str, char** endptr) {
    *endptr = const_cast<char*>(str);
    return 0;
}

struct Testcase {
    const char* test_name;
    int should_consume;
    const char* hex;
    const char* test_string;
};

static Testcase TESTCASES[] = {
    // What I came up with on my own
    {"BW0", 1, "0000000000000000", ".."},
    {"BW1", 2, "3ff0000000000000", "1..0"},
    {"BW2", 1, "3ff0000000000000", "1"},
    {"BW3", 2, "0000000000000000", ".0"},
    {"BW4", 2, "0000000000000000", "0."},
    {"BW5", 2, "0000000000000000", "0.."},
    // From the Serenity GitHub tracker:
    // https://github.com/SerenityOS/serenity/issues/1979
    {"SR1", -1, "4014000000000001", "5.000000000000001"},
    {"SR2", -1, "4014000000000000", "5.0000000000000001"},
    // 5.0000000000000001
    // 1.0000000000000000000000000000001
    // 1.00000000000000000000000000000001
    // 1.12345678912345678912345678912345
    // 1.123456789123456789123456789123456789123456789
};

constexpr size_t NUM_TESTCASES = sizeof(TESTCASES) / sizeof(TESTCASES[0]);

typedef double (*strtod_fn_t)(const char* str, char** endptr);

void display_strtod(strtod_fn_t strtod_fn, const char* test_string, const char* expect_hex, int expect_consume) {
    union readable_t {
        double as_double;
        unsigned char as_bytes[8];
    };
    typedef char assert_double_8bytes[sizeof(double) == 8 ? 1 : -1];
    (void)sizeof(assert_double_8bytes);
    typedef char assert_readable_8bytes[sizeof(readable_t) == 8 ? 1 : -1];
    (void)sizeof(assert_readable_8bytes);
    readable_t readable;
    char* endptr = (char*)0x123;

    readable.as_double = strtod_fn(test_string, &endptr);

    char actual_hex[16 + 1] = {0};
    for (size_t i = 0; i < 8; ++i) {
        // Little endian, need to reverse order. Ugh.
        snprintf(&actual_hex[2 * i], 3, "%02x", readable.as_bytes[8 - 1 - i]);
    }

    bool actual_consume_possible = false;
    int actual_consume;

    if (endptr < test_string) {
        actual_consume = 999;
    } else {
        const char* max_endptr = test_string + serenity_strlen(test_string);
        actual_consume_possible = endptr <= max_endptr;
        actual_consume = endptr - test_string;
    }

    bool wrong_hex = strcmp(expect_hex, actual_hex) != 0;
    bool error_cns = !actual_consume_possible;
    bool wrong_cns = !error_cns && (actual_consume != expect_consume);

    printf(" %s%s%s(%s%3u%s)",
           wrong_hex ? TEXT_WRONG : "",
           actual_hex,
           wrong_hex ? TEXT_RESET : "",
           error_cns ? TEXT_ERROR : wrong_cns ? TEXT_WRONG : "",
           actual_consume,
           (error_cns || wrong_cns) ? TEXT_RESET : "");
}

int main()
{
    if (sizeof(size_t) != 4) {
        printf("lolwut?!\n");
        return 1;
    }
    printf("Running %u testcases...\n", NUM_TESTCASES);
    printf("%5s(%-5s): %16s(%3s) %16s(%3s) %16s(%3s) %16s(%3s) -- %s\n", "num", "name", "correct", "cns", "builtin", "cns", "old_strtod", "cns", "new_strtod", "cns", "teststring");
    for (size_t i = 0; i < NUM_TESTCASES; i++) {
        Testcase& tc = TESTCASES[i];
        if (tc.should_consume == -1) {
            tc.should_consume = strlen(tc.test_string);
        }
        printf("%5u(%-5s):", i, tc.test_name);
        printf(" %s(%3d)", tc.hex, tc.should_consume);
        display_strtod(strtod, tc.test_string, tc.hex, tc.should_consume);
        display_strtod(old_strtod, tc.test_string, tc.hex, tc.should_consume);
        display_strtod(new_strtod, tc.test_string, tc.hex, tc.should_consume);
        printf(" -- %s\n", tc.test_string);
    }
    return 0;
}
