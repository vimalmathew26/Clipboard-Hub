#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void chub_transform_toggle_case(char *s);
void chub_transform_trim(char *s);
int  chub_transform_url_decode(char *s); /* returns 0 on success */

#ifdef __cplusplus
}
#endif
