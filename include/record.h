#ifndef RECORD_H
#define RECORD_H

#define MAX_NAME_LEN 50
#define MAX_DEPT_LEN 30
#define MAX_CONTACT_LEN 15
#define MAX_ROLL_LEN 15

typedef struct {
    char roll_no[MAX_ROLL_LEN];
    char name[MAX_NAME_LEN];
    char department[MAX_DEPT_LEN];
    char contact[MAX_CONTACT_LEN];
} StudentRecord;

#endif // RECORD_H