#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* returns 0 on success; buf filled with UTF-8 text (may be empty string) */
int chub_clip_read(char *buf, size_t buf_sz);
/* returns 0 on success */
int chub_clip_write(const char *data);

/* optional */
int chub_clip_smoketest(void);

#ifdef __cplusplus
}
#endif
