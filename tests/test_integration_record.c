#include <stdio.h>
#include <string.h>

#include "unity.h"
#include "auth.h"
#include "audit.h"
#include "record_service.h"
#include "storage_service.h"
#include "attendance.h"

/*
 * Issue #26: integration tests for Record + Storage + Audit + a write module.
 *
 * These use the REAL record_service, storage_service, audit and attendance code
 * (no fake student lookup). Attendance is the admin-only "write" used here because
 * the grades module is not in main yet; when it is merged, copy the attendance
 * scenarios below and swap in record_grade().
 */

#define STUDENTS_FILE  "students.dat"   /* hardcoded in storage_service.c */
#define TEMP_FILE      "temp.dat"
#define IT_AUDIT_LOG   "it_record_audit.log"
#define IT_ATT_FILE    "it_record_att.csv"

static Session admin_session;
static Session student_session;

static int count_lines(const char *path)
{
    FILE *f = fopen(path, "r");
    int c, n = 0;
    if (!f) return 0;
    while ((c = fgetc(f)) != EOF) if (c == '\n') n++;
    fclose(f);
    return n;
}

static int file_contains(const char *path, const char *needle)
{
    FILE *f = fopen(path, "r");
    char line[256];
    int found = 0;
    if (!f) return 0;
    while (fgets(line, sizeof line, f)) if (strstr(line, needle)) found = 1;
    fclose(f);
    return found;
}

/* Reads the raw storage file to prove the record really was written to disk. */
static int storage_file_has_roll(const char *roll_no)
{
    FILE *f = fopen(STUDENTS_FILE, "rb");
    StudentRecord r;
    int found = 0;
    if (!f) return 0;
    while (fread(&r, sizeof r, 1, f) == 1)
        if (strcmp(r.roll_no, roll_no) == 0) found = 1;
    fclose(f);
    return found;
}

void setUp(void)
{
    remove(STUDENTS_FILE);
    remove(TEMP_FILE);
    remove(IT_AUDIT_LOG);
    remove(IT_ATT_FILE);

    audit_init(IT_AUDIT_LOG);
    attendance_init(IT_ATT_FILE);
    attendance_set_student_check(NULL);   /* use the real get_student() lookup */

    memset(&admin_session, 0, sizeof admin_session);
    admin_session.role = ROLE_ADMIN;
    strcpy(admin_session.username, "admin");

    memset(&student_session, 0, sizeof student_session);
    student_session.role = ROLE_STUDENT;
    strcpy(student_session.username, "stud1");
    strcpy(student_session.roll_no, "PES001");
}

void tearDown(void)
{
    attendance_set_student_check(NULL);
    remove(STUDENTS_FILE);
    remove(TEMP_FILE);
    remove(IT_AUDIT_LOG);
    remove(IT_ATT_FILE);
}

/* Add a student, write a record for them, then check storage file and audit entries. */
void test_TC_Int_01_Add_Student_Then_Write_Updates_Storage_And_Audit(void)
{
    StudentRecord s = {"PES001", "Asha Rao", "CSE", "9876543210"};

    TEST_ASSERT_TRUE(add_student("admin", &s));
    TEST_ASSERT_TRUE(storage_file_has_roll("PES001"));

    TEST_ASSERT_EQUAL_INT(ATT_OK, mark_attendance(&admin_session, "PES001", "Maths",
                                                  "2026-10-20", ATT_PRESENT));
    TEST_ASSERT_TRUE(file_contains(IT_ATT_FILE, "PES001,Maths,2026-10-20,0"));

    /* exactly one audit entry per write operation (AC-SEC-003) */
    TEST_ASSERT_EQUAL_INT(2, count_lines(IT_AUDIT_LOG));
    TEST_ASSERT_TRUE(file_contains(IT_AUDIT_LOG, "| admin | ADD_STUDENT:PES001"));
    TEST_ASSERT_TRUE(file_contains(IT_AUDIT_LOG, "| ATTENDANCE | admin |"));
}

/* Write for a student who is not in storage must be rejected and leave no trace. */
void test_TC_Int_02_Write_For_Unknown_Student_Rejected(void)
{
    TEST_ASSERT_EQUAL_INT(ATT_ERR_UNKNOWN_STUDENT,
                          mark_attendance(&admin_session, "GHOST01", "Maths",
                                          "2026-10-20", ATT_PRESENT));
    TEST_ASSERT_FALSE(file_contains(IT_ATT_FILE, "GHOST01"));
    TEST_ASSERT_EQUAL_INT(0, count_lines(IT_AUDIT_LOG));
}

