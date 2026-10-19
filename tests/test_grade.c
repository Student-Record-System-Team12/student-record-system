#include "unity.h"
#include "grade.h"

void setUp(void) {}
void tearDown(void) {}

void test_validate_marks_in_range(void)
{
    TEST_ASSERT_TRUE(validate_marks(0));
    TEST_ASSERT_TRUE(validate_marks(50));
    TEST_ASSERT_TRUE(validate_marks(100));
}

void test_validate_marks_out_of_range(void)
{
    TEST_ASSERT_FALSE(validate_marks(-1));
    TEST_ASSERT_FALSE(validate_marks(101));
    TEST_ASSERT_FALSE(validate_marks(-1000));
    TEST_ASSERT_FALSE(validate_marks(1000));
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
}

void test_grade_invalid_marks(void)
{
    TEST_ASSERT_EQUAL_CHAR('\0', calculate_grade(-1));
    TEST_ASSERT_EQUAL_CHAR('\0', calculate_grade(101));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_validate_marks_in_range);
    RUN_TEST(test_validate_marks_out_of_range);
    RUN_TEST(test_grade_boundaries);
    RUN_TEST(test_grade_invalid_marks);
    return UNITY_END();
}
