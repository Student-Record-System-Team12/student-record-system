#include <string.h>

#include "unity.h"
#include "validate.h"

void setUp(void) {}
void tearDown(void) {}

/* TC-Sec-05: input validation (STU-SR004, AC-SEC-004) */

void test_TC_Sec_05a_Roll_Number_Accepted(void)
{
    TEST_ASSERT_EQUAL_INT(1, validate_roll_no("PES001"));
    TEST_ASSERT_EQUAL_INT(1, validate_roll_no("CS-101"));
    TEST_ASSERT_EQUAL_INT(1, validate_roll_no("a_1"));
    TEST_ASSERT_EQUAL_INT(1, validate_roll_no("123456789012345"));   /* 15 chars */
}

void test_TC_Sec_05b_Roll_Number_Rejected(void)
{
    TEST_ASSERT_EQUAL_INT(0, validate_roll_no(NULL));
    TEST_ASSERT_EQUAL_INT(0, validate_roll_no(""));
    TEST_ASSERT_EQUAL_INT(0, validate_roll_no("1234567890123456"));  /* 16 chars */
    TEST_ASSERT_EQUAL_INT(0, validate_roll_no("PES 001"));
    TEST_ASSERT_EQUAL_INT(0, validate_roll_no("PES,001"));
    TEST_ASSERT_EQUAL_INT(0, validate_roll_no("PES\n001"));
    TEST_ASSERT_EQUAL_INT(0, validate_roll_no("PES001;"));
    TEST_ASSERT_EQUAL_INT(0, validate_roll_no("-"));
}

void test_TC_Sec_05c_Dates_Accepted(void)
{
    TEST_ASSERT_EQUAL_INT(1, validate_date("2026-10-23"));
    TEST_ASSERT_EQUAL_INT(1, validate_date("2024-02-29"));   /* leap year */
    TEST_ASSERT_EQUAL_INT(1, validate_date("2000-02-29"));   /* divisible by 400 */
}

void test_TC_Sec_05d_Dates_Rejected(void)
{
    TEST_ASSERT_EQUAL_INT(0, validate_date(NULL));
    TEST_ASSERT_EQUAL_INT(0, validate_date(""));
    TEST_ASSERT_EQUAL_INT(0, validate_date("2025-02-29"));
    TEST_ASSERT_EQUAL_INT(0, validate_date("1900-02-29"));   /* not a leap year */
    TEST_ASSERT_EQUAL_INT(0, validate_date("2100-02-29"));
    TEST_ASSERT_EQUAL_INT(0, validate_date("2026-13-01"));
    TEST_ASSERT_EQUAL_INT(0, validate_date("2026-00-10"));
    TEST_ASSERT_EQUAL_INT(0, validate_date("2026-04-31"));
    TEST_ASSERT_EQUAL_INT(0, validate_date("2026-10-00"));
    TEST_ASSERT_EQUAL_INT(0, validate_date("2026-1-01"));
    TEST_ASSERT_EQUAL_INT(0, validate_date("23-10-2026"));
    TEST_ASSERT_EQUAL_INT(0, validate_date("abcd-ef-gh"));
    TEST_ASSERT_EQUAL_INT(0, validate_date("2026/10/23"));
    TEST_ASSERT_EQUAL_INT(0, validate_date("1899-12-31"));
}

void test_TC_Sec_05e_Marks_Accepted_And_Value_Returned(void)
{
    int v = -1;

    TEST_ASSERT_EQUAL_INT(1, validate_marks("0", &v));
    TEST_ASSERT_EQUAL_INT(0, v);
    TEST_ASSERT_EQUAL_INT(1, validate_marks("75", &v));
    TEST_ASSERT_EQUAL_INT(75, v);
    TEST_ASSERT_EQUAL_INT(1, validate_marks("100", &v));
    TEST_ASSERT_EQUAL_INT(100, v);
    TEST_ASSERT_EQUAL_INT(1, validate_marks("50", NULL));
}

void test_TC_Sec_05f_Out_Of_Range_Or_Non_Numeric_Marks_Rejected(void)
{
    int v = 42;

    TEST_ASSERT_EQUAL_INT(0, validate_marks("101", &v));
    TEST_ASSERT_EQUAL_INT(0, validate_marks("-1", &v));
    TEST_ASSERT_EQUAL_INT(0, validate_marks("1000", &v));
    TEST_ASSERT_EQUAL_INT(0, validate_marks("abc", &v));
    TEST_ASSERT_EQUAL_INT(0, validate_marks("1.5", &v));
    TEST_ASSERT_EQUAL_INT(0, validate_marks(" 50", &v));
    TEST_ASSERT_EQUAL_INT(0, validate_marks("5 0", &v));
    TEST_ASSERT_EQUAL_INT(0, validate_marks("", &v));
    TEST_ASSERT_EQUAL_INT(0, validate_marks(NULL, &v));
    TEST_ASSERT_EQUAL_INT(42, v);   /* untouched on rejection */
}

void test_TC_Sec_05g_Text_Rejects_Separators_And_Control_Chars(void)
{
    TEST_ASSERT_EQUAL_INT(1, validate_text("Maths", 31));
    TEST_ASSERT_EQUAL_INT(1, validate_text("Data Structures", 31));
    TEST_ASSERT_EQUAL_INT(0, validate_text("Maths,Physics", 31));
    TEST_ASSERT_EQUAL_INT(0, validate_text("Maths\nPhysics", 31));
    TEST_ASSERT_EQUAL_INT(0, validate_text("Maths\r", 31));
    TEST_ASSERT_EQUAL_INT(0, validate_text("Ma\tths", 31));
    TEST_ASSERT_EQUAL_INT(0, validate_text("", 31));
    TEST_ASSERT_EQUAL_INT(0, validate_text("   ", 31));
    TEST_ASSERT_EQUAL_INT(0, validate_text(NULL, 31));
}

void test_TC_Sec_05h_Text_Length_Limit(void)
{
    TEST_ASSERT_EQUAL_INT(1, validate_text("abcde", 5));
    TEST_ASSERT_EQUAL_INT(0, validate_text("abcdef", 5));
}

void test_TC_Sec_05i_Phone_Rules(void)
{
    TEST_ASSERT_EQUAL_INT(1, validate_phone("9876543210"));
    TEST_ASSERT_EQUAL_INT(1, validate_phone("+919876543210"));
    TEST_ASSERT_EQUAL_INT(0, validate_phone("12345"));
    TEST_ASSERT_EQUAL_INT(0, validate_phone("98765x3210"));
    TEST_ASSERT_EQUAL_INT(0, validate_phone("9876,43210"));
    TEST_ASSERT_EQUAL_INT(0, validate_phone("1234567890123456"));
    TEST_ASSERT_EQUAL_INT(0, validate_phone(NULL));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_TC_Sec_05a_Roll_Number_Accepted);
    RUN_TEST(test_TC_Sec_05b_Roll_Number_Rejected);
    RUN_TEST(test_TC_Sec_05c_Dates_Accepted);
    RUN_TEST(test_TC_Sec_05d_Dates_Rejected);
    RUN_TEST(test_TC_Sec_05e_Marks_Accepted_And_Value_Returned);
    RUN_TEST(test_TC_Sec_05f_Out_Of_Range_Or_Non_Numeric_Marks_Rejected);
    RUN_TEST(test_TC_Sec_05g_Text_Rejects_Separators_And_Control_Chars);
    RUN_TEST(test_TC_Sec_05h_Text_Length_Limit);
    RUN_TEST(test_TC_Sec_05i_Phone_Rules);

    return UNITY_END();
}
