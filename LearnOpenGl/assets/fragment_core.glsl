#version 330 core
out vec4 FragColor;

in vec4 color;

void main()
{
	//FragColor = vec4(0.1f, 0.2f, 0.6f, 1.0f);
	FragColor = color;
}