#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>

#include "term.h"

#ifndef PREFIX
#define PREFIX "./"
#endif

#define COLOR(c) c.r, c.g, c.b
#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))

typedef struct {
	int r, g, b;
} color_t;

typedef struct {
	char* left_sprite;
	char* right_sprite;
	int width, height;
} fish_type_t;

typedef struct {
	const fish_type_t* type;
	color_t color;
	float x, y;
	float vx, vy;
	int counter;
} fish_t;

typedef struct {
	float x, y;
	float vx, vy;
	int counter;
} bubble_t;

typedef struct {
	int x;
	int height;
	int wobblyness;
	int counter;
} seaweed_t;

typedef struct {
	char* sprite;
	int width, height;
} rock_type_t;

typedef struct {
	const rock_type_t* type;
	int x;
} rock_t;

static float clamp(float v, float a, float b);
static float random_number(float a, float b);
static bool is_file(const char* name);
static void delay(long ms);

static void add_bubble(bubble_t bub);
static void remove_bubble(int idx);
static void bubbles_update(void);
static void bubbles_draw(void);

static void generate_seaweed(int count);
static void seaweed_draw(void);

static void generate_rocks(int count);
static void rocks_draw(void);

static void sprite_fill_background_transparent(char* ft, int w, int h);
static void sprite_fill_background_transparent_no_bottom(char* ft, int w, int h);
static void sprite_fill_background_transparent_fill_from(char* ft, int w, int h, int x, int y);

static fish_type_t load_fish_type(const char* path);
static void free_fish_type(fish_type_t* ft);
static void load_fish_types_from_directory(const char* dir);
static void free_fish_types(void);

static void add_fish(fish_t fish);
static void fish_draw(const fish_t* fish);

static void bubbler_draw(void);
static void bubbler_bubble(void);

static void draw_fish_tank(void);

static bool is_colored = true;
static bool has_waves = true;

static int surface_level = 2;
static float water_density = 0.95;
static float water_boyancey = 0.015;

static int fish_think_speed = 100;
static float fish_speed = 0.4;

static float bubbler_pos = 15;
static float bubbler_height = 30;

static fish_type_t fish_types[128];
static int fish_types_count = 0;

static bubble_t bubbles[256];
static int bubble_count = 0;

static seaweed_t seaweeds[32];
static int seaweed_count = 0;

static rock_t rocks[128];
static int rock_count = 0;

static fish_t fishes[128];
static int fish_count = 0;

static bool interrupt = false;

static char big_rock_sprite[] =
	"     __   "
	"   _/  \\  "
	"  /     \\ "
	" /      | "
	"/        \\";

static rock_type_t big_rock = {
	big_rock_sprite, 10, 5
};

static char small_rock_sprite[] =
	" _--_ "
	"/    |";

static rock_type_t small_rock = {
	small_rock_sprite, 6, 2
};

float clamp(float v, float a, float b) {
	if (v < a) return a;
	if (v >= b) return b - 1.0;
	return v;
}

float random_number(float a, float b) {
	float d = b - a;
	return a + d * ((float)rand() / (float)RAND_MAX);
}

bool is_file(const char* name) {
	struct stat path;
	stat(name, &path);
	return S_ISREG(path.st_mode);
}

void delay(long ms) {
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	nanosleep(&ts, NULL);
}

void add_bubble(bubble_t bub) {
	assert(bubble_count < ARRAY_LEN(bubbles));
	bubbles[bubble_count++] = bub;
}

void remove_bubble(int idx) {
	for (int i = idx; i + 1 < bubble_count; i++) {
		bubbles[i] = bubbles[i + 1];
	}
	bubble_count -= 1;
}

