#include "chub/tui.h"
#include "chub/db.h"
#include "chub/clip.h"
#include "chub/transform.h"
#include "chub/util.h"

#include <ncursesw/curses.h>  // wide-capable library, but we'll print via UTF-8 multibyte APIs
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <windows.h>

#define MAX_LIST 500
#define SEARCH_BUF 256

static chub_item *g_items = NULL;
static int g_count = 0;
static int g_sel = 0;
static int g_scroll = 0;
static char g_search[SEARCH_BUF] = {0};
static volatile LONG g_needs_refresh = 1;

static void free_items(void) {
    chub_db_free_items(g_items, g_count);
    g_items = NULL; g_count = 0; g_sel = 0; g_scroll = 0;
}

static void load_items(void) {
    chub_item *arr = NULL; int n = 0;
    if (g_search[0]) chub_db_search(g_search, MAX_LIST, &arr, &n);
    else             chub_db_fetch_recent(MAX_LIST, &arr, &n);
    free_items();
    g_items = arr; g_count = n;
    if (g_sel >= g_count) g_sel = g_count ? g_count - 1 : 0;
    if (g_scroll > g_sel) g_scroll = g_sel;
}

/* ----- UTF-8 helpers (no wide curses API needed) ----- */

/* return number of BYTES that contain up to max_chars full UTF-8 codepoints */
static size_t utf8_prefix_by_chars(const char *s, int max_chars) {
    if (!s || max_chars <= 0) return 0;
    mbstate_t st; memset(&st, 0, sizeof(st));
    size_t i = 0; int count = 0;
    while (s[i] && count < max_chars) {
        wchar_t wc;
        size_t r = mbrtowc(&wc, s + i, MB_CUR_MAX, &st);
        if (r == (size_t)-1 || r == (size_t)-2) break;   // invalid or incomplete
        if (r == 0) { /* NUL */ i++; break; }
        i += r;
        count++;
    }
    return i;
}

/* draw wrapped UTF-8 (wrap by character count ~= columns; simple and fast) */
static void draw_wrapped_utf8(WINDOW *win, int start_row, int start_col,
                              int max_rows, int max_cols, const char *s) {
    int row = start_row;
    const char *p = s ? s : "";
    while (*p && row < start_row + max_rows) {
        /* stop at newline or at max_cols characters */
        const char *nl = strchr(p, '\n');
        if (nl && nl - p <= max_cols) {
            mvwaddnstr(win, row, start_col, p, (int)(nl - p));
            p = nl + 1;
            row++;
            continue;
        }
        size_t bytes = utf8_prefix_by_chars(p, max_cols);
        if (bytes == 0) break;
        mvwaddnstr(win, row, start_col, p, (int)bytes);
        p += bytes;
        row++;
    }
}

/* ----- drawing ----- */

static void draw_list(WINDOW *win, int h, int w) {
    werase(win);
    box(win, 0, 0);
    int inner_h = h - 2;
    int inner_w = w - 2;
    int start = g_scroll;
    int end = start + inner_h;
    if (end > g_count) end = g_count;

    for (int i = start, row = 0; i < end; ++i, ++row) {
        int prefix_cols = 7;                 /* "* 1234 " */
        int text_cols   = inner_w - prefix_cols;
        if (text_cols < 0) text_cols = 0;

        /* first line (until newline) */
        const char *txt = g_items[i].text ? g_items[i].text : "";
        const char *nl  = strchr(txt, '\n');
        int first_len   = (int)(nl ? (nl - txt) : (int)strlen(txt));

        if (i == g_sel) wattron(win, A_REVERSE);
        mvwprintw(win, row+1, 1, "%c %4d ", g_items[i].favorite ? '*' : ' ', g_items[i].id);

        size_t safe = utf8_prefix_by_chars(txt, text_cols);
        if (safe > (size_t)first_len) safe = (size_t)first_len;  /* don't go past first line */
        mvwaddnstr(win, row+1, 1 + prefix_cols, txt, (int)safe);

        if (i == g_sel) wattroff(win, A_REVERSE);
    }
    wnoutrefresh(win);
}

static void draw_preview(WINDOW *win, int h, int w) {
    werase(win);
    box(win, 0, 0);
    if (g_sel >= 0 && g_sel < g_count && g_items[g_sel].text) {
        draw_wrapped_utf8(win, 1, 1, h - 2, w - 2, g_items[g_sel].text);
    } else {
        mvwprintw(win, 1, 1, "(empty)");
    }
    wnoutrefresh(win);
}

static void draw_status(WINDOW *win, int w) {
    werase(win);
    mvwprintw(win, 0, 0, "/ search: %s   [Enter] copy  [f] fav  [d] del  [t] transform  [q] quit",
              g_search[0] ? g_search : "");
    /* pad remainder */
    int cur = (int)strlen(g_search) + 11;
    for (int i = cur; i < w; ++i) waddch(win, ' ');
    wnoutrefresh(win);
}

