#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>
#include <stdint.h>
#include <SDL.h>

#define FPS 60
#define FRAME_TARGET_TIME (1000 / FPS)

enum cull_method {
	CULL_NONE,
	CULL_BACKFACE
};

enum render_method {
	RENDER_WIRE,
	RENDER_WIRE_VERTEX,
	RENDER_FILL_TRIANGLE,
	RENDER_FILL_TRIANGLE_WIRE,
	RENDER_TEXTURED,
	RENDER_TEXTURED_WIRE
};

int get_window_width(void);
int get_window_height(void);

void set_render_method(int method);
bool should_render_filled_triangle(void);
bool should_render_textured_triangle(void);
bool should_render_wireframe(void);
bool should_render_wire_vertex(void);

void set_cull_method(int method);
bool is_cull_backface(void);

bool initialize_window(void);
void destroy_window(void);

void render_color_buffer(void);
void clear_color_buffer(uint32_t color);
void clear_z_buffer(void);

float get_zbuffer_at(int x, int y);
void update_zbuffer_at(int x, int y, float value);

void draw_grid(void);
void draw_pixel(int x, int y, uint32_t color);
void draw_line(int x0, int y0, int x1, int y1, uint32_t color);
void draw_rect(int x, int y, int w, int h, uint32_t color);

#endif