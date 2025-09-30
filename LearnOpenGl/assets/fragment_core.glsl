#version 330 core
out vec4 FragColor;
uniform vec4 ourColor; // CPU’dan gelen renk
void main()
{
    FragColor = ourColor;
}
