#include <stdio.h>
#include <string.h>

#include "report.h"
#include "attendance.h"
#include "audit.h"
#include "grade.h"
#include "validate.h"

#define SCAN_MAX 1000

static StudentRecord scan_buf[SCAN_MAX];

const char *report_result_message(ReportResult r)
{
    switch (r) {
    case REPORT_OK:             return "Done.";
    case REPORT_ERR_PERMISSION: return "Access denied.";
    case REPORT_ERR_INVALID:    return "Invalid input.";
    case REPORT_ERR_NOT_FOUND:  return "No such student.";
    case REPORT_ERR_IO:         return "Could not read or write the data file.";
    }
    return "Unknown error.";
}

static char lower(char c)
{
    return (c >= 'A' && c <= 'Z') ? (char)(c + ('a' - 'A')) : c;
}

/* Case-insensitive "needle occurs in hay" (portable, no strcasestr). */
static int contains_ci(const char *hay, const char *needle)
{
    size_t i, j;

    for (i = 0; hay[i] != '\0'; i++) {
        for (j = 0; needle[j] != '\0' && hay[i + j] != '\0'; j++) {
            if (lower(hay[i + j]) != lower(needle[j]))
                break;
        }
        if (needle[j] == '\0')
            return 1;
    }
    return 0;
}

/* Common checks: roll number valid, caller allowed, student exists. */
static ReportResult check_student(const Session *session, const char *roll_no,
                                  StudentRecord *rec)
{
    if (session == NULL)
        return REPORT_ERR_PERMISSION;
    if (!validate_roll_no(roll_no))
        return REPORT_ERR_INVALID;
    if (!auth_can_view_record(session, roll_no))
        return REPORT_ERR_PERMISSION;
    if (!get_student(roll_no, rec))
        return REPORT_ERR_NOT_FOUND;
    return REPORT_OK;
}

static ReportResult from_att(AttResult r)
{
    switch (r) {
    case ATT_OK:             return REPORT_OK;
    case ATT_ERR_PERMISSION: return REPORT_ERR_PERMISSION;
    case ATT_ERR_INVALID:    return REPORT_ERR_INVALID;
    default:                 return REPORT_ERR_IO;
    }
}

/* ---------- STU-F040 ---------- */

ReportResult search_students(const Session *session, const char *query,
                             StudentRecord *out, int max, int *count)
{
    int total, i, n = 0;

    if (session == NULL)
        return REPORT_ERR_PERMISSION;
    if (out == NULL || count == NULL || max <= 0 ||
        !validate_text(query, REPORT_QUERY_MAX))
        return REPORT_ERR_INVALID;

    *count = 0;
    total = list_students(scan_buf, SCAN_MAX);

    for (i = 0; i < total && n < max; i++) {
        if (!auth_can_view_record(session, scan_buf[i].roll_no))
            continue;
        if (contains_ci(scan_buf[i].roll_no, query) ||
            contains_ci(scan_buf[i].name, query))
            out[n++] = scan_buf[i];
    }

    *count = n;
    return REPORT_OK;
}

void report_print_students(FILE *out, const StudentRecord *list, int count)
{
    int i;

    if (out == NULL)
        return;
    if (list == NULL || count <= 0) {
        fprintf(out, "No matching students.\n");
        return;
    }

    fprintf(out, "\n%-12s %-24s %-12s %s\n", "Roll no", "Name", "Department", "Phone");
    fprintf(out, "--------------------------------------------------------------\n");
    for (i = 0; i < count; i++)
        fprintf(out, "%-12s %-24s %-12s %s\n", list[i].roll_no, list[i].name,
                list[i].dept, list[i].phone);
    fprintf(out, "%d match(es).\n", count);
}

/* ---------- STU-F041 ---------- */

ReportResult grade_sheet(const Session *session, const char *roll_no, FILE *out)
{
    StudentRecord rec;
    GradeEntry g[REPORT_MAX_SUBJECTS];
    ReportResult r;
    int n, i, sum = 0;

    if (out == NULL)
        return REPORT_ERR_INVALID;
    r = check_student(session, roll_no, &rec);
    if (r != REPORT_OK)
        return r;

    n = grade_list_for(roll_no, g, REPORT_MAX_SUBJECTS);

    fprintf(out, "\nGrade sheet: %s (%s), %s\n", rec.name, rec.roll_no, rec.dept);
    if (n == 0) {
        fprintf(out, "No grades recorded.\n");
        return REPORT_OK;
    }

    fprintf(out, "%-20s %-6s %s\n", "Subject", "Marks", "Grade");
    fprintf(out, "------------------------------------\n");
    for (i = 0; i < n; i++) {
        fprintf(out, "%-20s %-6d %c\n", g[i].subject, g[i].marks, g[i].letter);
        sum += g[i].marks;
    }
    fprintf(out, "------------------------------------\n");
    fprintf(out, "Average: %.1f (%c)\n", (double)sum / n,
            calculate_grade((int)((double)sum / n + 0.5)));
    return REPORT_OK;
}

