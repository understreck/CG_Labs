#version 410

in VS_OUT {
    vec2 texcoord;
} fs_in;

out vec4 frag_color;

void main() {
    frag_color = vec4(1.0);
}
