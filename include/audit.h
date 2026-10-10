#ifndef AUDIT_H
#define AUDIT_H

#include <stdbool.h>


typedef enum {
    AUDIT_ACTION_CREATE,
    AUDIT_ACTION_UPDATE,
    AUDIT_ACTION_DELETE
} AuditAction;

// Log an action to the audit trail
bool audit_log(const char *username, AuditAction action, const char *target_roll_no);

#endif