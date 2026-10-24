#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "unity.h"
#include "attendance.h"
#include "audit.h"

/* TC-Att-01+ : attendance module (STU-F030..033) */

static const char *att_file   = "test_attendance_data.csv";
static const char *audit_file = "test_att_audit.log";

static Session admin_session;
static Session student_session;    /* PES001 */
static Session other_student;      /* PES002 */

/* Fake student lookup so these tests do not depend on the record storage files. */
static int fake_student_exists(const char *roll_no)
{
    return strcmp(roll_no, "PES001") == 0 || strcmp(roll_no, "PES002") == 0;
}

static void write_file(const char *path, const char *text)
{
    FILE *f = fopen(path, "w");
    if (f != NULL) {
        fputs(text, f);
        fclose(f);
    }
}

static int file_contains(const char *path, const char *needle)
{
    FILE *f = fopen(path, "r");
    char line[256];
    int found = 0;

    if (f == NULL)
        return 0;
    while (fgets(line, sizeof line, f) != NULL) {
        if (strstr(line, needle) != NULL)
            found = 1;
    }
    fclose(f);
    return found;
}

void setUp(void)
{
    remove(att_file);
    remove(audit_file);

    audit_init(audit_file);
    attendance_init(att_file);
    attendance_set_student_check(fake_student_exists);

    memset(&admin_session, 0, sizeof admin_session);
    admin_session.role = ROLE_ADMIN;
    strcpy(admin_session.username, "admin");

    memset(&student_session, 0, sizeof student_session);
    student_session.role = ROLE_STUDENT;
    strcpy(student_session.username, "stud1");
    strcpy(student_session.roll_no, "PES001");

    memset(&other_student, 0, sizeof other_student);
    other_student.role = ROLE_STUDENT;
    strcpy(other_student.username, "stud2");
    strcpy(other_student.roll_no, "PES002");
}

void tearDown(void)
{
    attendance_set_student_check(NULL);
    remove(att_file);
    remove(audit_file);
}

void test_TC_Att_01_Admin_Marks_And_Entry_Is_Saved(void)
{
    TEST_ASSERT_EQUAL_INT(ATT_OK, mark_attendance(&admin_session, "PES001", "Maths",
                                                  "2026-10-12", ATT_PRESENT));
    TEST_ASSERT_TRUE(file_contains(att_file, "PES001,Maths,2026-10-12,0"));
}

void test_TC_Att_02_Absent_Is_Stored_As_One(void)
{
    TEST_ASSERT_EQUAL_INT(ATT_OK, mark_attendance(&admin_session, "PES001", "Maths",
                                                  "2026-10-12", ATT_ABSENT));
    TEST_ASSERT_TRUE(file_contains(att_file, "PES001,Maths,2026-10-12,1"));
}

void test_TC_Att_03_Student_Cannot_Mark(void)
{
    TEST_ASSERT_EQUAL_INT(ATT_ERR_PERMISSION, mark_attendance(&student_session, "PES001",
                                  "Maths", "2026-10-12", ATT_PRESENT));
    TEST_ASSERT_FALSE(file_contains(att_file, "PES001"));
}

void test_TC_Att_04_Null_Session_Cannot_Mark(void)
{
    TEST_ASSERT_EQUAL_INT(ATT_ERR_PERMISSION, mark_attendance(NULL, "PES001",
                                  "Maths", "2026-10-12", ATT_PRESENT));
}

void test_TC_Att_05_Mark_Is_Audit_Logged(void)
{
    TEST_ASSERT_EQUAL_INT(ATT_OK, mark_attendance(&admin_session, "PES001", "Maths",
                                                  "2026-10-12", ATT_PRESENT));
    TEST_ASSERT_TRUE(file_contains(audit_file, "ATTENDANCE"));
    TEST_ASSERT_TRUE(file_contains(audit_file, "admin"));
    TEST_ASSERT_TRUE(file_contains(audit_file, "PES001"));
}

void test_TC_Att_06_Rejected_Write_Is_Not_Audit_Logged(void)
{
    mark_attendance(&student_session, "PES001", "Maths", "2026-10-12", ATT_PRESENT);
    TEST_ASSERT_FALSE(file_contains(audit_file, "ATTENDANCE"));
}

