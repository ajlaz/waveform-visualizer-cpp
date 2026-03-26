#include "AppConfig.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstring>
#include <cstdlib>

AppConfig loadConfig(int argc, char* argv[])
{
    AppConfig cfg;

    // Locate --config override (scan before JSON parse)
    std::string configPath = "config.json";
    for (int i = 1; i + 1 < argc; ++i)
        if (std::strcmp(argv[i], "--config") == 0)
            configPath = argv[i + 1];

    // Load JSON file if present (missing file is not an error)
    std::ifstream f(configPath);
    if (f.is_open()) {
        auto j = nlohmann::json::parse(f, nullptr, /*exceptions=*/false);
        if (!j.is_discarded()) {
            if (j.contains("target_fps"))     cfg.targetFps     = j["target_fps"];
            if (j.contains("vsync"))          cfg.vsync         = j["vsync"];
            if (j.contains("tex_size"))       cfg.texSize       = j["tex_size"];
            if (j.contains("server_enabled")) cfg.serverEnabled = j["server_enabled"];
            if (j.contains("server_port"))    cfg.serverPort    = j["server_port"];
            if (j.contains("scheme"))         cfg.scheme        = j["scheme"].get<std::string>();
            if (j.contains("scheme_dir"))     cfg.schemeDir     = j["scheme_dir"].get<std::string>();
        }
    }

    // CLI overrides (take precedence over file)
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--fps")        == 0 && i + 1 < argc) cfg.targetFps     = std::atoi(argv[++i]);
        if (std::strcmp(argv[i], "--port")       == 0 && i + 1 < argc) cfg.serverPort    = std::atoi(argv[++i]);
        if (std::strcmp(argv[i], "--server")     == 0)                  cfg.serverEnabled = true;
        if (std::strcmp(argv[i], "--no-server")  == 0)                  cfg.serverEnabled = false;
        if (std::strcmp(argv[i], "--scheme")     == 0 && i + 1 < argc) cfg.scheme        = argv[++i];
        if (std::strcmp(argv[i], "--scheme-dir") == 0 && i + 1 < argc) cfg.schemeDir     = argv[++i];
    }

    return cfg;
}
