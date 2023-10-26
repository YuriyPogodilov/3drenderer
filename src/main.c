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
	set_render_method(RENDER_WIRE);
	set_cull_method(CULL_BACKFACE);

	init_light(vec3_new(0, 0, 1));

	// Initialize the perspective projection matrix
	float aspectx = (float)get_window_width() / (float)get_window_height();
	float aspecty = (float)get_window_height() / (float)get_window_width();
	float fovy = M_PI / 3.0; // 60 degrees
	float fovx = 2.0 * atan(tan(fovy / 2.0) * aspectx);
	float z_near = 0.1;
	float z_far = 100.0;
	proj_matrix = mat4_make_perspective(fovy, aspecty, z_near, z_far);

	// Initialize frustum planes with a point and a normal
	init_frustum_planes(fovx, fovy, z_near, z_far);
	
	load_mesh("./assets/f22.obj", "./assets/f22.png", vec3_new(1, 1, 1), vec3_new(-3, 0, 0), vec3_new(0, 0, 0));
	load_mesh("./assets/efa.obj", "./assets/efa.png", vec3_new(1, 1, 1), vec3_new(+3, 0, 0), vec3_new(0, 0, 0));
}

void process_input(void) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
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
				set_render_method(RENDER_WIRE_VERTEX);
				break;
			case SDLK_2:
				set_render_method(RENDER_WIRE);
				break;
			case SDLK_3:
				set_render_method(RENDER_FILL_TRIANGLE);
				break;
			case SDLK_4:
				set_render_method(RENDER_FILL_TRIANGLE_WIRE);
				break;
			case SDLK_5:
				set_render_method(RENDER_TEXTURED);
				break;
			case SDLK_6:
				set_render_method(RENDER_TEXTURED_WIRE);
				break;
			case SDLK_c:
				set_cull_method(CULL_BACKFACE);
				break;
			case SDLK_x:
				set_cull_method(CULL_NONE);
				break;
			case SDLK_UP:
			{
				update_camera_forward_velocity(vec3_mul(get_camera_direction(), 5.0 * delta_time));
				update_camera_position(vec3_add(get_camera_position(), get_camera_forward_velocity()));
				break;
			}
			case SDLK_DOWN:
			{
				update_camera_forward_velocity(vec3_mul(get_camera_direction(), 5.0 * delta_time));
				update_camera_position(vec3_sub(get_camera_position(), get_camera_forward_velocity()));
				break;
			}
			case SDLK_a:
			{
				rotate_camera_yaw(-1.0 * delta_time);
				break;
			}
			case SDLK_d:
			{
				rotate_camera_yaw(1.0 * delta_time);
				break;
			}
			case SDLK_w:
			{
				rotate_camera_pitch(+3.0 * delta_time);
				break;
			}
			case SDLK_s:
			{
				rotate_camera_pitch(-3.0 * delta_time);
				break;
			}
			default:
				break;
			}
			break;
		}
		default:
			break;
		}
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

	for (int i = 0; i < get_num_meshes(); i++) {
		mesh_t* mesh = get_mesh(i);
		//mesh.rotation.x += 0.6 * delta_time;
		//mesh.rotation.y += 0.6 * delta_time;
		//mesh.rotation.z += 0.6 * delta_time;
		//mesh.translation.z = 5;

		// Create a scale matrix that will be used to multiply the mesh vertices
		mat4_t scale_matrix = mat4_make_scale(mesh->scale.x, mesh->scale.y, mesh->scale.z);
		mat4_t translation_matrix = mat4_make_translation(mesh->translation.x, mesh->translation.y, mesh->translation.z);
		mat4_t rotation_matrix_x = mat4_make_rotation_x(mesh->rotation.x);
		mat4_t rotation_matrix_y = mat4_make_rotation_y(mesh->rotation.y);
		mat4_t rotation_matrix_z = mat4_make_rotation_z(mesh->rotation.z);

		// Update camera look at target to create view matrix
		vec3_t target = get_camera_lookat_target();
		vec3_t up_direction = vec3_new(0, 1, 0);
		view_matrix = mat4_look_at(get_camera_position(), target, up_direction);

		int num_faces = array_length(mesh->faces);
		for (int i = 0; i < num_faces; i++) {
			face_t mesh_face = mesh->faces[i];
			vec3_t face_vertices[3] = {
				mesh->vertices[mesh_face.a],
				mesh->vertices[mesh_face.b],
				mesh->vertices[mesh_face.c],
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

			if (is_cull_backface()) {
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
					projected_points[j].x *= (get_window_width() / 2.0);
					projected_points[j].y *= (get_window_height() / 2.0);

					// Invert the y values to account for flipped screen y coordinate
					projected_points[j].y *= -1;

					// Translate projected point to the middle of the screen
					projected_points[j].x += (get_window_width() / 2.0);
					projected_points[j].y += (get_window_height() / 2.0);
				}

				// Calculate light shading for the face
				float light_intensity = -vec3_dot(normal, get_light_direction());

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
}

void render(void) {
	clear_color_buffer(0xFF000000);
	clear_z_buffer();

	draw_grid();

	for (int i = 0; i < num_triangles_to_render; i++) {
		triangle_t triangle = triangles_to_render[i];

		if (should_render_filled_triangle()) {
			// Draw filled triangle
			draw_filled_triangle(
				triangle.points[0].x, triangle.points[0].y, triangle.points[0].z, triangle.points[0].w,
				triangle.points[1].x, triangle.points[1].y, triangle.points[1].z, triangle.points[1].w,
				triangle.points[2].x, triangle.points[2].y, triangle.points[2].z, triangle.points[2].w,
				triangle.color
			);
		}

		// Draw textured triangle
		if (should_render_textured_triangle()) {
			//draw_textured_triangle(
			//	triangle.points[0].x, triangle.points[0].y, triangle.points[0].z, triangle.points[0].w, triangle.texcoords[0].u, triangle.texcoords[0].v,
			//	triangle.points[1].x, triangle.points[1].y, triangle.points[1].z, triangle.points[1].w, triangle.texcoords[1].u, triangle.texcoords[1].v,
			//	triangle.points[2].x, triangle.points[2].y, triangle.points[2].z, triangle.points[2].w, triangle.texcoords[2].u, triangle.texcoords[2].v,
			//	mesh_texture
			//);
		}

		if (should_render_wireframe())
		{
			// Draw unfilled triangle
			draw_triangle(
				triangle.points[0].x, triangle.points[0].y,
				triangle.points[1].x, triangle.points[1].y,
				triangle.points[2].x, triangle.points[2].y,
				0xFFFFFFFF
			);
		}

		if (should_render_wire_vertex()) {
			// Draw vertex points
			draw_rect(triangle.points[0].x - 2, triangle.points[0].y - 2, 4, 4, 0xFFFF0000); // vertex A
			draw_rect(triangle.points[1].x - 2, triangle.points[1].y - 2, 4, 4, 0xFFFF0000); // vertex B
			draw_rect(triangle.points[2].x - 2, triangle.points[2].y - 2, 4, 4, 0xFFFF0000); // vertex C
		}
	}

	render_color_buffer();
}

void free_resources(void)
{
	free_meshes();
	destroy_window();
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
