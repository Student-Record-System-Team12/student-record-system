#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "storage_service.h"
#include "fileperm.h"

#define DATA_FILE "students.dat"

bool storage_save(const StudentRecord *record) {
    if (!record || record->roll_no[0] == '\0') {
        return false;
    }

    FILE *file = fopen(DATA_FILE, "rb");
    FILE *temp = fopen("temp.dat", "wb");
    
    if (!temp) {
        if (file) fclose(file);
        return false;
    }

    StudentRecord current;
    if (file) {
        while (fread(&current, sizeof(StudentRecord), 1, file) == 1) {
            if (current.roll_no[0] != '\0') {
                fwrite(&current, sizeof(StudentRecord), 1, temp);
            }
        }
        fclose(file);
    }

    if (fwrite(record, sizeof(StudentRecord), 1, temp) != 1) {
        fclose(temp);
        remove("temp.dat");
        return false;
    }

    fflush(temp);
    fclose(temp);

    remove(DATA_FILE);
    rename("temp.dat", DATA_FILE);
    fileperm_restrict(DATA_FILE);
    return true;
}

bool storage_load(const char *roll_no, StudentRecord *out_record) {
    if (!roll_no || !out_record) return false;
    
    FILE *file = fopen(DATA_FILE, "rb");
    if (!file) return false;
    
    StudentRecord temp;
    while (fread(&temp, sizeof(StudentRecord), 1, file) == 1) {
        if (temp.roll_no[0] != '\0' && strcmp(temp.roll_no, roll_no) == 0) {
            *out_record = temp;
            fclose(file);
            return true;
        }
    }
    
    fclose(file);
    return false;
}

bool storage_update(const StudentRecord *record) {
    if (!record || record->roll_no[0] == '\0') return false;
    
    FILE *file = fopen(DATA_FILE, "rb+");
    if (!file) return false;
    
    StudentRecord temp;
    while (fread(&temp, sizeof(StudentRecord), 1, file) == 1) {
        if (strcmp(temp.roll_no, record->roll_no) == 0) {
            fseek(file, -(long)sizeof(StudentRecord), SEEK_CUR);
            fwrite(record, sizeof(StudentRecord), 1, file);
            fclose(file);
            fileperm_restrict(DATA_FILE);
            return true;
        }
    }
    
    fclose(file);
    return false;
}

bool storage_delete(const char *roll_no) {
    if (!roll_no || roll_no[0] == '\0') return false;
    
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
    fileperm_restrict(DATA_FILE);
    return found;
}

int storage_list(StudentRecord *out, int max) {
    if (!out || max <= 0) return 0;

    FILE *file = fopen(DATA_FILE, "rb");
    if (!file) return 0;

    int n = 0;
    StudentRecord temp;
    while (n < max && fread(&temp, sizeof(StudentRecord), 1, file) == 1) {
        if (temp.roll_no[0] != '\0') {
            out[n++] = temp;
        }
    }

    fclose(file);
    return n;
}

/* ---------- Issue #33: user table storage (used by Auth) ---------- */

int storage_load_users(const char *path, User *out, int max) {
    if (!path || path[0] == '\0' || !out || max <= 0) return -1;

    FILE *f = fopen(path, "r");
    if (!f) return 0;   /* no file yet: empty user list */

    char line[256];
    int n = 0;
    while (n < max && fgets(line, sizeof line, f)) {
        User u;
        int role, failed, locked;
        memset(&u, 0, sizeof u);
        if (sscanf(line, "%31[^,],%64[^,],%d,%15[^,],%d,%d",
                   u.username, u.password_hash, &role, u.roll_no,
                   &failed, &locked) == 6) {
            u.role = (role == ROLE_ADMIN) ? ROLE_ADMIN : ROLE_STUDENT;
            u.failed_attempts = failed;
            u.locked = locked ? 1 : 0;
            out[n++] = u;
        }
    }
    fclose(f);
    return n;
}

bool storage_save_users(const char *path, const User *users, int count) {
    char tmp[300];
    if (!path || path[0] == '\0' || count < 0 || (count > 0 && !users)) return false;
    if (snprintf(tmp, sizeof tmp, "%s.tmp", path) >= (int)sizeof tmp) return false;

    FILE *f = fopen(tmp, "w");
    if (!f) return false;

    for (int i = 0; i < count; i++) {
        if (fprintf(f, "%s,%s,%d,%s,%d,%d\n",
                    users[i].username, users[i].password_hash, (int)users[i].role,
                    users[i].roll_no, users[i].failed_attempts, users[i].locked) < 0) {
            fclose(f);
            remove(tmp);
            return false;
        }
    }
    if (fclose(f) != 0) { remove(tmp); return false; }

    remove(path);                       /* Windows rename() fails if the target exists */
    if (rename(tmp, path) != 0) { remove(tmp); return false; }
    fileperm_restrict(path);
    return true;
}
