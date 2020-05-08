// Copyright (c) 2020, Ben Wiederhake <BenWiederhake.GitHub@gmx.de>

#include <assert.h>
#include <ctype.h>
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

void strtons(const char* str, char** endptr) {
    assert(endptr);
    char* ptr = const_cast<char*>(str);
    while (isspace(*ptr)) {
        ptr += 1;
    }
    *endptr = ptr;
}

enum Sign {
    Negative,
    Positive,
};

Sign strtosign(const char* str, char** endptr) {
    assert(endptr);
    if (*str == '+') {
        *endptr = const_cast<char*>(str + 1);
        return Sign::Positive;
    } else if (*str == '-') {
        *endptr = const_cast<char*>(str + 1);
        return Sign::Negative;
    } else {
        *endptr = const_cast<char*>(str);
        return Sign::Positive;
    }
}

enum DigitConsumeDecision {
    Consumed,
    PosOverflow,
    NegOverflow,
    Invalid,
};

template<typename T, T min_value, T max_value>
class NumParser {
public:
    NumParser(Sign sign, int base)
        : m_base(base), m_num(0), m_sign(sign)
    {
        m_cutoff = positive() ? (max_value / base) : (min_value / base);
        m_max_digit_after_cutoff = positive() ? (max_value % base) : (min_value % base);
    }

    int parse_digit(char ch) {
        int digit;
        if (isdigit(ch))
            digit = ch - '0';
        else if (islower(ch))
            digit = ch - ('a' - 10);
        else if (isupper(ch))
            digit = ch - ('A' - 10);
        else
            return -1;

        if (digit >= m_base)
            return -1;

        return digit;
    }

    DigitConsumeDecision consume(char ch) {
        int digit = parse_digit(ch);
        if (digit == -1)
            return DigitConsumeDecision::Invalid;

        if (!can_append_digit(digit)) {
            if (m_sign != Sign::Negative) {
                return DigitConsumeDecision::PosOverflow;
            } else {
                return DigitConsumeDecision::NegOverflow;
            }
        }

        m_num *= m_base;
        m_num += positive() ? digit : -digit;

        return DigitConsumeDecision::Consumed;
    }

    T number() const { return m_num; };

private:
    // FIXME: NOMOVE

    bool can_append_digit(int digit) {
        const bool is_below_cutoff = positive() ? (m_num < m_cutoff) : (m_num > m_cutoff);

        if (is_below_cutoff) {
            return true;
        } else {
            return m_num == m_cutoff && digit < m_max_digit_after_cutoff;
        }
    }

    bool positive() const {
        return m_sign != Sign::Negative;
    }

    const T m_base;
    T m_num;
    T m_cutoff;
    int m_max_digit_after_cutoff;
    Sign m_sign;
};

#define INT_MAX 2147483647L
#define INT_MIN (-LONG_MAX - 1L)
#define LONG_MAX 2147483647L
#define LONG_MIN (-LONG_MAX - 1L)
#define LONG_LONG_MAX 9223372036854775807LL
#define LONG_LONG_MIN (-LONG_LONG_MAX - 1LL)

typedef NumParser<int, INT_MIN, INT_MAX> IntParser;
typedef NumParser<long, LONG_MIN, LONG_MAX> LongParser;
typedef NumParser<long long, LONG_LONG_MIN, LONG_LONG_MAX> LongLongParser;

static const double MY_INFTY_POS = +1.0 / 0.0;
static const double MY_INFTY_NEG = -1.0 / 0.0;
static const double MY_NAN = -(0.0 / 0.0);

