#version 410

uniform samplerCube skybox_texture;

in VS_OUT {
	vec3 texcoord;
} fs_in;

out vec4 frag_color;

void main()
{
		frag_color = textureCube(skybox_texture, fs_in.texcoord);
}