void test_TC_Att_07_Duplicate_Entry_Rejected(void)
{
    TEST_ASSERT_EQUAL_INT(ATT_OK, mark_attendance(&admin_session, "PES001", "Maths",
                                                  "2026-10-12", ATT_PRESENT));
    TEST_ASSERT_EQUAL_INT(ATT_ERR_DUPLICATE, mark_attendance(&admin_session, "PES001",
                                  "Maths", "2026-10-12", ATT_ABSENT));
}

void test_TC_Att_08_Same_Date_Different_Subject_Allowed(void)
{
    TEST_ASSERT_EQUAL_INT(ATT_OK, mark_attendance(&admin_session, "PES001", "Maths",
                                                  "2026-10-12", ATT_PRESENT));
    TEST_ASSERT_EQUAL_INT(ATT_OK, mark_attendance(&admin_session, "PES001", "Physics",
                                                  "2026-10-12", ATT_ABSENT));
}

void test_TC_Att_09_Invalid_Date_Rejected(void)
{
    TEST_ASSERT_EQUAL_INT(ATT_ERR_INVALID, mark_attendance(&admin_session, "PES001",
                                  "Maths", "2026-02-30", ATT_PRESENT));
    TEST_ASSERT_EQUAL_INT(ATT_ERR_INVALID, mark_attendance(&admin_session, "PES001",
                                  "Maths", "12-10-2026", ATT_PRESENT));
}

void test_TC_Att_10_Invalid_Roll_Subject_Status_Rejected(void)
{
    TEST_ASSERT_EQUAL_INT(ATT_ERR_INVALID, mark_attendance(&admin_session, "PES,001",
                                  "Maths", "2026-10-12", ATT_PRESENT));
    TEST_ASSERT_EQUAL_INT(ATT_ERR_INVALID, mark_attendance(&admin_session, "PES001",
                                  "Maths,Physics", "2026-10-12", ATT_PRESENT));
    TEST_ASSERT_EQUAL_INT(ATT_ERR_INVALID, mark_attendance(&admin_session, "PES001",
                                  "", "2026-10-12", ATT_PRESENT));
    TEST_ASSERT_EQUAL_INT(ATT_ERR_INVALID, mark_attendance(&admin_session, "PES001",
                                  "Maths", "2026-10-12", (AttendanceStatus)9));
}

void test_TC_Att_11_Unknown_Student_Rejected(void)
{
    TEST_ASSERT_EQUAL_INT(ATT_ERR_UNKNOWN_STUDENT, mark_attendance(&admin_session, "NOBODY1",
                                  "Maths", "2026-10-12", ATT_PRESENT));
    TEST_ASSERT_FALSE(file_contains(att_file, "NOBODY1"));
}

void test_TC_Att_12_Prior_Entry_Can_Be_Corrected(void)
{
    TEST_ASSERT_EQUAL_INT(ATT_OK, mark_attendance(&admin_session, "PES001", "Maths",
                                                  "2026-10-12", ATT_ABSENT));
    TEST_ASSERT_EQUAL_INT(ATT_OK, update_attendance(&admin_session, "PES001", "Maths",
                                                    "2026-10-12", ATT_PRESENT));
    TEST_ASSERT_TRUE(file_contains(att_file, "PES001,Maths,2026-10-12,0"));
    TEST_ASSERT_FALSE(file_contains(att_file, "PES001,Maths,2026-10-12,1"));
    TEST_ASSERT_TRUE(file_contains(audit_file, "UPDATE"));
}

void test_TC_Att_13_Update_Keeps_Other_Entries(void)
{
    mark_attendance(&admin_session, "PES001", "Maths", "2026-10-12", ATT_ABSENT);
    mark_attendance(&admin_session, "PES001", "Maths", "2026-10-13", ATT_PRESENT);
    mark_attendance(&admin_session, "PES002", "Maths", "2026-10-12", ATT_PRESENT);

    TEST_ASSERT_EQUAL_INT(ATT_OK, update_attendance(&admin_session, "PES001", "Maths",
                                                    "2026-10-12", ATT_PRESENT));
    TEST_ASSERT_TRUE(file_contains(att_file, "PES001,Maths,2026-10-13,0"));
    TEST_ASSERT_TRUE(file_contains(att_file, "PES002,Maths,2026-10-12,0"));
}

void test_TC_Att_14_Update_Missing_Entry_Fails(void)
{
    TEST_ASSERT_EQUAL_INT(ATT_ERR_NOT_FOUND, update_attendance(&admin_session, "PES001",
                                  "Maths", "2026-10-12", ATT_PRESENT));
}