double new_strtod(const char* str, char** endptr) {
    // Parse spaces, sign, and base
    char* parse_ptr = const_cast<char*>(str);
    strtons(parse_ptr, &parse_ptr);
    const Sign sign = strtosign(parse_ptr, &parse_ptr);

    // Parse inf/nan, if applicable.
    if (*parse_ptr == 'i' || *parse_ptr == 'I') {
        if (*(parse_ptr + 1) == 'n' || *(parse_ptr + 1) == 'N') {
            if (*(parse_ptr + 2) == 'f' || *(parse_ptr + 2) == 'F') {
                if (endptr)
                    *endptr = parse_ptr + 3;
                if (sign != Sign::Negative) {
                    return MY_INFTY_POS;
                } else {
                    return MY_INFTY_NEG;
                }
            }
        }
    }
    if (*parse_ptr == 'n' || *parse_ptr == 'N') {
        if (*(parse_ptr + 1) == 'a' || *(parse_ptr + 1) == 'A') {
            if (*(parse_ptr + 2) == 'n' || *(parse_ptr + 2) == 'N') {
                if (endptr)
                    *endptr = parse_ptr + 3;
                return MY_NAN;
            }
        }
    }

    // Parse base
    char exponent_lower;
    char exponent_upper;
    int base = 10;
    if (*parse_ptr == '0') {
        const char base_ch = *(parse_ptr + 1);
        if (base_ch == 'x' || base_ch == 'X') {
            base = 16;
            parse_ptr += 2;
        }
    }

    if (base == 10) {
        exponent_lower = 'e';
        exponent_upper = 'E';
    } else {
        exponent_lower = 'p';
        exponent_upper = 'P';
    }

    // Parse "digits", possibly keeping track of the exponent offset.
    // We parse the most significant digits and the position in the
    // base-`base` representation separately. This allows us to handle
    // numbers like `0.0000000000000000000000000000000000001234` or
    // `1234567890123456789012345678901234567890` with ease.
    LongLongParser digits{sign, base};
    bool digits_usable = false;
    bool should_continue = true;
    bool digits_overflow = false;
    bool after_decimal = false;
    int exponent = 0;
    do {
        if (!after_decimal && *parse_ptr == '.') {
            after_decimal = true;
            parse_ptr += 1;
            continue;
        }

        bool is_a_digit;
        if (digits_overflow) {
            is_a_digit = digits.parse_digit(*parse_ptr) != -1;
        } else {
            DigitConsumeDecision decision = digits.consume(*parse_ptr);
            switch (decision) {
            case DigitConsumeDecision::Consumed:
                is_a_digit = true;
                // The very first actual digit must pass here:
                digits_usable = true;
                break;
            case DigitConsumeDecision::PosOverflow:
                // fallthrough
            case DigitConsumeDecision::NegOverflow:
                is_a_digit = true;
                digits_overflow = true;
                break;
            case DigitConsumeDecision::Invalid:
                is_a_digit = false;
                break;
            default:
                assert(false); // ASSERT_NOT_REACHED();
            }
        }

        if (is_a_digit) {
            exponent -= after_decimal ? 1 : 0;
            exponent += digits_overflow ? 1 : 0;
        }

        should_continue = is_a_digit;
        parse_ptr += should_continue;
    } while (should_continue);

    if (!digits_usable) {
        // No actual number value available.
        if (endptr)
            *endptr = const_cast<char*>(str);
        return 0.0;
    }

    // Parse exponent.
    // We already know the next character is not a digit in the current base,
    // nor a valid decimal point. Check whether it's an exponent sign.
    if (*parse_ptr == exponent_lower || *parse_ptr == exponent_upper) {
        // Need to keep the old parse_ptr around, in case of rollback.
        char* old_parse_ptr = parse_ptr;
        parse_ptr += 1;

        // Can't use atol or strtol here: Must accept excessive exponents,
        // even exponents >64 bits.
        Sign exponent_sign = strtosign(parse_ptr, &parse_ptr);
        IntParser exponent_parser{exponent_sign, base};
        bool exponent_usable = false;
        bool exponent_overflow = false;
        should_continue = true;
        do {
            bool is_a_digit;
            if (exponent_overflow) {
                is_a_digit = exponent_parser.parse_digit(*parse_ptr) != -1;
            } else {
                DigitConsumeDecision decision = exponent_parser.consume(*parse_ptr);
                switch (decision) {
                case DigitConsumeDecision::Consumed:
                    is_a_digit = true;
                    // The very first actual digit must pass here:
                    exponent_usable = true;
                    break;
                case DigitConsumeDecision::PosOverflow:
                    // fallthrough
                case DigitConsumeDecision::NegOverflow:
                    is_a_digit = true;
                    exponent_overflow = true;
                    break;
                case DigitConsumeDecision::Invalid:
                    is_a_digit = false;
                    break;
                default:
                    assert(false); // ASSERT_NOT_REACHED();
                }
            }

            should_continue = is_a_digit;
            parse_ptr += should_continue;
        } while (should_continue);

        if (!exponent_usable) {
            parse_ptr = old_parse_ptr;
        } else if (exponent_overflow) {
            // Technically this is wrong. If someone gives us 5GB of digits,
            // and then an exponent of -5_000_000_000, the resulting exponent
            // should be around 0.
            // However, I think it's safe to assume that we never have to deal
            // with that many digits anyway.
            if (sign != Sign::Negative) {
                exponent = INT_MIN;
            } else {
                exponent = INT_MAX;
            }
        } else {
            // Literal exponent is usable and fits in an int.
            // However, `exponent + exponent_parser.number()` might overflow an int.
            // This would result in the wrong sign of the exponent!
            long long new_exponent =
                static_cast<long long>(exponent) + static_cast<long long>(exponent_parser.number());
            if (new_exponent < INT_MIN) {
                exponent = INT_MIN;
            } else if (new_exponent > INT_MAX) {
                exponent = INT_MAX;
            } else {
                exponent = static_cast<int>(new_exponent);
            }
        }
    }

    // Parsing finished. now we only have to compute the result.
    if (endptr)
        *endptr = const_cast<char*>(parse_ptr);

    // If `digits` is zero, we don't even have to look at `exponent`.
    if (digits.number() == 0) {
        if (sign != Sign::Negative) {
            return 0.0;
        } else {
            return -0.0;
        }
    }

    // Deal with extreme exponents.
    // The smallest normal is 2^-1022.
    // The smallest denormal is 2^-1074.
    // The largest number in `digits` is 2^63 - 1.
    // Therefore, if "base^exponent" is smaller than 2^-(1074+63), the result is 0.0 anyway.
    // This threshold is roughly 5.3566 * 10^-343.
    // So if the resulting exponent is -344 or lower (closer to -inf),
    // the result is 0.0 anyway.
    // We only need to avoid false positives, so we can ignore base 16.
    if (exponent <= -344) {
        // Definitely can't be represented more precisely.
        // I lied, sometimes the result is +0.0, and sometimes -0.0.
        if (sign != Sign::Negative) {
            return 0.0;
        } else {
            return -0.0;
        }
    }
    // The largest normal is 2^+1024-eps.
    // The smallest number in `digits` is 1.
    // Therefore, if "base^exponent" is 2^+1024, the result is INF anyway.
    // This threshold is roughly 1.7977 * 10^-308.
    // So if the resulting exponent is +309 or higher,
    // the result is INF anyway.
    // We only need to avoid false positives, so we can ignore base 16.
    if (exponent >= 309) {
        // Definitely can't be represented more precisely.
        // I lied, sometimes the result is +INF, and sometimes -INF.
        if (sign != Sign::Negative) {
            return MY_INFTY_POS;
        } else {
            return -1.0 / 0.0;
        }
    }

    // TODO: If `exponent` is large, this is slow.
    double value = digits.number();
    if (sign == Sign::Negative) {
        value *= -1;
    }
    if (exponent < 0) {
        exponent = -exponent;
        for (int i = 0; i < exponent; ++i) {
            // strtod is not supposed to return denormals.
            // I don't think this is reasonable; return denormals anyway.
            value /= base;
        }
    } else if (exponent > 0) {
        for (int i = 0; i < exponent; ++i) {
            value *= base;
        }
    }

    return value;
}

