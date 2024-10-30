#version 130

in vec4 vertex;
out vec4 pos;

void main()
{
	gl_Position = vertex;
	pos = gl_Position;
}
