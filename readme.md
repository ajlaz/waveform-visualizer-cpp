# Waveform Visualizer
A real-time audio waveform visualizer built in C++ using PortAudio and SDL2. It captures audio input from your microphone or other audio devices and renders dynamic waveforms on the screen.

## Features
- Real-time audio capture and visualization
- Multiple visualization modes (e.g., spectrum, waveform, oscilloscope)
- Customizable color schemes
- Visualization filters (crt, fisheye, etc...)


## Usage
1. Build the project using the provided build scripts for your platform (e.g., `build_macos.sh` for macOS)

2. Run the visualizer executable. By default, it will use the default audio input device. You can also specify a specific device index or color scheme:
   - `visualizer` - use default input device
   - `visualizer <index>` - use specific device index
   - `visualizer --scheme <scheme_name>` - use specific color scheme
3. Use the following keys to interact with the visualizer:
    - `M` - cycle through visualization modes
    - `F` - cycle through visualization filters
    - `Esc` or `Q` - quit the application
4. To list available audio devices and their indices, run:
    - `visualizer --list`

## Supported Platforms
- macOS
- (COMING SOON) Raspberry Pi 5 (Linux)


## Capturing System Audio (MacOS)
To capture system audio on macOS, you can use a virtual audio device like [BlackHole](https://formulae.brew.sh/cask/blackhole-2ch). Then do the following:
1. Create a multi-output device that includes both your physical audio output and the virtual device.
2. Set this multi-output device as your default output.
3. The visualizer will capture the system audio.
Make sure to select BlackHole2ch as the input device in the visualizer if it's not set as the default input.

## Upcoming features:
- Support for Linux (Raspberry Pi 5)
- Additional visualization modes and filters
- Web interface for remote control
- Performance optimizations and GPU acceleration