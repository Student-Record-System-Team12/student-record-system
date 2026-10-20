#include <stdio.h>
#include <string.h>
#include <time.h>
#include "audit.h"

#define AUDIT_PATH_MAX   260
#define AUDIT_FIELD_MAX  128

static char log_path[AUDIT_PATH_MAX] = "";

/* Copy src into dst, replacing characters that could forge a second entry or a fake field:
   CR/LF become a space and '|' becomes '/'. NULL or empty input becomes "-". */
static void sanitise(char *dst, size_t dst_size, const char *src)
{
    size_t i = 0;
    if (!src || src[0] == '\0') src = "-";
    for (; *src && i + 1 < dst_size; src++, i++) {
        char c = *src;
        if (c == '\r' || c == '\n') c = ' ';
        else if (c == '|')          c = '/';
        dst[i] = c;
    }
    dst[i] = '\0';
}

int audit_init(const char *log_file)
{
    if (!log_file || log_file[0] == '\0') return -1;
    if (strlen(log_file) >= sizeof log_path) return -1;
    snprintf(log_path, sizeof log_path, "%s", log_file);
    return 0;
}

int audit_log_module(const char *module, const char *user, const char *action)
{
    FILE *f;
    char ts[32];
    char m[AUDIT_FIELD_MAX], u[AUDIT_FIELD_MAX], a[AUDIT_FIELD_MAX];
    time_t now = time(NULL);

    if (log_path[0] == '\0') return -1;

    sanitise(m, sizeof m, module);
    sanitise(u, sizeof u, user);
    sanitise(a, sizeof a, action);

    f = fopen(log_path, "a");            /* "a": every write goes to the end, never overwrites */
    if (!f) return -1;
    strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", localtime(&now));
    if (fprintf(f, "%s | %s | %s | %s\n", ts, m, u, a) < 0) { fclose(f); return -1; }
    return fclose(f) == 0 ? 0 : -1;
}

int audit_log(const char *user, const char *action)
{
    return audit_log_module("APP", user, action);
}
