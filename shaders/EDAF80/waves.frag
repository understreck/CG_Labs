#version 420

uniform samplerCube skyboxTexture;

uniform vec3 cameraPosition;

in VS_OUT {
	vec3 vertex;
	vec3 normal;
} fs_in;

out vec4 frag_colour;

vec4 colourDeep = vec4(0.0, 0.0, 0.1, 1.0);
vec4 colourShallow = vec4(0.0, 0.5, 0.5, 1.0);

void main()
{
	vec3 fragmentToCamera = normalize(cameraPosition - fs_in.vertex);
	float facing = 1.0 - max(dot(fragmentToCamera, fs_in.normal), 0.0);

	vec4 waterColour = mix(colourDeep, colourShallow, facing);
	vec3 reflection = reflect(-fragmentToCamera, fs_in.normal);
	vec4 reflectionColour = texture(skyboxTexture, reflection);

	frag_colour = waterColour + reflectionColour;
}
