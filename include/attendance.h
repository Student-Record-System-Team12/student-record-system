#ifndef ATTENDANCE_H
#define ATTENDANCE_H

#include "auth.h"

/*
 * STU-F030..033 (SRS 4.4): attendance per student, subject and date.
 * Storage: CSV, one record per line:  roll_no,subject,date,status
 *          status 0 = present, 1 = absent.
 */

#define ATT_ROLL_MAX    16
#define ATT_SUBJECT_MAX 32
#define ATT_DATE_MAX    11

typedef enum {
    ATT_PRESENT = 0,
    ATT_ABSENT  = 1
} AttendanceStatus;

typedef enum {
    ATT_OK = 0,
    ATT_ERR_PERMISSION,       /* not an admin / not allowed to view */
    ATT_ERR_INVALID,          /* bad roll number, subject, date or status */
    ATT_ERR_UNKNOWN_STUDENT,  /* no such student record */
    ATT_ERR_DUPLICATE,        /* entry for student+subject+date already exists */
    ATT_ERR_NOT_FOUND,        /* nothing to update */
    ATT_ERR_IO                /* file missing, unwritable or corrupt */
} AttResult;

/* Opens (creating if missing) the attendance file. Returns ATT_OK or ATT_ERR_IO/INVALID. */
AttResult attendance_init(const char *file_path);

/* Admin only. Rejects duplicates and unknown students. Audit-logged (module ATTENDANCE). */
AttResult mark_attendance(const Session *session, const char *roll_no,
                          const char *subject, const char *date,
                          AttendanceStatus status);

/* Admin only. Corrects an existing entry. Audit-logged. */
AttResult update_attendance(const Session *session, const char *roll_no,
                            const char *subject, const char *date,
                            AttendanceStatus status);

/* percentage = present / total * 100 for one subject. 0 classes gives 0.0 (no divide by zero).
   Students may only query their own roll number (auth_can_view_record). */
AttResult calculate_percentage(const Session *session, const char *roll_no,
                               const char *subject, double *out_percent);

/* Prints every record and a per-subject percentage. Students see only their own. */
AttResult attendance_view_student(const Session *session, const char *roll_no);

/* Human readable text for a result code, for the menu. */
const char *attendance_result_message(AttResult r);

/* Test hook: replace the "does this student exist" check. NULL restores the
   default, which uses get_student() from record_service.h. */
typedef int (*AttStudentCheck)(const char *roll_no);
void attendance_set_student_check(AttStudentCheck fn);

#endif