/* ---------- STU-F042 ---------- */

ReportResult attendance_summary(const Session *session, const char *roll_no, FILE *out)
{
    StudentRecord rec;
    AttSubjectSummary a[REPORT_MAX_SUBJECTS];
    ReportResult r;
    int n = 0, i, present = 0, total = 0;

    if (out == NULL)
        return REPORT_ERR_INVALID;
    r = check_student(session, roll_no, &rec);
    if (r != REPORT_OK)
        return r;

    r = from_att(attendance_subject_summary(session, roll_no, a, REPORT_MAX_SUBJECTS, &n));
    if (r != REPORT_OK)
        return r;

    fprintf(out, "\nAttendance summary: %s (%s)\n", rec.name, rec.roll_no);
    if (n == 0) {
        fprintf(out, "No attendance records found.\n");
        return REPORT_OK;
    }

    fprintf(out, "%-20s %-9s %-8s %s\n", "Subject", "Present", "Percent", "");
    fprintf(out, "--------------------------------------------\n");
    for (i = 0; i < n; i++) {
        fprintf(out, "%-20s %3d/%-5d %6.1f%%  %s\n", a[i].subject, a[i].present,
                a[i].total, a[i].percent,
                a[i].percent < REPORT_ATT_THRESHOLD ? "LOW" : "");
        present += a[i].present;
        total   += a[i].total;
    }
    fprintf(out, "--------------------------------------------\n");
    fprintf(out, "%-20s %3d/%-5d %6.1f%%\n", "Overall", present, total,
            total == 0 ? 0.0 : (present * 100.0) / total);
    return REPORT_OK;
}

/* Writes one CSV cell: neutralises spreadsheet formulas and quotes cells that
   contain a comma, quote or line break (RFC 4180, quotes doubled). */
static void csv_cell(FILE *f, const char *s)
{
    int quote = strpbrk(s, ",\"\r\n") != NULL;

    if (quote)
        fputc('"', f);
    if (s[0] == '=' || s[0] == '+' || s[0] == '-' || s[0] == '@')
        fputc('\'', f);
    for (; *s; s++) {
        if (*s == '"')
            fputc('"', f);
        fputc(*s, f);
    }
    if (quote)
        fputc('"', f);
}

ReportResult export_report_csv(const Session *session, const char *roll_no, const char *path)
{
    StudentRecord rec;
    GradeEntry g[REPORT_MAX_SUBJECTS];
    AttSubjectSummary a[REPORT_MAX_SUBJECTS];
    char subjects[2 * REPORT_MAX_SUBJECTS][32];
    ReportResult r;
    FILE *f;
    int ng, na = 0, ns = 0, i, j;
    char action[160];

    if (path == NULL || path[0] == '\0' || strpbrk(path, "\r\n") != NULL)
        return REPORT_ERR_INVALID;
    r = check_student(session, roll_no, &rec);
    if (r != REPORT_OK)
        return r;

    ng = grade_list_for(roll_no, g, REPORT_MAX_SUBJECTS);
    r = from_att(attendance_subject_summary(session, roll_no, a, REPORT_MAX_SUBJECTS, &na));
    if (r != REPORT_OK)
        return r;

    /* union of subjects: grades first, then attendance-only subjects */
    for (i = 0; i < ng; i++)
        snprintf(subjects[ns++], sizeof subjects[0], "%s", g[i].subject);
    for (i = 0; i < na; i++) {
        int seen = 0;
        for (j = 0; j < ns; j++)
            if (strcmp(subjects[j], a[i].subject) == 0) { seen = 1; break; }
        if (!seen)
            snprintf(subjects[ns++], sizeof subjects[0], "%s", a[i].subject);
    }

    f = fopen(path, "w");
    if (f == NULL)
        return REPORT_ERR_IO;

    fputs("roll_no,name,subject,marks,grade,present,total,attendance_percent\n", f);
    for (i = 0; i < ns; i++) {
        const GradeEntry *ge = NULL;
        const AttSubjectSummary *as = NULL;

        for (j = 0; j < ng; j++)
            if (strcmp(g[j].subject, subjects[i]) == 0) { ge = &g[j]; break; }
        for (j = 0; j < na; j++)
            if (strcmp(a[j].subject, subjects[i]) == 0) { as = &a[j]; break; }

        csv_cell(f, rec.roll_no);  fputc(',', f);
        csv_cell(f, rec.name);     fputc(',', f);
        csv_cell(f, subjects[i]);  fputc(',', f);
        if (ge) fprintf(f, "%d,%c,", ge->marks, ge->letter); else fputs(",,", f);
        if (as) fprintf(f, "%d,%d,%.1f\n", as->present, as->total, as->percent);
        else    fputs(",,\n", f);
    }

    if (ferror(f) || fclose(f) != 0) {
        remove(path);
        return REPORT_ERR_IO;
    }

    snprintf(action, sizeof action, "EXPORT_CSV roll=%s file=%s", roll_no, path);
    audit_log_module("REPORT", session->username, action);
    return REPORT_OK;
}
