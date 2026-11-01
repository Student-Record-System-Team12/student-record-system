#include <sys/types.h>
#include <sys/stat.h>
#include "fileperm.h"

#ifdef _WIN32
#include <io.h>
#endif

int fileperm_restrict(const char *path)
{
    if (!path || path[0] == '\0') return -1;
#ifdef _WIN32
    return _chmod(path, _S_IREAD | _S_IWRITE) == 0 ? 0 : -1;
#else
    return chmod(path, S_IRUSR | S_IWUSR) == 0 ? 0 : -1;
#endif
}

void fileperm_init(void)
{
#ifndef _WIN32
    umask(S_IRWXG | S_IRWXO);   /* 077: no group/other bits on new files */
#endif
}
