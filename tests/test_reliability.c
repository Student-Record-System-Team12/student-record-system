#include "unity.h"
#include "record_service.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setUp(void) {
    remove("students.dat");
    remove("temp.dat");
}

void tearDown(void) {
    remove("students.dat");
    remove("temp.dat");
}

void test_TC_Rel_01_Crash_Recovery(void) {
    // 1. Establish a known good database state
    StudentRecord s1 = {"REL001", "Alice CrashTest", "CSE", "555-0001"};
    TEST_ASSERT_TRUE(add_student("admin1", &s1));

    // 2. SIMULATE THE CRASH
    // We intentionally create a corrupted temp.dat file with half-written data.
    // This perfectly mimics a power failure right before rename() is called.
    FILE *temp = fopen("temp.dat", "wb");
    TEST_ASSERT_NOT_NULL(temp);
    fputs("CRASHED HALF-WRITTEN GARBAGE DATA...", temp);
    fclose(temp);

    // 3. Prove Acceptance Criteria: Data file remains readable and consistent
    StudentRecord retrieved;
    TEST_ASSERT_TRUE_MESSAGE(get_student("REL001", &retrieved), "Database became unreadable after simulated crash!");
    TEST_ASSERT_EQUAL_STRING("Alice CrashTest", retrieved.name);

    // 4. Prove Self-Healing: Ensure the system can overwrite the abandoned temp file on the next save
    StudentRecord s2 = {"REL002", "Bob Survivor", "ECE", "555-0002"};
    TEST_ASSERT_TRUE_MESSAGE(add_student("admin1", &s2), "System failed to recover and process new writes!");
    
    TEST_ASSERT_TRUE(get_student("REL002", &retrieved));
    TEST_ASSERT_EQUAL_STRING("Bob Survivor", retrieved.name);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_TC_Rel_01_Crash_Recovery);
    return UNITY_END();
}