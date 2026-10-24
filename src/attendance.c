#include <stdio.h>
#include <string.h>

#include "attendance.h"
#include "audit.h"
#include "record_service.h"
#include "validate.h"

#define MAX_RECORDS 10000

typedef struct {
    char roll_no[ATT_ROLL_MAX];
    char subject[ATT_SUBJECT_MAX];
    char date[ATT_DATE_MAX];
    int  status;
} Row;

static Row rows[MAX_RECORDS];
static char att_path[260] = "";
static AttStudentCheck student_check = NULL;

void attendance_set_student_check(AttStudentCheck fn)
{
    student_check = fn;
}

const char *attendance_result_message(AttResult r)
{
    switch (r) {
    case ATT_OK:                  return "Done.";
    case ATT_ERR_PERMISSION:      return "Access denied.";
    case ATT_ERR_INVALID:         return "Invalid input (check roll number, subject, date, status).";
    case ATT_ERR_UNKNOWN_STUDENT: return "No such student.";
    case ATT_ERR_DUPLICATE:       return "Attendance already recorded for that student, subject and date.";
    case ATT_ERR_NOT_FOUND:       return "No existing entry to update.";
    case ATT_ERR_IO:              return "Attendance file error.";
    }
    return "Unknown error.";
}

static int student_exists(const char *roll_no)
{
    StudentRecord rec;

    if (student_check != NULL)
        return student_check(roll_no);

    return get_student(roll_no, &rec) ? 1 : 0;
}

static int valid_status(int status)
{
    return status == ATT_PRESENT || status == ATT_ABSENT;
}

static int valid_subject(const char *subject)
{
    return validate_text(subject, ATT_SUBJECT_MAX - 1);
}

/* Reads the whole file into rows[]. Any malformed line is an error. */
static AttResult load_all(int *count)
{
    FILE *f;
    char line[160];
    int n = 0;

    if (att_path[0] == '\0')
        return ATT_ERR_IO;

    f = fopen(att_path, "r");
    if (f == NULL)
        return ATT_ERR_IO;

    while (fgets(line, sizeof line, f) != NULL) {
        Row *r;

        if (line[strspn(line, " \t\r\n")] == '\0')
            continue;

        if (n >= MAX_RECORDS) {
            fclose(f);
            return ATT_ERR_IO;
        }

        r = &rows[n];

        if (sscanf(line, "%15[^,],%31[^,],%10[^,],%d",
                   r->roll_no, r->subject, r->date, &r->status) != 4 ||
            !validate_roll_no(r->roll_no) ||
            !validate_date(r->date) ||
            !valid_status(r->status)) {
            fclose(f);
            return ATT_ERR_IO;
        }

        n++;
    }

    if (ferror(f)) {
        fclose(f);
        return ATT_ERR_IO;
    }

    fclose(f);
    *count = n;
    return ATT_OK;
}

/* Rewrites the file: write temp, then swap (works on Windows, where rename
   cannot overwrite an existing file). The old file is restored on failure. */
static AttResult save_all(int count)
{
    char tmp[280], bak[280];
    FILE *f;
    int i;

    snprintf(tmp, sizeof tmp, "%s.tmp", att_path);
    snprintf(bak, sizeof bak, "%s.bak", att_path);

    f = fopen(tmp, "w");
    if (f == NULL)
        return ATT_ERR_IO;

    for (i = 0; i < count; i++) {
        if (fprintf(f, "%s,%s,%s,%d\n", rows[i].roll_no, rows[i].subject,
                    rows[i].date, rows[i].status) < 0) {
            fclose(f);
            remove(tmp);
            return ATT_ERR_IO;
        }
    }

    if (fclose(f) != 0) {
        remove(tmp);
        return ATT_ERR_IO;
    }

    remove(bak);

    if (rename(att_path, bak) != 0) {
        remove(tmp);
        return ATT_ERR_IO;
    }

    if (rename(tmp, att_path) != 0) {
        rename(bak, att_path);
        remove(tmp);
        return ATT_ERR_IO;
    }

    remove(bak);
    return ATT_OK;
}

/* Shared checks for mark and update. */
static AttResult check_write(const Session *session, const char *roll_no,
                             const char *subject, const char *date, int status)
{
    if (!auth_can_modify(session))
        return ATT_ERR_PERMISSION;

    if (!validate_roll_no(roll_no) || !valid_subject(subject) ||
        !validate_date(date) || !valid_status(status))
        return ATT_ERR_INVALID;

    if (att_path[0] == '\0')
        return ATT_ERR_IO;

    if (!student_exists(roll_no))
        return ATT_ERR_UNKNOWN_STUDENT;

    return ATT_OK;
}

static void log_write(const Session *session, const char *verb,
                      const char *roll_no, const char *subject,
                      const char *date, int status)
{
    char action[160];

    snprintf(action, sizeof action, "%s roll=%s subject=%s date=%s status=%s",
             verb, roll_no, subject, date,
             status == ATT_PRESENT ? "PRESENT" : "ABSENT");
    audit_log_module("ATTENDANCE", session->username, action);
}

