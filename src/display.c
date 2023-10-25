#include <stdio.h>
#include "display.h"

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* color_buffer_texture = NULL;

static uint32_t* color_buffer = NULL;
static float* z_buffer = NULL;
static int window_width = 800;
static int window_height = 600;

static int render_method = 0;
static int cull_method = 0;

int get_window_width() {
	return window_width;
}

int get_window_height() {
	return window_height;
}

int get_render_method()
{
	return render_method;
}

void set_render_method(int method)
{
	render_method = method;
}

bool should_render_filled_triangle(void)
{
	return render_method == RENDER_FILL_TRIANGLE || render_method == RENDER_FILL_TRIANGLE_WIRE;
}

bool should_render_textured_triangle(void)
{
	return render_method == RENDER_TEXTURED || render_method == RENDER_TEXTURED_WIRE;
}

bool should_render_wireframe(void)
{
	return render_method == RENDER_WIRE ||
		render_method == RENDER_WIRE_VERTEX ||
		render_method == RENDER_FILL_TRIANGLE_WIRE ||
		render_method == RENDER_TEXTURED_WIRE;
}

bool should_render_wire_vertex(void)
{
	return render_method == RENDER_WIRE_VERTEX;
}

bool is_cull_backface()
{
	return cull_method == CULL_BACKFACE;
}

void set_cull_method(int method)
{
	cull_method = method;
}

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

	// Allocate the required memory in bytes to hold the color buffer
	color_buffer = (uint32_t*)malloc(sizeof(uint32_t) * window_width * window_height);
	z_buffer = (float*)malloc(sizeof(float) * window_width * window_height);

	// Creating a SDL texture that is used to display the color buffer
	color_buffer_texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_RGBA32,
		SDL_TEXTUREACCESS_STREAMING,
		window_width,
		window_height
	);

	return true;
}

void destroy_window(void) {
	free(color_buffer);
	free(z_buffer);
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
	SDL_RenderPresent(renderer);
}

void clear_color_buffer(uint32_t color) {
	for (int i = 0; i < window_width * window_height; i++) {
		color_buffer[i] = color;
	}
}

void clear_z_buffer(void) {
	for (int i = 0; i < window_width * window_height; i++) {
		z_buffer[i] = 1.0;
	}
}

float get_zbuffer_at(int x, int y)
{
	if (x < 0 || x >= window_width || y < 0 || y >= window_height) {
		return 1.0;
	}
	return z_buffer[window_width * y + x];
}

void update_zbuffer_at(int x, int y, float value)
{
	if (x < 0 || x >= window_width || y < 0 || y >= window_height) {
		return;
	}
	z_buffer[window_width * y + x] = value;
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
	if (x < 0 || x >= window_width || y < 0 || y >= window_height) {
		return;
	}
	color_buffer[window_width * y + x] = color;
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

