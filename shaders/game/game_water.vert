#version 430
layout (location = 0) in vec3 vertex;
layout (location = 2) in vec2 textureCoord;

#define NUM_WAVES 2

uniform mat4 vertex_model_to_world;
uniform mat4 vertex_world_to_clip;


uniform float elapsedTimeSeconds;

struct Wave {
	vec2 direction;
	float amplitude;
	float frequency;
	float phase;
	float sharpness;
};

uniform Wave mainWave;

uniform vec3 island;

const Wave staticWaves[NUM_WAVES] = {
	{{-0.5, 0.0}, 0.01, 0.2, 0.5, 2.0},
	{{-0.7, 0.7}, 0.02, 0.4, 1.3, 2.0}
};

out VS_OUT {
	vec3 tangent;
	vec3 binormal;
	vec3 normal;
	vec3 vertex;
	vec2 textureCoord;
	float distanceToIsland;
} vs_out;

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
	vec2 islandToVertex = (vertex_model_to_world * vec4(vertex, 1.0)).xz - island.xy;
	float distanceToIsland = max(sqrt(dot(islandToVertex, islandToVertex)) - island.z, 0.0);
	vs_out.distanceToIsland = distanceToIsland;

	vec3 waveVertex = vec3(0.0, 0.0, 0.0);

	if(distanceToIsland > 0.0) {
    float dampen = distanceToIsland < 10.0 ?
		  pow((distanceToIsland / 10.0), 2) :
			1.0;

    for(uint wave = 0; wave < NUM_WAVES; wave++) {
    	waveVertex +=
    		G(
    			vertex.xz, staticWaves[wave].direction, staticWaves[wave].amplitude * dampen,
    			staticWaves[wave].frequency, staticWaves[wave].phase, staticWaves[wave].sharpness
    		);
    }

    waveVertex +=
    	G(
    		vertex.xz, mainWave.direction, mainWave.amplitude * dampen,
    		mainWave.frequency, mainWave.phase, mainWave.sharpness
    	);
	}
	else {
		waveVertex += vec3(0.0, 10.0, 0.0);
	}

	vs_out.tangent = normalize(vec3(1.0, waveVertex.x, 0.0));
	vs_out.binormal = normalize(vec3(0.0, waveVertex.z, 1.0));
	vs_out.normal = normalize(vec3(-waveVertex.x, 1.0, -waveVertex.z));

	vs_out.vertex = vec3(vertex_model_to_world * vec4(vertex + waveVertex, 1.0));
	vs_out.textureCoord = textureCoord;

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex + waveVertex, 1.0);
}



