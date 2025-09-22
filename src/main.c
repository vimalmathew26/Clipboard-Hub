#include "chub/tui.h"
#include "chub/db.h"
#include "chub/clip.h"
#include "chub/util.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <process.h>

#ifndef CHUB_VERSION
#define CHUB_VERSION "0.1.0"
#endif

static volatile LONG g_stop = 0;
static int g_retention = 500;
static int g_interval_ms = 500;
static char g_db_path[MAX_PATH * 4];

typedef struct {
    unsigned long long last_h;
    int initialized;
} poll_state;

static void compute_default_db_path(char out[], size_t out_sz) {
    const char *base = getenv("LOCALAPPDATA");
    if (!base) base = ".";

    char dir[MAX_PATH * 4];
    if (chub_path_join(base, "ClipboardHub", dir, sizeof(dir)) != 0) {
        /* fallback to cwd */
        strncpy(out, "chub.db", out_sz - 1);
        out[out_sz - 1] = '\0';
        return;
    }
    if (chub_mkdir_p(dir) != 0) {
        /* fallback to cwd if we couldn't create %LOCALAPPDATA%\ClipboardHub */
        strncpy(out, "chub.db", out_sz - 1);
        out[out_sz - 1] = '\0';
        return;
    }
    chub_path_join(dir, "chub.db", out, out_sz);
}


static void normalize_newlines(char *s) {
    if (!s) return;
    size_t w = 0;
    for (size_t r = 0; s[r]; ++r) {
        if (s[r] == '\r' && s[r+1] == '\n') continue;
        s[w++] = s[r];
    }
    s[w] = '\0';
}

static int is_only_whitespace(const char *s) {
    if (!s) return 1;
    for (; *s; ++s) if ((unsigned char)*s > ' ') return 0;
    return 1;
}

static void notify_tui_refresh(void) {
    extern void chub__tui__request_refresh__export(void);
    chub__tui__request_refresh__export();
}

static unsigned __stdcall poller_thread(void *arg) {
    (void)arg;
    poll_state st = {0, 0};
    char buf[64 * 1024];
    while (!InterlockedCompareExchange(&g_stop, 0, 0)) {
        buf[0] = '\0';
        if (chub_clip_read(buf, sizeof(buf)) == 0) {
            normalize_newlines(buf);
            if (!is_only_whitespace(buf)) {
                unsigned long long h = chub_hash64(buf);
                if (!st.initialized || h != st.last_h) {
                    long long ts = chub_now_millis();
                    if (chub_db_insert(buf, h, ts) == 0) {
                        chub_db_prune(g_retention);
                        notify_tui_refresh();
                        st.last_h = h; st.initialized = 1;
                    }
                }
            }
        }
        Sleep((DWORD)g_interval_ms);
    }
    return 0;
}

static void usage(const char *exe) {
    printf("Usage: %s [--version] [--db PATH] [--retention N] [--interval MS]\n", exe);
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--version") == 0) {
        printf("ClipboardHub %s\n", CHUB_VERSION);
        return 0;
    }

    compute_default_db_path(g_db_path, sizeof(g_db_path));

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--db") == 0 && i+1 < argc) {
            strncpy(g_db_path, argv[++i], sizeof(g_db_path)-1);
            g_db_path[sizeof(g_db_path)-1] = '\0';
        } else if (strcmp(argv[i], "--retention") == 0 && i+1 < argc) {
            g_retention = atoi(argv[++i]); if (g_retention <= 0) g_retention = 500;
        } else if (strcmp(argv[i], "--interval") == 0 && i+1 < argc) {
            g_interval_ms = atoi(argv[++i]); if (g_interval_ms < 100) g_interval_ms = 100;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage(argv[0]); return 0;
        }
    }

    if (chub_db_open(g_db_path) != 0) {
        chub_log("ERR", "Failed to open DB at %s", g_db_path);
        return 1;
    }

    HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, poller_thread, NULL, 0, NULL);
    if (!hThread) {
        chub_log("ERR", "Failed to start poller thread");
        chub_db_close();
        return 1;
    }

    int rc = chub_tui_mainloop();

    InterlockedExchange(&g_stop, 1);
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    chub_db_close();
    return rc;
}
