#include "chub/util.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <direct.h>
#include <errno.h>

void chub_log(const char *level, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[%s] ", level ? level : "LOG");
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
}

unsigned long long chub_hash64(const char *s) {
    /* FNV-1a 64-bit */
    const unsigned long long FNV_OFFSET = 1469598103934665603ULL;
    const unsigned long long FNV_PRIME  = 1099511628211ULL;
    unsigned long long h = FNV_OFFSET;
    if (!s) return h;
    for (const unsigned char *p = (const unsigned char*)s; *p; ++p) {
        h ^= (unsigned long long)(*p);
        h *= FNV_PRIME;
    }
    return h;
}

long long chub_now_millis(void) {
    /* Windows epoch: use FILETIME -> Unix epoch */
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER uli;
    uli.LowPart  = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    // 100-ns intervals since Jan 1, 1601
    unsigned long long t100ns = uli.QuadPart;
    // Unix epoch offset
    const unsigned long long EPOCH_DIFF_100NS = 116444736000000000ULL;
    unsigned long long unix100ns = t100ns - EPOCH_DIFF_100NS;
    return (long long)(unix100ns / 10000ULL);
}

static int _mkdir_one(const char *path) {
    if (_mkdir(path) == 0) return 0;
    if (errno == EEXIST)   return 0;
    return -1;
}

int chub_mkdir_p(const char *path) {
    if (!path || !*path) return -1;

    char tmp[MAX_PATH * 4];
    size_t len = strlen(path);
    if (len >= sizeof(tmp)) return -1;

    /* normalize to backslashes */
    strcpy(tmp, path);
    for (size_t i = 0; i < len; ++i) if (tmp[i] == '/') tmp[i] = '\\';

    /* start after drive root like "C:\" or skip UNC host/share */
    size_t i = 0;
    if (len >= 3 && isalpha((unsigned char)tmp[0]) && tmp[1] == ':' && (tmp[2] == '\\')) {
        i = 3; /* skip "C:\" */
    } else if (len >= 2 && tmp[0] == '\\' && tmp[1] == '\\') {
        /* UNC path: \\server\share\... -> skip server\share */
        int backslashes = 0;
        i = 2;
        while (i < len && backslashes < 2) {
            if (tmp[i] == '\\') backslashes++;
            i++;
        }
    }

    /* create each component */
    for (; i < len; ++i) {
        if (tmp[i] == '\\') {
            char save = tmp[i];
            tmp[i] = '\0';
            if (strlen(tmp) > 0) {
                if (!CreateDirectoryA(tmp, NULL)) {
                    DWORD err = GetLastError();
                    if (err != ERROR_ALREADY_EXISTS) {
                        tmp[i] = save;
                        return -1;
                    }
                }
            }
            tmp[i] = save;
        }
    }
    /* final leaf */
    if (!CreateDirectoryA(tmp, NULL)) {
        DWORD err = GetLastError();
        if (err != ERROR_ALREADY_EXISTS) return -1;
    }
    return 0;
}


int chub_path_join(const char *a, const char *b, char *out, size_t out_sz) {
    if (!a || !b || !out || out_sz == 0) return -1;
    size_t na = strlen(a), nb = strlen(b);
    int need_slash = (na > 0 && a[na-1] != '\\' && a[na-1] != '/');
    size_t total = na + (need_slash ? 1 : 0) + nb + 1;
    if (total > out_sz) return -1;
    strcpy(out, a);
    if (need_slash) strcat(out, "\\");
    strcat(out, b);
    return 0;
}
