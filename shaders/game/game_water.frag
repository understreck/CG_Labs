#version 430

uniform mat4 normal_model_to_world;

//uniform samplerCube skyboxTexture;
uniform sampler2D normalTexture;

uniform vec3 cameraPosition;

uniform float elapsedTimeSeconds;

#define M_PI 3.1415926535897932384626433832795

layout(std430, binding = 2) buffer WhirlPools {
	vec2 whirlPools[10];
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

out vec4 frag_colour;

const vec4 colourDeep = vec4(0.0, 0.0, 0.1, 1.0);
const vec4 colourShallow = vec4(0.0, 0.5, 0.5, 1.0);
const vec4 islandColour = vec4(0.0, 0.7, 0.0, 1.0);

const vec2 normalTextureScale = vec2(8, 4);
float normalTime = mod(elapsedTimeSeconds, 100.0);
const vec2 normalSpeed = vec2(-0.05, 0.0);

float whirlpool_calc() {
	return fs_in.whirlPoolCoord.y > 0 ?
			(fs_in.whirlPoolCoord.x / fs_in.whirlPoolDistance + 1) / 4 + 0.5 :
			0.5 - (fs_in.whirlPoolCoord.x / fs_in.whirlPoolDistance + 1) / 4;
}

void main()
{
	#define NUM_NORMAL_COORDS 3
	vec2 normalCoords[NUM_NORMAL_COORDS] = {
		fs_in.textureCoord * normalTextureScale + normalTime * normalSpeed,
		fs_in.textureCoord * normalTextureScale * 2 + normalTime * normalSpeed * 4,
		fs_in.textureCoord * normalTextureScale * 4 + normalTime * normalSpeed * 8,
	};

	vec3 normal = vec3(0.0, 0.0, 0.0);
	for(uint i = 0; i < NUM_NORMAL_COORDS; i++) {
		normal += texture(normalTexture, normalCoords[i]).rbg * 2.0 - 1.0;
	}

	mat3 TNB = {
		normalize(fs_in.tangent), normalize(fs_in.normal), normalize(fs_in.binormal)
	};

	normal = TNB * normalize(normal);

	normal = normal;

	vec3 fragmentToCamera = normalize(cameraPosition - fs_in.vertex);
	float facing = 1.0 - max(dot(fragmentToCamera, normal), 0.0);

	vec4 waterColour = mix(colourDeep, colourShallow, facing);

	float wp = whirlpool_calc();

	if(fs_in.whirlPoolDistance < 10.0) {
		waterColour = vec4(vec3(1.0, 1.0, 1.0) * cos(M_PI * fs_in.whirlPoolDistance + acos(wp) * 4 + M_PI * elapsedTimeSeconds), 1.0);
	}

	frag_colour = waterColour;
}
