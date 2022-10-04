#version 420

uniform sampler2D normal_texture;
uniform samplerCube skybox_texture;

uniform vec3 camera_position;

in VS_OUT {
    mat3 BNT;
    vec3 pos;
    vec2 texCoords;
} fs_in;

out vec4 frag_color;

vec3 lightVec = normalize(vec3(0.0, -1.0, 0.0));

void main() {
    vec3 texNormal = texture(normal_texture, fs_in.texCoords).xzy * 2.0 - 1.0;
    vec3 normal = 
        normalize(fs_in.BNT * texNormal);
        //normalize(fs_in.BNT * vec3(0.0, 1.0, 0.0));

    vec3 camToPos = normalize(fs_in.pos - camera_position);

    vec3 skyboxPos = normalize(reflect(camToPos, normal));

    float amountReflected = pow(abs(dot(-lightVec, normal)), 32.0);
    vec3 colour = vec3(0.1, 0.1, 0.5);

    vec3 skybox = texture(skybox_texture, skyboxPos).xyz;
    frag_color = vec4(
            skybox * 0.3 +
            //vec3(1.0) * amountReflected,
            (colour + (vec3(1.0) - colour) * amountReflected * 0.4) * 0.7,
            //skybox,
            1.0
    );
}
