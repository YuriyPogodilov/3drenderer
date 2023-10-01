#include <stdio.h>
#include "display.h"

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* color_buffer_texture = NULL;

uint32_t* color_buffer = NULL;
float* z_buffer = NULL;
int window_width = 800;
int window_height = 600;


bool initialize_window(void) {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError());
		return false;
	}

	// Use SDL to query what is fullscreen max. width and height
	SDL_DisplayMode display_mode;
	SDL_GetCurrentDisplayMode(0, &display_mode);
	window_width = display_mode.w;
	window_height = display_mode.h;

	// Create a SDL window
	window = SDL_CreateWindow(
		NULL,
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		window_width,
		window_height,
		SDL_WINDOW_BORDERLESS
	);
	if (!window) {
		fprintf(stderr, "Error creating SDL window: %s\n", SDL_GetError());
		return false;
	}

	// Create a SDL renderer
	renderer = SDL_CreateRenderer(window, -1, 0);
	if (!renderer) {
		fprintf(stderr, "Error creating SDL renderer: %s\n", SDL_GetError());
		return false;
	}

	SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);

	return true;
}

void destroy_window(void) {
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void render_color_buffer(void) {
	SDL_UpdateTexture(
		color_buffer_texture,
		NULL,
		color_buffer,
		(int)(window_width * sizeof(uint32_t))
	);
	SDL_RenderCopy(renderer, color_buffer_texture, NULL, NULL);
}

void clear_color_buffer(uint32_t color) {
	for (int y = 0; y < window_height; y++) {
		for (int x = 0; x < window_width; x++) {
			color_buffer[window_width * y + x] = color;
		}
	}
}

void clear_z_buffer(void) {
	for (int y = 0; y < window_height; y++) {
		for (int x = 0; x < window_width; x++) {
			z_buffer[window_width * y + x] = 1.0;
		}
	}
}

void draw_grid(void) {
	for (int y = 0; y < window_height; y++) {
		for (int x = 0; x < window_width; x++) {
			if (x % 10 == 0 || y % 10 == 0 || x % 100 == 1 || x % 100 == 99 || y % 100 == 1 || y % 100 == 99) {
				color_buffer[window_width * y + x] = 0xFF333333;
			}
		}
	}
}

void draw_pixel(int x, int y, uint32_t color)
{
	if (x >= 0 && x < window_width && y >= 0 && y < window_height) {
		color_buffer[window_width * y + x] = color;
	}
}

void draw_line(int x0, int y0, int x1, int y1, uint32_t color)
{
	int dx = x1 - x0;
	int dy = y1 - y0;

	int length = abs(dx) >= abs(dy) ? abs(dx) : abs(dy);

	float inc_x = dx / (float)length;
	float inc_y = dy / (float)length;

	float current_x = x0;
	float current_y = y0;

	for (int i = 0; i <= length; i++) {
		draw_pixel(round(current_x), round(current_y), color);
		current_x += inc_x;
		current_y += inc_y;
	}
}

void draw_rect(int x, int y, int w, int h, uint32_t color) {
	for (int _x = x; _x < x + w; _x++) {
		for (int _y = y; _y < y + h; _y++) {
			draw_pixel(_x, _y, color);
		}
	}
}

