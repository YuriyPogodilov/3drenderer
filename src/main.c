#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL.h>
#include "array.h"
#include "display.h"
#include "vector.h"
#include "mesh.h"
#include "matrix.h"
#include "light.h"
#include "camera.h"
#include "clipping.h"

#define MAX_TRIANGLES_TO_RENDER 10000
triangle_t triangles_to_render[MAX_TRIANGLES_TO_RENDER];
int num_triangles_to_render = 0;

mat4_t world_matrix;
mat4_t proj_matrix;
mat4_t view_matrix;

bool is_running = false;
int previous_frame_time = 0;
float delta_time = 0;

void setup(void) {
	render_method = RENDER_WIRE;
	cull_method = CULL_BACKFACE;

	// Allocate the required memory in bytes to hold the color buffer
	color_buffer = (uint32_t*)malloc(sizeof(uint32_t) * window_width * window_height);
	z_buffer = (float*)malloc(sizeof(float) * window_width * window_height);

	if (!color_buffer) {
		fprintf(stderr, "Error allocating color buffer\n");
		is_running = false;
		return;
	}

	// Creating a SDL texture that is used to display the color buffer
	color_buffer_texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_RGBA32,
		SDL_TEXTUREACCESS_STREAMING,
		window_width,
		window_height
	);

	// Initialize the perspective projection matrix
	float aspectx = (float)window_width / (float)window_height;
	float aspecty = (float)window_height / (float)window_width;
	float fovy = M_PI / 3.0; // 60 degrees
	float fovx = 2.0 * atan(tan(fovy / 2.0) * aspectx);
	float z_near = 0.1;
	float z_far = 100.0;
	proj_matrix = mat4_make_perspective(fovy, aspecty, z_near, z_far);

	// Initialize frustum planes with a point and a normal
	init_frustum_planes(fovx, fovy, z_near, z_far);

	// Load the vertex and face values for the mesh data structure
	//load_cube_mesh_data();
	load_obj_file_data("./assets/cube.obj");
	//load_obj_file_data("./assets/f22.obj");

	// Load the texture information from an external PNG file
	load_png_texture_data("./assets/cube.png");
}

void process_input(void) {
	SDL_Event event;
	SDL_PollEvent(&event);

	switch (event.type) {
	case SDL_QUIT:
		is_running = false;
		break;
	case SDL_KEYDOWN:
	{
		switch (event.key.keysym.sym) {
		case SDLK_ESCAPE:
			is_running = false;
			break;
		case SDLK_1:
			render_method = RENDER_WIRE_VERTEX;
			break;
		case SDLK_2:
			render_method = RENDER_WIRE;
			break;
		case SDLK_3:
			render_method = RENDER_FILL_TRIANGLE;
			break;
		case SDLK_4:
			render_method = RENDER_FILL_TRIANGLE_WIRE;
			break;
		case SDLK_5:
			render_method = RENDER_TEXTURED;
			break;
		case SDLK_6:
			render_method = RENDER_TEXTURED_WIRE;
			break;
		case SDLK_c:
			cull_method = CULL_BACKFACE;
			break;
		case SDLK_x:
			cull_method = CULL_NONE;
			break;
		case SDLK_UP:
			camera.position.y += 3.0 * delta_time;
			break;
		case SDLK_DOWN:
			camera.position.y -= 3.0 * delta_time;
			break;
		case SDLK_w:
			camera.forward_velocity = vec3_mul(camera.direction, 5.0 * delta_time);
			camera.position = vec3_add(camera.position, camera.forward_velocity);
			break;
		case SDLK_s:
			camera.forward_velocity = vec3_mul(camera.direction, 5.0 * delta_time);
			camera.position = vec3_sub(camera.position, camera.forward_velocity);
			break;
		case SDLK_a:
			camera.yaw -= 1.0 * delta_time;
			break;
		case SDLK_d:
			camera.yaw += 1.0 * delta_time;
			break;
		}
		break;
	}
	default:
		break;
	}
}

