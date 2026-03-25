#pragma once
#include "../ColorSchemes.h"
#include <string>

// Applies a color scheme override from a JSON file.
// File structure: { "oscilloscope": { ... }, "spectrum": { ... }, ... }
// Missing sections or keys are ignored (scheme retains current values).
bool applyColorSchemeFromFile(VisualizerColorScheme &scheme,
                              const std::string &filePath);

// Convenience helper: loads <dir>/<schemeName>.json
bool applyColorSchemeFromDir(VisualizerColorScheme &scheme,
                             const std::string &dir,
                             const std::string &schemeName);
