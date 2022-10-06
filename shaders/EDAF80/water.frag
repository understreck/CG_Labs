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

    float diffuseReflected = max(dot(-lightVec, normal), 0.0);
    float specularReflected = pow(max(dot(-camToPos, normal), 0.0), 16.0);
    vec3 surfaceColour = vec3(0.2, 0.2, 0.9);
    vec3 ambientColour = vec3(0.1);

    vec3 skybox = texture(skybox_texture, skyboxPos).xyz;
    frag_color = vec4(
            surfaceColour * diffuseReflected +
            surfaceColour * specularReflected
            //skybox * 0.5,
            //vec3(1.0) * amountReflected,
            //(colour + (vec3(1.0) - colour) * amountReflected * 0.4) * 0.7,
            //skybox,
            ,1.0
    );
}
