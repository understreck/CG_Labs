#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

uniform mat4 vertex_model_to_world;
uniform mat4 vertex_world_to_clip;

out VS_OUT {
	vec2 texcoord;
	vec3 fragPos;
	vec3 normal;
	mat3 TNB;
} vs_out;


void main()
{
	vs_out.texcoord = texcoord;
	vs_out.fragPos 	= (vertex_model_to_world * vec4(vertex, 1.0)).xyz;
	vs_out.normal 	= normal;
	vs_out.TNB 			= mat3(tangent, normal, binormal);

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
}
