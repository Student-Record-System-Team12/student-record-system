#include <stdio.h>
#include <string.h>
#include "grade.h"

#define MAX_GRADES   1000
#define ROLL_LEN     16
#define SUBJECT_LEN  32

typedef struct {
    char roll_no[ROLL_LEN];
    char subject[SUBJECT_LEN];
    int  marks;
    char letter;
} GradeRec;

static GradeRec grades[MAX_GRADES];
static int      grade_total = 0;
static char     grade_path[260] = "";

int grade_marks_valid(int marks)
{
    return marks >= MARKS_MIN && marks <= MARKS_MAX;
}

char calculate_grade(int marks)
{
    if (!grade_marks_valid(marks)) return '\0';
    if (marks >= 90) return 'A';
    if (marks >= 80) return 'B';
    if (marks >= 70) return 'C';
    if (marks >= 60) return 'D';
    if (marks >= 50) return 'E';
    return 'F';
}

/* non-empty, fits the buffer, no CSV-breaking characters */
static int valid_text(const char *t, size_t max)
{
    size_t n;
    if (!t) return 0;
    n = strlen(t);
    if (n == 0 || n >= max) return 0;
    return strpbrk(t, ",\r\n") == NULL;
}

static int find_grade(const char *roll_no, const char *subject)
{
    int i;
    for (i = 0; i < grade_total; i++)
        if (strcmp(grades[i].roll_no, roll_no) == 0 &&
            strcmp(grades[i].subject, subject) == 0) return i;
    return -1;
}

static int save_grades(void)
{
    int i;
    FILE *f;
    if (grade_path[0] == '\0') return 0;   /* in-memory only */
    f = fopen(grade_path, "w");
    if (!f) return -1;
    for (i = 0; i < grade_total; i++)
        fprintf(f, "%s,%s,%d,%c\n", grades[i].roll_no, grades[i].subject,
                grades[i].marks, grades[i].letter);
    fclose(f);
    return 0;
}

int grade_init(const char *file)
{
    FILE *f;
    char line[128];

    grade_total = 0;
    grade_path[0] = '\0';
    if (!file) return 0;                    /* in-memory only */
    if (strlen(file) >= sizeof grade_path) return -1;
    strcpy(grade_path, file);

    f = fopen(grade_path, "r");
    if (!f) return 0;                       /* missing file = empty list */
    while (fgets(line, sizeof line, f) && grade_total < MAX_GRADES) {
        GradeRec r;
        char roll[ROLL_LEN], subj[SUBJECT_LEN], letter;
        int  marks;
        if (sscanf(line, "%15[^,],%31[^,],%d,%c", roll, subj, &marks, &letter) != 4)
            continue;                       /* skip corrupted line */
        if (!grade_marks_valid(marks)) continue;
        strcpy(r.roll_no, roll);
        strcpy(r.subject, subj);
        r.marks  = marks;
        r.letter = calculate_grade(marks);  /* recompute, never trust the file */
        grades[grade_total++] = r;
    }
    fclose(f);
    return 0;
}

GradeResult record_grade(const Session *s, const char *roll_no,
                         const char *subject, int marks)
{
    GradeRec *r;
    if (!s || !auth_can_modify(s)) return GRADE_NOT_ADMIN;
    if (!valid_text(roll_no, ROLL_LEN) || !valid_text(subject, SUBJECT_LEN) ||
        !grade_marks_valid(marks)) return GRADE_INVALID_INPUT;
    if (find_grade(roll_no, subject) >= 0) return GRADE_DUPLICATE;
    if (grade_total >= MAX_GRADES) return GRADE_FULL;

    r = &grades[grade_total];
    strcpy(r->roll_no, roll_no);
    strcpy(r->subject, subject);
    r->marks  = marks;
    r->letter = calculate_grade(marks);
    grade_total++;

    if (save_grades() != 0) { grade_total--; return GRADE_IO_ERROR; }
    /* TODO: call audit_log() here once Mem 2's audit module is merged */
    return GRADE_OK;
}

GradeResult update_grade(const Session *s, const char *roll_no,
                         const char *subject, int marks)
{
    int i, old_marks;
    char old_letter;
    if (!s || !auth_can_modify(s)) return GRADE_NOT_ADMIN;
    if (!valid_text(roll_no, ROLL_LEN) || !valid_text(subject, SUBJECT_LEN) ||
        !grade_marks_valid(marks)) return GRADE_INVALID_INPUT;
    i = find_grade(roll_no, subject);
    if (i < 0) return GRADE_NOT_FOUND;

    old_marks  = grades[i].marks;
    old_letter = grades[i].letter;
    grades[i].marks  = marks;
    grades[i].letter = calculate_grade(marks);

    if (save_grades() != 0) {
        grades[i].marks  = old_marks;
        grades[i].letter = old_letter;
        return GRADE_IO_ERROR;
    }
    /* TODO: call audit_log() here once Mem 2's audit module is merged */
    return GRADE_OK;
}

GradeResult get_grade(const char *roll_no, const char *subject,
                      int *marks, char *letter)
{
    int i;
    if (!valid_text(roll_no, ROLL_LEN) || !valid_text(subject, SUBJECT_LEN))
        return GRADE_INVALID_INPUT;
    i = find_grade(roll_no, subject);
    if (i < 0) return GRADE_NOT_FOUND;
    if (marks)  *marks  = grades[i].marks;
    if (letter) *letter = grades[i].letter;
    return GRADE_OK;
}

int grade_count(void) { return grade_total; }

int grade_list_for(const char *roll_no, GradeEntry *out, int max)
{
    int i, n = 0;
    if (!out || max <= 0 || !valid_text(roll_no, ROLL_LEN)) return 0;
    for (i = 0; i < grade_total && n < max; i++) {
        if (strcmp(grades[i].roll_no, roll_no) != 0) continue;
        strcpy(out[n].subject, grades[i].subject);
        out[n].marks  = grades[i].marks;
        out[n].letter = grades[i].letter;
        n++;
    }
    return n;
}