#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "unity.h"
#include "audit.h"

/* TC-Sec-04 (STU-SR003, AC-SEC-003): audit log format and append behaviour. */

#define TA_LOG "tsec_audit.log"

static void read_file(const char *path, char *buf, size_t n)
{
    size_t r;
    FILE *f = fopen(path, "r");
    buf[0] = '\0';
    if (!f) return;
    r = fread(buf, 1, n - 1, f);
    buf[r] = '\0';
    fclose(f);
}

static int count_lines(const char *s)
{
    int n = 0;
    for (; *s; s++) if (*s == '\n') n++;
    return n;
}

void setUp(void)
{
    remove(TA_LOG);
    audit_init(TA_LOG);
}

void tearDown(void)
{
    remove(TA_LOG);
}

/* Format: YYYY-MM-DD HH:MM:SS | module | user | action */
void test_entry_has_timestamp_module_user_action(void)
{
    char buf[512];
    int y, mo, d, h, mi, s, n = 0;
    TEST_ASSERT_EQUAL_INT(0, audit_log_module("STUDENT", "admin", "CREATE student PES001"));
    read_file(TA_LOG, buf, sizeof buf);

    TEST_ASSERT_EQUAL_INT(6, sscanf(buf, "%4d-%2d-%2d %2d:%2d:%2d%n", &y, &mo, &d, &h, &mi, &s, &n));
    TEST_ASSERT_EQUAL_STRING(" | STUDENT | admin | CREATE student PES001\n", buf + n);
}

void test_two_argument_form_uses_app_module(void)
{
    char buf[512];
    TEST_ASSERT_EQUAL_INT(0, audit_log("admin", "SOMETHING"));
    read_file(TA_LOG, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(strstr(buf, " | APP | admin | SOMETHING\n"));
}

void test_entries_are_appended_in_order(void)
{
    char buf[1024];
    char *a, *b;
    audit_log_module("GRADES", "admin", "FIRST");
    audit_log_module("GRADES", "admin", "SECOND");
    read_file(TA_LOG, buf, sizeof buf);
    TEST_ASSERT_EQUAL_INT(2, count_lines(buf));
    a = strstr(buf, "FIRST");
    b = strstr(buf, "SECOND");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_TRUE(a < b);
}

void test_existing_entries_are_never_changed(void)
{
    char before[1024], after[1024];
    audit_log_module("STUDENT", "admin", "OLD ENTRY");
    read_file(TA_LOG, before, sizeof before);
    audit_log_module("STUDENT", "admin", "NEW ENTRY");
    read_file(TA_LOG, after, sizeof after);
    TEST_ASSERT_EQUAL_INT(0, strncmp(after, before, strlen(before)));
    TEST_ASSERT_TRUE(strlen(after) > strlen(before));
}

void test_reinit_does_not_truncate_log(void)
{
    char buf[1024];
    audit_log_module("STUDENT", "admin", "BEFORE RESTART");
    audit_init(TA_LOG);                      /* simulate restarting the program */
    audit_log_module("STUDENT", "admin", "AFTER RESTART");
    read_file(TA_LOG, buf, sizeof buf);
    TEST_ASSERT_EQUAL_INT(2, count_lines(buf));
    TEST_ASSERT_NOT_NULL(strstr(buf, "BEFORE RESTART"));
}

/* AC-SEC-003: one entry per write operation */
void test_one_entry_per_call(void)
{
    char buf[4096];
    int i;
    for (i = 0; i < 10; i++) audit_log_module("ATTENDANCE", "admin", "WRITE attendance");
    read_file(TA_LOG, buf, sizeof buf);
    TEST_ASSERT_EQUAL_INT(10, count_lines(buf));
}

/* A value containing a newline must not forge a second entry or extra fields */
void test_newline_and_separator_injection_is_neutralised(void)
{
    char buf[1024];
    audit_log_module("STUDENT", "admin", "X\n2026-01-01 00:00:00 | AUTH | root | LOGIN_SUCCESS");
    read_file(TA_LOG, buf, sizeof buf);
    TEST_ASSERT_EQUAL_INT(1, count_lines(buf));
    TEST_ASSERT_NULL(strstr(buf, "| root |"));
}

void test_null_or_empty_user_is_logged_as_dash(void)
{
    char buf[512];
    audit_log_module("AUTH", NULL, "LOGIN_FAILED");
    audit_log_module("AUTH", "", "LOGIN_FAILED");
    read_file(TA_LOG, buf, sizeof buf);
    TEST_ASSERT_EQUAL_INT(2, count_lines(buf));
    TEST_ASSERT_NOT_NULL(strstr(buf, " | AUTH | - | LOGIN_FAILED\n"));
}

void test_init_rejects_bad_path(void)
{
    TEST_ASSERT_EQUAL_INT(-1, audit_init(NULL));
    TEST_ASSERT_EQUAL_INT(-1, audit_init(""));
}

void test_unwritable_path_returns_error(void)
{
    audit_init("no_such_dir/xyz/audit.log");
    TEST_ASSERT_EQUAL_INT(-1, audit_log_module("STUDENT", "admin", "X"));
    audit_init(TA_LOG);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_entry_has_timestamp_module_user_action);
    RUN_TEST(test_two_argument_form_uses_app_module);
    RUN_TEST(test_entries_are_appended_in_order);
    RUN_TEST(test_existing_entries_are_never_changed);
    RUN_TEST(test_reinit_does_not_truncate_log);
    RUN_TEST(test_one_entry_per_call);
    RUN_TEST(test_newline_and_separator_injection_is_neutralised);
    RUN_TEST(test_null_or_empty_user_is_logged_as_dash);
    RUN_TEST(test_init_rejects_bad_path);
    RUN_TEST(test_unwritable_path_returns_error);
    return UNITY_END();
}
