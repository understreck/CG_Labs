#version 410

uniform sampler2D diffuse_texture;
uniform sampler2D specular_texture;
uniform sampler2D normal_texture;

uniform vec3 light_position;
uniform vec3 camera_position;

uniform vec3 ambient_colour;
uniform vec3 diffuse_colour;
uniform vec3 specular_colour;

uniform float shininess_value;
uniform int   use_normal_mapping;

uniform mat4 normal_model_to_world;

in VS_OUT {
    vec2 texcoord;
    vec3 vertex;
    mat3 TBN;
} fs_in;

out vec4 frag_colour;

void main()
{
    vec3 diffuseTex = texture(diffuse_texture, fs_in.texcoord).xyz;
    vec3 specularTex = texture(specular_texture, fs_in.texcoord).xyz;
    vec3 normalTex = normalize(
            texture(normal_texture, fs_in.texcoord).xyz * 2.0 - 1.0);

    vec3 ambient = ambient_colour * diffuseTex;

    vec3 lightDir = normalize(light_position - fs_in.vertex);

    vec3 normal = use_normal_mapping== 1?
    normalize(fs_in.TBN * normalTex) :
    fs_in.TBN[2];

    normal = vec3(normal_model_to_world * vec4(normal, 0.0)).xyz;

    vec3 diffuse =
        clamp(dot(normal, lightDir), 0.0, 1.0) *
        diffuseTex;

    vec3 viewDir            = normalize(camera_position - fs_in.vertex);
    vec3 lightReflectDir    = reflect(lightDir, normal);
    float specularVal       = pow(
            clamp(dot(-viewDir, lightReflectDir), 0.0, 1.0),
            shininess_value);

    vec3 specular = specularVal * specular_colour *specularTex;

    frag_colour =
        vec4(ambient + diffuse + specularVal, 1.0);
}
