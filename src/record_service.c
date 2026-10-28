#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "record_service.h"
#include "storage_service.h"
#include "audit.h"

bool add_student(const char *user, const StudentRecord *record) {
    if (!record || record->roll_no[0] == '\0') {
        return false;
    }
    
    // STU-F013: Duplicate check before write
    StudentRecord existing;
    if (storage_load(record->roll_no, &existing)) {
        printf("Error: Student with roll number %s already exists.\n", record->roll_no);
        return false;
    }
    
    if (storage_save(record)) {
        char action[128];
        snprintf(action, sizeof(action), "ADD_STUDENT:%s", record->roll_no);
        audit_log(user, action);
        return true;
    }
    return false;
}

bool update_student(const char *user, const StudentRecord *record) {
    if (!record || record->roll_no[0] == '\0') return false;
    
    StudentRecord existing;
    if (!storage_load(record->roll_no, &existing)) {
        return false;
    }
    
    if (storage_update(record)) {
        char action[128];
        snprintf(action, sizeof(action), "UPDATE_STUDENT:%s", record->roll_no);
        audit_log(user, action);
        return true;
    }
    return false;
}

bool delete_student(const char *user, const char *roll_no) {
    if (!roll_no || roll_no[0] == '\0') return false;
    
    if (storage_delete(roll_no)) {
        char action[128];
        snprintf(action, sizeof(action), "DELETE_STUDENT:%s", roll_no);
        audit_log(user, action);
        return true;
    }
    return false;
}

bool get_student(const char *roll_no, StudentRecord *out_record) {
    return storage_load(roll_no, out_record);
}

int list_students(StudentRecord *out, int max) {
    return storage_list(out, max);
}
