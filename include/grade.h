#ifndef GRADE_H
#define GRADE_H

#define MARKS_MIN 0
#define MARKS_MAX 100

/* STU-F-020: returns 1 if marks is within 0..100, else 0. */
int  validate_marks(int marks);

/* STU-F-021: marks -> letter grade (A..F), per SAD 4.6 proposed scheme.
 * Returns '\0' if marks are invalid. */
char calculate_grade(int marks);

#endif
