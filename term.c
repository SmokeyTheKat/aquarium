#include "term.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/termios.h>

void term_clear(void) {
	int w, h;
	term_get_size(&w, &h);
	term_write_char_n(' ', w * h);
}

void term_clear_before(void) {
	printf("\e[1J");
}

void term_clear_after(void) {
	printf("\e[0J");
}

void term_save_screen(void) {
	printf("\e[?1049h");
}

void term_load_screen(void) {
	printf("\e[?1049l");
}

void term_get_size(int* width, int* height) {
	struct winsize w;
	ioctl(0, TIOCGWINSZ, &w);
	*width = w.ws_col;
	*height = w.ws_row;
}

void term_write(const char* str) {
	for (int i = 0; str[i] != 0; i++) {
		fputc(str[i], stdout);
	}
}

void term_write_transparent(const char* str) {
	term_write_transparent_length(str, strlen(str));
}

void term_write_length(const char* str, int length) {
	for (int i = 0; i < length && str[i] != 0; i++) {
		fputc(str[i], stdout);
	}
}

void term_write_transparent_length(const char* str, int len) {
	int end = len - 1;
	while (end > 0 && str[end] == ' ') end--;
	end++;

	bool in_shape = false;
	for (int i = 0; i < end; i++) {
		if (in_shape == false) {
			if (str[i] == ' ') {
				printf("\e[C");
			} else {
				in_shape = true;
				fputc(str[i], stdout);
			}
		} else {
			fputc(str[i], stdout);
		}
	}

	if (len - end > 0) {
		printf("\e[%dC", len - end);
	}
}

void term_write_char_n(char c, int n) {
	for (int i = 0; i < n; i++) {
		fputc(c, stdout);
	}
}

void term_style_none(void) {
	printf("\e[7m");
}

void term_style_fg_color(int r, int g, int b) {
	printf("\e[38;2;%d;%d;%dm", r, g, b);
}

void term_style_bg_color(int r, int g, int b) {
	printf("\e[48;2;%d;%d;%dm", r, g, b);
}

void term_cursor_hide(void) {
	printf("\e[?25l");
}

void term_cursor_show(void) {
	printf("\e[?25h");
}

void term_cursor_move_to(int x, int y) {
	x += 1;
	y += 1;
	printf("\e[%d;%dH", y, x);
}

void term_cursor_home(void) {
	term_cursor_move_to(0, 0);
}

void term_cursor_move(int dx, int dy) {
	if (dx > 0) {
		printf("\e[%dC", dx);
	} else if (dx < 0) {
		printf("\e[%dD", -dx);
	}

	if (dy > 0) {
		printf("\e[%dB", dy);
	} else if (dy < 0) {
		printf("\e[%dA", -dy);
	}
}
