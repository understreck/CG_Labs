#version 430

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

#define NUM_WAVES 2
#define NUM_WHIRLPOOLS 5
#define M_PI 3.1415926535897932384626433832795

uniform mat4 vertex_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform float elapsedTimeSeconds;
uniform vec3 boatPosition;

layout(std430, binding = 2) buffer WhirlPools {
	vec2 whirlPools[NUM_WHIRLPOOLS];
} whirlPools;

struct Wave {
	vec2 direction;
	float amplitude;
	float frequency;
	float phase;
	float sharpness;
};

uniform Wave mainWave;

const Wave staticWaves[NUM_WAVES] = {
	{{-0.5, 0.0}, 0.1, 0.2, 0.5, 2.0},
	{{-0.7, 0.7}, 0.2, 0.4, 1.3, 2.0}
};

out VS_OUT {
	vec3 tangent;
	vec3 binormal;
	vec3 normal;
	vec3 vertex;
	vec2 textureCoord;

	vec2 whirlPoolCoord;
	float whirlPoolDistance;
} vs_out;

vec3 closest_whirlpool(vec2 position) {
	vec2 v = position - whirlPools.whirlPools[0];
	float closestDistance = length(v);
	for(int i = 1; i < NUM_WHIRLPOOLS; i++) {
		float distance = distance(position, whirlPools.whirlPools[i]);
		if(distance < closestDistance) {
			v = position - whirlPools.whirlPools[i];
			closestDistance = length(v);
		}
	}

	return vec3(v, closestDistance);
}

float alpha(vec2 position, vec2 direction, float frequency, float phase) {
	return
	  sin(
			(direction.x * position.x + direction.y * position.y) * frequency +
			elapsedTimeSeconds * phase
		) * 0.5 + 0.5;
}

//Term common to derivatives
float beta(vec2 position, vec2 direction, float alpha,
					 float amplitude, float frequency, float phase, float sharpness) {
	return
		0.5 * sharpness * frequency * amplitude * pow(alpha, sharpness - 1.0) *
		cos((direction.x * position.x + direction.y * position.y) * frequency +
			elapsedTimeSeconds * phase
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
  			boatPosition.xz + boatPosition.xz, staticWaves[wave].direction, staticWaves[wave].amplitude,
  			staticWaves[wave].frequency, staticWaves[wave].phase, staticWaves[wave].sharpness
  		);
  }

  waveVertex +=
  	G(boatPosition.xz + vec2(50, 50), mainWave.direction, mainWave.amplitude,
  		mainWave.frequency, mainWave.phase, mainWave.sharpness
  	);

	vs_out.tangent = normalize(vec3(1.0, waveVertex.x, 0.0));
	vs_out.binormal = normalize(vec3(0.0, waveVertex.z, 1.0));
	vs_out.normal = normalize(vec3(-waveVertex.x, 1.0, -waveVertex.z));

	vec3 closestWhirlpool = closest_whirlpool(boatPosition.xz + vec2(50, 50));
	vs_out.whirlPoolCoord = closestWhirlpool.xy;
	vs_out.whirlPoolDistance = closestWhirlpool.z;

	if(vs_out.whirlPoolDistance < 5.0) {
		vs_out.vertex = (vertex_model_to_world * vec4(vertex.x, vertex.y + (1 - 1 / pow(vs_out.whirlPoolDistance / 5.0, 2.0)) / 2, vertex.z, 0.0)).xyz;
	}
	else if(vs_out.whirlPoolDistance < 15.0) {
		vs_out.vertex = (vertex_model_to_world * vec4(vertex + mix(waveVertex, vec3(0.0, 0.0, 0.0), 1 - (vs_out.whirlPoolDistance - 5.0) / 10.0), 1.0)).xyz;
	}
	else {
		vs_out.vertex = (vertex_model_to_world * vec4(vertex, 0.0)).xyz + waveVertex;
	}

	gl_Position = vertex_world_to_clip * vec4(vs_out.vertex, 1.0);
}
