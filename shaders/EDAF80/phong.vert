#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

// This is the custom output of this shader. If you want to retrieve this data
// from another shader further down the pipeline, you need to declare the exact
// same structure as in (for input), with matching name for the structure
// members and matching structure type. Have a look at
// shaders/EDAF80/diffuse.frag.
out VS_OUT {
    vec2 texcoord;
    vec3 vertex;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
} vs_out;


void main()
{
    vs_out.texcoord = texcoord.xy;
    vs_out.vertex = vec3(vertex_model_to_world * vec4(vertex, 1.0));
    vs_out.normal = vec3(normal_model_to_world * vec4(normal, 0.0));
    vs_out.normal = vec3(normal_model_to_world * vec4(normal, 0.0));
    vs_out.normal = vec3(normal_model_to_world * vec4(normal, 0.0));

    gl_Position = 
        vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
}



