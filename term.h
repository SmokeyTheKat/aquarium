#ifndef __AQURAIUM_TERMINAL_H__
#define __AQURAIUM_TERMINAL_H__

void term_clear(void);

void term_save_screen(void);
void term_load_screen(void);

void term_get_size(int* width, int* height);

void term_write_transparent(const char* str);
void term_write_transparent_length(const char* str, int length);
void term_write_char_n(char c, int n);

void term_style_none(void);
void term_style_fg_color(int r, int g, int b);

void term_cursor_hide(void);
void term_cursor_show(void);
void term_cursor_move_to(int x, int y);
void term_cursor_home(void);
void term_cursor_move(int dx, int dy);

#endif
