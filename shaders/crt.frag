#version 330 core
uniform sampler2D uScene;
uniform float     uBarrelStrength;   // 0 = flat, ~0.18 = strong CRT curve
uniform float     uScanlineAlpha;    // 0 = off, 0.10 = subtle
uniform float     uVignetteRadius;   // 0.75 typical; lower = darker corners
uniform vec2      uResolution;

in  vec2 vTexCoord;
out vec4 fragColor;

// Barrel / pincushion distortion.
// Positive k curves outward (barrel), producing the classic CRT glass look.
vec2 barrelDistort(vec2 uv, float k) {
    vec2  cc = uv - 0.5;
    float r2 = dot(cc, cc);
    return uv + cc * r2 * k;
}

void main() {
    // ---- Barrel distortion ------------------------------------------------
    vec2 distUV = barrelDistort(vTexCoord, uBarrelStrength);

    // Pixels outside the warped boundary → black bezel
    if (distUV.x < 0.0 || distUV.x > 1.0 || distUV.y < 0.0 || distUV.y > 1.0) {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec4 col = texture(uScene, distUV);

    // ---- Scanlines (darken every other row) --------------------------------
    float scanline = mod(gl_FragCoord.y, 2.0) < 1.0 ? (1.0 - uScanlineAlpha) : 1.0;
    col.rgb *= scanline;

    // ---- Vignette ----------------------------------------------------------
    // Smooth falloff from centre; uVignetteRadius controls how far out it starts.
    vec2  centred = vTexCoord - 0.5;
    float dist    = length(centred);
    float vignette = smoothstep(uVignetteRadius + 0.25, uVignetteRadius - 0.1, dist);
    col.rgb *= vignette;

    fragColor = vec4(col.rgb, 1.0);
}
