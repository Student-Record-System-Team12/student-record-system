#ifndef AUDIT_H
#define AUDIT_H

/*
 * STU-SR003 (SRS 5.1.2): append-only audit log.
 * Each entry is one line:   timestamp | module | user | action
 * There is deliberately no function to read, edit or delete entries.
 */

/* Set the log file. Existing content is never truncated. Returns 0 on success, -1 on bad argument. */
int audit_init(const char *log_file);

/* Append one entry with an explicit module name (e.g. "STUDENT", "GRADES", "ATTENDANCE", "AUTH").
   Returns 0 if the entry was written, -1 otherwise (not initialised, bad argument, or I/O error). */
int audit_log_module(const char *module, const char *user, const char *action);

/* Two-argument form from the SRS: logs under module "APP". */
int audit_log(const char *user, const char *action);

#endif
