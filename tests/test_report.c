#include <stdio.h>
#include <string.h>

#include "unity.h"
#include "report.h"
#include "attendance.h"
#include "audit.h"
#include "grade.h"
#include "record_service.h"

/* TC-Rpt-01+ : search and reports (STU-F040..042) */

#define ATT_FILE   "test_report_att.csv"
#define AUDIT_FILE "test_report_audit.log"
#define OUT_FILE   "test_report_out.txt"
#define CSV_FILE   "test_report_export.csv"

static Session admin, stud1, stud2;

static int file_contains(const char *path, const char *needle)
{
    FILE *f = fopen(path, "r");
    char line[256];
    int found = 0;

    if (f == NULL)
        return 0;
    while (fgets(line, sizeof line, f) != NULL)
        if (strstr(line, needle) != NULL)
            found = 1;
    fclose(f);
    return found;
}

static int line_count(const char *path)
{
    FILE *f = fopen(path, "r");
    char line[256];
    int n = 0;

    if (f == NULL)
        return -1;
    while (fgets(line, sizeof line, f) != NULL)
        n++;
    fclose(f);
    return n;
}

/* runs a report function that prints to a FILE and returns its result */
static ReportResult run_sheet(const Session *s, const char *roll, int attendance)
{
    ReportResult r;
    FILE *f = fopen(OUT_FILE, "w");

    TEST_ASSERT_NOT_NULL(f);
    r = attendance ? attendance_summary(s, roll, f) : grade_sheet(s, roll, f);
    fclose(f);
    return r;
}

static void admin_marks(const char *roll, const char *subj, const char *date, AttendanceStatus st)
{
    TEST_ASSERT_EQUAL_INT(ATT_OK, mark_attendance(&admin, roll, subj, date, st));
}

void setUp(void)
{
    StudentRecord a = {"PES001", "Asha Rao", "CSE", "9876543210"};
    StudentRecord b = {"PES002", "Ravi Kumar", "ECE", "9876500000"};
    StudentRecord c = {"PES003", "Ashwin Rao", "CSE", "9876511111"};

    remove("students.dat"); remove("temp.dat");
    remove(ATT_FILE); remove(AUDIT_FILE); remove(OUT_FILE); remove(CSV_FILE);

    audit_init(AUDIT_FILE);
    attendance_init(ATT_FILE);
    grade_init(NULL);                       /* in-memory, empty */

    add_student("admin", &a);
    add_student("admin", &b);
    add_student("admin", &c);

    memset(&admin, 0, sizeof admin);
    admin.role = ROLE_ADMIN; strcpy(admin.username, "admin"); strcpy(admin.roll_no, "-");
    memset(&stud1, 0, sizeof stud1);
    stud1.role = ROLE_STUDENT; strcpy(stud1.username, "s1"); strcpy(stud1.roll_no, "PES001");
    memset(&stud2, 0, sizeof stud2);
    stud2.role = ROLE_STUDENT; strcpy(stud2.username, "s2"); strcpy(stud2.roll_no, "PES002");
}

void tearDown(void)
{
    remove("students.dat"); remove("temp.dat");
    remove(ATT_FILE); remove(AUDIT_FILE); remove(OUT_FILE); remove(CSV_FILE);
}

/* ---------- STU-F040 search ---------- */

void test_TC_Rpt_01_Search_By_Roll_Number(void)
{
    StudentRecord out[10];
    int n = -1;

    TEST_ASSERT_EQUAL_INT(REPORT_OK, search_students(&admin, "PES002", out, 10, &n));
    TEST_ASSERT_EQUAL_INT(1, n);
    TEST_ASSERT_EQUAL_STRING("Ravi Kumar", out[0].name);
}

void test_TC_Rpt_02_Search_By_Partial_Name_Case_Insensitive(void)
{
    StudentRecord out[10];
    int n = -1;

    TEST_ASSERT_EQUAL_INT(REPORT_OK, search_students(&admin, "RAO", out, 10, &n));
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL_STRING("PES001", out[0].roll_no);
    TEST_ASSERT_EQUAL_STRING("PES003", out[1].roll_no);
}

