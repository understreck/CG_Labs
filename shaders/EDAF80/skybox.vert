#version 410

layout (location = 0) in vec3 vertex;

uniform mat4 center_on_camera;
uniform mat4 vertex_world_to_clip;

out VS_OUT {
    vec3 texcoord;
} vs_out;


void main()
{
    vs_out.texcoord = vertex;
    gl_Position =
        vertex_world_to_clip * center_on_camera * vec4(vertex, 1.0);
}
