#version 420

uniform sampler2D normal_texture;
uniform sampler3D skybox_texture;


in VS_OUT {
    vec2 texCoords;
    mat3 BTN;
} fs_in;

out vec4 frag_color;

void main() {
    frag_color = vec4(
            normalize(fs_in.BTN * texture(normal_texture, fs_in.texCoords).xyz),
            1.0
    );
}