void test_TC_Rpt_03_Search_No_Match_Is_Ok_With_Zero(void)
{
    StudentRecord out[10];
    int n = -1;

    TEST_ASSERT_EQUAL_INT(REPORT_OK, search_students(&admin, "nobody", out, 10, &n));
    TEST_ASSERT_EQUAL_INT(0, n);
}

void test_TC_Rpt_04_Student_Search_Only_Sees_Own_Record(void)
{
    StudentRecord out[10];
    int n = -1;

    TEST_ASSERT_EQUAL_INT(REPORT_OK, search_students(&stud1, "rao", out, 10, &n));
    TEST_ASSERT_EQUAL_INT(1, n);
    TEST_ASSERT_EQUAL_STRING("PES001", out[0].roll_no);

    TEST_ASSERT_EQUAL_INT(REPORT_OK, search_students(&stud1, "Ravi", out, 10, &n));
    TEST_ASSERT_EQUAL_INT(0, n);
}

void test_TC_Rpt_05_Search_Rejects_Bad_Input(void)
{
    StudentRecord out[10];
    int n = 7;

    TEST_ASSERT_EQUAL_INT(REPORT_ERR_INVALID, search_students(&admin, "", out, 10, &n));
    TEST_ASSERT_EQUAL_INT(REPORT_ERR_INVALID, search_students(&admin, "   ", out, 10, &n));
    TEST_ASSERT_EQUAL_INT(REPORT_ERR_INVALID, search_students(&admin, NULL, out, 10, &n));
    TEST_ASSERT_EQUAL_INT(REPORT_ERR_INVALID, search_students(&admin, "a,b", out, 10, &n));
    TEST_ASSERT_EQUAL_INT(REPORT_ERR_INVALID, search_students(&admin, "Rao", out, 0, &n));
    TEST_ASSERT_EQUAL_INT(REPORT_ERR_INVALID, search_students(&admin, "Rao", NULL, 10, &n));
    TEST_ASSERT_EQUAL_INT(REPORT_ERR_INVALID, search_students(&admin, "Rao", out, 10, NULL));
    TEST_ASSERT_EQUAL_INT(REPORT_ERR_INVALID,
        search_students(&admin, "0123456789012345678901234567890123456789012345678901234567890123",
                        out, 10, &n));
    TEST_ASSERT_EQUAL_INT(REPORT_ERR_PERMISSION, search_students(NULL, "Rao", out, 10, &n));
}

void test_TC_Rpt_06_Search_Respects_Max(void)
{
    StudentRecord out[1];
    int n = -1;

    TEST_ASSERT_EQUAL_INT(REPORT_OK, search_students(&admin, "PES", out, 1, &n));
    TEST_ASSERT_EQUAL_INT(1, n);
}

/* ---------- STU-F041 grade sheet ---------- */

void test_TC_Rpt_07_Grade_Sheet_Lists_Subjects_And_Average(void)
{
    record_grade(&admin, "PES001", "Maths", 90);
    record_grade(&admin, "PES001", "Physics", 70);
    record_grade(&admin, "PES002", "Maths", 10);      /* other student, must not appear */

    TEST_ASSERT_EQUAL_INT(REPORT_OK, run_sheet(&admin, "PES001", 0));
    TEST_ASSERT_TRUE(file_contains(OUT_FILE, "Asha Rao"));
    TEST_ASSERT_TRUE(file_contains(OUT_FILE, "Maths"));
    TEST_ASSERT_TRUE(file_contains(OUT_FILE, "Physics"));
    TEST_ASSERT_TRUE(file_contains(OUT_FILE, "Average: 80.0 (B)"));
    TEST_ASSERT_FALSE(file_contains(OUT_FILE, "Ravi"));
}

