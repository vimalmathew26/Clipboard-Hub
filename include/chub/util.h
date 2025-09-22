#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void chub_log(const char *level, const char *fmt, ...);
unsigned long long chub_hash64(const char *s);
long long chub_now_millis(void);
int chub_mkdir_p(const char *path);
int chub_path_join(const char *a, const char *b, char *out, size_t out_sz);

#ifdef __cplusplus
}
#endif
