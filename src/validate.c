#include <ctype.h>
#include <string.h>

#include "validate.h"

static int leap_year(int year)
{
    return year % 400 == 0 || (year % 4 == 0 && year % 100 != 0);
}

int validate_roll_no(const char *s)
{
    size_t i, len;
    int has_alnum = 0;

    if (s == NULL)
        return 0;

    len = strlen(s);
    if (len == 0 || len > VALIDATE_ROLL_MAX_LEN)
        return 0;

    for (i = 0; i < len; i++) {
        unsigned char ch = (unsigned char)s[i];

        if (isalnum(ch))
            has_alnum = 1;
        else if (ch != '-' && ch != '_')
            return 0;
    }

    return has_alnum;
}

int validate_date(const char *s)
{
    int i, year, month, day;
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (s == NULL || strlen(s) != 10)
        return 0;

    if (s[4] != '-' || s[7] != '-')
        return 0;

    for (i = 0; i < 10; i++) {
        if (i == 4 || i == 7)
            continue;
        if (!isdigit((unsigned char)s[i]))
            return 0;
    }

    year  = (s[0] - '0') * 1000 + (s[1] - '0') * 100 +
            (s[2] - '0') * 10 + (s[3] - '0');
    month = (s[5] - '0') * 10 + (s[6] - '0');
    day   = (s[8] - '0') * 10 + (s[9] - '0');

    if (year < 1900 || year > 2100 || month < 1 || month > 12)
        return 0;

    if (leap_year(year))
        days[1] = 29;

    return day >= 1 && day <= days[month - 1];
}

int validate_marks(const char *s, int *out)
{
    size_t i, len;
    int value = 0;

    if (s == NULL)
        return 0;

    len = strlen(s);
    if (len == 0 || len > 3)
        return 0;

    for (i = 0; i < len; i++) {
        if (!isdigit((unsigned char)s[i]))
            return 0;
        value = value * 10 + (s[i] - '0');
    }

    if (value < 0 || value > 100)
        return 0;

    if (out != NULL)
        *out = value;

    return 1;
}

int validate_text(const char *s, size_t max_len)
{
    size_t i, len;
    int has_visible = 0;

    if (s == NULL)
        return 0;

    len = strlen(s);
    if (len == 0 || len > max_len)
        return 0;

    for (i = 0; i < len; i++) {
        unsigned char ch = (unsigned char)s[i];

        if (ch == ',' || iscntrl(ch))
            return 0;
        if (!isspace(ch))
            has_visible = 1;
    }

    return has_visible;
}

int validate_phone(const char *s)
{
    size_t i, len, digits = 0;

    if (s == NULL)
        return 0;

    len = strlen(s);
    if (len == 0 || len > 15)
        return 0;

    for (i = 0; i < len; i++) {
        unsigned char ch = (unsigned char)s[i];

        if (i == 0 && ch == '+')
            continue;
        if (!isdigit(ch))
            return 0;
        digits++;
    }

    return digits >= 7;
}