void test_TC_Rpt_08_Grade_Sheet_No_Grades(void)
{
    TEST_ASSERT_EQUAL_INT(REPORT_OK, run_sheet(&admin, "PES001", 0));
    TEST_ASSERT_TRUE(file_contains(OUT_FILE, "No grades recorded."));
}

void test_TC_Rpt_09_Grade_Sheet_Access_Control(void)
{
    record_grade(&admin, "PES001", "Maths", 90);

    TEST_ASSERT_EQUAL_INT(REPORT_OK, run_sheet(&stud1, "PES001", 0));        /* own */
    TEST_ASSERT_EQUAL_INT(REPORT_ERR_PERMISSION, run_sheet(&stud2, "PES001", 0));
    TEST_ASSERT_EQUAL_INT(REPORT_ERR_PERMISSION, run_sheet(NULL, "PES001", 0));
}

void test_TC_Rpt_10_Grade_Sheet_Unknown_Or_Invalid_Student(void)
{
    TEST_ASSERT_EQUAL_INT(REPORT_ERR_NOT_FOUND, run_sheet(&admin, "PES999", 0));
    TEST_ASSERT_EQUAL_INT(REPORT_ERR_INVALID, run_sheet(&admin, "bad roll", 0));
    TEST_ASSERT_EQUAL_INT(REPORT_ERR_INVALID, grade_sheet(&admin, "PES001", NULL));
}

/* ---------- STU-F042 attendance summary ---------- */

void test_TC_Rpt_11_Attendance_Summary_Percentages_And_Low_Flag(void)
{
    admin_marks("PES001", "Maths", "2026-10-01", ATT_PRESENT);
    admin_marks("PES001", "Maths", "2026-10-02", ATT_PRESENT);
    admin_marks("PES001", "Maths", "2026-10-03", ATT_PRESENT);
    admin_marks("PES001", "Maths", "2026-10-04", ATT_ABSENT);     /* 75.0% : not low */
    admin_marks("PES001", "Physics", "2026-10-01", ATT_PRESENT);
    admin_marks("PES001", "Physics", "2026-10-02", ATT_ABSENT);   /* 50.0% : low */

    TEST_ASSERT_EQUAL_INT(REPORT_OK, run_sheet(&admin, "PES001", 1));
    TEST_ASSERT_TRUE(file_contains(OUT_FILE, "75.0%"));
    TEST_ASSERT_TRUE(file_contains(OUT_FILE, "50.0%  LOW"));
    TEST_ASSERT_FALSE(file_contains(OUT_FILE, "75.0%  LOW"));
    TEST_ASSERT_TRUE(file_contains(OUT_FILE, "Overall"));
    TEST_ASSERT_TRUE(file_contains(OUT_FILE, "66.7%"));           /* 4 of 6 */
}

void test_TC_Rpt_12_Attendance_Summary_Empty_And_Access_Control(void)
{
    TEST_ASSERT_EQUAL_INT(REPORT_OK, run_sheet(&admin, "PES001", 1));
    TEST_ASSERT_TRUE(file_contains(OUT_FILE, "No attendance records found."));

    TEST_ASSERT_EQUAL_INT(REPORT_ERR_PERMISSION, run_sheet(&stud2, "PES001", 1));
    TEST_ASSERT_EQUAL_INT(REPORT_ERR_NOT_FOUND, run_sheet(&admin, "PES999", 1));
}

/* ---------- STU-F042 CSV export ---------- */