void test_TC_Att_15_Student_Cannot_Update(void)
{
    mark_attendance(&admin_session, "PES001", "Maths", "2026-10-12", ATT_ABSENT);
    TEST_ASSERT_EQUAL_INT(ATT_ERR_PERMISSION, update_attendance(&student_session, "PES001",
                                  "Maths", "2026-10-12", ATT_PRESENT));
    TEST_ASSERT_TRUE(file_contains(att_file, "PES001,Maths,2026-10-12,1"));
}

void test_TC_Att_16_Percentage_Zero_Classes_No_Divide_By_Zero(void)
{
    double p = -1.0;

    TEST_ASSERT_EQUAL_INT(ATT_OK, calculate_percentage(&admin_session, "PES001", "Maths", &p));
    TEST_ASSERT_TRUE(p > -0.001 && p < 0.001);
}

void test_TC_Att_17_Percentage_All_Present_Is_100(void)
{
    double p = 0.0;

    mark_attendance(&admin_session, "PES001", "Maths", "2026-10-12", ATT_PRESENT);
    mark_attendance(&admin_session, "PES001", "Maths", "2026-10-13", ATT_PRESENT);

    TEST_ASSERT_EQUAL_INT(ATT_OK, calculate_percentage(&admin_session, "PES001", "Maths", &p));
    TEST_ASSERT_TRUE(p > 99.99 && p < 100.01);
}

void test_TC_Att_18_Percentage_All_Absent_Is_0(void)
{
    double p = 50.0;

    mark_attendance(&admin_session, "PES001", "Maths", "2026-10-12", ATT_ABSENT);
    mark_attendance(&admin_session, "PES001", "Maths", "2026-10-13", ATT_ABSENT);

    TEST_ASSERT_EQUAL_INT(ATT_OK, calculate_percentage(&admin_session, "PES001", "Maths", &p));
    TEST_ASSERT_TRUE(p > -0.001 && p < 0.001);
}

void test_TC_Att_19_Percentage_Is_Per_Subject(void)
{
    double maths = 0.0, physics = 0.0;

    mark_attendance(&admin_session, "PES001", "Maths", "2026-10-12", ATT_PRESENT);
    mark_attendance(&admin_session, "PES001", "Maths", "2026-10-13", ATT_PRESENT);
    mark_attendance(&admin_session, "PES001", "Maths", "2026-10-14", ATT_PRESENT);
    mark_attendance(&admin_session, "PES001", "Maths", "2026-10-15", ATT_ABSENT);
    mark_attendance(&admin_session, "PES001", "Physics", "2026-10-12", ATT_ABSENT);

    TEST_ASSERT_EQUAL_INT(ATT_OK, calculate_percentage(&admin_session, "PES001", "Maths", &maths));
    TEST_ASSERT_EQUAL_INT(ATT_OK, calculate_percentage(&admin_session, "PES001", "Physics", &physics));
    TEST_ASSERT_TRUE(maths > 74.99 && maths < 75.01);
    TEST_ASSERT_TRUE(physics > -0.001 && physics < 0.001);
}

void test_TC_Att_20_Percentage_After_Correction(void)
{
    double p = 0.0;

    mark_attendance(&admin_session, "PES001", "Maths", "2026-10-12", ATT_ABSENT);
    mark_attendance(&admin_session, "PES001", "Maths", "2026-10-13", ATT_PRESENT);
    update_attendance(&admin_session, "PES001", "Maths", "2026-10-12", ATT_PRESENT);

    TEST_ASSERT_EQUAL_INT(ATT_OK, calculate_percentage(&admin_session, "PES001", "Maths", &p));
    TEST_ASSERT_TRUE(p > 99.99 && p < 100.01);
}

void test_TC_Att_21_Student_Views_Only_Own_Attendance(void)
{
    double p = 0.0;

    mark_attendance(&admin_session, "PES001", "Maths", "2026-10-12", ATT_PRESENT);
    mark_attendance(&admin_session, "PES002", "Maths", "2026-10-12", ATT_PRESENT);

    TEST_ASSERT_EQUAL_INT(ATT_OK, attendance_view_student(&student_session, "PES001"));
    TEST_ASSERT_EQUAL_INT(ATT_ERR_PERMISSION, attendance_view_student(&student_session, "PES002"));
    TEST_ASSERT_EQUAL_INT(ATT_ERR_PERMISSION, attendance_view_student(&other_student, "PES001"));
    TEST_ASSERT_EQUAL_INT(ATT_ERR_PERMISSION, calculate_percentage(&student_session, "PES002", "Maths", &p));
    TEST_ASSERT_EQUAL_INT(ATT_OK, calculate_percentage(&student_session, "PES001", "Maths", &p));
}

