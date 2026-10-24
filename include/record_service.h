#ifndef RECORD_SERVICE_H
#define RECORD_SERVICE_H

#include <stdbool.h>

typedef struct {
    char roll_no[16];
    char name[64];
    char dept[32];
    char phone[16];
} StudentRecord;

bool add_student(const char *user, const StudentRecord *record);
bool update_student(const char *user, const StudentRecord *record);
bool delete_student(const char *user, const char *roll_no);
bool get_student(const char *roll_no, StudentRecord *out_record);

#endif