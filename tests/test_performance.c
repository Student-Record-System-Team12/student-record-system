#include "unity.h"
#include "record_service.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Helper function to sort time records for percentile calculation
int compare_doubles(const void *a, const void *b) {
    double diff = (*(double*)a - *(double*)b);
    return (diff > 0) - (diff < 0);
}

void setUp(void) {
    remove("students.dat");
    
    // 1. Seed the database with 500 dummy records 
    // Testing performance on an empty file is cheating; we need real I/O load.
    for (int i = 0; i < 500; i++) {
        StudentRecord s;
        sprintf(s.roll_no, "STU%d", i);
        sprintf(s.name, "Load Test %d", i);
        strcpy(s.dept, "CSE");
        strcpy(s.phone, "000-0000");
        add_student("admin_perf", &s);
    }
}

void tearDown(void) {
    remove("students.dat");
    remove("temp.dat");
}

void test_TC_Perf_01_Lookup_Update(void) {
    double times[20];
    StudentRecord retrieved;

    // 2. Measure 20 scattered lookup operations
    for (int i = 0; i < 20; i++) {
        char target_roll[16];
        sprintf(target_roll, "STU%d", i * 20); // Pick records spread across the file

        clock_t start = clock(); // Start stopwatch
        
        get_student(target_roll, &retrieved);
        
        clock_t end = clock();   // Stop stopwatch
        
        // Calculate elapsed time in seconds
        times[i] = ((double)(end - start)) / CLOCKS_PER_SEC;
    }

    // 3. Sort the array in ascending order to find percentiles
    qsort(times, 20, sizeof(double), compare_doubles);

    // 4. Extract the 90th percentile (Index 17 out of 20 elements)
    double p90_time = times[17];

    // Log the result to the console so it is "results recorded"
    printf("\n[PERF RESULT] 90th Percentile Time: %f seconds\n", p90_time);

    // 5. Assert it meets the Acceptance Criteria (<= 2 seconds)
    TEST_ASSERT_TRUE_MESSAGE(p90_time <= 2.0, "P90 lookup exceeded 2 seconds!");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_TC_Perf_01_Lookup_Update);
    return UNITY_END();
}