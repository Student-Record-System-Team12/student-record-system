#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "storage_service.h"

#define DATA_FILE "students.dat"

bool storage_save(const StudentRecord *record) {
    if (!record || record->roll_no[0] == '\0') return false;
    
    FILE *file = fopen(DATA_FILE, "ab");
    if (!file) return false;
    
    size_t written = fwrite(record, sizeof(StudentRecord), 1, file);
    fclose(file);
    return written == 1;
}

bool storage_load(const char *roll_no, StudentRecord *out_record) {
    if (!roll_no || !out_record) return false;
    
    FILE *file = fopen(DATA_FILE, "rb");
    if (!file) return false;
    
    StudentRecord temp;
    while (fread(&temp, sizeof(StudentRecord), 1, file) == 1) {
        if (strcmp(temp.roll_no, roll_no) == 0) {
            *out_record = temp;
            fclose(file);
            return true;
        }
    }
    
    fclose(file);
    return false;
}

bool storage_update(const StudentRecord *record) {
    if (!record) return false;
    
    FILE *file = fopen(DATA_FILE, "rb+");
    if (!file) return false;
    
    StudentRecord temp;
    while (fread(&temp, sizeof(StudentRecord), 1, file) == 1) {
        if (strcmp(temp.roll_no, record->roll_no) == 0) {
            fseek(file, -(long)sizeof(StudentRecord), SEEK_CUR);
            fwrite(record, sizeof(StudentRecord), 1, file);
            fclose(file);
            return true;
        }
    }
    
    fclose(file);
    return false;
}

bool storage_delete(const char *roll_no) {
    if (!roll_no) return false;
    
    FILE *file = fopen(DATA_FILE, "rb");
    if (!file) return false;
    
    FILE *temp_file = fopen("temp.dat", "wb");
    if (!temp_file) {
        fclose(file);
        return false;
    }
    
    StudentRecord temp;
    bool found = false;
    while (fread(&temp, sizeof(StudentRecord), 1, file) == 1) {
        if (strcmp(temp.roll_no, roll_no) == 0) {
            found = true;
            continue;
        }
        fwrite(&temp, sizeof(StudentRecord), 1, temp_file);
    }
    
    fclose(file);
    fclose(temp_file);
    remove(DATA_FILE);
    rename("temp.dat", DATA_FILE);
    return found;
}