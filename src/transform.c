#include "chub/transform.h"
#include <ctype.h>
#include <string.h>
#include <stddef.h>

void chub_transform_toggle_case(char *s) {
    if (!s) return;
    for (size_t i = 0; s[i] != '\0'; ++i) {
        unsigned char c = (unsigned char)s[i];
        if (isupper(c)) s[i] = (char)tolower(c);
        else if (islower(c)) s[i] = (char)toupper(c);
    }
}

void chub_transform_trim(char *s) {
    if (!s) return;
    size_t len = strlen(s);
    size_t start = 0;
    while (start < len && (unsigned char)s[start] <= ' ') start++;
    size_t end = len;
    while (end > start && (unsigned char)s[end-1] <= ' ') end--;
    size_t newlen = end - start;
    if (start > 0 && newlen > 0) memmove(s, s + start, newlen);
    s[newlen] = '\0';
}

/* decode %XX (hex) and '+' -> space like urlencoded query */
int chub_transform_url_decode(char *s) {
    if (!s) return -1;
    size_t r = 0, w = 0;
    while (s[r]) {
        if (s[r] == '%' && s[r+1] && s[r+2]) {
            unsigned char hi = s[r+1], lo = s[r+2];
            unsigned char v = 0;
            if      (hi >= '0' && hi <= '9') v = (hi - '0') << 4;
            else if (hi >= 'A' && hi <= 'F') v = (hi - 'A' + 10) << 4;
            else if (hi >= 'a' && hi <= 'f') v = (hi - 'a' + 10) << 4;
            else { s[w++] = s[r++]; continue; }
            if      (lo >= '0' && lo <= '9') v |= (lo - '0');
            else if (lo >= 'A' && lo <= 'F') v |= (lo - 'A' + 10);
            else if (lo >= 'a' && lo <= 'f') v |= (lo - 'a' + 10);
            else { s[w++] = s[r++]; continue; }
            s[w++] = (char)v;
            r += 3;
        } else if (s[r] == '+') {
            s[w++] = ' ';
            r++;
        } else {
            s[w++] = s[r++];
        }
    }
    s[w] = '\0';
    return 0;
}
