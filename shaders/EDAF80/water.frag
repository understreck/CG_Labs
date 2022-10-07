#version 420

uniform sampler2D normal_texture;
uniform samplerCube skybox_texture;

uniform vec3 camera_position;

uniform float fresnel_factor;

uniform mat4 normal_model_to_world;

in VS_OUT {
    mat3 BTN;
    vec3 pos;
    vec2 normalCoord0;
    vec2 normalCoord1;
    vec2 normalCoord2;
} fs_in;

out vec4 frag_color;

vec3 lightVec = normalize(vec3(0.0, -1.0, 0.0));

void main() {
    vec3 texNormal0 =
        texture(normal_texture, fs_in.normalCoord0).xyz * 2.0 - 1.0;
    vec3 texNormal1 =
        texture(normal_texture, fs_in.normalCoord1).xyz * 2.0 - 1.0;
    vec3 texNormal2 =
        texture(normal_texture, fs_in.normalCoord2).xyz * 2.0 - 1.0;
    vec3 texNormal = normalize(texNormal0 + texNormal1 + texNormal2);
    vec3 normal = 
        normalize(fs_in.BTN * texNormal);
        //fs_in.BTN * vec3(0.0, 0.0, 1.0);
    float facingSign = gl_FrontFacing ? 1.0 : -1.0;
    normal =
        normalize(vec4(normal_model_to_world * vec4(normal, 0.0)).xyz) *
        facingSign;

    vec3 camToPos = normalize(fs_in.pos - camera_position);

    vec4 deepC = gl_FrontFacing ?
        vec4(0.0, 0.0, 0.1, 1.0) : vec4(0.0, 0.4, 0.4, 1.0);
    vec4 shallowC = gl_FrontFacing ?
        vec4(0.0, 0.5, 0.5, 1.0) : vec4(0.0, 0.3, 0.3, 1.0);
    float facing =
        1.0 - max(dot(-camToPos, normal), 0.0);
    vec4 waterC = mix(deepC, shallowC, facing);

    const float refIndexAir = 1.0;
    const float refIndexWater = 1.33;

    const float airToWater = pow(
        (refIndexAir - refIndexWater) / (refIndexAir + refIndexWater), 2.0);
    const float waterToAir = pow(
        (refIndexWater - refIndexAir) / (refIndexWater + refIndexAir), 2.0);

    float fresnelAtW = 
        airToWater +
        (1.0 - airToWater) * pow(facing, fresnel_factor);
    float fresnelWtA = 
        waterToAir +
        (1.0 - waterToAir) * pow(facing, fresnel_factor);

    vec3 reflectDir = reflect(camToPos, normal);
    vec4 skyboxReflect = texture(skybox_texture, reflectDir);
    vec3 refractDir = refract(
        camToPos, normal,
        gl_FrontFacing ?
            (refIndexAir / refIndexWater) : (refIndexWater / refIndexAir)
    );
    vec4 skyboxRefract = texture(skybox_texture, refractDir);

    frag_color =
        waterC +
        skyboxReflect * (gl_FrontFacing ? fresnelAtW : fresnelWtA) +
        skyboxRefract * (1.0 - (gl_FrontFacing ? fresnelAtW : fresnelWtA));
}
