#include "term.h"

#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

void term_clear(void) {
	printf("\x1b[2J");
}

void term_save_screen(void) {
	printf("\x1b[?1049h");
}

void term_load_screen(void) {
	printf("\x1b[?1049l");
}

void term_get_size(int* width, int* height) {
	struct winsize w;
	ioctl(0, TIOCGWINSZ, &w);
	*width = w.ws_col;
	*height = w.ws_row;
}

void term_write_transparent(const char* str) {
	term_write_transparent_length(str, strlen(str));
}

void term_write_transparent_length(const char* str, int length) {
	for (int i = 0; i < length && str[i] != 0; i++) {
		if (str[i] == '\t') {
			term_cursor_move(1, 0);
		} else {
			fputc(str[i], stdout);
		}
	}
}

void term_write_char_n(char c, int n) {
	for (int i = 0; i < n; i++) {
		fputc(c, stdout);
	}
}

void term_style_none(void) {
	printf("\x1b[0m");
}

void term_style_fg_color(int r, int g, int b) {
	printf("\x1b[38;2;%d;%d;%dm", r, g, b);
}

void term_cursor_hide(void) {
	printf("\x1b[?25l");
}

void term_cursor_show(void) {
	printf("\x1b[?25h");
}

void term_cursor_move_to(int x, int y) {
	x += 1;
	y += 1;
	printf("\x1b[%d;%dH", y, x);
}

void term_cursor_home(void) {
	term_cursor_move_to(0, 0);
}

void term_cursor_move(int dx, int dy) {
	if (dx > 0) {
		printf("\x1b[%dC", dx);
	} else if (dx < 0) {
		printf("\x1b[%dD", -dx);
	}

	if (dy > 0) {
		printf("\x1b[%dB", dy);
	} else if (dy < 0) {
		printf("\x1b[%dA", -dy);
	}
}
