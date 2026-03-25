#version 330 core
uniform sampler1D uSpectrum;   // normalised dB [0,1] per pixel column
uniform vec3      uColorLow;
uniform vec3      uColorMid;
uniform vec3      uColorHigh;
uniform vec3      uBackground;
uniform vec3      uTipColor;
uniform float     uTipWidth;
in  vec2 vTexCoord;
out vec4 fragColor;

// Map a 0-1 intensity to a heat-map colour (green → yellow → orange → red)
vec3 heatColour(float t) {
    t = clamp(t, 0.0, 1.0);
    vec3 a = uColorLow;
    vec3 b = uColorMid;
    vec3 c = uColorHigh;
    if (t < 0.5)
        return mix(a, b, t * 2.0);
    else
        return mix(b, c, (t - 0.5) * 2.0);
}

void main() {
    // uSpectrum stores the bar height (normalised 0-1) at each x position.
    // vTexCoord.y is 0 at bottom, 1 at top; the spectrum bar grows from the bottom.
    float barHeight = texture(uSpectrum, vTexCoord.x).r;

    // vTexCoord.y is 0 at bottom, 1 at top; bar grows upward from the bottom.
    float screenY = vTexCoord.y;

    if (screenY > barHeight) {
        // Above the bar → background
        fragColor = vec4(uBackground, 1.0);
        return;
    }

    // Intensity increases toward the bar top (screenY / barHeight ≈ 1 at top)
    float intensity = screenY / max(barHeight, 0.001);

    vec3 col = heatColour(intensity);

    // Soft highlight at the bar tip
    if (barHeight > 0.01) {
        float tipDist = abs(screenY - barHeight);
        float tip = 1.0 - smoothstep(0.0, uTipWidth, tipDist);
        col = mix(col, uTipColor, tip);
    }

    fragColor = vec4(col, 1.0);
}
