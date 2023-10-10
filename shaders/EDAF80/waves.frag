#version 420

uniform mat4 normal_model_to_world;

uniform samplerCube skyboxTexture;
uniform sampler2D normalTexture;

uniform vec3 cameraPosition;

uniform float elapsedTime;
uniform float fresnelFactor;
uniform bool useNormalMapping;

in VS_OUT {
	vec3 tangent;
	vec3 binormal;
	vec3 normal;
	vec3 vertex;
	vec2 textureCoord;
} fs_in;

out vec4 frag_colour;

const vec4 colourDeep = vec4(0.0, 0.0, 0.1, 1.0);
const vec4 colourShallow = vec4(0.0, 0.5, 0.5, 1.0);

const vec2 normalTextureScale = vec2(8, 4);
float normalTime = mod(elapsedTime, 100.0);
const vec2 normalSpeed = vec2(-0.05, 0.0);

const float refractIdxAir 	= 1.00;
const float refractIdxWater = 1.33;

float fresnel(float refractIdxFrom, float refractIdxTo, float facing, float factor) {
	float R = pow((refractIdxFrom - refractIdxTo) / (refractIdxFrom + refractIdxTo), 2.0);
	return R + (1 - R) * pow(facing, factor);
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

	normal = useNormalMapping ?
	  TNB * normalize(normal) :
		TNB[1];

	normal = normal * (gl_FrontFacing ? 1.0 : -1.0);

	vec3 fragmentToCamera = normalize(cameraPosition - fs_in.vertex);
	float facing = 1.0 - max(dot(fragmentToCamera, normal), 0.0);

	vec4 waterColour = mix(colourDeep, colourShallow, facing);
	vec3 reflection = reflect(-fragmentToCamera, normal);
	vec4 reflectionColour = texture(skyboxTexture, reflection);

	vec3 refraction = refract(
		-fragmentToCamera, normal, gl_FrontFacing ?
			refractIdxAir / refractIdxWater :
			refractIdxWater / refractIdxAir
	);
	vec4 refractionColour = texture(skyboxTexture, refraction);

	float fresnel = gl_FrontFacing ?
		fresnel(refractIdxAir, refractIdxWater, facing, fresnelFactor) :
		fresnel(refractIdxWater, refractIdxAir, facing, fresnelFactor);

	frag_colour = waterColour + reflectionColour * fresnel + refractionColour * (1 - fresnel);
}
