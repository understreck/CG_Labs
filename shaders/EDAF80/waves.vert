#version 420
layout (location = 0) in vec3 vertex;
layout (location = 2) in vec2 textureCoord;

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

Wave waves[NUM_WAVES] = {
	{{-1.0, 0.0}, 1.0, 0.2, 0.5, 2.0},
	{{-0.7, 0.7}, 0.5, 0.4, 1.3, 2.0}
};

out VS_OUT {
	vec3 tangent;
	vec3 binormal;
	vec3 normal;
	vec3 vertex;
	vec2 textureCoord;
} vs_out;

float alpha(vec2 position, vec2 direction, float frequency, float phase) {
	return
	  sin(
			(direction.x * position.x + direction.y * position.y) * frequency +
			elapsedSeconds * phase
		) * 0.5 + 0.5;
}

//Term common to derivatives
float beta(vec2 position, vec2 direction, float alpha,
					 float amplitude, float frequency, float phase, float sharpness) {
	return
		0.5 * sharpness * frequency * amplitude * pow(alpha, sharpness - 1.0) *
		cos((direction.x * position.x + direction.y * position.y) * frequency +
			elapsedSeconds * phase
		);
}

float Gy(float amplitude, float sharpness, float alpha) {
	return amplitude * pow(alpha, sharpness);
}

float Gdx(vec2 direction, float beta) {
	return beta * direction.x;
}

float Gdz(vec2 direction, float beta) {
	return beta * direction.y;
}

vec3 G(vec2 position, vec2 direction,
			 float amplitude, float frequency, float phase, float sharpness) {
	float alphaTerm = alpha(position, direction, frequency, phase);
	float betaTerm = beta(
		position, direction, alphaTerm,
		amplitude, frequency, phase, sharpness);

	return vec3(
		Gdx(direction, betaTerm),
		Gy(amplitude, sharpness, alphaTerm),
		Gdz(direction, betaTerm)
	);
}

void main()
{
	vec3 waveVertex = vec3(0.0, 0.0, 0.0);
	for(uint wave = 0; wave < NUM_WAVES; wave++) {
		waveVertex +=
			G(
				vertex.xz, waves[wave].direction, waves[wave].amplitude,
				waves[wave].frequency, waves[wave].phase, waves[wave].sharpness
			);
	}

	vs_out.tangent = normalize(vec3(1.0, waveVertex.x, 0.0));
	vs_out.binormal = normalize(vec3(0.0, waveVertex.z, 1.0));
	vs_out.normal = normalize(vec3(-waveVertex.x, 1.0, -waveVertex.z));

	vs_out.vertex = vec3(vertex_model_to_world * vec4(vertex + waveVertex, 1.0));
	vs_out.textureCoord = textureCoord;

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex + waveVertex, 1.0);
}