struct Testcase {
    const char* test_name;
    int should_consume;
    const char* hex;
    const char* test_string;
    bool skip_old = false;
};

static Testcase TESTCASES[] = {
    // What I came up with on my own:
    {"BW00", 0, "0000000000000000", ".."},
    {"BW01", 2, "3ff0000000000000", "1..0"},
    {"BW02", 1, "3ff0000000000000", "1"},
    {"BW03", 2, "0000000000000000", ".0"},
    {"BW04", 2, "0000000000000000", "0."},
    {"BW05", 2, "0000000000000000", "0.."},
    // Second 'e' overwrites first exponent
    {"BW06", 3, "40af400000000000", "4e3e4"},
    // Minus sign in exponent is ignored (`atof` only)
    {"BW07", 4, "3f747ae147ae147b", "5e-3"},
    // "e" is ignored if followed by only zeros
    {"BW08", 3, "401c000000000000", "7e0"},
    // Exponent overflow:
    {"BW09", -1, "0000000000000000", "1e-4294967296"},
    // Excessive time use(!):
    {"BW10", -1, "0000000000000000", "1e-999999999", true},
    // Excessively large exponent (>64 bits):
    {"BW10", -1, "0000000000000000", "1e-9999999999999999999999", true},

    // Bugs I nearly introduced
    {"BWN01", 2, "3ff0000000000000", "1.-2"},
    {"BWN02", 2, "3ff0000000000000", "1. 2"},
    {"BWN03", 0, "0000000000000000", "++2"},
    {"BWN04", 1, "0000000000000000", "0b101"},
    {"BWN05", 3, "0000000000000000", "  0b101"},
    {"BWN06", 1, "0000000000000000", "0o123"},
    {"BWN07", 1, "0000000000000000", "0y2"},
    {"BWN08", 1, "3ff0000000000000", "1e"},
    {"BWN09", 1, "3ff0000000000000", "1e-"},
    {"BWN10", 6, "4034000000000000", "2e0001"},
    {"BWN11", 6, "41ddcd6500000000", "2e0009"},
    {"BWN12", 1, "0000000000000000", "0d1"},
    {"BWN13", -1, "7ff0000000000000", "184467440737095516151234567890e2147483639", true},
    {"BWN14", -1, "0000000000000000", ".1234567890e-2147483639", true},
    {"BWN15", -1, "8000000000000000", "-1e-9999"},

    // From the Serenity GitHub tracker:
    // https://github.com/SerenityOS/serenity/issues/1979
    {"SR1", -1, "4014000000000001", "5.000000000000001"},
    {"SR2", -1, "4014000000000000", "5.0000000000000001"},
    {"SR3", -1, "3ff0000000000000", "1.0000000000000000000000000000001"},
    {"SR4", -1, "3ff0000000000000", "1.00000000000000000000000000000001"},
    {"SR5", -1, "3ff1f9add37c1216", "1.12345678912345678912345678912345"},
    {"SR6", -1, "3ff1f9add37c1216", "1.123456789123456789123456789123456789123456789"},

    // Inspired from Abraham Hrvoje's "negativeFormattingTests":
    // https://github.com/ahrvoje/numerics/blob/master/strtod/strtod_tests.toml
    // Note that my interpretation is slightly stricter than what Abraham Hrvoje wrote.
    {"AHN01", 3, "7ff0000000000000", "inf1"},
    {"AHN02", 3, "7ff0000000000000", "inf+"},
    {"AHN03", 0, "0000000000000000", ".E"},
    {"AHN04", 3, "3ff0000000000000", "1.0e"},
    {"AHN05", 4, "400399999999999a", "2.45+e+3"},
    {"AHN06", 2, "4037000000000000", "23e.23"},
    {"AHN07", 0, "0000000000000000", "e9"},
    {"AHN08", 0, "0000000000000000", "+e"},
    {"AHN09", 0, "0000000000000000", "e+"},
    {"AHN10", 0, "0000000000000000", "."},
    {"AHN11", 0, "0000000000000000", "e"},
    {"AHN12", 2, "3fe6666666666666", ".7+"},
    {"AHN13", 3, "3fcae147ae147ae1", ".21e"},
    {"AHN14", 0, "0000000000000000", "+"},
    {"AHN15", 0, "0000000000000000", ""},
    {"AHN16", 3, "7ff0000000000000", "infe"},
    {"AHN17", 3, "7ff8000000000000", "nan(err"},
    {"AHN18", 3, "7ff8000000000000", "nan)"},
    {"AHN19", 3, "7ff8000000000000", "NAN(test_)_)"},
    {"AHN20", 3, "7ff8000000000000", "nan0"},
    {"AHN21", 0, "0000000000000000", "-.e+"},
    {"AHN22", 0, "0000000000000000", "-+12.34"},

    // For a better description, see:
    // https://github.com/ahrvoje/numerics/blob/master/strtod/strtod_tests.toml
    // (Comments removed for ease of format conversion.)
    // My machine generates the NaN "fff8000000000000" for 0/0, so I replaced the "correct" result by that.
    {"F0", -1, "348834c13cbf331d", "12.34E-56"},
    {"F1", -1, "c07c800000000000", "-456."},
    {"F2", -1, "405ec00000000000", "+123"},
    {"F3", -1, "7ff8000000000000", "nan"},
    {"F4", -1, "7ff8000000000000", "NaN"},
    {"F5", -1, "7ff8000000000000", "NAN"},
    {"F6", -1, "7ff0000000000000", "inf"},
    {"F7", -1, "7ff0000000000000", "Inf"},
    {"F8", -1, "7ff0000000000000", "INF"},
    {"F9", -1, "fff0000000000000", "-inf"},
    {"F10", -1, "7ff0000000000000", "+inF"},
    {"F11", -1, "7ff0000000000000", "+INF"},
    {"F12", -1, "3ff0000000000000", "1.0"},
    {"F13", -1, "4059000000000000", "1e2"},
    {"F14", -1, "44ada56a4b0835c0", "7.e22"},
    {"F15", -1, "44ada56a4b0835c0", "7.0e22"},
    {"F16", -1, "44ada56a4b0835c0", "7.0e+22"},
    {"F17", -1, "3b8a71fc0e147309", "7.0e-22"},
    {"F18", -1, "c02699999999999a", "-1.13e1"},
    {"F19", -1, "402699999999999a", "+1.13e+1"},
    {"F20", -1, "36e069d1347fd4b5", "23e-45"},
    {"F21", -1, "402a000000000000", ".13e2"},
    {"F22", -1, "beb5cf751db94e6b", "-.13e-5"},
    {"F23", -1, "405ec00000000000", "123"},
    {"F24", -1, "7ff8000000000000", "+nan"},
    {"F25", -1, "7ff0000000000000", "infinity"},
    {"F26", -1, "7ff0000000000000", "Infinity"},
    {"F27", 3, "7ff8000000000000", "nan(type-0)"},
    {"F28", 4, "7ff8000000000000", "+nan(catch_22)"},
    {"F29", -1, "7ff0000000000000", "INFINITY"},
    {"F30", -1, "3ff0000000000000", "0.00000001e+8"},
    {"F31", -1, "fff0000000000000", "-infinity"},
    {"F32", -1, "3705f1a59c73408e", "123.e-45"},
    {"F33", -1, "4085300000000000", "678."},
    {"F34", 4, "fff8000000000000", "-nan()"},
    {"C0", -1, "0000000000000000", "0.000e+00"},
    {"C1", -1, "0000000000000000", "1e-400"},
    {"C2", -1, "0000000000000000", "2.4703282292062326e-324"},
    {"C3", -1, "0000000000000000", "2.4703282292062327e-324"},
    {"C4", -1, "0000000000000001", "2.4703282292062328e-324"},
    {"C5", -1, "0000000000000001", "4.9406564584124654e-324"},
    {"C6", -1, "00000000000007e8", "1e-320"},
    {"C7", -1, "000fffffffffffff", "2.2250738585072009e-308"},
    {"C8", -1, "0010000000000000", "2.2250738585072014e-308"},
    {"C9", -1, "3abef2d0f5da7dd9", "1e-25"},
    {"C10", -1, "3b282db34012b251", "1.0e-23"},
    {"C11", -1, "3ff3c0ca428c59fb", "1.2345678901234567890"},
    {"C12", -1, "402699999999999a", "1.13e1"},
    {"C13", -1, "43e158e460913d00", "1e+19"},
    {"C14", -1, "449017f7df96be18", "1.9e+22"},
    {"C15", -1, "4496deb1154f79ec", "2.7e22"},
    {"C16", -1, "449a420db02bd7d6", "3.1e22"},
    {"C17", -1, "44ada56a4b0835c0", "7e22"},
    {"C18", -1, "7fefffffffffffff", "1.7976931348623158e+308"},
    {"C19", -1, "7ff0000000000000", "1.7976931348623159e+308"},
    {"C20", -1, "7ff0000000000000", "1e+400"},
    {"C21", -1, "000fffffffffffff", "2.225073858507201136057409796709131975934819546351645648023426109724822222021076945516529523908135087914149158913039621106870086438694594645527657207407820621743379988141063267329253552286881372149012981122451451889849057222307285255133155755015914397476397983411801999323962548289017107081850690630666655994938275772572015763062690663332647565300009245888316433037779791869612049497390377829704905051080609940730262937128958950003583799967207254304360284078895771796150945516748243471030702609144621572289880258182545180325707018860872113128079512233426288368622321503775666622503982534335974568884423900265498198385487948292206894721689831099698365846814022854243330660339850886445804001034933970427567186443383770486037861622771738545623065874679014086723327636718749999999999999999999999999999999999999e-308"},
    {"C22", -1, "0010000000000000", "2.22507385850720113605740979670913197593481954635164564802342610972482222202107694551652952390813508791414915891303962110687008643869459464552765720740782062174337998814106326732925355228688137214901298112245145188984905722230728525513315575501591439747639798341180199932396254828901710708185069063066665599493827577257201576306269066333264756530000924588831643303777979186961204949739037782970490505108060994073026293712895895000358379996720725430436028407889577179615094551674824347103070260914462157228988025818254518032570701886087211312807951223342628836862232150377566662250398253433597456888442390026549819838548794829220689472168983109969836584681402285424333066033985088644580400103493397042756718644338377048603786162277173854562306587467901408672332763671875e-308"},
    {"C23", -1, "0010000000000000", "0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000222507385850720138309023271733240406421921598046233183055332741688720443481391819585428315901251102056406733973103581100515243416155346010885601238537771882113077799353200233047961014744258363607192156504694250373420837525080665061665815894872049117996859163964850063590877011830487479978088775374994945158045160505091539985658247081864511353793580499211598108576605199243335211435239014879569960959128889160299264151106346631339366347758651302937176204732563178148566435087212282863764204484681140761391147706280168985324411002416144742161856716615054015428508471675290190316132277889672970737312333408698898317506783884692609277397797285865965494109136909540613646756870239867831529068098461721092462539672851562500000000000000001"},
    {"C24", -1, "7ff0000000000000", "179769313486231580793728971405303415079934132710037826936173778980444968292764750946649017977587207096330286416692887910946555547851940402630657488671505820681908902000708383676273854845817711531764475730270069855571366959622842914819860834936475292719074168444365510704342711559699508093042880177904174497792"},
    {"C25", -1, "7fefffffffffffff", "179769313486231580793728971405303415079934132710037826936173778980444968292764750946649017977587207096330286416692887910946555547851940402630657488671505820681908902000708383676273854845817711531764475730270069855571366959622842914819860834936475292719074168444365510704342711559699508093042880177904174497791.9999999999999999999999999999999999999999999999999999999999999999999999"},
    {"C26", -1, "0010000000000000", "2.2250738585072012e-308"},
    {"C27", -1, "0006123400000001", "8.44291197326099e-309"},
    {"C28", -1, "42c0000000000000", "35184372088831.999999999999999999999999999999999999"},
    {"C29", -1, "0000000000000000", "2.47032822920623272e-324"},
    {"C30", -1, "3ff199999999999a", "1.100000000000000088817841970012523233890533447265626"},
    {"C31", -1, "3f847ae147ae147b", ".010000000000000000057612911342378542997169"},
    {"C32", -1, "3ffd34fd8378ea83", "1.8254370818746402660437411213933955878019332885742187"},
    {"C33", -1, "43389e56ee5e7a58", "6929495644600919.5"},
    {"C34", -1, "432a9d28ff412a75", "3.7455744005952583e15"},
    {"C35", -1, "000fffffffffffff", "2.2250738585072011e-308"},
    {"C36", -1, "4c20000000000001", "5.0216813883093451685872615018317116712748411717802652598273e58"},
    {"C37", -1, "0000000008000000", "6.631236871469758276785396630275967243399099947355303144249971758736286630139265439618068200788048744105960420552601852889715006376325666595539603330361800519107591783233358492337208057849499360899425128640718856616503093444922854759159988160304439909868291973931426625698663157749836252274523485312442358651207051292453083278116143932569727918709786004497872322193856150225415211997283078496319412124640111777216148110752815101775295719811974338451936095907419622417538473679495148632480391435931767981122396703443803335529756003353209830071832230689201383015598792184172909927924176339315507402234836120730914783168400715462440053817592702766213559042115986763819482654128770595766806872783349146967171293949598850675682115696218943412532098591327667236328125E-316"},
    {"C38", -1, "0000000000010000", "3.237883913302901289588352412501532174863037669423108059901297049552301970670676565786835742587799557860615776559838283435514391084153169252689190564396459577394618038928365305143463955100356696665629202017331344031730044369360205258345803431471660032699580731300954848363975548690010751530018881758184174569652173110473696022749934638425380623369774736560008997404060967498028389191878963968575439222206416981462690113342524002724385941651051293552601421155333430225237291523843322331326138431477823591142408800030775170625915670728657003151953664260769822494937951845801530895238439819708403389937873241463484205608000027270531106827387907791444918534771598750162812548862768493201518991668028251730299953143924168545708663913273994694463908672332763671875E-319"},
    {"C39", -1, "0000800000000100", "6.953355807847677105972805215521891690222119817145950754416205607980030131549636688806115726399441880065386399864028691275539539414652831584795668560082999889551357784961446896042113198284213107935110217162654939802416034676213829409720583759540476786936413816541621287843248433202369209916612249676005573022703244799714622116542188837770376022371172079559125853382801396219552418839469770514904192657627060319372847562301074140442660237844114174497210955449896389180395827191602886654488182452409583981389442783377001505462015745017848754574668342161759496661766020028752888783387074850773192997102997936619876226688096314989645766000479009083731736585750335262099860150896718774401964796827166283225641992040747894382698751809812609536720628966577351093292236328125E-310"},
    {"C40", -1, "0000000000010800", "3.339068557571188581835713701280943911923401916998521771655656997328440314559615318168849149074662609099998113009465566426808170378434065722991659642619467706034884424989741080790766778456332168200464651593995817371782125010668346652995912233993254584461125868481633343674905074271064409763090708017856584019776878812425312008812326260363035474811532236853359905334625575404216060622858633280744301892470300555678734689978476870369853549413277156622170245846166991655321535529623870646888786637528995592800436177901746286272273374471701452991433047257863864601424252024791567368195056077320885329384322332391564645264143400798619665040608077549162173963649264049738362290606875883456826586710961041737908872035803481241600376705491726170293986797332763671875E-319"},
    {"C41", -1, "4e3fa69165a8eea2", "8.533e+68"},
    {"C42", -1, "19dbe0d1c7ea60c9", "4.1006e-184"},
    {"C43", -1, "7fe1cc0a350ca87b", "9.998e+307"},
    {"C44", -1, "0602117ae45cde43", "9.9538452227e-280"},
    {"C45", -1, "0a1fdd9e333badad", "6.47660115e-260"},
    {"C46", -1, "49e033d7eca0adef", "7.4e+47"},
    {"C47", -1, "4a1033d7eca0adef", "5.92e+48"},
    {"C48", -1, "4dd172b70eababa9", "7.35e+66"},
    {"C49", -1, "4b8b2628393e02cd", "8.32116e+55"},
    {"C50", -1, "bfed35696e58a32f", "-0.91276999999999997026378650843980722129344940185546876"},
    {"C51", -1, "c070a3d70a3d70a4", "-266.240000000000009094947017729282379150390624"},
    {"C52", -1, "3c97cb9433617c9c", "8.255628858767918002472043289952338102302250764062685473021474535926245152950286865234374e-17"},
    {"C53", -1, "43405e6cec57761a", "9214843084008499"},
    {"C54", -1, "3fe0000000000002", "0.500000000000000166533453693773481063544750213623046875"},
    {"C55", -1, "42c0000000000002", "3.518437208883201171875e13"},
    {"C56", -1, "404f44abd5aa7ca4", "62.5364939768271845828"},
    {"C57", -1, "3e0bd5cbaef0fd0c", "8.10109172351e-10"},
    {"C58", -1, "3ff8000000000000", "1.50000000000000011102230246251565404236316680908203125"},
    {"C59", -1, "433fffffffffffff", "9007199254740991.4999999999999999999999999999999995"},
    {"C60", -1, "44997a3c7271b021", "30078505129381147446200"},
    {"C61", -1, "4458180d5bad2e3e", "1777820000000000000001"},
    {"C62", -1, "3fe0000000000002", "0.50000000000000016656055874808561867439493653364479541778564453125"},
    {"C63", -1, "3fd92bb352c4623a", "0.3932922657273"},
    {"C64", -1, "0000000000000000", "2.4703282292062327208828439643411068618252990130716238221279284125033775363510437593264991818081799618989828234772285886546332835517796989819938739800539093906315035659515570226392290858392449105184435931802849936536152500319370457678249219365623669863658480757001585769269903706311928279558551332927834338409351978015531246597263579574622766465272827220056374006485499977096599470454020828166226237857393450736339007967761930577506740176324673600968951340535537458516661134223766678604162159680461914467291840300530057530849048765391711386591646239524912623653881879636239373280423891018672348497668235089863388587925628302755995657524455507255189313690836254779186948667994968324049705821028513185451396213837722826145437693412532098591327667236328124999e-324"},
    {"C65", -1, "0000000000000000", "2.4703282292062327208828439643411068618252990130716238221279284125033775363510437593264991818081799618989828234772285886546332835517796989819938739800539093906315035659515570226392290858392449105184435931802849936536152500319370457678249219365623669863658480757001585769269903706311928279558551332927834338409351978015531246597263579574622766465272827220056374006485499977096599470454020828166226237857393450736339007967761930577506740176324673600968951340535537458516661134223766678604162159680461914467291840300530057530849048765391711386591646239524912623653881879636239373280423891018672348497668235089863388587925628302755995657524455507255189313690836254779186948667994968324049705821028513185451396213837722826145437693412532098591327667236328125e-324"},
    {"C66", -1, "0000000000000001", "2.4703282292062327208828439643411068618252990130716238221279284125033775363510437593264991818081799618989828234772285886546332835517796989819938739800539093906315035659515570226392290858392449105184435931802849936536152500319370457678249219365623669863658480757001585769269903706311928279558551332927834338409351978015531246597263579574622766465272827220056374006485499977096599470454020828166226237857393450736339007967761930577506740176324673600968951340535537458516661134223766678604162159680461914467291840300530057530849048765391711386591646239524912623653881879636239373280423891018672348497668235089863388587925628302755995657524455507255189313690836254779186948667994968324049705821028513185451396213837722826145437693412532098591327667236328125001e-324"},
    {"C67", -1, "0000000000000001", "7.4109846876186981626485318930233205854758970392148714663837852375101326090531312779794975454245398856969484704316857659638998506553390969459816219401617281718945106978546710679176872575177347315553307795408549809608457500958111373034747658096871009590975442271004757307809711118935784838675653998783503015228055934046593739791790738723868299395818481660169122019456499931289798411362062484498678713572180352209017023903285791732520220528974020802906854021606612375549983402671300035812486479041385743401875520901590172592547146296175134159774938718574737870961645638908718119841271673056017045493004705269590165763776884908267986972573366521765567941072508764337560846003984904972149117463085539556354188641513168478436313080237596295773983001708984374999e-324"},
    {"C68", -1, "0000000000000002", "7.4109846876186981626485318930233205854758970392148714663837852375101326090531312779794975454245398856969484704316857659638998506553390969459816219401617281718945106978546710679176872575177347315553307795408549809608457500958111373034747658096871009590975442271004757307809711118935784838675653998783503015228055934046593739791790738723868299395818481660169122019456499931289798411362062484498678713572180352209017023903285791732520220528974020802906854021606612375549983402671300035812486479041385743401875520901590172592547146296175134159774938718574737870961645638908718119841271673056017045493004705269590165763776884908267986972573366521765567941072508764337560846003984904972149117463085539556354188641513168478436313080237596295773983001708984375e-324"},
    {"C69", -1, "0000000000000002", "7.4109846876186981626485318930233205854758970392148714663837852375101326090531312779794975454245398856969484704316857659638998506553390969459816219401617281718945106978546710679176872575177347315553307795408549809608457500958111373034747658096871009590975442271004757307809711118935784838675653998783503015228055934046593739791790738723868299395818481660169122019456499931289798411362062484498678713572180352209017023903285791732520220528974020802906854021606612375549983402671300035812486479041385743401875520901590172592547146296175134159774938718574737870961645638908718119841271673056017045493004705269590165763776884908267986972573366521765567941072508764337560846003984904972149117463085539556354188641513168478436313080237596295773983001708984375001e-324"},
    {"C70", -1, "7fe0000000000000", "8.9884656743115805365666807213050294962762414131308158973971342756154045415486693752413698006024096935349884403114202125541629105369684531108613657287705365884742938136589844238179474556051429647415148697857438797685859063890851407391008830874765563025951597582513936655578157348020066364210154316532161708031999e+307"},
    {"C71", -1, "7fe0000000000000", "8.9884656743115805365666807213050294962762414131308158973971342756154045415486693752413698006024096935349884403114202125541629105369684531108613657287705365884742938136589844238179474556051429647415148697857438797685859063890851407391008830874765563025951597582513936655578157348020066364210154316532161708032e+307"},
    {"C72", -1, "7fe0000000000001", "8.9884656743115805365666807213050294962762414131308158973971342756154045415486693752413698006024096935349884403114202125541629105369684531108613657287705365884742938136589844238179474556051429648741514697857438797685859063890851407391008830874765563025951597582513936655578157348020066364210154316532161708032001e+307"},
    {"C73", -1, "3ff0000010000000", "1.00000005960464477550"},
    {"C74", -1, "36c6000000000000", "7.7071415537864938e-45"},
    {"C75", -1, "0000000000000000", "2183167012312112312312.23538020374420446192e-370"},
    {"C76", -1, "0006c9a143590c14", "94393431193180696942841837085033647913224148539854e-358"},
    {"C77", -1, "3ff0000000000000", "99999999999999994487665465554760717039532578546e-47"},
    {"C78", -1, "44b52d02c7e14af6", "10000000000000000000000000000000000000000e-17"},
    {"C79", -1, "0007802665fd9600", "104308485241983990666713401708072175773165034278685682646111762292409330928739751702404658197872319129036519947435319418387839758990478549477777586673075945844895981012024387992135617064532141489278815239849108105951619997829153633535314849999674266169258928940692239684771590065027025835804863585454872499320500023126142553932654370362024104462255244034053203998964360882487378334860197725139151265590832887433736189468858614521708567646743455601905935595381852723723645799866672558576993978025033590728687206296379801363024094048327273913079612469982585674824156000783167963081616214710691759864332339239688734656548790656486646106983450809073750535624894296242072010195710276073042036425579852459556183541199012652571123898996574563824424330960027873516082763671875e-1075"},
    {"C80", -1, "4025cccccccccccd", "10.900000000000000012345678912345678912345"},
    // I'm impressed that my stdlib actually generates the right double values!
    // … although it has some funny ideas about which suffixes it accepts.

    // For the future: hexadecimal floats.
    // Note that "0x579a" is "0xabcd << 1" with the top bit cut off, just as expected.
    {"Fp1", -1, "7c2579a000000000", "0xab.cdpef"},
    // Sneaky floating point :P
    {"Fp2", -1, "43e9400000000000", "0xCAPE"},
};