void update(void) {
	int time_to_wait = FRAME_TARGET_TIME - (SDL_GetTicks() - previous_frame_time);
	if (time_to_wait > 0 && time_to_wait <= FRAME_TARGET_TIME) {
		SDL_Delay(time_to_wait);
	}

	// Get a delta time factor converted to seconds to be used to update our game objects
	delta_time = (SDL_GetTicks() - previous_frame_time) / 1000.0;

	previous_frame_time = SDL_GetTicks();

	num_triangles_to_render = 0;

	//mesh.rotation.x += 0.6 * delta_time;
	//mesh.rotation.y += 0.6 * delta_time;
	//mesh.rotation.z += 0.6 * delta_time;
	mesh.translation.z = 5;

	// Initialize the target looking at the positive z-axis
	vec3_t target = { 0, 0, 1 };
	mat4_t camera_yaw_rotation = mat4_make_rotation_y(camera.yaw);
	camera.direction = vec3_from_vec4(mat4_mul_vec4(camera_yaw_rotation, vec4_from_vec3(target)));

	// Offset the camera position in the direction where is the camera pointing at
	target = vec3_add(camera.position, camera.direction);
	vec3_t up_direction = { 0, 1, 0 };

	view_matrix = mat4_look_at(camera.position, target, up_direction);

	// Create a scale matrix that will be used to multiply the mesh vertices
	mat4_t scale_matrix = mat4_make_scale(mesh.scale.x, mesh.scale.y, mesh.scale.z);
	mat4_t translation_matrix = mat4_make_translation(mesh.translation.x, mesh.translation.y, mesh.translation.z);
	mat4_t rotation_matrix_x = mat4_make_rotation_x(mesh.rotation.x);
	mat4_t rotation_matrix_y = mat4_make_rotation_y(mesh.rotation.y);
	mat4_t rotation_matrix_z = mat4_make_rotation_z(mesh.rotation.z);

	int num_faces = array_length(mesh.faces);
	for (int i = 0; i < num_faces; i++) {
		face_t mesh_face = mesh.faces[i];
		vec3_t face_vertices[3] = {
			mesh.vertices[mesh_face.a],
			mesh.vertices[mesh_face.b],
			mesh.vertices[mesh_face.c],
		};

		vec4_t transformed_vertices[3];

		// Loop all three vertices of this current face and apply transformation
		for (int j = 0; j < 3; j++) {
			vec4_t transformed_vertex = vec4_from_vec3(face_vertices[j]);

			// Create a world matrix combining scale, rotation, and translation matrices
			world_matrix = mat4_identity();

			// Order matters: First scale, then rotate, then translate. [T]*[R]*[S]*v
			world_matrix = mat4_mul_mat4(scale_matrix, world_matrix);
			world_matrix = mat4_mul_mat4(rotation_matrix_x, world_matrix);
			world_matrix = mat4_mul_mat4(rotation_matrix_y, world_matrix);
			world_matrix = mat4_mul_mat4(rotation_matrix_z, world_matrix);
			world_matrix = mat4_mul_mat4(translation_matrix, world_matrix);

			// Multiply the world matrix by the original vector
			transformed_vertex = mat4_mul_vec4(world_matrix, transformed_vertex);

			// Multiply the view matrix by the vector to transform the scene to camera space
			transformed_vertex = mat4_mul_vec4(view_matrix, transformed_vertex);

			// Save transformed vertex in the array of transformed vertices
			transformed_vertices[j] = transformed_vertex;
		}

		// Calculate face normal
		vec3_t vector_a = vec3_from_vec4(transformed_vertices[0]); /*   A   */
		vec3_t vector_b = vec3_from_vec4(transformed_vertices[1]); /*  / \  */
		vec3_t vector_c = vec3_from_vec4(transformed_vertices[2]); /* C---B */

		// Get the vector subtraction of B-A and C-A
		vec3_t vector_ab = vec3_sub(vector_b, vector_a);
		vec3_t vector_ac = vec3_sub(vector_c, vector_a);
		vec3_normalize(&vector_ab);
		vec3_normalize(&vector_ac);

		// Compute the face normal (using cross product to find the perpendicular)
		vec3_t normal = vec3_cross(vector_ab, vector_ac);
		vec3_normalize(&normal);

		if (cull_method == CULL_BACKFACE) {
			// Find the vector between a point in the triangle and the camera origin
			vec3_t origin = { 0, 0, 0 };
			vec3_t camera_ray = vec3_sub(origin, vector_a);

			// Calculate how aligned the camera ray is with the face normal (using dot product)
			float dot_normal_camera = vec3_dot(normal, camera_ray);

			// Bypass the triangles that are looking away from the camera
			if (dot_normal_camera < 0) {
				continue;
			}
		}

		// Create a polygon from the original transformed triangle to be clipped
		polygon_t polygon = create_polygon_from_triangle(
			vec3_from_vec4(transformed_vertices[0]),
			vec3_from_vec4(transformed_vertices[1]),
			vec3_from_vec4(transformed_vertices[2]),
			mesh_face.a_uv,
			mesh_face.b_uv,
			mesh_face.c_uv
		);

		// Clip the polygon and returns a new polygon with potential new vertices
		clip_polygon(&polygon);

		// Break the clipped polygon apart back to the individual triangles
		triangle_t triangles_after_clipping[MAX_NUM_POLY_TRIANGLES];
		int num_triangles_after_clipping = 0;

		triangles_from_polygons(&polygon, triangles_after_clipping, &num_triangles_after_clipping);

		// Loops all the assembled triangles after clipping
		for (int i = 0; i < num_triangles_after_clipping; i++) {
			triangle_t triangle_after_clipping = triangles_after_clipping[i];

			vec4_t projected_points[3];
			// Loop all three vertices to perform projection
			for (int j = 0; j < 3; j++) {
				// Project current vertex
				projected_points[j] = mat4_mul_vec4_project(proj_matrix, triangle_after_clipping.points[j]);

				// Scale into view
				projected_points[j].x *= (window_width / 2.0);
				projected_points[j].y *= (window_height / 2.0);

				// Invert the y values to account for flipped screen y coordinate
				projected_points[j].y *= -1;

				// Translate projected point to the middle of the screen
				projected_points[j].x += (window_width / 2.0);
				projected_points[j].y += (window_height / 2.0);
			}

			// Calculate light shading for the face
			float light_intensity = -vec3_dot(normal, global_light.direction);

			uint32_t face_color_lighted = light_apply_intensity(mesh_face.color, light_intensity);

			triangle_t triangle_to_render = {
				.points = {
					{ projected_points[0].x, projected_points[0].y, projected_points[0].z, projected_points[0].w },
					{ projected_points[1].x, projected_points[1].y, projected_points[1].z, projected_points[1].w },
					{ projected_points[2].x, projected_points[2].y, projected_points[2].z, projected_points[2].w },
				},
				.texcoords = {
					{ triangle_after_clipping.texcoords[0].u, triangle_after_clipping.texcoords[0].v },
					{ triangle_after_clipping.texcoords[1].u, triangle_after_clipping.texcoords[1].v },
					{ triangle_after_clipping.texcoords[2].u, triangle_after_clipping.texcoords[2].v }
				},
				.color = face_color_lighted
			};

			if (num_triangles_to_render < MAX_TRIANGLES_TO_RENDER) {
				triangles_to_render[num_triangles_to_render++] = triangle_to_render;
			}
		}
	}
}

