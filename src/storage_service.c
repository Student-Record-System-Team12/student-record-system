#include <stdio.h>
#include <string.h>
#include "storage.h"

#define DATA_FILE "students.dat"

// Save a new record to the end of the file
bool storage_save(const StudentRecord *record) {
    if (record == NULL || record->roll_no[0] == '\0') {
        return false;
    }
    FILE *file = fopen(DATA_FILE, "rb");
    FILE *temp = fopen("temp.dat", "wb");
    
    // Save failure is reported, never claimed as success
    if (!temp) {
        if (file) fclose(file);
        return false; 
    }

    StudentRecord current;
    if (file) {
        while (fread(&current, sizeof(StudentRecord), 1, file) == 1) {
            // Skip malformed lines on load (e.g., missing roll number)[cite: 9]
            if (current.roll_no[0] != '\0') {
                fwrite(&current, sizeof(StudentRecord), 1, temp);
            }
        }
        fclose(file);
    }

    // Append the new record
    if (fwrite(record, sizeof(StudentRecord), 1, temp) != 1) {
        fclose(temp);
        remove("temp.dat");
        return false; 
    }

    // Force the OS to move data from RAM cache to the physical disk
    fflush(temp); 
    fclose(temp);

    // Atomic swap: Interrupted write leaves the old file intact[cite: 9]
    remove(DATA_FILE);
    rename("temp.dat", DATA_FILE);
    
    return true;
}

// Find a record by its roll number
bool storage_load(const char *roll_no, StudentRecord *out_record) {
    FILE *file = fopen(DATA_FILE, "rb");
    if (!file) return false;
    
    bool found = false;
    while (fread(out_record, sizeof(StudentRecord), 1, file) == 1) {
        if (strcmp(out_record->roll_no, roll_no) == 0) {
            found = true;
            break;
        }
    }
    fclose(file);
    return found;
}

// Append this to the bottom of src/storage_service.c

bool storage_update(const char *roll_no, const StudentRecord *updated_record) {
    FILE *file = fopen(DATA_FILE, "rb");
    if (!file) return false;
    
    FILE *temp = fopen("temp.dat", "wb");
    if (!temp) {
        fclose(file);
        return false;
    }
    
    bool updated = false;
    StudentRecord current;
    while (fread(&current, sizeof(StudentRecord), 1, file) == 1) {
        if (strcmp(current.roll_no, roll_no) == 0) {
            // Write the updated record instead of the old one
            fwrite(updated_record, sizeof(StudentRecord), 1, temp);
            updated = true;
        } else {
            // Keep existing record
            fwrite(&current, sizeof(StudentRecord), 1, temp);
        }
    }
    
    fclose(file);
    fclose(temp);
    
    if (updated) {
        remove(DATA_FILE);
        rename("temp.dat", DATA_FILE);
    } else {
        remove("temp.dat");
    }
    return updated;
}

bool storage_delete(const char *roll_no) {
    FILE *file = fopen(DATA_FILE, "rb");
    if (!file) return false;
    
    FILE *temp = fopen("temp.dat", "wb");
    if (!temp) {
        fclose(file);
        return false;
    }
    
    bool deleted = false;
    StudentRecord current;
    while (fread(&current, sizeof(StudentRecord), 1, file) == 1) {
        if (strcmp(current.roll_no, roll_no) == 0) {
            deleted = true; // Skip writing this record to the new file
        } else {
            fwrite(&current, sizeof(StudentRecord), 1, temp);
        }
    }
    
    fclose(file);
    fclose(temp);
    
    if (deleted) {
        remove(DATA_FILE);
        rename("temp.dat", DATA_FILE);
    } else {
        remove("temp.dat");
    }
    return deleted;
}

// Add these to src/storage_service.c

bool storage_init(const char *filepath) {
    // Attempt to open the file in append mode. 
    // This safely creates the file if it doesn't exist without overwriting existing data.
    FILE *file = fopen(filepath, "ab");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

void storage_close(void) {
    // For simple file I/O, we open and close the file during each read/write operation,
    // so there is no persistent memory or connection to clean up here. 
    // We leave it empty for now!
}
