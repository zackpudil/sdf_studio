#version 330 core

in vec2 tex;
out vec4 out_fragColor;

uniform sampler2D mainImage;

void main() {
    out_fragColor = texture(mainImage, tex);
}