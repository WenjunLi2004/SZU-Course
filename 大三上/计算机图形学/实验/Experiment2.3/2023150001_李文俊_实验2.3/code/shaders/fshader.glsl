#version 330 core

in vec3 color;
out vec4 fColor;

uniform float brightness;  // 控制颜色明暗的uniform变量

void main()
{
    // 使用brightness来调节颜色的明暗
    vec3 adjustedColor = color * brightness;
    fColor = vec4(adjustedColor, 1.0);
}

