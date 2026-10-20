#ifndef GRADE_H
#define GRADE_H

#include "auth.h"

#define MARKS_MIN 0
#define MARKS_MAX 100

typedef enum {
    GRADE_OK,
    GRADE_NOT_ADMIN,      /* STU-SR002: only admins may write grades */
    GRADE_INVALID_INPUT,  /* bad marks, roll number or subject */
    GRADE_DUPLICATE,      /* record_grade: roll_no + subject already exists */
    GRADE_NOT_FOUND,      /* update_grade: no such roll_no + subject */
    GRADE_FULL,
    GRADE_IO_ERROR
} GradeResult;

/* STU-F020: returns 1 if marks is within 0..100, else 0. */
int  validate_marks(int marks);

/* STU-F021: marks -> letter grade (A..F), per SAD 4.6 proposed scheme.
 * Returns '\0' if marks are invalid. */
char calculate_grade(int marks);

/* Load grades from file (missing file = empty list). Returns 0 on success. */
int  grade_init(const char *file);

/* STU-F020: add marks for a student/subject. Admin only. */
GradeResult record_grade(const Session *s, const char *roll_no,
                         const char *subject, int marks);

/* STU-F022: change existing marks. Admin only. */
GradeResult update_grade(const Session *s, const char *roll_no,
                         const char *subject, int marks);

/* Look up one grade. Either out pointer may be NULL. */
GradeResult get_grade(const char *roll_no, const char *subject,
                      int *marks, char *letter);

int  grade_count(void);

#endif