AttResult attendance_init(const char *file_path)
{
    FILE *f;

    if (file_path == NULL || file_path[0] == '\0')
        return ATT_ERR_INVALID;

    if (strlen(file_path) >= sizeof att_path - 8)
        return ATT_ERR_INVALID;

    f = fopen(file_path, "a");
    if (f == NULL)
        return ATT_ERR_IO;

    if (fclose(f) != 0)
        return ATT_ERR_IO;

    snprintf(att_path, sizeof att_path, "%s", file_path);
    return ATT_OK;
}

AttResult mark_attendance(const Session *session, const char *roll_no,
                          const char *subject, const char *date,
                          AttendanceStatus status)
{
    AttResult r;
    FILE *f;
    int n = 0, i;

    r = check_write(session, roll_no, subject, date, (int)status);
    if (r != ATT_OK)
        return r;

    r = load_all(&n);
    if (r != ATT_OK)
        return r;

    if (n >= MAX_RECORDS)
        return ATT_ERR_IO;

    for (i = 0; i < n; i++) {
        if (strcmp(rows[i].roll_no, roll_no) == 0 &&
            strcmp(rows[i].subject, subject) == 0 &&
            strcmp(rows[i].date, date) == 0)
            return ATT_ERR_DUPLICATE;
    }

    f = fopen(att_path, "a");
    if (f == NULL)
        return ATT_ERR_IO;

    if (fprintf(f, "%s,%s,%s,%d\n", roll_no, subject, date, (int)status) < 0) {
        fclose(f);
        return ATT_ERR_IO;
    }

    if (fclose(f) != 0)
        return ATT_ERR_IO;

    log_write(session, "MARK", roll_no, subject, date, (int)status);
    return ATT_OK;
}

AttResult update_attendance(const Session *session, const char *roll_no,
                            const char *subject, const char *date,
                            AttendanceStatus status)
{
    AttResult r;
    int n = 0, i;

    r = check_write(session, roll_no, subject, date, (int)status);
    if (r != ATT_OK)
        return r;

    r = load_all(&n);
    if (r != ATT_OK)
        return r;

    for (i = 0; i < n; i++) {
        if (strcmp(rows[i].roll_no, roll_no) == 0 &&
            strcmp(rows[i].subject, subject) == 0 &&
            strcmp(rows[i].date, date) == 0)
            break;
    }

    if (i == n)
        return ATT_ERR_NOT_FOUND;

    rows[i].status = (int)status;

    r = save_all(n);
    if (r != ATT_OK)
        return r;

    log_write(session, "UPDATE", roll_no, subject, date, (int)status);
    return ATT_OK;
}

/* Counts classes for one student+subject using the already loaded rows. */
static void count_rows(int n, const char *roll_no, const char *subject,
                       int *present, int *total)
{
    int i;

    *present = 0;
    *total = 0;

    for (i = 0; i < n; i++) {
        if (strcmp(rows[i].roll_no, roll_no) == 0 &&
            strcmp(rows[i].subject, subject) == 0) {
            (*total)++;
            if (rows[i].status == ATT_PRESENT)
                (*present)++;
        }
    }
}

AttResult calculate_percentage(const Session *session, const char *roll_no,
                               const char *subject, double *out_percent)
{
    AttResult r;
    int n = 0, present, total;

    if (out_percent == NULL || !validate_roll_no(roll_no) ||
        !valid_subject(subject))
        return ATT_ERR_INVALID;

    if (!auth_can_view_record(session, roll_no))
        return ATT_ERR_PERMISSION;

    r = load_all(&n);
    if (r != ATT_OK)
        return r;

    count_rows(n, roll_no, subject, &present, &total);

    *out_percent = (total == 0) ? 0.0 : (present * 100.0) / total;
    return ATT_OK;
}

AttResult attendance_view_student(const Session *session, const char *roll_no)
{
    AttResult r;
    int n = 0, i, j, found = 0;

    if (!validate_roll_no(roll_no))
        return ATT_ERR_INVALID;

    if (!auth_can_view_record(session, roll_no))
        return ATT_ERR_PERMISSION;

    r = load_all(&n);
    if (r != ATT_OK)
        return r;

    printf("\nAttendance for %s\n", roll_no);
    printf("%-12s %-20s %s\n", "Date", "Subject", "Status");
    printf("--------------------------------------------\n");

    for (i = 0; i < n; i++) {
        if (strcmp(rows[i].roll_no, roll_no) != 0)
            continue;

        printf("%-12s %-20s %s\n", rows[i].date, rows[i].subject,
               rows[i].status == ATT_PRESENT ? "PRESENT" : "ABSENT");
        found = 1;
    }

    if (!found) {
        printf("No attendance records found.\n");
        return ATT_OK;
    }

    printf("\nPercentage per subject\n");

    for (i = 0; i < n; i++) {
        int seen = 0, present, total;

        if (strcmp(rows[i].roll_no, roll_no) != 0)
            continue;

        for (j = 0; j < i; j++) {
            if (strcmp(rows[j].roll_no, roll_no) == 0 &&
                strcmp(rows[j].subject, rows[i].subject) == 0) {
                seen = 1;
                break;
            }
        }

        if (seen)
            continue;

        count_rows(n, roll_no, rows[i].subject, &present, &total);
        printf("%-20s %d/%d = %.1f%%\n", rows[i].subject, present, total,
               (present * 100.0) / total);
    }

    return ATT_OK;
}
