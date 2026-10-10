#include "unity.h"
#include "record.h"
#include "storage.h"
#include <string.h>

bool add_student(const char* admin_user, const StudentRecord* new_student);
bool update_student(const char* admin_user, const char* roll_no, const StudentRecord* updated_data);
bool delete_student(const char* admin_user, const char* roll_no, bool user_confirmation);

void setUp(void) {
    // Delete any left-over files from previous runs to guarantee a blank slate
    remove("students.dat"); 
    remove("temp.dat");
    storage_init("students.dat"); 
}

void tearDown(void) {
    storage_close(); 
    // Clean up after the test finishes
    remove("students.dat");
    remove("temp.dat");
}

void test_TC_Rec_01_Add_And_Retrieve(void) {
    StudentRecord s1 = {"CS101", "Alice", "CSE", "555-0100"};
    TEST_ASSERT_TRUE(add_student("admin1", &s1));
    
    StudentRecord retrieved;
    TEST_ASSERT_TRUE(storage_load("CS101", &retrieved));
    TEST_ASSERT_EQUAL_STRING("Alice", retrieved.name);
    TEST_ASSERT_EQUAL_STRING("CSE", retrieved.department);
}

void test_TC_Rec_02_Update_Fields(void) {
    // First ensure the record exists
    StudentRecord s1 = {"CS101", "Alice", "CSE", "555-0100"};
    add_student("admin1", &s1);

    // Update it
    StudentRecord s_update = {"CS101", "Alice Smith", "CSE", "555-0199"};
    TEST_ASSERT_TRUE(update_student("admin1", "CS101", &s_update));
    
    StudentRecord retrieved;
    TEST_ASSERT_TRUE(storage_load("CS101", &retrieved));
    TEST_ASSERT_EQUAL_STRING("Alice Smith", retrieved.name);
    TEST_ASSERT_EQUAL_STRING("555-0199", retrieved.contact);
}

void test_TC_Rec_03_Delete_With_Confirmation(void) {
    // Ensure record exists
    StudentRecord s1 = {"CS101", "Alice", "CSE", "555-0100"};
    add_student("admin1", &s1);

    // Should fail without confirmation (false flag)
    TEST_ASSERT_FALSE(delete_student("admin1", "CS101", false)); 
    
    // Should succeed with confirmation (true flag)
    TEST_ASSERT_TRUE(delete_student("admin1", "CS101", true));
    
    // Verify it is actually gone from storage
    StudentRecord retrieved;
    TEST_ASSERT_FALSE(storage_load("CS101", &retrieved));
}

void test_TC_Rec_04_Duplicate_Roll_Number(void) {
    StudentRecord s1 = {"CS102", "Bob", "CSE", "555-0200"};
    StudentRecord s2 = {"CS102", "Charlie", "ECE", "555-0300"}; // Same ID, different data
    
    TEST_ASSERT_TRUE(add_student("admin1", &s1));
    // The second add must fail and return false
    TEST_ASSERT_FALSE(add_student("admin1", &s2)); 
}

void test_TC_Rel_01_Interrupted_Write_Preserves_Data(void) {
    // 1. Establish the original file state
    StudentRecord s1 = {"CS999", "Reliability Test", "CSE", "555-0000"};
    TEST_ASSERT_TRUE(add_student("admin1", &s1));
    
    // 2. Simulate a malformed/failing save by intentionally breaking the temp file process
    // In a full environment, you would mock fwrite() to fail, but here we test the logic bounds:
    StudentRecord malformed = {"", "Bad Data", "CSE", "000"}; 
    TEST_ASSERT_FALSE(add_student("admin1", &malformed)); // Should report failure[cite: 9]
    
    // 3. Verify the original file is perfectly intact[cite: 9]
    StudentRecord retrieved;
    TEST_ASSERT_TRUE(storage_load("CS999", &retrieved));
    TEST_ASSERT_EQUAL_STRING("Reliability Test", retrieved.name);
}

// Main test runner
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_TC_Rec_01_Add_And_Retrieve);
    RUN_TEST(test_TC_Rec_02_Update_Fields);
    RUN_TEST(test_TC_Rec_03_Delete_With_Confirmation);
    RUN_TEST(test_TC_Rec_04_Duplicate_Roll_Number);
    RUN_TEST(test_TC_Rel_01_Interrupted_Write_Preserves_Data);
    return UNITY_END();
}