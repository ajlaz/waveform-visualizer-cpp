#pragma once
#include <string>

struct AppConfig {
    int         targetFps     = 60;
    bool        vsync         = true;
    int         texSize       = 512;
    bool        serverEnabled = false;
    int         serverPort    = 8080;
    std::string scheme        = "default";
    std::string schemeDir     = "color_schemes";
};

// Loads config.json (or path from --config flag), then applies CLI overrides.
// Handles: --config <path>, --fps <n>, --port <n>, --server, --no-server
// Existing flags --list, --device, --scheme, --scheme-dir are left for main.cpp
// to handle after calling loadConfig().
AppConfig loadConfig(int argc, char* argv[]);