void bubbles_update(void) {
	int w, h;
	term_get_size(&w, &h);

	for (int i = 0; i < bubble_count; i++) {
		bubble_t* b = &bubbles[i];
		b->counter += 1;
		b->vx *= water_density;
		b->vy -= water_boyancey;

		b->x = clamp(b->x + b->vx, 0, w);
		b->y = clamp(b->y + b->vy, surface_level, h);
		if (b->y <= surface_level) {
			remove_bubble(i);
			i -= 1;
			continue;
		}
	}
}

void bubbles_draw(void) {
	term_style_fg_color(255, 255, 255);
	for (int i = 0; i < bubble_count; i++) {
		bubble_t* b = &bubbles[i];
		term_cursor_move_to(b->x, b->y);
		if (b->counter < 17) {
			term_write_char_n('.', 1);
		} else if (b->counter < 43) {
			term_write_char_n('o', 1);
		} else {
			term_write_char_n('O', 1);
		}
	}
}

void generate_seaweed(int count) {
	int w, h;
	term_get_size(&w, &h);

	for (int i = 0; i < count; i++) {
		seaweed_t sw = {
			random_number(0, w),
			random_number(3, 8),
			random_number(20, 80),
			0
		};
		seaweeds[seaweed_count++] = sw;
	}
}

void seaweed_draw(void) {
	const char* seaweed_segements[2] = { " )", "( " };

	int w, h;
	term_get_size(&w, &h);

	if (is_colored) {
		term_style_fg_color(30, 255, 30);
	}
	for (int i = 0; i < seaweed_count; i++) {
		seaweed_t* sw = &seaweeds[i];
		sw->counter += 1;
		if (sw->counter > sw->wobblyness) {
			sw->counter = 0;
		}

		int sw_char = sw->counter > sw->wobblyness / 2;
		term_cursor_move_to(sw->x, h - 1);
		for (int y = 0; y < sw->height; y++) {
			term_write_transparent(seaweed_segements[sw_char]);
			term_cursor_move(-2, -1);
			sw_char = (sw_char + 1) % 2;
		}
	}
}

void generate_rocks(int count) {
	int w, h;
	term_get_size(&w, &h);

	int small_count = count * 0.75;
	int big_count = count - small_count;
	for (int i = 0; i < big_count; i++) {
		rock_t r = {
			&big_rock,
			random_number(0, w - big_rock.width)
		};
		rocks[rock_count++] = r;
	}
	for (int i = 0; i < small_count; i++) {
		rock_t r = {
			&small_rock,
			random_number(0, w - small_rock.width)
		};
		rocks[rock_count++] = r;
	}
}

void rocks_draw(void) {
	int w, h;
	term_get_size(&w, &h);

	if (is_colored) {
		term_style_fg_color(180, 180, 180);
	}

	for (int i = 0; i < rock_count; i++) {
		rock_t* r = &rocks[i];
		term_cursor_move_to(r->x, h - r->type->height);
		const char* sprite = r->type->sprite;
		for (int y = 0; y < r->type->height; y++) {
			term_write_transparent_length(sprite, r->type->width);
			term_cursor_move(-r->type->width, 1);
			sprite += r->type->width;
		}
	}
}

void sprite_fill_background_transparent_fill_from(char* ft, int w, int h, int x, int y) {
	if (ft[y * w + x] != ' ') return;
	ft[y * w + x] = '\t';

	if (x + 1 < w) {
		sprite_fill_background_transparent_fill_from(ft, w, h, x + 1, y);
	}

	if (x - 1 >= 0) {
		sprite_fill_background_transparent_fill_from(ft, w, h, x - 1, y);
	}

	if (y + 1 < h) {
		sprite_fill_background_transparent_fill_from(ft, w, h, x, y + 1);
	}

	if (y - 1 >= 0) {
		sprite_fill_background_transparent_fill_from(ft, w, h, x, y - 1);
	}
}

