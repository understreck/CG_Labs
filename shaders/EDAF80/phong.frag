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

in VS_OUT {
    vec2 texcoord;
    vec3 vertex;
    vec3 normal;
} fs_in;

out vec4 frag_colour;

void main()
{
    vec3 lightDir = normalize(light_position - fs_in.vertex);

    vec3 diffuseTex = texture(diffuse_texture, fs_in.texcoord).xyz;
    vec3 specularTex = texture(specular_texture, fs_in.texcoord).xyz;
    vec3 normalTex = texture(normal_texture, fs_in.texcoord).xyz;

    vec3 ambient = ambient_colour * diffuseTex;
    vec3 diffuse = clamp(dot(normalize(fs_in.normal), lightDir), 0.0, 1.0) * diffuseTex;

    frag_colour =
        vec4(ambient + diffuse, 1.0);
        //clamp(dot(normalize(fs_in.normal), lightDir), 0.0, 1.0);
}