void test_TC_Rpt_13_Csv_Export_Combines_Grades_And_Attendance(void)
{
    record_grade(&admin, "PES001", "Maths", 85);
    record_grade(&admin, "PES001", "Chemistry", 60);              /* grade only */
    admin_marks("PES001", "Maths", "2026-10-01", ATT_PRESENT);
    admin_marks("PES001", "Maths", "2026-10-02", ATT_ABSENT);
    admin_marks("PES001", "Physics", "2026-10-01", ATT_PRESENT);  /* attendance only */

    TEST_ASSERT_EQUAL_INT(REPORT_OK, export_report_csv(&admin, "PES001", CSV_FILE));
    TEST_ASSERT_EQUAL_INT(4, line_count(CSV_FILE));               /* header + 3 subjects */
    TEST_ASSERT_TRUE(file_contains(CSV_FILE,
        "roll_no,name,subject,marks,grade,present,total,attendance_percent"));
    TEST_ASSERT_TRUE(file_contains(CSV_FILE, "PES001,Asha Rao,Maths,85,B,1,2,50.0"));
    TEST_ASSERT_TRUE(file_contains(CSV_FILE, "PES001,Asha Rao,Chemistry,60,D,,,"));
    TEST_ASSERT_TRUE(file_contains(CSV_FILE, "PES001,Asha Rao,Physics,,,1,1,100.0"));
}

void test_TC_Rpt_14_Csv_Export_Access_Control_And_Bad_Path(void)
{
    TEST_ASSERT_EQUAL_INT(REPORT_ERR_PERMISSION, export_report_csv(&stud2, "PES001", CSV_FILE));
    TEST_ASSERT_EQUAL_INT(REPORT_ERR_PERMISSION, export_report_csv(NULL, "PES001", CSV_FILE));
    TEST_ASSERT_EQUAL_INT(REPORT_ERR_NOT_FOUND, export_report_csv(&admin, "PES999", CSV_FILE));
    TEST_ASSERT_EQUAL_INT(REPORT_ERR_INVALID, export_report_csv(&admin, "PES001", ""));
    TEST_ASSERT_EQUAL_INT(REPORT_ERR_INVALID, export_report_csv(&admin, "PES001", NULL));
    TEST_ASSERT_EQUAL_INT(REPORT_ERR_IO,
        export_report_csv(&admin, "PES001", "no_such_dir/x/report.csv"));
    TEST_ASSERT_EQUAL_INT(-1, line_count(CSV_FILE));              /* nothing written */
}

void test_TC_Rpt_15_Student_Can_Export_Own_Report(void)
{
    record_grade(&admin, "PES001", "Maths", 85);

    TEST_ASSERT_EQUAL_INT(REPORT_OK, export_report_csv(&stud1, "PES001", CSV_FILE));
    TEST_ASSERT_TRUE(file_contains(CSV_FILE, "PES001,Asha Rao,Maths,85,B"));
}

void test_TC_Rpt_16_Csv_Neutralises_Formula_Cells(void)
{
    StudentRecord evil = {"PES004", "=cmd|calc", "CSE", "9876522222"};

    add_student("admin", &evil);
    record_grade(&admin, "PES004", "Maths", 50);

    TEST_ASSERT_EQUAL_INT(REPORT_OK, export_report_csv(&admin, "PES004", CSV_FILE));
    TEST_ASSERT_TRUE(file_contains(CSV_FILE, "PES004,'=cmd|calc,Maths,50,E"));
    TEST_ASSERT_FALSE(file_contains(CSV_FILE, ",=cmd"));
}

void test_TC_Rpt_17_Csv_Export_Is_Audit_Logged(void)
{
    TEST_ASSERT_EQUAL_INT(REPORT_OK, export_report_csv(&admin, "PES001", CSV_FILE));
    TEST_ASSERT_TRUE(file_contains(AUDIT_FILE, "REPORT"));
    TEST_ASSERT_TRUE(file_contains(AUDIT_FILE, "EXPORT_CSV roll=PES001"));

    remove(AUDIT_FILE);
    export_report_csv(&stud2, "PES001", CSV_FILE);                /* denied: no entry */
    TEST_ASSERT_FALSE(file_contains(AUDIT_FILE, "EXPORT_CSV"));
}

