#ifndef MESH_H
#define MESH_H

#include "vector.h"
#include "triangle.h"
#include "upng.h"

// Define a struct for dynamic size meshes, with array of vertices and faces;
typedef struct {
	vec3_t* vertices;
	face_t* faces;
	upng_t* texture;
	vec3_t rotation;
	vec3_t scale;
	vec3_t translation;
} mesh_t;

void load_mesh(const char* obj_filename, const char* png_filename, vec3_t scale, vec3_t translation, vec3_t rotation);
void load_mesh_obj_data(mesh_t* mesh, const char* obj_filename);
void load_mesh_png_data(mesh_t* mesh, const char* png_filename);

int get_num_meshes();
mesh_t* get_mesh(int index);

void free_meshes(void);

#endif