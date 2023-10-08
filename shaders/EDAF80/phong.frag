#version 410

uniform sampler2D normal_texture;
uniform sampler2D specular_texture;
uniform sampler2D diffuse_texture;

uniform vec3 light_position;
uniform vec3 camera_position;

uniform bool use_normal_mapping;
uniform float shininess_value;

uniform vec3 ambient_colour;
uniform vec3 diffuse_colour;
uniform vec3 specular_colour;

out vec4 frag_color;

in VS_OUT {
	vec2 texcoord;
	vec3 fragPos;
	vec3 normal;
	mat3 TBN;
} fs_in;

void main() {
	vec3 diffuse_texture_colour = texture2D(diffuse_texture, fs_in.texcoord).rbg;
	vec3 specular_texture_colour = texture2D(specular_texture, fs_in.texcoord).rbg;
	vec3 normal_texture_colour = use_normal_mapping ?
		normalize(texture2D(normal_texture, fs_in.texcoord).xyz * 2.0 - 1.0) : vec3(0.0, 0.0, 0.0);

	vec3 fragPosToLight = normalize(light_position - fs_in.fragPos);
	vec3 fragPosToCamera = normalize(camera_position - fs_in.fragPos);

	vec3 normal = use_normal_mapping ?
		fs_in.TBN * normal_texture_colour :
		fs_in.normal;

	frag_color = vec4(
		ambient_colour * diffuse_colour +
		diffuse_colour * diffuse_texture_colour * max(dot(normal, fragPosToLight), 0.0) +
		specular_colour * specular_texture_colour *
			pow(max(dot(reflect(-fragPosToLight, normal), fragPosToCamera), 0.0), shininess_value)
		,
		1.0f);
}
