#version 330 core
uniform sampler2D uAccum;       // GL_R32F accumulation buffer
uniform vec3      uBackground;
uniform vec3      uDot;
uniform vec2      uResolution;  // viewport pixel dimensions

in  vec2 vTexCoord;
out vec4 fragColor;

void main() {
    // Map the quad to a square centred in the viewport.
    float aspect = uResolution.x / uResolution.y;
    vec2 uv = vTexCoord;
    if (aspect >= 1.0) {
        // Landscape: letterbox left/right
        float pad = 0.5 * (1.0 - 1.0 / aspect);
        uv.x = (uv.x - pad) * aspect;
    } else {
        // Portrait: letterbox top/bottom
        float pad = 0.5 * (1.0 - aspect);
        uv.y = (uv.y - pad) / aspect;
    }

    // Area outside the square shows background.
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        fragColor = vec4(uBackground, 1.0);
        return;
    }

    // Faint reference guides (center cross + circle).
    vec2  c    = uv - vec2(0.5);
    float dist = length(c);
    bool  onCrossX = abs(c.x) < 0.004;
    bool  onCrossY = abs(c.y) < 0.004;
    bool  onCircle = abs(dist - 0.42) < 0.003;

    float guide = 0.0;
    if (onCrossX || onCrossY || onCircle)
        guide = 0.12;

    // Accumulation value → brightness (gamma for nicer glow falloff).
    float t = texture(uAccum, uv).r;
    t = pow(clamp(t, 0.0, 1.0), 0.45);

    vec3 col = mix(uBackground, uDot, max(t, guide));
    fragColor = vec4(col, 1.0);
}
