#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "grade.h"

static Session admin, student;
#define TMP_FILE "build/test_grades.csv"

void setUp(void)
{
    memset(&admin, 0, sizeof admin);
    memset(&student, 0, sizeof student);
    strcpy(admin.username, "admin");     admin.role = ROLE_ADMIN;   strcpy(admin.roll_no, "-");
    strcpy(student.username, "s1");      student.role = ROLE_STUDENT; strcpy(student.roll_no, "R001");
    remove(TMP_FILE);
    grade_init(NULL);   /* in-memory, empty */
}
void tearDown(void) { remove(TMP_FILE); }

/* --- validate_marks / calculate_grade --- */
void test_validate_marks(void)
{
    TEST_ASSERT_TRUE(validate_marks(0));
    TEST_ASSERT_TRUE(validate_marks(100));
    TEST_ASSERT_FALSE(validate_marks(-1));
    TEST_ASSERT_FALSE(validate_marks(101));
}

void test_grade_boundaries(void)
{
    TEST_ASSERT_EQUAL_CHAR('A', calculate_grade(100));
    TEST_ASSERT_EQUAL_CHAR('A', calculate_grade(90));
    TEST_ASSERT_EQUAL_CHAR('B', calculate_grade(89));
    TEST_ASSERT_EQUAL_CHAR('B', calculate_grade(80));
    TEST_ASSERT_EQUAL_CHAR('C', calculate_grade(79));
    TEST_ASSERT_EQUAL_CHAR('C', calculate_grade(70));
    TEST_ASSERT_EQUAL_CHAR('D', calculate_grade(69));
    TEST_ASSERT_EQUAL_CHAR('D', calculate_grade(60));
    TEST_ASSERT_EQUAL_CHAR('E', calculate_grade(59));
    TEST_ASSERT_EQUAL_CHAR('E', calculate_grade(50));
    TEST_ASSERT_EQUAL_CHAR('F', calculate_grade(49));
    TEST_ASSERT_EQUAL_CHAR('F', calculate_grade(0));
    TEST_ASSERT_EQUAL_CHAR('\0', calculate_grade(-1));
    TEST_ASSERT_EQUAL_CHAR('\0', calculate_grade(101));
}

/* --- record_grade --- */
void test_record_grade_ok(void)
{
    int m; char l;
    TEST_ASSERT_EQUAL_INT(GRADE_OK, record_grade(&admin, "R001", "Maths", 85));
    TEST_ASSERT_EQUAL_INT(1, grade_count());
    TEST_ASSERT_EQUAL_INT(GRADE_OK, get_grade("R001", "Maths", &m, &l));
    TEST_ASSERT_EQUAL_INT(85, m);
    TEST_ASSERT_EQUAL_CHAR('B', l);
}

void test_record_grade_student_denied(void)
{
    TEST_ASSERT_EQUAL_INT(GRADE_NOT_ADMIN, record_grade(&student, "R001", "Maths", 85));
    TEST_ASSERT_EQUAL_INT(GRADE_NOT_ADMIN, record_grade(NULL, "R001", "Maths", 85));
    TEST_ASSERT_EQUAL_INT(0, grade_count());
}

void test_record_grade_invalid_input(void)
{
    TEST_ASSERT_EQUAL_INT(GRADE_INVALID_INPUT, record_grade(&admin, "R001", "Maths", -1));
    TEST_ASSERT_EQUAL_INT(GRADE_INVALID_INPUT, record_grade(&admin, "R001", "Maths", 101));
    TEST_ASSERT_EQUAL_INT(GRADE_INVALID_INPUT, record_grade(&admin, "", "Maths", 50));
    TEST_ASSERT_EQUAL_INT(GRADE_INVALID_INPUT, record_grade(&admin, "R001", "", 50));
    TEST_ASSERT_EQUAL_INT(GRADE_INVALID_INPUT, record_grade(&admin, NULL, "Maths", 50));
    TEST_ASSERT_EQUAL_INT(GRADE_INVALID_INPUT, record_grade(&admin, "R,1", "Maths", 50));
    TEST_ASSERT_EQUAL_INT(GRADE_INVALID_INPUT,
        record_grade(&admin, "R0123456789012345", "Maths", 50));  /* too long */
    TEST_ASSERT_EQUAL_INT(0, grade_count());
}

