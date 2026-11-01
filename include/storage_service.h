#ifndef STORAGE_SERVICE_H
#define STORAGE_SERVICE_H

#include "record_service.h"
#include "auth.h"

bool storage_save(const StudentRecord *record);
bool storage_load(const char *roll_no, StudentRecord *out_record);
bool storage_update(const StudentRecord *record);
bool storage_delete(const char *roll_no);

/* STU-F040: copies up to max stored records into out. Returns the number copied (0 if no file). */
int  storage_list(StudentRecord *out, int max);

/* Issue #33: user table storage for Auth.
   One user per line: username,hash,role,roll_no,failed_attempts,locked */

/* Reads up to max users from path into out. A missing file gives 0 users.
   Malformed lines are skipped. Returns the number loaded, or -1 on bad arguments. */
int  storage_load_users(const char *path, User *out, int max);

/* Writes all users to path via a temp file + rename, so a crash never
   leaves a half-written users file. Returns true on success. */
bool storage_save_users(const char *path, const User *users, int count);

#endif