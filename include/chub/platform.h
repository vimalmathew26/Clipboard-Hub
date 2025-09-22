#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int chub_run_command(const char *cmd);
int chub_run_capture_stdout(const char *cmd, char *buf, size_t buf_sz);
/* data may be UTF-8 text */
int chub_run_pipe_stdin(const char *cmd, const char *data, size_t len);

#ifdef __cplusplus
}
#endif
