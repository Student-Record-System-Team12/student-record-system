#ifndef FILEPERM_H
#define FILEPERM_H

/*
 * STU-SR005: data files must not be world-readable or world-writable.
 * Sets owner read/write only (0600) on POSIX. On Windows there are no
 * Unix permission bits: files inherit the NTFS ACL of the user's folder,
 * so this only makes sure the file is not marked read-only.
 * Returns 0 on success, -1 on bad argument or failure.
 */
int fileperm_restrict(const char *path);

/* Call once at startup: files the program creates get 0600 from the start (POSIX). */
void fileperm_init(void);

#endif
