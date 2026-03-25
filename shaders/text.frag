#version 330 core
uniform sampler2D uFont;
uniform vec3 uColor;
in  vec2 vUV;
out vec4 fragColor;
void main() {
    float a = texture(uFont, vUV).r;
    if (a < 0.5) discard;
    fragColor = vec4(uColor, 1.0);
}
