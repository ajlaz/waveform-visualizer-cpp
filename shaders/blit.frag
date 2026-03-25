#version 330 core
uniform sampler2D uScene;
in  vec2 vTexCoord;
out vec4 fragColor;
void main() {
    fragColor = texture(uScene, vTexCoord);
}
