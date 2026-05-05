#version 460 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D tex;

void main() {
    FragColor = texture(tex, vUV);
}