/*
 * STU-NF004: console menu and UI wiring.
 * Login (auth.h) -> role based menu -> attendance / student records.
 * All typed input is checked with validate.h before it reaches a module.
 */
#include <stdio.h>
#include <string.h>

#include "attendance.h"
#include "audit.h"
#include "auth.h"
#include "grade.h"
#include "record_service.h"
#include "report.h"
#include "validate.h"

#define USERS_FILE      "users.dat"
#define AUDIT_FILE      "audit.log"
#define ATTENDANCE_FILE "attendance.csv"
#define GRADES_FILE     "grades.csv"

static int input_closed = 0;   /* set when stdin hits end-of-file */

static void ask(const char *prompt, char *buf, size_t size)
{
    int ch;

    printf("%s", prompt);
    fflush(stdout);

    if (fgets(buf, (int)size, stdin) == NULL) {
        buf[0] = '\0';
        input_closed = 1;
        return;
    }

    if (strchr(buf, '\n') == NULL) {
        while ((ch = getchar()) != '\n' && ch != EOF) {
            /* discard the rest of an over-long line */
        }
    }

    buf[strcspn(buf, "\r\n")] = '\0';
}

static int ask_choice(void)
{
    char buf[32];
    char extra;
    int choice;

    ask("Enter choice: ", buf, sizeof buf);

    if (sscanf(buf, "%d %c", &choice, &extra) != 1)
        return -1;

    return choice;
}

/* Returns 1 and sets *status for P/A, otherwise 0. */
static int ask_status(AttendanceStatus *status)
{
    char buf[16];

    ask("Status (P = Present, A = Absent): ", buf, sizeof buf);

    if (strcmp(buf, "P") == 0 || strcmp(buf, "p") == 0) {
        *status = ATT_PRESENT;
        return 1;
    }
    if (strcmp(buf, "A") == 0 || strcmp(buf, "a") == 0) {
        *status = ATT_ABSENT;
        return 1;
    }

    printf("Invalid status. Enter P or A.\n");
    return 0;
}

