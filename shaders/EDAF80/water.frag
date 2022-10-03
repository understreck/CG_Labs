#version 420

uniform sampler2D normals;

in VS_OUT {
    vec2 texCoords;
    mat3 BTN;
} fs_in;

out vec4 frag_color;

void main() {
    frag_color = vec4(
            normalize(fs_in.BTN * texture(normals, fs_in.texCoords).xyz),
            1.0
    );
}