void test_record_grade_duplicate(void)
{
    TEST_ASSERT_EQUAL_INT(GRADE_OK, record_grade(&admin, "R001", "Maths", 85));
    TEST_ASSERT_EQUAL_INT(GRADE_DUPLICATE, record_grade(&admin, "R001", "Maths", 70));
    TEST_ASSERT_EQUAL_INT(GRADE_OK, record_grade(&admin, "R001", "Physics", 70));
    TEST_ASSERT_EQUAL_INT(2, grade_count());
}

/* --- update_grade --- */
void test_update_grade_ok(void)
{
    int m; char l;
    record_grade(&admin, "R001", "Maths", 85);
    TEST_ASSERT_EQUAL_INT(GRADE_OK, update_grade(&admin, "R001", "Maths", 45));
    get_grade("R001", "Maths", &m, &l);
    TEST_ASSERT_EQUAL_INT(45, m);
    TEST_ASSERT_EQUAL_CHAR('F', l);
    TEST_ASSERT_EQUAL_INT(1, grade_count());
}

void test_update_grade_errors(void)
{
    int m;
    record_grade(&admin, "R001", "Maths", 85);
    TEST_ASSERT_EQUAL_INT(GRADE_NOT_FOUND, update_grade(&admin, "R999", "Maths", 50));
    TEST_ASSERT_EQUAL_INT(GRADE_NOT_ADMIN, update_grade(&student, "R001", "Maths", 50));
    TEST_ASSERT_EQUAL_INT(GRADE_INVALID_INPUT, update_grade(&admin, "R001", "Maths", 150));
    get_grade("R001", "Maths", &m, NULL);
    TEST_ASSERT_EQUAL_INT(85, m);   /* unchanged by every failed update */
}

void test_get_grade_not_found(void)
{
    TEST_ASSERT_EQUAL_INT(GRADE_NOT_FOUND, get_grade("R001", "Maths", NULL, NULL));
}

/* --- persistence --- */
void test_persistence_roundtrip(void)
{
    int m; char l;
    TEST_ASSERT_EQUAL_INT(0, grade_init(TMP_FILE));
    record_grade(&admin, "R001", "Maths", 92);
    record_grade(&admin, "R002", "Maths", 55);
    update_grade(&admin, "R002", "Maths", 65);

    TEST_ASSERT_EQUAL_INT(0, grade_init(TMP_FILE));   /* reload from disk */
    TEST_ASSERT_EQUAL_INT(2, grade_count());
    get_grade("R002", "Maths", &m, &l);
    TEST_ASSERT_EQUAL_INT(65, m);
    TEST_ASSERT_EQUAL_CHAR('D', l);
}

void test_corrupted_file_lines_skipped(void)
{
    FILE *f;
    f = fopen(TMP_FILE, "w");
    TEST_ASSERT_NOT_NULL(f);
    fputs("R001,Maths,80,B\ngarbage line\nR002,Maths,999,A\nR003,Maths,abc,A\n", f);
    fclose(f);
    TEST_ASSERT_EQUAL_INT(0, grade_init(TMP_FILE));
    TEST_ASSERT_EQUAL_INT(1, grade_count());          /* only the good line */
    TEST_ASSERT_EQUAL_INT(GRADE_OK, get_grade("R001", "Maths", NULL, NULL));
}

void test_missing_file_is_empty(void)
{
    TEST_ASSERT_EQUAL_INT(0, grade_init("build/does_not_exist.csv"));
    TEST_ASSERT_EQUAL_INT(0, grade_count());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_validate_marks);
    RUN_TEST(test_grade_boundaries);
    RUN_TEST(test_record_grade_ok);
    RUN_TEST(test_record_grade_student_denied);
    RUN_TEST(test_record_grade_invalid_input);
    RUN_TEST(test_record_grade_duplicate);
    RUN_TEST(test_update_grade_ok);
    RUN_TEST(test_update_grade_errors);
    RUN_TEST(test_get_grade_not_found);
    RUN_TEST(test_persistence_roundtrip);
    RUN_TEST(test_corrupted_file_lines_skipped);
    RUN_TEST(test_missing_file_is_empty);
    return UNITY_END();
}
