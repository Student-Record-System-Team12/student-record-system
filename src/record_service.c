#include <stdio.h>
#include <string.h>
#include "record.h"
#include "storage.h"
#include "audit.h"

// STU-F-010: Add student record
// STU-F-013: Reject duplicate roll number
bool add_student(const char* admin_user, const StudentRecord* new_student) {
    StudentRecord temp;
    
    // AC-STU-F-013: Check for duplicate roll number before adding
    if (storage_load(new_student->roll_no, &temp)) {
        printf("Error: Student with roll number %s already exists.\n", new_student->roll_no);
        return false; 
    }
    
    // AC-STU-F-010: Persist new record
    if (storage_save(new_student)) {
        audit_log(admin_user, AUDIT_ACTION_CREATE, new_student->roll_no);
        return true;
    }
    return false;
}

// STU-F-011: Update student record
bool update_student(const char* admin_user, const char* roll_no, const StudentRecord* updated_data) {
    StudentRecord existing;
    
    if (!storage_load(roll_no, &existing)) {
        printf("Error: Student record not found.\n");
        return false;
    }
    
    // AC-STU-F-011: Save edited fields
    if (storage_update(roll_no, updated_data)) {
        audit_log(admin_user, AUDIT_ACTION_UPDATE, roll_no);
        return true;
    }
    return false;
}

// STU-F-012: Delete student record with explicit confirmation
bool delete_student(const char* admin_user, const char* roll_no, bool user_confirmation) {
    // AC-STU-F-012: Delete only after explicit confirmation
    if (!user_confirmation) {
        printf("Deletion cancelled by user.\n");
        return false;
    }

    StudentRecord existing;
    if (!storage_load(roll_no, &existing)) {
        printf("Error: Student record not found.\n");
        return false;
    }

    if (storage_delete(roll_no)) {
        // AC-STU-F-012: Deletion logged
        audit_log(admin_user, AUDIT_ACTION_DELETE, roll_no);
        return true;
    }
    return false;
}