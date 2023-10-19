#version 430

#define NUM_WHIRLPOOLS 5
#define M_PI 3.1415926535897932384626433832795

uniform vec3 ambient_colour;
uniform vec3 diffuse_colour;
uniform vec3 specular_colour;

uniform vec3 cameraPosition;

uniform float elapsedTimeSeconds;

layout(std430, binding = 2) buffer WhirlPools {
	vec2 whirlPools[NUM_WHIRLPOOLS];
} whirlPools;

in VS_OUT {
	vec3 tangent;
	vec3 binormal;
	vec3 normal;
	vec3 vertex;
	vec2 textureCoord;

	vec2 whirlPoolCoord;
	float whirlPoolDistance;
} fs_in;

out vec4 frag_color;

void main() {
	frag_color = vec4(diffuse_colour, 1.0);
}