static void ensure_visible(int h_inner) {
    if (g_sel < g_scroll) g_scroll = g_sel;
    if (g_sel >= g_scroll + h_inner) g_scroll = g_sel - h_inner + 1;
    if (g_scroll < 0) g_scroll = 0;
}

static void do_copy_selected(void) {
    if (g_sel >= 0 && g_sel < g_count)
        chub_clip_write(g_items[g_sel].text ? g_items[g_sel].text : "");
}

static void do_toggle_fav_selected(void) {
    if (g_sel >= 0 && g_sel < g_count) {
        int id = g_items[g_sel].id;
        int newf = g_items[g_sel].favorite ? 0 : 1;
        if (chub_db_mark_favorite(id, newf) == 0)
            g_items[g_sel].favorite = newf;
    }
}

static void do_delete_selected(void) {
    if (g_sel >= 0 && g_sel < g_count) {
        int id = g_items[g_sel].id;
        if (chub_db_delete(id) == 0)
            load_items();
    }
}

static void do_transform_menu(void) {
    if (g_sel < 0 || g_sel >= g_count || !g_items[g_sel].text) return;
    char *tmp = _strdup(g_items[g_sel].text);
    if (!tmp) return;
    int h; int w; getmaxyx(stdscr, h, w); (void)w;
    mvprintw(h-1, 0, "Transform: [1] Trim  [2] ToggleCase  [3] URL-Decode  [Esc] cancel   ");
    wrefresh(stdscr);
    int ch = getch();
    if (ch == '1') chub_transform_trim(tmp);
    else if (ch == '2') chub_transform_toggle_case(tmp);
    else if (ch == '3') chub_transform_url_decode(tmp);
    else { free(tmp); return; }
    chub_clip_write(tmp);
    free(tmp);
}

static void handle_search_input(void) {
    echo();
    nocbreak();
    curs_set(1);
    int h; int w; getmaxyx(stdscr, h, w); (void)h; (void)w;
    move(getmaxy(stdscr)-1, 10);
    getnstr(g_search, SEARCH_BUF - 1);
    noecho();
    cbreak();
    curs_set(0);
    load_items();
}

/* ----- main loop ----- */

int chub_tui_mainloop(void) {
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    nodelay(stdscr, TRUE);

    int H, W;
    getmaxyx(stdscr, H, W);
    int list_w = W * 2 / 5;
    int prev_w = W - list_w;
    WINDOW *listw = newwin(H-1, list_w, 0, 0);
    WINDOW *prevw = newwin(H-1, prev_w, 0, list_w);
    WINDOW *status = newwin(1, W, H-1, 0);

    load_items();
    int running = 1;
    while (running) {
        if (InterlockedExchange((volatile LONG*)&g_needs_refresh, 0) != 0)
            load_items();

        getmaxyx(stdscr, H, W);
        list_w = W * 2 / 5;
        prev_w = W - list_w;
        wresize(listw, H-1, list_w);
        wresize(prevw, H-1, prev_w);
        mvwin(prevw, 0, list_w);
        wresize(status, 1, W);
        mvwin(status, H-1, 0);

        int list_inner_h = (H-1) - 2;
        ensure_visible(list_inner_h);

        draw_list(listw, H-1, list_w);
        draw_preview(prevw, H-1, prev_w);
        draw_status(status, W);
        doupdate();

        int ch = getch();
        if (ch == ERR) { Sleep(30); continue; }
        switch (ch) {
            case 'q': running = 0; break;
            case KEY_UP:    if (g_sel > 0) { g_sel--; } break;
            case KEY_DOWN:  if (g_sel+1 < g_count) { g_sel++; } break;
            case KEY_PPAGE: g_sel -= list_inner_h; if (g_sel < 0) g_sel = 0; break;
            case KEY_NPAGE: g_sel += list_inner_h; if (g_sel >= g_count) g_sel = g_count ? g_count - 1 : 0; break;
            case '\n':
            case '\r': do_copy_selected(); break;
            case 'f': do_toggle_fav_selected(); break;
            case 'd': do_delete_selected(); break;
            case 't': do_transform_menu(); break;
            case '/': handle_search_input(); break;
            default: break;
        }
    }

    delwin(listw); delwin(prevw); delwin(status);
    endwin();
    return 0;
}

/* signal from poller */
void chub_tui_request_refresh(void) {
    InterlockedExchange((volatile LONG*)&g_needs_refresh, 1);
}

/* expose symbol for main.c */
__declspec(dllexport) void chub__tui__request_refresh__export(void) { chub_tui_request_refresh(); }
