#version 330 core
uniform sampler2D uScroll;     // circular RGB heat-map texture
uniform float     uWriteCol;   // normalised write position [0,1]
in  vec2 vTexCoord;
out vec4 fragColor;

void main() {
    // Offset x UV so the display scrolls right-to-left without CPU memory copies
    float scrolledU = fract(vTexCoord.x + uWriteCol);
    fragColor = texture(uScroll, vec2(scrolledU, vTexCoord.y));
}
