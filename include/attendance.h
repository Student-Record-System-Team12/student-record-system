#ifndef ATTENDANCE_H
#define ATTENDANCE_H

#include "auth.h"

#define ATTENDANCE_ROLL_MAX 16
#define ATTENDANCE_DATE_MAX 11

typedef enum {
    ATTENDANCE_PRESENT = 0,
    ATTENDANCE_ABSENT = 1
} AttendanceStatus;

typedef struct {
    char roll_no[ATTENDANCE_ROLL_MAX];
    char date[ATTENDANCE_DATE_MAX]; /* YYYY-MM-DD */
    AttendanceStatus status;
} AttendanceRecord;

/*
 * Initialize attendance storage.
 * A missing file is treated as an empty attendance file.
 * Returns 0 on success and -1 on failure.
 */
int attendance_init(const char *file_path);

/*
 * Mark attendance for a student.
 * Only an authenticated administrator may make changes.
 * Returns 0 on success and -1 on invalid input, permission
 * failure, duplicate record, or file error.
 */
int attendance_mark(const Session *session,
                    const char *roll_no,
                    const char *date,
                    AttendanceStatus status);

/* Print attendance records for one student. */
int attendance_view_student(const char *roll_no);

/* Validate a roll number and a YYYY-MM-DD date. */
int attendance_valid_roll_no(const char *roll_no);
int attendance_valid_date(const char *date);

#endif
