#include "unity.h"
#include "record_service.h"
#include "storage_service.h"
#include <stdio.h>
#include <string.h>

void setUp(void) {
    remove("students.dat");
    remove("temp.dat");
}

void tearDown(void) {
    remove("students.dat");
    remove("temp.dat");
}

void test_TC_Rec_01_Add_And_Retrieve(void) {
    StudentRecord s = {"CS101", "John Doe", "CSE", "555-0101"};
    TEST_ASSERT_TRUE(add_student("admin1", &s));

    StudentRecord retrieved;
    TEST_ASSERT_TRUE(get_student("CS101", &retrieved));
    TEST_ASSERT_EQUAL_STRING("John Doe", retrieved.name);
}

void test_TC_Rec_02_Update_Fields(void) {
    StudentRecord s = {"CS101", "John Doe", "CSE", "555-0101"};
    add_student("admin1", &s);

    StudentRecord updated = {"CS101", "John Smith", "CSE", "555-0199"};
    TEST_ASSERT_TRUE(update_student("admin1", &updated));

    StudentRecord retrieved;
    TEST_ASSERT_TRUE(get_student("CS101", &retrieved));
    TEST_ASSERT_EQUAL_STRING("John Smith", retrieved.name);
}

void test_TC_Rec_03_Delete_With_Confirmation(void) {
    StudentRecord s = {"CS101", "John Doe", "CSE", "555-0101"};
    add_student("admin1", &s);

    TEST_ASSERT_TRUE(delete_student("admin1", "CS101"));

    StudentRecord retrieved;
    TEST_ASSERT_FALSE(get_student("CS101", &retrieved));
}

void test_TC_Rec_04_Duplicate_Roll_Number(void) {
    StudentRecord s1 = {"CS102", "Alice", "ECE", "555-0102"};
    TEST_ASSERT_TRUE(add_student("admin1", &s1));

    StudentRecord s2 = {"CS102", "Bob", "ECE", "555-0103"};
    TEST_ASSERT_FALSE(add_student("admin1", &s2));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_TC_Rec_01_Add_And_Retrieve);
    RUN_TEST(test_TC_Rec_02_Update_Fields);
    RUN_TEST(test_TC_Rec_03_Delete_With_Confirmation);
    RUN_TEST(test_TC_Rec_04_Duplicate_Roll_Number);
    return UNITY_END();
}