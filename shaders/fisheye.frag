#version 330 core
uniform sampler2D uScene;
uniform float     uStrength;       // 0 = off, higher = stronger fisheye
uniform float     uVignetteRadius;  // optional vignette control

in  vec2 vTexCoord;
out vec4 fragColor;

vec2 fisheye(vec2 uv, float k) {
    vec2  d = uv - 0.5;
    float rmax = 0.70710678; // length of vec2(0.5)
    float s = rmax / (rmax + k * rmax * rmax);
    d *= s;
    float r = length(d);
    if (r == 0.0) return uv;
    float rn = r + k * r * r;
    return 0.5 + d * (rn / r);
}

void main() {
    vec2 distUV = fisheye(vTexCoord, uStrength);

    if (distUV.x < 0.0 || distUV.x > 1.0 || distUV.y < 0.0 || distUV.y > 1.0) {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec4 col = texture(uScene, distUV);

    vec2 centred = vTexCoord - 0.5;
    float dist = length(centred);
    float vignette = smoothstep(uVignetteRadius + 0.25, uVignetteRadius - 0.1, dist);
    col.rgb *= vignette;

    fragColor = vec4(col.rgb, 1.0);
}