constexpr size_t NUM_TESTCASES = sizeof(TESTCASES) / sizeof(TESTCASES[0]);

typedef double (*strtod_fn_t)(const char* str, char** endptr);

bool evaluate_strtod(strtod_fn_t strtod_fn, const char* test_string, const char* expect_hex, int expect_consume) {
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

    printf(" %s%s%s(%s%2u%s)",
           wrong_hex ? TEXT_WRONG : "",
           actual_hex,
           wrong_hex ? TEXT_RESET : "",
           error_cns ? TEXT_ERROR : wrong_cns ? TEXT_WRONG : "",
           actual_consume,
           (error_cns || wrong_cns) ? TEXT_RESET : "");

    return wrong_hex || error_cns || wrong_cns;
}

int main()
{
    if (sizeof(size_t) != 4) {
        printf("lolwut?!\n");
        return 1;
    }
    printf("Running %u testcases...\n", NUM_TESTCASES);
    printf("%3s(%-5s): %16s(%2s) %16s(%2s) %16s(%2s) %16s(%2s) – %s\n", "num", "name", "correct", "cs", "builtin", "cs", "old_strtod", "cs", "new_strtod", "cs", "teststring");

    int stay_good = 0;
    int stay_bad = 0;
    int regressions = 0;
    int fixes = 0;
    for (size_t i = 0; i < NUM_TESTCASES; i++)
    {
        Testcase& tc = TESTCASES[i];
        if (tc.should_consume == -1) {
            tc.should_consume = strlen(tc.test_string);
        }
        printf("%3u(%-5s):", i, tc.test_name);
        printf(" %s(%2d)", tc.hex, tc.should_consume);
        evaluate_strtod(strtod, tc.test_string, tc.hex, tc.should_consume);
        bool old_bad = true;
        if (tc.skip_old) {
            printf(" %s_____(skip)_____%s(__)", TEXT_WRONG, TEXT_RESET);
        } else {
            old_bad = evaluate_strtod(old_strtod, tc.test_string, tc.hex, tc.should_consume);
        }
        bool new_bad = evaluate_strtod(new_strtod, tc.test_string, tc.hex, tc.should_consume);
        printf(" – %s\n", tc.test_string);
        switch ((old_bad ? 2 : 0) | (new_bad ? 1 : 0))
        {
            case 0b00: stay_good += 1; break;
            case 0b01: regressions += 1; break;
            case 0b10: fixes += 1; break;
            case 0b11: stay_bad += 1; break;
            default: assert(false);
        }
    }
    printf("Out of %d tests, the new strtod regresses %d and fixes %d.\n", NUM_TESTCASES, regressions, fixes);
    printf("(%d stayed good and %d stayed bad.)\n", stay_good, stay_bad);
    return 0;
}