/* A non-admin write must be rejected: nothing stored, nothing logged. */
void test_TC_Int_03_Non_Admin_Write_Rejected(void)
{
    StudentRecord s = {"PES001", "Asha Rao", "CSE", "9876543210"};
    TEST_ASSERT_TRUE(add_student("admin", &s));
    TEST_ASSERT_EQUAL_INT(1, count_lines(IT_AUDIT_LOG));   /* only the add */

    TEST_ASSERT_EQUAL_INT(0, auth_can_modify(&student_session));
    TEST_ASSERT_EQUAL_INT(ATT_ERR_PERMISSION,
                          mark_attendance(&student_session, "PES001", "Maths",
                                          "2026-10-20", ATT_PRESENT));

    TEST_ASSERT_FALSE(file_contains(IT_ATT_FILE, "PES001"));
    TEST_ASSERT_EQUAL_INT(1, count_lines(IT_AUDIT_LOG));   /* no new entry */
}

/* Update and delete each leave one audit entry and change the storage file. */
void test_TC_Int_04_Update_And_Delete_Are_Stored_And_Audited(void)
{
    StudentRecord s   = {"PES001", "Asha Rao", "CSE", "9876543210"};
    StudentRecord upd = {"PES001", "Asha R",   "ECE", "9876543211"};
    StudentRecord out;

    TEST_ASSERT_TRUE(add_student("admin", &s));
    TEST_ASSERT_TRUE(update_student("admin", &upd));
    TEST_ASSERT_TRUE(get_student("PES001", &out));
    TEST_ASSERT_EQUAL_STRING("Asha R", out.name);

    TEST_ASSERT_TRUE(delete_student("admin", "PES001"));
    TEST_ASSERT_FALSE(storage_file_has_roll("PES001"));

    TEST_ASSERT_EQUAL_INT(3, count_lines(IT_AUDIT_LOG));
    TEST_ASSERT_TRUE(file_contains(IT_AUDIT_LOG, "ADD_STUDENT:PES001"));
    TEST_ASSERT_TRUE(file_contains(IT_AUDIT_LOG, "UPDATE_STUDENT:PES001"));
    TEST_ASSERT_TRUE(file_contains(IT_AUDIT_LOG, "DELETE_STUDENT:PES001"));
}

/* A rejected duplicate add must not write to storage twice or add an audit entry. */
void test_TC_Int_05_Duplicate_Add_Not_Logged(void)
{
    StudentRecord s1 = {"PES001", "Asha Rao", "CSE", "9876543210"};
    StudentRecord s2 = {"PES001", "Someone Else", "ECE", "9000000000"};
    StudentRecord out;

    TEST_ASSERT_TRUE(add_student("admin", &s1));
    TEST_ASSERT_FALSE(add_student("admin", &s2));

    TEST_ASSERT_TRUE(get_student("PES001", &out));
    TEST_ASSERT_EQUAL_STRING("Asha Rao", out.name);
    TEST_ASSERT_EQUAL_INT(1, count_lines(IT_AUDIT_LOG));
}

/* A deleted student can no longer receive writes. */
void test_TC_Int_06_Write_After_Delete_Rejected(void)
{
    StudentRecord s = {"PES001", "Asha Rao", "CSE", "9876543210"};
    TEST_ASSERT_TRUE(add_student("admin", &s));
    TEST_ASSERT_TRUE(delete_student("admin", "PES001"));

    TEST_ASSERT_EQUAL_INT(ATT_ERR_UNKNOWN_STUDENT,
                          mark_attendance(&admin_session, "PES001", "Maths",
                                          "2026-10-20", ATT_PRESENT));
    TEST_ASSERT_EQUAL_INT(2, count_lines(IT_AUDIT_LOG));   /* add + delete only */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_TC_Int_01_Add_Student_Then_Write_Updates_Storage_And_Audit);
    RUN_TEST(test_TC_Int_02_Write_For_Unknown_Student_Rejected);
    RUN_TEST(test_TC_Int_03_Non_Admin_Write_Rejected);
    RUN_TEST(test_TC_Int_04_Update_And_Delete_Are_Stored_And_Audited);
    RUN_TEST(test_TC_Int_05_Duplicate_Add_Not_Logged);
    RUN_TEST(test_TC_Int_06_Write_After_Delete_Rejected);
    return UNITY_END();
}
