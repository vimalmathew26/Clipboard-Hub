#include "chub/platform.h"
#include "chub/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define POPEN  _popen
#define PCLOSE _pclose
#else
#define POPEN  popen
#define PCLOSE pclose
#endif

/* fire-and-forget; minimal */
int chub_run_command(const char *cmd) {
    if (!cmd) return -1;
    return system(cmd);
}

/* capture stdout of a command into UTF-8 buffer (caller-provided) */
int chub_run_capture_stdout(const char *cmd, char *buf, size_t buf_sz) {
    if (!cmd || !buf || buf_sz == 0) return -1;
    FILE *p = POPEN(cmd, "rb");
    if (!p) return -1;
    size_t off = 0;
    while (!feof(p)) {
        if (off + 4096 >= buf_sz) break;
        size_t r = fread(buf + off, 1, 4096, p);
        off += r;
    }
    buf[off < buf_sz ? off : buf_sz-1] = '\0';
    int rc = PCLOSE(p);
    /* normalize CRLF to LF */
    for (size_t i = 0; buf[i]; ++i) {
        if (buf[i] == '\r' && buf[i+1] == '\n') {
            memmove(&buf[i], &buf[i+1], strlen(&buf[i+1]) + 1);
        }
    }
    return rc == 0 ? 0 : 1;
}

/* pipe UTF-8 stdin to a command */
int chub_run_pipe_stdin(const char *cmd, const char *data, size_t len) {
    if (!cmd) return -1;
    FILE *p = POPEN(cmd, "wb");
    if (!p) return -1;
    if (data && len) {
        size_t w = fwrite(data, 1, len, p);
        (void)w;
    }
    int rc = PCLOSE(p);
    return rc == 0 ? 0 : 1;
}