void sprite_fill_background_transparent(char* ft, int w, int h) {
	for (int x = 0; x < w; x++) {
		sprite_fill_background_transparent_fill_from(ft, w, h, x, 0);
		sprite_fill_background_transparent_fill_from(ft, w, h, x, h - 1);
	}

	for (int y = 0; y < h; y++) {
		sprite_fill_background_transparent_fill_from(ft, w, h, 0, y);
		sprite_fill_background_transparent_fill_from(ft, w, h, w - 1, y);
	}
}

void sprite_fill_background_transparent_no_bottom(char* ft, int w, int h) {
	for (int x = 0; x < w; x++) {
		sprite_fill_background_transparent_fill_from(ft, w, h, x, 0);
	}

	for (int y = 0; y < h; y++) {
		sprite_fill_background_transparent_fill_from(ft, w, h, 0, y);
		sprite_fill_background_transparent_fill_from(ft, w, h, w - 1, y);
	}
}

fish_type_t load_fish_type(const char* path) {
	FILE* fp = fopen(path, "r");
	if (fp == NULL) return (fish_type_t){0};

	fseek(fp, 0, SEEK_END);
	long length = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char* data = malloc(length + 1);
	fread(data, 1, length, fp);
	data[length] = 0;

	fish_type_t fish_type = {0};

	for (int i = 0; i < length; i++) {
		if (data[i] == '\n') {
			fish_type.width = i;
			break;
		}
	}

	for (int i = 0; i < length; i++) {
		if (data[i] == '\n') {
			fish_type.height += 1;
			if (i + 1 < length && data[i + 1] == '\n') break;
		}
	}

	int i = 0;

	fish_type.left_sprite = malloc(fish_type.width * fish_type.height + 1);
	fish_type.left_sprite[fish_type.width * fish_type.height] = 0;
	for (int y = 0; y < fish_type.height; y++) {
		memcpy(
			fish_type.left_sprite + y * fish_type.width,
			&data[i],
			fish_type.width
		);
		i += fish_type.width + 1;
	}

	i += 1;

	fish_type.right_sprite = malloc(fish_type.width * fish_type.height + 1);
	fish_type.right_sprite[fish_type.width * fish_type.height] = 0;
	for (int y = 0; y < fish_type.height; y++) {
		memcpy(
			fish_type.right_sprite + y * fish_type.width,
			&data[i],
			fish_type.width
		);
		i += fish_type.width + 1;
	}

	free(data);
	fclose(fp);

	sprite_fill_background_transparent(fish_type.left_sprite, fish_type.width, fish_type.height);
	sprite_fill_background_transparent(fish_type.right_sprite, fish_type.width, fish_type.height);

	return fish_type;
}

void free_fish_type(fish_type_t* ft) {
	free(ft->left_sprite);
	free(ft->right_sprite);
}

void load_fish_types_from_directory(const char* dir_path) {
	if (strlen(dir_path) == 0) return;
	DIR* d = opendir(dir_path);
	if (d == NULL) return;

	struct dirent* dir;
	while ((dir = readdir(d)) != NULL) {
		char* fish_path = malloc(strlen(dir_path) + strlen(dir->d_name) + 2);
		fish_path[0] = 0;

		char* end = strcat(fish_path, dir_path) - 1;
		if (*end != '/') {
			strcat(fish_path, "/");
		}
		strcat(fish_path, dir->d_name);

		if (is_file(fish_path)) {
			fish_type_t fish = load_fish_type(fish_path);

			assert(fish_types_count < ARRAY_LEN(fish_types));
			fish_types[fish_types_count++] = fish;
		}

		free(fish_path);
	}
}

void free_fish_types(void) {
	for (int i = 0; i < fish_types_count; i++) {
		free_fish_type(&fish_types[i]);
	}
}

void add_fish(fish_t fish) {
	assert(fish_count < ARRAY_LEN(fishes));
	fishes[fish_count++] = fish;
}

