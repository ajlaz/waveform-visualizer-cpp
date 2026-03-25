#version 330 core
uniform float uLevel;       // normalised 0-1 (maps DB_FLOOR..DB_CEIL)
uniform float uPeakLevel;   // normalised 0-1 peak hold
uniform vec2  uResolution;
uniform vec3  uBackground;
uniform vec3  uUnfilled;
uniform vec3  uGreen;
uniform vec3  uYellow;
uniform vec3  uRed;
uniform vec3  uPeak;
uniform vec3  uBorder;
uniform vec3  uTicks;

in  vec2 vTexCoord;
out vec4 fragColor;

// Bar layout in UV space
const float BAR_X0  = 0.07;
const float BAR_X1  = 0.93;
const float BAR_Y0  = 0.38;
const float BAR_Y1  = 0.62;
const float BAR_W   = BAR_X1 - BAR_X0;

// dB scale positions for -40, -20, -10, -3, 0 dB (normalised into the bar)
// DB_FLOOR=-40, DB_CEIL=3, range=43
float dbToBarX(float db) {
    return clamp((db - (-40.0)) / 43.0, 0.0, 1.0);
}

void main() {
    float x = vTexCoord.x;
    float y = vTexCoord.y;

    // Background
    vec3 bg = uBackground;

    if (y < BAR_Y0 || y > BAR_Y1 || x < BAR_X0 || x > BAR_X1) {
        // Outside bar area: check for dB tick marks
        float col = uBackground.r;  // default background
        float[6] marks = float[6](dbToBarX(-40.0), dbToBarX(-20.0),
                                  dbToBarX(-10.0), dbToBarX(-3.0),
                                  dbToBarX(0.0),   dbToBarX(3.0));
        float barX = (x - BAR_X0) / BAR_W;
        bool  onTick = false;
        for (int i = 0; i < 6; i++) {
            if (abs(barX - marks[i]) < 0.005 / BAR_W &&
                y > BAR_Y0 - 0.05 && y < BAR_Y0 &&
                x >= BAR_X0 && x <= BAR_X1)
                onTick = true;
        }
        fragColor = onTick ? vec4(uTicks, 1.0) : vec4(bg, 1.0);
        return;
    }

    // Inside bar
    float barX = (x - BAR_X0) / BAR_W;   // 0 = left (-40dB), 1 = right (+3dB)

    vec3 col;
    if (barX > uLevel) {
        // Unfilled
        col = uUnfilled;
    } else {
        // Filled — colour by zone
        if (barX < 0.75)
            col = uGreen;
        else if (barX < 0.87)
            col = uYellow;
        else
            col = uRed;
    }

    // Peak hold indicator — thin bright vertical line
    float peakX = uPeakLevel;
    if (abs(barX - peakX) < 0.008)
        col = mix(col, uPeak, 0.9);

    // Thin border
    float bx = (x - BAR_X0) / BAR_W;
    float by = (y - BAR_Y0) / (BAR_Y1 - BAR_Y0);
    if (bx < 0.003 || bx > 0.997 || by < 0.04 || by > 0.96)
        col = uBorder;

    fragColor = vec4(col, 1.0);
}
