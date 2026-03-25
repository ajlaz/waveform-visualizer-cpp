#!/bin/bash
set -e
cd "$(dirname "$0")"

echo "=== Waveform Visualizer — macOS build ==="

# ---------------------------------------------------------------------------
# Check for Homebrew dependencies
# ---------------------------------------------------------------------------
MISSING=()
for pkg in sdl2 portaudio fftw; do
    if ! brew list "$pkg" &>/dev/null; then
        MISSING+=("$pkg")
    fi
done

if [ ${#MISSING[@]} -gt 0 ]; then
    echo "Installing missing Homebrew packages: ${MISSING[*]}"
    brew install "${MISSING[@]}"
fi

# ---------------------------------------------------------------------------
# Configure and build
# ---------------------------------------------------------------------------
BUILD_TYPE="${1:-Release}"
BUILD_DIR="build/macos-$(echo "$BUILD_TYPE" | tr '[:upper:]' '[:lower:]')"

mkdir -p "$BUILD_DIR"

cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_OSX_ARCHITECTURES="$(uname -m)" \
    -G "Unix Makefiles"

cmake --build "$BUILD_DIR" --parallel "$(sysctl -n hw.ncpu)"

cp "$BUILD_DIR/visualizer" . 

echo ""
echo "Build complete: $BUILD_DIR/visualizer"
echo ""
echo "Usage:"
echo " visualizer                             # use default input device"
echo " visualizer <index>                     # use specific device index"
echo " visualizer --scheme <scheme_name>      # use specific color scheme"
echo ""
echo "Keys: M = cycle modes | F = cycle filters |  Esc/Q = quit"
echo ""
echo "Tip: to list audio devices run: $BUILD_DIR/visualizer --list"

