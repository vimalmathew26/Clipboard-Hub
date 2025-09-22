#include "chub/db.h"
#include "chub/util.h"
#include <sqlite3.h>
#include <windows.h>
#include <stdio.h>     // <-- added for snprintf
#include <string.h>
#include <stdlib.h>

static sqlite3 *G = NULL;
static CRITICAL_SECTION g_cs;

static int exec_sql(const char *sql) {
    char *err = NULL;
    int rc = sqlite3_exec(G, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        chub_log("DB", "SQL error: %s", err ? err : "(unknown)");
        sqlite3_free(err);
    }
    return rc;
}

int chub_db_open(const char *path) {
    if (G) return 0;
    InitializeCriticalSection(&g_cs);
    int rc = sqlite3_open(path, &G);
    if (rc != SQLITE_OK) {
        chub_log("DB", "open failed: %s", sqlite3_errmsg(G));
        sqlite3_close(G); G = NULL;
        return 1;
    }
    exec_sql("PRAGMA journal_mode=WAL;");
    exec_sql("PRAGMA synchronous=NORMAL;");
    const char *schema =
        "CREATE TABLE IF NOT EXISTS items ("
        " id INTEGER PRIMARY KEY,"
        " ts INTEGER NOT NULL,"
        " text TEXT NOT NULL,"
        " hash INTEGER NOT NULL,"
        " favorite INTEGER NOT NULL DEFAULT 0"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_items_ts ON items(ts DESC);"
        "CREATE INDEX IF NOT EXISTS idx_items_hash ON items(hash);";
    if (exec_sql(schema) != SQLITE_OK) return 2;
    return 0;
}

void chub_db_close(void) {
    if (!G) return;
    EnterCriticalSection(&g_cs);
    sqlite3_close(G);
    G = NULL;
    LeaveCriticalSection(&g_cs);
    DeleteCriticalSection(&g_cs);
}

int chub_db_insert(const char *text, unsigned long long h, long long ts) {
    if (!G || !text) return 1;
    EnterCriticalSection(&g_cs);
    const char *sql = "INSERT INTO items(ts,text,hash) VALUES(?,?,?)";
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(G, sql, -1, &st, NULL) != SQLITE_OK) { LeaveCriticalSection(&g_cs); return 2; }
    sqlite3_bind_int64(st, 1, (sqlite3_int64)ts);
    sqlite3_bind_text (st, 2, text, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st, 3, (sqlite3_int64)h);
    int rc = sqlite3_step(st);
    sqlite3_finalize(st);
    LeaveCriticalSection(&g_cs);
    return rc == SQLITE_DONE ? 0 : 3;
}

int chub_db_mark_favorite(int id, int fav) {
    if (!G) return 1;
    EnterCriticalSection(&g_cs);
    const char *sql = "UPDATE items SET favorite=? WHERE id=?";
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(G, sql, -1, &st, NULL) != SQLITE_OK) { LeaveCriticalSection(&g_cs); return 2; }
    sqlite3_bind_int(st, 1, fav ? 1 : 0);
    sqlite3_bind_int(st, 2, id);
    int rc = sqlite3_step(st);
    sqlite3_finalize(st);
    LeaveCriticalSection(&g_cs);
    return rc == SQLITE_DONE ? 0 : 3;
}

int chub_db_delete(int id) {
    if (!G) return 1;
    EnterCriticalSection(&g_cs);
    const char *sql = "DELETE FROM items WHERE id=?";
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(G, sql, -1, &st, NULL) != SQLITE_OK) { LeaveCriticalSection(&g_cs); return 2; }
    sqlite3_bind_int(st, 1, id);
    int rc = sqlite3_step(st);
    sqlite3_finalize(st);
    LeaveCriticalSection(&g_cs);
    return rc == SQLITE_DONE ? 0 : 3;
}

