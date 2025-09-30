#version 330 core
layout (location = 0) in vec3 aPos;

out vec4 color;

void main()
{
	gl_Position = vec4(aPos, 1.0);

	if(aPos.x < 0.0)
		color = vec4(1.0, 0.0, 0.0, 1.0); // Red
	else if (aPos.x == 0.0)
		color = vec4(0.0, 1.0, 0.0, 1.0); // Green
	else
		color = vec4(0.0, 0.0, 1.0, 1.0); // Blue)
	
}