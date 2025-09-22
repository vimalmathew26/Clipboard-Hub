#include "chub/clip.h"
#include "chub/platform.h"
#include "chub/util.h"
#include <string.h>

/* PowerShell: force UTF-8 output for Get-Clipboard */
static const char *PS_READ_CMD =
  "powershell.exe -NoProfile -Command \""
  "[Console]::OutputEncoding=[Text.Encoding]::UTF8; "
  "$v = Get-Clipboard -Raw; "
  "[Console]::Write($v)"
  "\"";

/* Use PowerShell to read UTF-8 stdin and set clipboard */
static const char *PS_WRITE_CMD =
  "powershell.exe -NoProfile -Command \""
  "[Console]::InputEncoding=[Text.Encoding]::UTF8; "
  "$in=[Console]::In.ReadToEnd(); "
  "Set-Clipboard -Value $in"
  "\"";

int chub_clip_read(char *buf, size_t buf_sz) {
    if (!buf || buf_sz == 0) return -1;
    buf[0] = '\0';
    int rc = chub_run_capture_stdout(PS_READ_CMD, buf, buf_sz);
    if (rc != 0) {
        /* If clipboard empty, PowerShell may yield empty string — treat as success */
        if (buf[0] == '\0') return 0;
        return rc;
    }
    return 0;
}

int chub_clip_write(const char *data) {
    if (!data) data = "";
    return chub_run_pipe_stdin(PS_WRITE_CMD, data, strlen(data));
}

int chub_clip_smoketest(void) {
    const char *msg = "ClipboardHub smoke ✅";
    if (chub_clip_write(msg) != 0) return 1;
    char buf[4096];
    if (chub_clip_read(buf, sizeof(buf)) != 0) return 2;
    return strcmp(buf, msg) == 0 ? 0 : 3;
}