int chub_db_prune(int keep_limit) {
    if (!G || keep_limit <= 0) return 0;
    EnterCriticalSection(&g_cs);
    const char *sql =
        "DELETE FROM items WHERE id NOT IN ("
        "  SELECT id FROM items ORDER BY ts DESC LIMIT ?"
        ")";
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(G, sql, -1, &st, NULL) != SQLITE_OK) { LeaveCriticalSection(&g_cs); return 2; }
    sqlite3_bind_int(st, 1, keep_limit);
    int rc = sqlite3_step(st);
    sqlite3_finalize(st);
    LeaveCriticalSection(&g_cs);
    return rc == SQLITE_DONE ? 0 : 3;
}

static chub_item *alloc_items(int cap) {
    chub_item *arr = (chub_item*)calloc((size_t)cap, sizeof(chub_item));
    return arr;
}

static void fill_item(sqlite3_stmt *st, chub_item *dst) {
    dst->id       = sqlite3_column_int(st, 0);
    dst->ts       = (long long)sqlite3_column_int64(st, 1);
    const unsigned char *utxt = sqlite3_column_text(st, 2);
    const int fav = sqlite3_column_int(st, 3);
    const unsigned long long h = (unsigned long long)sqlite3_column_int64(st, 4);
    size_t len = utxt ? strlen((const char*)utxt) : 0;
    dst->text = (char*)malloc(len + 1);
    if (dst->text) {
        if (len) memcpy(dst->text, (const char*)utxt, len);
        dst->text[len] = '\0';
    }
    dst->favorite = fav;
    dst->h = h;
}

int chub_db_fetch_recent(int limit, chub_item **out_arr, int *out_count) {
    if (!G || !out_arr || !out_count || limit <= 0) return 1;
    *out_arr = NULL; *out_count = 0;
    EnterCriticalSection(&g_cs);
    const char *sql = "SELECT id,ts,text,favorite,hash FROM items ORDER BY ts DESC LIMIT ?";
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(G, sql, -1, &st, NULL) != SQLITE_OK) { LeaveCriticalSection(&g_cs); return 2; }
    sqlite3_bind_int(st, 1, limit);
    chub_item *arr = alloc_items(limit);
    int n = 0;
    int rc;
    while ((rc = sqlite3_step(st)) == SQLITE_ROW && n < limit) {
        fill_item(st, &arr[n++]);
    }
    sqlite3_finalize(st);
    LeaveCriticalSection(&g_cs);
    if (n == 0) { free(arr); return 0; }
    *out_arr = arr; *out_count = n;
    return 0;
}

int chub_db_search(const char *needle, int limit, chub_item **out_arr, int *out_count) {
    if (!G || !needle || !out_arr || !out_count || limit <= 0) return 1;
    *out_arr = NULL; *out_count = 0;
    EnterCriticalSection(&g_cs);
    const char *sql =
        "SELECT id,ts,text,favorite,hash FROM items "
        "WHERE text LIKE ? ESCAPE '\\' "
        "ORDER BY ts DESC LIMIT ?";
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(G, sql, -1, &st, NULL) != SQLITE_OK) { LeaveCriticalSection(&g_cs); return 2; }
    char pat[1024];
    snprintf(pat, sizeof(pat), "%%%s%%", needle);
    sqlite3_bind_text(st, 1, pat, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (st, 2, limit);
    chub_item *arr = alloc_items(limit);
    int n = 0, rc;
    while ((rc = sqlite3_step(st)) == SQLITE_ROW && n < limit) {
        fill_item(st, &arr[n++]);
    }
    sqlite3_finalize(st);
    LeaveCriticalSection(&g_cs);
    if (n == 0) { free(arr); return 0; }
    *out_arr = arr; *out_count = n;
    return 0;
}

void chub_db_free_items(chub_item *arr, int count) {
    if (!arr) return;
    for (int i = 0; i < count; ++i) free(arr[i].text);
    free(arr);
}
