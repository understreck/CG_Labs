#version 410

in VS_OUT {
    vec2 pos;
} fs_in;

out vec4 frag_color;

void main() {
    frag_color = vec4(normalize(fs_in.pos), 0.0, 1.0);
}
