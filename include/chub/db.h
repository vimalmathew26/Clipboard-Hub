#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int id;
    long long ts;
    int favorite;
    char *text;            /* heap-allocated UTF-8 text */
    unsigned long long h;  /* hash of text */
} chub_item;

int chub_db_open(const char *path);
void chub_db_close(void);

int chub_db_insert(const char *text, unsigned long long h, long long ts);
int chub_db_mark_favorite(int id, int fav);
int chub_db_delete(int id);
int chub_db_prune(int keep_limit);
int chub_db_fetch_recent(int limit, chub_item **out_arr, int *out_count);
int chub_db_search(const char *needle, int limit, chub_item **out_arr, int *out_count);

void chub_db_free_items(chub_item *arr, int count);

#ifdef __cplusplus
}
#endif
