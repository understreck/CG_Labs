#version 410
layout (location = 0) in vec3 vertex;

#define NUM_WAVES 2

uniform mat4 vertex_model_to_world;
uniform mat4 vertex_world_to_clip;


uniform float elapsedSeconds;

struct Wave {
	vec2 direction;
	float amplitude;
	float frequency;
	float phase;
	float sharpness;
};

Wave waves[NUM_WAVES] {
	{{-1.0, 0.0}, 1.0, 0.2, 0.5, 2.0},
	{{-0.7, 0.7}, 0.5, 0.4, 1.3, 2.0}
};

out VS_OUT {
	vec3 vertex;
	vec3 normal;
} vs_out;

float alpha(vec2 position, vec2 direction, float frequency, float phase) {
	return
	  sin(
			(direction.x * position.x + direction.y * position.y) * frequency +
			elapsedSeconds * phase
		) * 0.5 + 0.5;
}

//cos-term common to derivatives
float beta(vec2 position, vec2 direction, float frequency, float phase) {
	return
		cos((direction.x * position.x + direction.y * position.y) * frequency +
			elapsedSeconds * phase
		);
}

float Gy(float amplitude, float sharpness, float alpha) {
	return amplitude * pow(alpha, sharpness);
}

float Gdx(float amplitude, float sharpness, float alpha, float beta) {

}

void main()
{
	vs_out.vertex = vec3(vertex_model_to_world * vec4(vertex, 1.0));
	vs_out.normal = vec3(normal_model_to_world * vec4(normal, 0.0));

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
}



