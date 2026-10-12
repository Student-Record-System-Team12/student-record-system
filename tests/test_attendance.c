#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "unity.h"
#include "attendance.h"

static Session admin_session;
static Session student_session;
static const char *test_file = "test_attendance_data.csv";

void setUp(void)
{
    FILE *f = fopen(test_file, "w");
    if (f != NULL)
        fclose(f);

    attendance_init(test_file);

    memset(&admin_session, 0, sizeof admin_session);
    admin_session.role = ROLE_ADMIN;
    strcpy(admin_session.username, "admin");

    memset(&student_session, 0, sizeof student_session);
    student_session.role = ROLE_STUDENT;
    strcpy(student_session.username, "student");
    strcpy(student_session.roll_no, "PES001");
}

void tearDown(void)
{
    remove(test_file);
}

void test_valid_roll_number(void)
{
    TEST_ASSERT_EQUAL_INT(1, attendance_valid_roll_no("PES001"));
    TEST_ASSERT_EQUAL_INT(1, attendance_valid_roll_no("CS-101"));
}

void test_invalid_roll_number(void)
{
    TEST_ASSERT_EQUAL_INT(0, attendance_valid_roll_no(""));
    TEST_ASSERT_EQUAL_INT(0, attendance_valid_roll_no("PES 001"));
    TEST_ASSERT_EQUAL_INT(0, attendance_valid_roll_no("PES,001"));
}

void test_valid_dates(void)
{
    TEST_ASSERT_EQUAL_INT(1, attendance_valid_date("2024-02-29"));
    TEST_ASSERT_EQUAL_INT(1, attendance_valid_date("2026-10-12"));
}

void test_invalid_dates(void)
{
    TEST_ASSERT_EQUAL_INT(0, attendance_valid_date("2025-02-29"));
    TEST_ASSERT_EQUAL_INT(0, attendance_valid_date("2026-13-01"));
    TEST_ASSERT_EQUAL_INT(0, attendance_valid_date("2026-04-31"));
    TEST_ASSERT_EQUAL_INT(0, attendance_valid_date("12-10-2026"));
}

void test_admin_can_mark_attendance(void)
{
    TEST_ASSERT_EQUAL_INT(
        0, attendance_mark(&admin_session, "PES001",
                           "2026-10-12", ATTENDANCE_PRESENT));
}

void test_student_cannot_mark_attendance(void)
{
    TEST_ASSERT_EQUAL_INT(
        -1, attendance_mark(&student_session, "PES001",
                            "2026-10-12", ATTENDANCE_PRESENT));
}

void test_null_session_cannot_mark_attendance(void)
{
    TEST_ASSERT_EQUAL_INT(
        -1, attendance_mark(NULL, "PES001",
                            "2026-10-12", ATTENDANCE_PRESENT));
}

void test_duplicate_attendance_rejected(void)
{
    TEST_ASSERT_EQUAL_INT(
        0, attendance_mark(&admin_session, "PES001",
                           "2026-10-12", ATTENDANCE_PRESENT));

    TEST_ASSERT_EQUAL_INT(
        -1, attendance_mark(&admin_session, "PES001",
                            "2026-10-12", ATTENDANCE_ABSENT));
}

void test_invalid_date_rejected_when_marking(void)
{
    TEST_ASSERT_EQUAL_INT(
        -1, attendance_mark(&admin_session, "PES001",
                            "2026-02-30", ATTENDANCE_PRESENT));
}

void test_invalid_status_rejected(void)
{
    TEST_ASSERT_EQUAL_INT(
        -1, attendance_mark(&admin_session, "PES001",
                            "2026-10-12", (AttendanceStatus)9));
}

void test_attendance_saved_to_file(void)
{
    FILE *f;
    char line[128];

    TEST_ASSERT_EQUAL_INT(
        0, attendance_mark(&admin_session, "PES001",
                           "2026-10-12", ATTENDANCE_PRESENT));

    f = fopen(test_file, "r");
    TEST_ASSERT_NOT_NULL(f);

    TEST_ASSERT_NOT_NULL(fgets(line, sizeof line, f));
    fclose(f);

    TEST_ASSERT_EQUAL_STRING("PES001,2026-10-12,0\n", line);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_valid_roll_number);
    RUN_TEST(test_invalid_roll_number);
    RUN_TEST(test_valid_dates);
    RUN_TEST(test_invalid_dates);
    RUN_TEST(test_admin_can_mark_attendance);
    RUN_TEST(test_student_cannot_mark_attendance);
    RUN_TEST(test_null_session_cannot_mark_attendance);
    RUN_TEST(test_duplicate_attendance_rejected);
    RUN_TEST(test_invalid_date_rejected_when_marking);
    RUN_TEST(test_invalid_status_rejected);
    RUN_TEST(test_attendance_saved_to_file);

    return UNITY_END();
}
