#version 420

in VS_OUT {
	vec3 vertex;
} fs_in;

out vec4 frag_color;

void main()
{
	frag_color = vec4(vec3(fs_in.vertex.y), 1.0);
}
