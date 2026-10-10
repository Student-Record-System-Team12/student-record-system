#ifndef STORAGE_H
#define STORAGE_H

#include "record.h"
#include <stdbool.h>

// Initialize the storage (e.g., open file or connect to DB)
bool storage_init(const char *filepath);

// CRUD Operations
bool storage_save(const StudentRecord *record);
bool storage_load(const char *roll_no, StudentRecord *out_record);
bool storage_update(const char *roll_no, const StudentRecord *updated_record);
bool storage_delete(const char *roll_no);

void storage_close(void);

#endif