#include "grade.h"

int validate_marks(int marks)
{
    return marks >= MARKS_MIN && marks <= MARKS_MAX;
}

char calculate_grade(int marks)
{
    if (!validate_marks(marks)) return '\0';
    if (marks >= 90) return 'A';
    if (marks >= 80) return 'B';
    if (marks >= 70) return 'C';
    if (marks >= 60) return 'D';
    if (marks >= 50) return 'E';
    return 'F';
}
