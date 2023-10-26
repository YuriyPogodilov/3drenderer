#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mesh.h"
#include "array.h"

#define MAXIMUM_NUM_MESHES 10
#define STRING_MAX_LENGTH 512

static mesh_t meshes[MAXIMUM_NUM_MESHES];
static int mesh_count = 0;

void load_mesh(const char* obj_filename, const char* png_filename, vec3_t scale, vec3_t translation, vec3_t rotation)
{
	load_mesh_obj_data(&meshes[mesh_count], obj_filename);
	load_mesh_png_data(&meshes[mesh_count], png_filename);

	meshes[mesh_count].scale = scale;
	meshes[mesh_count].translation = translation;
	meshes[mesh_count].rotation = rotation;

	mesh_count++;
}

void load_mesh_obj_data(mesh_t* mesh, const char* obj_filename) {
	char line[STRING_MAX_LENGTH];
	char* token = NULL;
	char space_delim[2] = " ";

	FILE* file = NULL;
	if (fopen_s(&file, obj_filename, "r") != 0) {
		return;
	}

	tex2_t* tex_coords = NULL;

	while (fgets(line, STRING_MAX_LENGTH, file)) {
		// Vertex information
		if (strncmp(line, "v ", 2) == 0) {
			vec3_t vertex;
			sscanf_s(line, "v %f %f %f", &vertex.x, &vertex.y, &vertex.z);
			array_push(mesh->vertices, vertex);
		}

		// Texture coordinate information
		if (strncmp(line, "vt ", 3) == 0) {
			tex2_t tex_coord;
			sscanf_s(line, "vt %f %f", &tex_coord.u, &tex_coord.v);
			// Flip the V component to account for inverted UV-coordinates (V grows downwards)
			tex_coord.v = 1 - tex_coord.v;
			array_push(tex_coords, tex_coord);
		}

		// Face information
		if (strncmp(line, "f ", 2) == 0) {
			int vertex_indices[3];
			int texture_indices[3];
			int normal_indices[3];
			sscanf_s(
				line, "f %d/%d/%d %d/%d/%d %d/%d/%d", 
				&vertex_indices[0], &texture_indices[0], &normal_indices[0],
				&vertex_indices[1], &texture_indices[1], &normal_indices[1],
				&vertex_indices[2], &texture_indices[2], &normal_indices[2]
			);
			face_t face = {
				.a = vertex_indices[0] - 1,
				.b = vertex_indices[1] - 1,
				.c = vertex_indices[2] - 1,
				.a_uv = tex_coords[texture_indices[0] - 1],
				.b_uv = tex_coords[texture_indices[1] - 1],
				.c_uv = tex_coords[texture_indices[2] - 1],
				.color = 0xFFFFFFFF
			};
			array_push(mesh->faces, face);
		}
	}

	array_free(tex_coords);
}

void load_mesh_png_data(mesh_t* mesh, const char* png_filename)
{
	upng_t* png_image = upng_new_from_file(png_filename);
	if (png_image != NULL) {
		upng_decode(png_image);
		if (upng_get_error(png_image) == UPNG_EOK)
		{
			mesh->texture = png_image;
		}
	}
}

int get_num_meshes()
{
	return mesh_count;
}

mesh_t* get_mesh(int index) {
	if (index >= MAXIMUM_NUM_MESHES)
	{
		return NULL;
	}

	return &meshes[index];
}

void free_meshes(void) 
{
	for (int i = 0; i < mesh_count; i++)
	{
		upng_free(meshes[i].texture);
		array_free(meshes[i].faces);
		array_free(meshes[i].vertices);
	}
}