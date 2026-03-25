#version 330 core
// Circular scrolling waveform texture (lows=red, mids=green, highs=blue, additive blended on CPU).
uniform sampler2D uWaveform;
uniform float     uWriteCol;   // normalised write column [0,1]

in  vec2 vTexCoord;
out vec4 fragColor;

void main() {
    // Offset x UV by write position so the waveform scrolls left without any CPU copy
    float scrolledU = fract(vTexCoord.x + uWriteCol);
    fragColor = texture(uWaveform, vec2(scrolledU, vTexCoord.y));

}