void test_TC_Rpt_19_Csv_Quotes_Names_With_Commas(void)
{
    StudentRecord comma = {"PES005", "Rao, Asha \"AR\"", "CSE", "9876533333"};

    add_student("admin", &comma);
    record_grade(&admin, "PES005", "Maths", 72);

    TEST_ASSERT_EQUAL_INT(REPORT_OK, export_report_csv(&admin, "PES005", CSV_FILE));
    TEST_ASSERT_TRUE(file_contains(CSV_FILE, "PES005,\"Rao, Asha \"\"AR\"\"\",Maths,72,C"));
}

void test_TC_Rpt_20_Csv_Export_Student_With_No_Grades_Or_Attendance(void)
{
    TEST_ASSERT_EQUAL_INT(REPORT_OK, export_report_csv(&admin, "PES003", CSV_FILE));
    TEST_ASSERT_EQUAL_INT(1, line_count(CSV_FILE));               /* header only */
}

void test_TC_Rpt_21_Search_Exact_Full_Name_And_Unknown_Roll(void)
{
    StudentRecord out[10];
    int n = -1;

    TEST_ASSERT_EQUAL_INT(REPORT_OK, search_students(&admin, "Ravi Kumar", out, 10, &n));
    TEST_ASSERT_EQUAL_INT(1, n);
    TEST_ASSERT_EQUAL_STRING("PES002", out[0].roll_no);

    TEST_ASSERT_EQUAL_INT(REPORT_OK, search_students(&admin, "PES999", out, 10, &n));
    TEST_ASSERT_EQUAL_INT(0, n);
}

void test_TC_Rpt_18_Result_Messages_Are_Not_Empty(void)
{
    TEST_ASSERT_EQUAL_STRING("Access denied.", report_result_message(REPORT_ERR_PERMISSION));
    TEST_ASSERT_EQUAL_STRING("No such student.", report_result_message(REPORT_ERR_NOT_FOUND));
    TEST_ASSERT_EQUAL_STRING("Done.", report_result_message(REPORT_OK));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_TC_Rpt_01_Search_By_Roll_Number);
    RUN_TEST(test_TC_Rpt_02_Search_By_Partial_Name_Case_Insensitive);
    RUN_TEST(test_TC_Rpt_03_Search_No_Match_Is_Ok_With_Zero);
    RUN_TEST(test_TC_Rpt_04_Student_Search_Only_Sees_Own_Record);
    RUN_TEST(test_TC_Rpt_05_Search_Rejects_Bad_Input);
    RUN_TEST(test_TC_Rpt_06_Search_Respects_Max);
    RUN_TEST(test_TC_Rpt_07_Grade_Sheet_Lists_Subjects_And_Average);
    RUN_TEST(test_TC_Rpt_08_Grade_Sheet_No_Grades);
    RUN_TEST(test_TC_Rpt_09_Grade_Sheet_Access_Control);
    RUN_TEST(test_TC_Rpt_10_Grade_Sheet_Unknown_Or_Invalid_Student);
    RUN_TEST(test_TC_Rpt_11_Attendance_Summary_Percentages_And_Low_Flag);
    RUN_TEST(test_TC_Rpt_12_Attendance_Summary_Empty_And_Access_Control);
    RUN_TEST(test_TC_Rpt_13_Csv_Export_Combines_Grades_And_Attendance);
    RUN_TEST(test_TC_Rpt_14_Csv_Export_Access_Control_And_Bad_Path);
    RUN_TEST(test_TC_Rpt_15_Student_Can_Export_Own_Report);
    RUN_TEST(test_TC_Rpt_16_Csv_Neutralises_Formula_Cells);
    RUN_TEST(test_TC_Rpt_17_Csv_Export_Is_Audit_Logged);
    RUN_TEST(test_TC_Rpt_18_Result_Messages_Are_Not_Empty);
    RUN_TEST(test_TC_Rpt_19_Csv_Quotes_Names_With_Commas);
    RUN_TEST(test_TC_Rpt_20_Csv_Export_Student_With_No_Grades_Or_Attendance);
    RUN_TEST(test_TC_Rpt_21_Search_Exact_Full_Name_And_Unknown_Roll);
    return UNITY_END();
}
