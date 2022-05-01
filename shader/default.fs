#version 330 core
out vec4 out_color;

in vec3 fs_color;

void main()
{
	out_color = vec4(fs_color,1);
}