/* Copies an already validated string into a fixed size field. */
static void copy_field(char *dst, size_t dst_size, const char *src)
{
    size_t n = strlen(src);

    if (n >= dst_size)
        n = dst_size - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static void do_attendance_entry(const Session *s, int update)
{
    char roll[ATT_ROLL_MAX + 8];
    char subject[ATT_SUBJECT_MAX + 8];
    char date[ATT_DATE_MAX + 8];
    AttendanceStatus status;
    AttResult r;

    ask("Student roll number: ", roll, sizeof roll);
    ask("Subject: ", subject, sizeof subject);
    ask("Date (YYYY-MM-DD): ", date, sizeof date);

    if (!ask_status(&status))
        return;

    r = update ? update_attendance(s, roll, subject, date, status)
               : mark_attendance(s, roll, subject, date, status);

    if (r == ATT_OK)
        printf(update ? "Attendance updated.\n" : "Attendance marked.\n");
    else
        printf("%s\n", attendance_result_message(r));
}

static void do_view_attendance(const Session *s, int use_own_roll)
{
    char roll[ATT_ROLL_MAX + 8];
    AttResult r;

    if (use_own_roll)
        snprintf(roll, sizeof roll, "%s", s->roll_no);
    else
        ask("Student roll number: ", roll, sizeof roll);

    r = attendance_view_student(s, roll);
    if (r != ATT_OK)
        printf("%s\n", attendance_result_message(r));
}

static void do_add_student(const Session *s)
{
    StudentRecord rec;
    char roll[32], name[96], dept[48], phone[32];

    ask("Roll number: ", roll, sizeof roll);
    ask("Name: ", name, sizeof name);
    ask("Department: ", dept, sizeof dept);
    ask("Phone: ", phone, sizeof phone);

    if (!validate_roll_no(roll) ||
        !validate_text(name, sizeof rec.name - 1) ||
        !validate_text(dept, sizeof rec.dept - 1) ||
        !validate_phone(phone)) {
        printf("Invalid input. Nothing was saved.\n");
        return;
    }

    memset(&rec, 0, sizeof rec);
    copy_field(rec.roll_no, sizeof rec.roll_no, roll);
    copy_field(rec.name, sizeof rec.name, name);
    copy_field(rec.dept, sizeof rec.dept, dept);
    copy_field(rec.phone, sizeof rec.phone, phone);

    if (add_student(s->username, &rec))
        printf("Student added.\n");
}

static void do_view_student(const Session *s, int use_own_roll)
{
    StudentRecord rec;
    char roll[32];

    if (use_own_roll)
        snprintf(roll, sizeof roll, "%s", s->roll_no);
    else
        ask("Roll number: ", roll, sizeof roll);

    if (!validate_roll_no(roll)) {
        printf("Invalid roll number.\n");
        return;
    }

    if (!auth_can_view_record(s, roll)) {
        printf("Access denied.\n");
        return;
    }

    if (!get_student(roll, &rec)) {
        printf("No such student.\n");
        return;
    }

    printf("\nRoll no   : %s\nName      : %s\nDepartment: %s\nPhone     : %s\n",
           rec.roll_no, rec.name, rec.dept, rec.phone);
}

static void do_create_login(void)
{
    char user[48], roll[32], pass[65];

    ask("New username: ", user, sizeof user);
    ask("Linked student roll number: ", roll, sizeof roll);

    if (!validate_text(user, 31) || !validate_roll_no(roll)) {
        printf("Invalid input.\n");
        return;
    }

    printf("Password: ");
    fflush(stdout);
    auth_read_password(pass, (int)sizeof pass);
    printf("\n");

    if (auth_add_user(user, pass, ROLE_STUDENT, roll) == 0)
        printf("Student login created.\n");
    else
        printf("Could not create login (duplicate user or invalid input).\n");

    memset(pass, 0, sizeof pass);
}

/* STU-F020/F022: record a grade, or update it if one already exists. */
static void do_enter_grade(const Session *s)
{
    char roll[32], subject[48], marks_s[16];
    int marks;
    GradeResult r;

    ask("Student roll number: ", roll, sizeof roll);
    ask("Subject: ", subject, sizeof subject);
    ask("Marks (0-100): ", marks_s, sizeof marks_s);

    if (!validate_roll_no(roll) || !validate_text(subject, 31) ||
        !validate_marks(marks_s, &marks)) {
        printf("Invalid input. Nothing was saved.\n");
        return;
    }
    {
        StudentRecord rec;
        if (!get_student(roll, &rec)) {
            printf("No such student.\n");
            return;
        }
    }

    r = record_grade(s, roll, subject, marks);
    if (r == GRADE_DUPLICATE)
        r = update_grade(s, roll, subject, marks);

    printf(r == GRADE_OK ? "Grade saved.\n" : "Could not save grade.\n");
}

/* STU-F040 */
static void do_search(const Session *s)
{
    StudentRecord found[50];
    char query[REPORT_QUERY_MAX + 8];
    int count = 0;
    ReportResult r;

    ask("Search (roll number or name): ", query, sizeof query);
    r = search_students(s, query, found, 50, &count);
    if (r != REPORT_OK)
        printf("%s\n", report_result_message(r));
    else
        report_print_students(stdout, found, count);
}

/* roll number for a report: admins type it, students use their own */
static void pick_roll(const Session *s, int use_own_roll, char *roll, size_t size)
{
    if (use_own_roll)
        snprintf(roll, size, "%s", s->roll_no);
    else
        ask("Student roll number: ", roll, size);
}

/* kind: 0 = grade sheet, 1 = attendance summary */
static void do_report(const Session *s, int kind, int use_own_roll)
{
    char roll[32];
    ReportResult r;

    pick_roll(s, use_own_roll, roll, sizeof roll);
    r = kind == 0 ? grade_sheet(s, roll, stdout)
                  : attendance_summary(s, roll, stdout);
    if (r != REPORT_OK)
        printf("%s\n", report_result_message(r));
}

/* STU-F042 */
static void do_export(const Session *s, int use_own_roll)
{
    char roll[32], path[96];
    ReportResult r;

    pick_roll(s, use_own_roll, roll, sizeof roll);
    snprintf(path, sizeof path, "report_%s.csv", roll);

    if (!validate_roll_no(roll)) {
        printf("Invalid roll number.\n");
        return;
    }
    r = export_report_csv(s, roll, path);
    if (r == REPORT_OK)
        printf("Exported to %s\n", path);
    else
        printf("%s\n", report_result_message(r));
}

static void admin_menu(Session *s)
{
    while (!input_closed) {
        printf("\n--- Admin menu (%s) ---\n", s->username);
        printf("1. Mark attendance\n");
        printf("2. Update attendance\n");
        printf("3. View student attendance\n");
        printf("4. Add student record\n");
        printf("5. View student record\n");
        printf("6. Create student login\n");
        printf("7. Enter / update grade\n");
        printf("8. Search students\n");
        printf("9. Grade sheet\n");
        printf("10. Attendance summary\n");
        printf("11. Export report (CSV)\n");
        printf("0. Logout\n");

        switch (ask_choice()) {
        case 0: return;
        case 1: do_attendance_entry(s, 0); break;
        case 2: do_attendance_entry(s, 1); break;
        case 3: do_view_attendance(s, 0); break;
        case 4: do_add_student(s); break;
        case 5: do_view_student(s, 0); break;
        case 6: do_create_login(); break;
        case 7: do_enter_grade(s); break;
        case 8: do_search(s); break;
        case 9: do_report(s, 0, 0); break;
        case 10: do_report(s, 1, 0); break;
        case 11: do_export(s, 0); break;
        default: printf("Invalid choice. Please select 0-11.\n");
        }
    }
}

static void student_menu(Session *s)
{
    while (!input_closed) {
        printf("\n--- Student menu (%s) ---\n", s->username);
        printf("1. View my record\n");
        printf("2. View my attendance\n");
        printf("3. My grade sheet\n");
        printf("4. My attendance summary\n");
        printf("5. Export my report (CSV)\n");
        printf("0. Logout\n");

        switch (ask_choice()) {
        case 0: return;
        case 1: do_view_student(s, 1); break;
        case 2: do_view_attendance(s, 1); break;
        case 3: do_report(s, 0, 1); break;
        case 4: do_report(s, 1, 1); break;
        case 5: do_export(s, 1); break;
        default: printf("Invalid choice. Please select 0-5.\n");
        }
    }
}

/* Returns 1 if logged in, 0 to quit (blank username, EOF or 3 failures). */
static int login(Session *s)
{
    char user[48], pass[65];
    int tries;

    for (tries = 0; tries < 3 && !input_closed; tries++) {
        ask("\nUsername (blank to exit): ", user, sizeof user);
        if (user[0] == '\0')
            return 0;

        printf("Password: ");
        fflush(stdout);
        auth_read_password(pass, (int)sizeof pass);
        printf("\n");

        switch (auth_login(user, pass, s)) {
        case AUTH_OK:
            memset(pass, 0, sizeof pass);
            printf("Login successful.\n");
            return 1;
        case AUTH_LOCKED:
            printf("Account locked after too many failed attempts.\n");
            break;
        case AUTH_INVALID_INPUT:
            printf("Invalid username or password format.\n");
            break;
        default:
            printf("Invalid username or password.\n");
        }
    }

    memset(pass, 0, sizeof pass);
    return 0;
}

int main(void)
{
    Session session;

    memset(&session, 0, sizeof session);

    printf("================================\n");
    printf("     STUDENT RECORD SYSTEM\n");
    printf("================================\n");

    if (auth_init(USERS_FILE, AUDIT_FILE) != 0) {
        printf("Error: cannot load users.\n");
        return 1;
    }
    audit_init(AUDIT_FILE);
    auth_ensure_default_admin();

    if (attendance_init(ATTENDANCE_FILE) != ATT_OK) {
        printf("Error: cannot initialise attendance storage.\n");
        return 1;
    }

    if (grade_init(GRADES_FILE) != 0) {
        printf("Error: cannot load grades.\n");
        return 1;
    }

    while (!input_closed && login(&session)) {
        if (session.role == ROLE_ADMIN)
            admin_menu(&session);
        else
            student_menu(&session);

        auth_logout(&session);
    }

    printf("Goodbye.\n");
    return 0;
}
