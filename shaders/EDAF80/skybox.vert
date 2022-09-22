#version 410

layout (location = 0) in vec3 vertex;

uniform vec3 camera_position;
uniform mat4 vertex_model_to_world;
uniform mat4 vertex_world_to_clip;

out VS_OUT {
    vec3 texcoord;
} vs_out;


void main()
{
    vs_out.texcoord = vertex;
    gl_Position =
        vertex_world_to_clip  *
            (vertex_model_to_world * vec4(vertex, 1.0) 
                + vec4(camera_position, 0.0));
}