void fish_draw(const fish_t* fish) {
	const fish_type_t* ft = fish->type;

	const char* line;
	if (fish->vx >= 0) {
		line = ft->right_sprite;
	} else {
		line = ft->left_sprite;
	}

	if (is_colored) {
		term_style_fg_color(COLOR(fish->color));
	}
	term_cursor_move_to(fish->x, fish->y);
	for (int y = 0; y < ft->height; y++) {
		term_write_transparent_length(line, ft->width);
		term_cursor_move(-(ft->width), 1);
		line += ft->width;
	}
}

void fish_update(fish_t* fish) {
	int w, h;
	term_get_size(&w, &h);

	fish->x = clamp(fish->x + fish->vx, 0, w - fish->type->width);
	fish->y = clamp(fish->y + fish->vy, surface_level + 1, h - fish->type->height + 1);

	fish->counter += 1;
	if (fish->counter > fish_think_speed) {
		fish->vx = random_number(-fish_speed, fish_speed);
		fish->vy = random_number(-fish_speed / 2.0, fish_speed / 2.0);
		fish->counter = fish->counter % fish_think_speed;

		float bub_x = fish->x;
		float bub_vx = 0.25;
		if (fish->vx > 0) {
			bub_x += fish->type->width;
		} else {
			bub_vx *= -1.0;
		}
		float bub_y = fish->y + (fish->type->height / 2.0);
		add_bubble((bubble_t){ bub_x, bub_y, fish->vx + bub_vx, fish->vy - 0.05 });
	}
}

void bubbler_draw(void) {
	term_style_fg_color(255, 255, 255);

	float x = bubbler_pos;
	float y = surface_level;
	for (int i = 0; i < bubbler_height; i++) {
		term_cursor_move_to(x, y);
		term_write_char_n('|', 2);
		y += 1;
	}
	term_cursor_move_to(x, y);
	term_write_char_n('#', 2);

	term_cursor_move_to(bubbler_pos - 2, surface_level - 1);
	term_write_char_n('|', 1);
	term_write_char_n('_', 4);
	term_write_char_n('|', 1);

	term_cursor_move_to(bubbler_pos - 1, surface_level - 2);
	term_write_char_n('-', 4);
}

void bubbler_bubble(void) {
	static int counter = 0;
	counter += 1;
	if (counter >= 2) {
		counter = 0;
		add_bubble((bubble_t){ bubbler_pos - 1 + random_number(0, 5), surface_level + bubbler_height + 2, random_number(-0.5, 0.5), -0.20 });
	}
}

void draw_fish_tank(void) {
	int w, h;
	term_get_size(&w, &h);

	if (is_colored) {
		term_style_fg_color(0, 255, 255);
	}

	term_cursor_move_to(0, surface_level);

	if (has_waves) {
		static int counter = 0;
		counter -= 1;
		int wave_pos = counter / 2;
		const char wave_chars[] = { '_', ',', '.', '-', '\'', '`' };
		for (int i = 0; i < w; i++) {
			float x = wave_pos + i;
			float v = ((sin(0.3 * x) + sin(0.2 * x)) / 2.0 + 1.0) / 2.0;
			int idx = v * ARRAY_LEN(wave_chars);
			term_write_char_n(wave_chars[idx], 1);
		}
	} else {
		term_write_char_n('~', w);
	}
}

void sigint_handle(int sig) {
	signal(sig, SIG_IGN);
	interrupt = true;
}

