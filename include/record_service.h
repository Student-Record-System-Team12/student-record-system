#ifndef RECORD_SERVICE_H
#define RECORD_SERVICE_H

#include <stdbool.h>
#include "auth.h"

typedef struct {
    char roll_no[16];
    char name[64];
    char dept[32];
    char phone[16];
} StudentRecord;

/* STU-SR002: writes need a logged-in admin session. A NULL or student
   session is rejected (returns false) and the attempt is audited. */
bool add_student(const Session *s, const StudentRecord *record);
bool update_student(const Session *s, const StudentRecord *record);
bool delete_student(const Session *s, const char *roll_no);
bool get_student(const char *roll_no, StudentRecord *out_record);

/* STU-F040: up to max records, in file order. Returns the count. */
int  list_students(StudentRecord *out, int max);

#endif