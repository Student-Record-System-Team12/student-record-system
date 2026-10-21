#ifndef STORAGE_SERVICE_H
#define STORAGE_SERVICE_H

#include "record_service.h"

bool storage_save(const StudentRecord *record);
bool storage_load(const char *roll_no, StudentRecord *out_record);
bool storage_update(const StudentRecord *record);
bool storage_delete(const char *roll_no);

#endif