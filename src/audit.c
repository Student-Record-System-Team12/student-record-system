#include <stdio.h>
#include <time.h>
#include "audit.h"

#define AUDIT_FILE "audit.log"

bool audit_log(const char *username, AuditAction action, const char *target_roll_no) {
    FILE *file = fopen(AUDIT_FILE, "a"); // "a" appends text to the log
    if (!file) return false;

    // Get current time
    time_t now = time(NULL);
    char *date_str = ctime(&now);
    date_str[24] = '\0'; // Remove the trailing newline added by ctime

    const char *action_str;
    switch(action) {
        case AUDIT_ACTION_CREATE: action_str = "CREATE"; break;
        case AUDIT_ACTION_UPDATE: action_str = "UPDATE"; break;
        case AUDIT_ACTION_DELETE: action_str = "DELETE"; break;
        default: action_str = "UNKNOWN";
    }

    fprintf(file, "[%s] User: %s | Action: %s | Target Roll No: %s\n", 
            date_str, username, action_str, target_roll_no);
    
    fclose(file);
    return true;
}