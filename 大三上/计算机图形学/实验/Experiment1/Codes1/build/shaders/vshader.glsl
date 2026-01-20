#version 330 core

in vec3 vPosition;
uniform vec3 uColor;

out vec3 color;

void main()
{
    gl_Position = vec4(vPosition, 1.0);
    color = uColor;
    gl_PointSize = 2.0;
}

