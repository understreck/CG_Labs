#version 420

uniform vec3 cameraPosition;

in VS_OUT {
	vec3 vertex;
	vec3 normal;
} fs_in;

out vec4 frag_color;

vec4 colourDeep = vec4(0.0, 0.0, 0.1, 1.0);
vec4 colourShallow = vec4(0.0, 0.5, 0.5, 1.0);

void main()
{
	float facing = 1.0 - max(dot(normalize(cameraPosition - fs_in.vertex), fs_in.normal), 0.0);

	frag_color = mix(colourDeep, colourShallow, facing);
}
