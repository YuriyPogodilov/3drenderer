#ifndef CAMERA_H
#define CAMERA_H

#include "vector.h"

typedef struct {
	vec3_t position;
	vec3_t direction;
	vec3_t forward_velocity;
	float yaw;
	float pitch;
} camera_t;

void init_camera(vec3_t position, vec3_t direction);

vec3_t get_camera_position(void);
vec3_t get_camera_direction(void);
vec3_t get_camera_forward_velocity(void);

void update_camera_position(vec3_t position);
void update_camera_direction(vec3_t direction);
void update_camera_forward_velocity(vec3_t forward_velocity);

float get_camera_yaw(void);
float get_camera_pitch(void);

void rotate_camera_yaw(float yaw);
void rotate_camera_pitch(float pitch);

vec3_t get_camera_lookat_target(void);

#endif