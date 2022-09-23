
#version 410

uniform vec3 light_position;

in VS_OUT {
    vec2 texcoord;
    vec3 vertex;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
} fs_in;

out vec4 frag_color;

void main()
{
    vec3 L = normalize(light_position - fs_in.vertex);
    frag_color = vec4(1.0) * clamp(dot(normalize(fs_in.normal), L), 0.0, 1.0);
}
