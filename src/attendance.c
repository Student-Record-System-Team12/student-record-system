#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "attendance.h"

#define MAX_ATTENDANCE_RECORDS 10000

static char attendance_path[260] = "";

static int leap_year(int year)
{
    return year % 400 == 0 ||
           (year % 4 == 0 && year % 100 != 0);
}

int attendance_valid_roll_no(const char *roll_no)
{
    size_t i;
    size_t len;

    if (roll_no == NULL)
        return 0;

    len = strlen(roll_no);

    if (len == 0 || len >= ATTENDANCE_ROLL_MAX)
        return 0;

    for (i = 0; i < len; i++) {
        unsigned char ch = (unsigned char)roll_no[i];

        if (!isalnum(ch) && ch != '-' && ch != '_')
            return 0;
    }

    return 1;
}

int attendance_valid_date(const char *date)
{
    int year, month, day;
    int days[] = {31, 28, 31, 30, 31, 30,
                  31, 31, 30, 31, 30, 31};

    if (date == NULL || strlen(date) != 10)
        return 0;

    if (date[4] != '-' || date[7] != '-')
        return 0;

    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7)
            continue;

        if (!isdigit((unsigned char)date[i]))
            return 0;
    }

    year = (date[0] - '0') * 1000 +
           (date[1] - '0') * 100 +
           (date[2] - '0') * 10 +
           date[3] - '0';

    month = (date[5] - '0') * 10 + date[6] - '0';
    day = (date[8] - '0') * 10 + date[9] - '0';

    if (year < 1 || month < 1 || month > 12)
        return 0;

    if (leap_year(year))
        days[1] = 29;

    return day >= 1 && day <= days[month - 1];
}

int attendance_init(const char *file_path)
{
    FILE *file;

    if (file_path == NULL || file_path[0] == '\0')
        return -1;

    if (strlen(file_path) >= sizeof attendance_path)
        return -1;

    file = fopen(file_path, "a");

    if (file == NULL)
        return -1;

    if (fclose(file) != 0)
        return -1;

    snprintf(attendance_path, sizeof attendance_path,
             "%s", file_path);

    return 0;
}

int attendance_mark(const Session *session,
                    const char *roll_no,
                    const char *date,
                    AttendanceStatus status)
{
    FILE *file;
    char line[128];
    char old_roll[ATTENDANCE_ROLL_MAX];
    char old_date[ATTENDANCE_DATE_MAX];
    int old_status;
    int count = 0;

    if (!auth_can_modify(session))
        return -1;

    if (!attendance_valid_roll_no(roll_no) ||
        !attendance_valid_date(date))
        return -1;

    if (status != ATTENDANCE_PRESENT &&
        status != ATTENDANCE_ABSENT)
        return -1;

    if (attendance_path[0] == '\0')
        return -1;

    file = fopen(attendance_path, "r");

    if (file == NULL)
        return -1;

    while (fgets(line, sizeof line, file) != NULL) {
        count++;

        if (sscanf(line, "%15[^,],%10[^,],%d",
                   old_roll, old_date, &old_status) != 3) {
            fclose(file);
            return -1;
        }

        if (strcmp(old_roll, roll_no) == 0 &&
            strcmp(old_date, date) == 0) {
            fclose(file);
            return -1;
        }

        if (count >= MAX_ATTENDANCE_RECORDS) {
            fclose(file);
            return -1;
        }
    }

    if (ferror(file)) {
        fclose(file);
        return -1;
    }

    fclose(file);

    file = fopen(attendance_path, "a");

    if (file == NULL)
        return -1;

    if (fprintf(file, "%s,%s,%d\n",
                roll_no, date, (int)status) < 0) {
        fclose(file);
        return -1;
    }

    if (fclose(file) != 0)
        return -1;

    return 0;
}

int attendance_view_student(const char *roll_no)
{
    FILE *file;
    char line[128];
    char stored_roll[ATTENDANCE_ROLL_MAX];
    char stored_date[ATTENDANCE_DATE_MAX];
    int status;
    int found = 0;

    if (!attendance_valid_roll_no(roll_no) ||
        attendance_path[0] == '\0')
        return -1;

    file = fopen(attendance_path, "r");

    if (file == NULL)
        return -1;

    printf("\nAttendance for %s\n", roll_no);
    printf("Date        Status\n");
    printf("------------------\n");

    while (fgets(line, sizeof line, file) != NULL) {
        if (sscanf(line, "%15[^,],%10[^,],%d",
                   stored_roll, stored_date, &status) != 3) {
            fclose(file);
            return -1;
        }

        if (strcmp(stored_roll, roll_no) == 0) {
            const char *label =
                status == ATTENDANCE_PRESENT ? "PRESENT" :
                status == ATTENDANCE_ABSENT ? "ABSENT" : "INVALID";

            printf("%s  %s\n", stored_date, label);
            found = 1;
        }
    }

    if (ferror(file)) {
        fclose(file);
        return -1;
    }

    fclose(file);

    if (!found)
        printf("No attendance records found.\n");

    return 0;
}
