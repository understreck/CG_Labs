#version 420

uniform sampler2D normal_texture;
uniform samplerCube skybox_texture;

uniform vec3 camera_position;

in VS_OUT {
    mat3 BTN;
    vec3 pos;
    vec2 texCoords;
} fs_in;

out vec4 frag_color;

void main() {
    vec3 texNormal = texture(normal_texture, fs_in.texCoords).xyz * 2.0 - 1.0;
    vec3 normal = 
        normalize(fs_in.BTN * texNormal);

    vec3 camToPos = normalize(fs_in.pos - camera_position);

    vec3 skyboxPos = normalize(reflect(camToPos, normal));

    vec3 skybox = texture(skybox_texture, skyboxPos).xyz;
    frag_color = vec4(
            skybox,
            0.2
    );
}
