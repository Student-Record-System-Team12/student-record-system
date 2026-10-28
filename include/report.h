#ifndef REPORT_H
#define REPORT_H

#include <stdio.h>
#include "auth.h"
#include "record_service.h"

/*
 * STU-F040..042 (SRS 4.5): search and reports.
 *   STU-F040  search students by roll number or name
 *   STU-F041  grade sheet for one student
 *   STU-F042  attendance summary for one student, and CSV export of both
 *
 * Access rule (STU-F004): admins may report on any student, a student only on
 * their own roll number.
 */

#define REPORT_QUERY_MAX       63
#define REPORT_MAX_SUBJECTS    32
#define REPORT_ATT_THRESHOLD   75.0   /* attendance % below this is flagged LOW */

typedef enum {
    REPORT_OK = 0,
    REPORT_ERR_PERMISSION,   /* no session, or student asking about someone else */
    REPORT_ERR_INVALID,      /* bad query, roll number, path or argument */
    REPORT_ERR_NOT_FOUND,    /* no such student record */
    REPORT_ERR_IO            /* data or output file cannot be read or written */
} ReportResult;

/* STU-F040: case-insensitive substring match on roll number or name.
   Copies up to max matches into out and sets *count (0 is still REPORT_OK).
   Students only ever see their own record in the results. */
ReportResult search_students(const Session *session, const char *query,
                             StudentRecord *out, int max, int *count);

/* Prints the matches as a table. */
void report_print_students(FILE *out, const StudentRecord *list, int count);

/* STU-F041: subject, marks and letter grade per subject, plus average. */
ReportResult grade_sheet(const Session *session, const char *roll_no, FILE *out);

/* STU-F042: present/total and percentage per subject, plus overall; subjects
   below REPORT_ATT_THRESHOLD are marked LOW. */
ReportResult attendance_summary(const Session *session, const char *roll_no, FILE *out);

/* STU-F042: writes one CSV file for the student:
     roll_no,name,subject,marks,grade,present,total,attendance_percent
   one row per subject (grades and attendance combined; blank where missing).
   Cells containing a comma or quote are quoted; cells starting with = + - @ are
   prefixed with ' so spreadsheets do not run them.
   Audit-logged (module REPORT). */
ReportResult export_report_csv(const Session *session, const char *roll_no, const char *path);

/* Human readable text for a result code, for the menu. */
const char *report_result_message(ReportResult r);

#endif
