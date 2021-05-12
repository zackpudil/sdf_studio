#version 330 core

in vec2 tex;
out vec4 out_fragColor;

uniform float exposure;
uniform sampler2D lastPass;

void main() {
    vec4 data = texture(lastPass, tex);
    vec3 col = data.rgb/data.a;

    col = 1.0 - exp(-exposure*col);
    col = pow(abs(col), vec3(1.0/2.2));

    out_fragColor = vec4(col, 1.0);
}