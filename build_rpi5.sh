#!/bin/bash
set -e
cd "$(dirname "$0")"

echo "=== Waveform Visualizer — Raspberry Pi 5 build ==="

# ---------------------------------------------------------------------------
# Check / install apt dependencies
# ---------------------------------------------------------------------------
PKGS=(
    libsdl2-dev
    libportaudio2
    portaudio19-dev
    libfftw3-dev
    libglew-dev
    libgl1-mesa-dev
    cmake
    build-essential
    pkg-config
)

MISSING=()
for pkg in "${PKGS[@]}"; do
    if ! dpkg -s "$pkg" &>/dev/null; then
        MISSING+=("$pkg")
    fi
done

if [ ${#MISSING[@]} -gt 0 ]; then
    echo "Installing missing packages: ${MISSING[*]}"
    sudo apt-get update
    sudo apt-get install -y "${MISSING[@]}"
fi

# ---------------------------------------------------------------------------
# Configure and build
# ---------------------------------------------------------------------------
BUILD_TYPE="${1:-Release}"
BUILD_DIR="build/rpi5-${BUILD_TYPE,,}"

mkdir -p "$BUILD_DIR"

# Enable NEON SIMD + tune for Cortex-A76 (Pi 5)
CXXFLAGS="-march=armv8.2-a+simd -mtune=cortex-a76"

cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_CXX_FLAGS="$CXXFLAGS" \
    -G "Unix Makefiles"

cmake --build "$BUILD_DIR" --parallel "$(nproc)"

echo ""
echo "Build complete: $BUILD_DIR/visualizer"
echo ""
echo "Usage:"
echo "  $BUILD_DIR/visualizer                                     # use default input device"
echo "  $BUILD_DIR/visualizer <index>                             # use specific device index"
echo "  $BUILD_DIR/visualizer <index> --scheme <scheme_name>      #use specific color scheme"
echo ""
echo "Keys: M = cycle modes | F = cycle filters |  Esc/Q = quit"
echo ""
echo "Tip: to list audio devices run: $BUILD_DIR/visualizer --list"