void render(void) {
	//draw_grid();

	for (int i = 0; i < num_triangles_to_render; i++) {
		triangle_t triangle = triangles_to_render[i];

		if (render_method == RENDER_FILL_TRIANGLE|| render_method == RENDER_FILL_TRIANGLE_WIRE) {
			// Draw filled triangle
			draw_filled_triangle(
				triangle.points[0].x, triangle.points[0].y, triangle.points[0].z, triangle.points[0].w,
				triangle.points[1].x, triangle.points[1].y, triangle.points[1].z, triangle.points[1].w,
				triangle.points[2].x, triangle.points[2].y, triangle.points[2].z, triangle.points[2].w,
				triangle.color
			);
		}

		// Draw textured triangle
		if (render_method == RENDER_TEXTURED || render_method == RENDER_TEXTURED_WIRE) {
			draw_textured_triangle(
				triangle.points[0].x, triangle.points[0].y, triangle.points[0].z, triangle.points[0].w, triangle.texcoords[0].u, triangle.texcoords[0].v,
				triangle.points[1].x, triangle.points[1].y, triangle.points[1].z, triangle.points[1].w, triangle.texcoords[1].u, triangle.texcoords[1].v,
				triangle.points[2].x, triangle.points[2].y, triangle.points[2].z, triangle.points[2].w, triangle.texcoords[2].u, triangle.texcoords[2].v,
				mesh_texture
			);
		}

		if (render_method == RENDER_WIRE|| 
			render_method == RENDER_WIRE_VERTEX ||
			render_method == RENDER_FILL_TRIANGLE_WIRE ||
			render_method == RENDER_TEXTURED_WIRE)
		{
			// Draw unfilled triangle
			draw_triangle(
				triangle.points[0].x, triangle.points[0].y,
				triangle.points[1].x, triangle.points[1].y,
				triangle.points[2].x, triangle.points[2].y,
				0xFFFFFFFF
			);
		}

		if (render_method == RENDER_WIRE_VERTEX) {
			// Draw vertex points
			draw_rect(triangle.points[0].x - 2, triangle.points[0].y - 2, 4, 4, 0xFFFF0000); // vertex A
			draw_rect(triangle.points[1].x - 2, triangle.points[1].y - 2, 4, 4, 0xFFFF0000); // vertex B
			draw_rect(triangle.points[2].x - 2, triangle.points[2].y - 2, 4, 4, 0xFFFF0000); // vertex C
		}
	}

	render_color_buffer();
	clear_color_buffer(0xFF000000);
	clear_z_buffer();

	SDL_RenderPresent(renderer);
}

void free_resources(void) {
	free(color_buffer);
	free(z_buffer);
	upng_free(png_texture);
	array_free(mesh.faces);
	array_free(mesh.vertices);
}

int main(int argc, char* args[]) {
	
	is_running = initialize_window();

	setup();

	while (is_running) {
		process_input();
		update();
		render();
	}

	destroy_window();
	free_resources();

	return 0;
}