int main(int argc, char** argv) {
	signal(SIGINT, sigint_handle);
	srand(time(NULL));

	char* fish_path = PREFIX "/share/aquarium/fish";
	bool show_bubbler = true;
	bool show_fish_tank = true;
	bool show_seaweed = true;
	bool show_rocks = true;
	bool show_bubbles = true;
	bool on_of_each_fish = false;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
			fish_path = argv[i + 1];
			i += 1;
		} else if (strcmp(argv[i], "-b") == 0) {
			show_bubbler = false;
		} else if (strcmp(argv[i], "-x") == 0) {
			srand(atoi(argv[i + 1]));
			i += 1;
		} else if (strcmp(argv[i], "-s") == 0) {
			show_seaweed = false;
		} else if (strcmp(argv[i], "-r") == 0) {
			show_rocks = false;
		} else if (strcmp(argv[i], "-e") == 0) {
			on_of_each_fish = true;
		} else if (strcmp(argv[i], "-c") == 0) {
			is_colored = false;
		} else if (strcmp(argv[i], "-t") == 0) {
			show_fish_tank = false;
		} else if (strcmp(argv[i], "-u") == 0) {
			show_bubbles = false;
		} else if (strcmp(argv[i], "-w") == 0) {
			has_waves = false;
		} else if (
			strcmp(argv[i], "-h") == 0 ||
			strcmp(argv[i], "--help") == 0
		) {
			printf("aquarium\n");
			printf("arguments:\n");
			printf("    -f PATH        - specify folder to read fish from\n");
			printf("    -e             - put one of every fish type in the tank\n");
			printf("    -x             - fishtank seed\n");
			printf("    -c             - remove colors\n");
			printf("    -w             - remove waves\n");
			printf("    -b             - hide bubbler\n");
			printf("    -s             - hide seaweed\n");
			printf("    -r             - hide rocks\n");
			printf("    -t             - hide the tank\n");
			printf("    -u             - hide the bubbles\n");
			return 0;
		}
	}

	load_fish_types_from_directory(fish_path);

	sprite_fill_background_transparent_no_bottom(big_rock.sprite, big_rock.width, big_rock.height);
	sprite_fill_background_transparent_no_bottom(small_rock.sprite, small_rock.width, small_rock.height);

	term_save_screen();
	term_clear();
	term_cursor_home();
	term_cursor_hide();

	int w, h;
	term_get_size(&w, &h);

	bubbler_height = h * 0.4;

	generate_seaweed(w / 10);

	generate_rocks(w / 20);

	const color_t colors[] = {
		{ 255, 255, 255 },
		{ 255, 0, 0 },
		{ 0, 255, 0 },
		{ 0, 0, 255 },
		{ 255, 255, 0 },
		{ 255, 0, 255 },
		{ 0, 255, 255 },
		{ 90, 50, 190 },
		{ 0, 102, 204 },
		{ 255, 51, 51 },
		{ 255, 51, 255 },
		{ 255, 51, 153 }
	};

	if (on_of_each_fish) {
		for (int i = 0; i < fish_types_count; i++) {
			add_fish((fish_t){
				.type = &fish_types[i],
				.color = colors[(int)random_number(0, ARRAY_LEN(colors))],
				.x = random_number(0, w - 20),
				.y = random_number(0, h - 4),
				.vx = -0.2,
				.vy = 0.05,
				.counter = random_number(0, 200)
			});
		}
	} else {
		int fish_count = (w * h / 2) / 220;
		for (int i = 0; i < fish_count; i++) {
			add_fish((fish_t){
				.type = &fish_types[(int)random_number(0, fish_types_count)],
				.color = colors[(int)random_number(0, ARRAY_LEN(colors))],
				.x = random_number(0, w - 20),
				.y = random_number(0, h - 4),
				.vx = -0.2,
				.vy = 0.05,
				.counter = random_number(0, 200)
			});
		}
	}

	while (interrupt == false) {
		term_style_none();
		term_clear();

		if (show_fish_tank) {
			draw_fish_tank();
		}

		for (int i = 0; i < fish_count; i++) {
			fish_update(&fishes[i]);
			fish_draw(&fishes[i]);
		}

		if (show_bubbles) {
			bubbles_update();
			bubbles_draw();
		}

		if (show_bubbler) {
			bubbler_bubble();
			bubbler_draw();
		}

		if (show_seaweed) {
			seaweed_draw();
		}

		if (show_rocks) {
			rocks_draw();
		}

		fflush(stdout);
		delay(50);
	}

	free_fish_types();

	term_cursor_show();
	term_load_screen();

	return 0;
}