void test_TC_Att_22_Admin_Views_Any_Student_And_Null_Session_Denied(void)
{
    TEST_ASSERT_EQUAL_INT(ATT_OK, attendance_view_student(&admin_session, "PES002"));
    TEST_ASSERT_EQUAL_INT(ATT_ERR_PERMISSION, attendance_view_student(NULL, "PES001"));
}

void test_TC_Att_23_View_With_Invalid_Roll_Rejected(void)
{
    TEST_ASSERT_EQUAL_INT(ATT_ERR_INVALID, attendance_view_student(&admin_session, "PES,001"));
    TEST_ASSERT_EQUAL_INT(ATT_ERR_INVALID, attendance_view_student(&admin_session, NULL));
}

void test_TC_Att_24_Corrupt_File_Is_Reported_Not_Processed(void)
{
    write_file(att_file, "PES001,Maths,2026-10-12,0\nthis is not a record\n");

    TEST_ASSERT_EQUAL_INT(ATT_ERR_IO, mark_attendance(&admin_session, "PES002", "Maths",
                                                      "2026-10-12", ATT_PRESENT));
    TEST_ASSERT_EQUAL_INT(ATT_ERR_IO, attendance_view_student(&admin_session, "PES001"));
}

void test_TC_Att_25_Init_Rejects_Bad_Path(void)
{
    TEST_ASSERT_EQUAL_INT(ATT_ERR_INVALID, attendance_init(NULL));
    TEST_ASSERT_EQUAL_INT(ATT_ERR_INVALID, attendance_init(""));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_TC_Att_01_Admin_Marks_And_Entry_Is_Saved);
    RUN_TEST(test_TC_Att_02_Absent_Is_Stored_As_One);
    RUN_TEST(test_TC_Att_03_Student_Cannot_Mark);
    RUN_TEST(test_TC_Att_04_Null_Session_Cannot_Mark);
    RUN_TEST(test_TC_Att_05_Mark_Is_Audit_Logged);
    RUN_TEST(test_TC_Att_06_Rejected_Write_Is_Not_Audit_Logged);
    RUN_TEST(test_TC_Att_07_Duplicate_Entry_Rejected);
    RUN_TEST(test_TC_Att_08_Same_Date_Different_Subject_Allowed);
    RUN_TEST(test_TC_Att_09_Invalid_Date_Rejected);
    RUN_TEST(test_TC_Att_10_Invalid_Roll_Subject_Status_Rejected);
    RUN_TEST(test_TC_Att_11_Unknown_Student_Rejected);
    RUN_TEST(test_TC_Att_12_Prior_Entry_Can_Be_Corrected);
    RUN_TEST(test_TC_Att_13_Update_Keeps_Other_Entries);
    RUN_TEST(test_TC_Att_14_Update_Missing_Entry_Fails);
    RUN_TEST(test_TC_Att_15_Student_Cannot_Update);
    RUN_TEST(test_TC_Att_16_Percentage_Zero_Classes_No_Divide_By_Zero);
    RUN_TEST(test_TC_Att_17_Percentage_All_Present_Is_100);
    RUN_TEST(test_TC_Att_18_Percentage_All_Absent_Is_0);
    RUN_TEST(test_TC_Att_19_Percentage_Is_Per_Subject);
    RUN_TEST(test_TC_Att_20_Percentage_After_Correction);
    RUN_TEST(test_TC_Att_21_Student_Views_Only_Own_Attendance);
    RUN_TEST(test_TC_Att_22_Admin_Views_Any_Student_And_Null_Session_Denied);
    RUN_TEST(test_TC_Att_23_View_With_Invalid_Roll_Rejected);
    RUN_TEST(test_TC_Att_24_Corrupt_File_Is_Reported_Not_Processed);
    RUN_TEST(test_TC_Att_25_Init_Rejects_Bad_Path);

    return UNITY_END();
}
