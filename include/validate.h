#ifndef VALIDATE_H
#define VALIDATE_H

#include <stddef.h>

/*
 * STU-SR004 / AC-SEC-004: shared input validation.
 * Every function returns 1 if the input is acceptable and 0 if it must be
 * rejected. Rejected input must never be processed or written to a file.
 */

#define VALIDATE_ROLL_MAX_LEN 15   /* matches roll_no[16] in User/StudentRecord */

/* 1-15 chars: letters, digits, '-' or '_' (at least one letter or digit). */
int validate_roll_no(const char *s);

/* Strict YYYY-MM-DD, real calendar date (leap years handled), year 1900-2100. */
int validate_date(const char *s);

/* Marks: digits only, 0..100. If out is not NULL it receives the value. */
int validate_marks(const char *s, int *out);

/* Free text: 1..max_len chars, not blank, no comma, CR/LF or other control
   characters (they would corrupt the CSV/pipe-separated files). */
int validate_text(const char *s, size_t max_len);

/* Phone: 7-15 digits, optional leading '+'. */
int validate_phone(const char *s);

#endif
