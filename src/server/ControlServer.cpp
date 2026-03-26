#include "ControlServer.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

ControlServer::ControlServer(CommandQueue& queue, std::string schemeDir)
    : queue_(queue), schemeDir_(std::move(schemeDir)),
      svr_(std::make_unique<httplib::Server>())
{
    setupRoutes();
}

ControlServer::~ControlServer()
{
    stop();
}

bool ControlServer::start(int port)
{
    if (!svr_->bind_to_port("0.0.0.0", port))
        return false;
    thread_ = std::thread([this]{ svr_->listen_after_bind(); });
    return true;
}

void ControlServer::stop()
{
    stopped_ = true;
    sseCv_.notify_all();
    svr_->stop();
    if (thread_.joinable()) thread_.join();
}

void ControlServer::pushState(const std::string& stateJson)
{
    {
        std::lock_guard<std::mutex> lk(sseMutex_);
        latestState_ = stateJson;
        stateVersion_++;
    }
    sseCv_.notify_all();
}

std::string ControlServer::listSchemes() const
{
    nlohmann::json arr = nlohmann::json::array();
    try {
        for (const auto& entry : std::filesystem::directory_iterator(schemeDir_)) {
            if (entry.path().extension() == ".json")
                arr.push_back(entry.path().stem().string());
        }
    } catch (...) {}
    return arr.dump();
}

void ControlServer::setupRoutes()
{
    // Serve web UI from web/index.html relative to cwd
    svr_->Get("/", [](const httplib::Request&, httplib::Response& res) {
        std::ifstream f("web/index.html");
        if (f.is_open()) {
            std::string body((std::istreambuf_iterator<char>(f)),
                              std::istreambuf_iterator<char>());
            res.set_content(body, "text/html");
        } else {
            res.set_content(
                "<h1>Visualizer Control</h1><p>web/index.html not found next to binary.</p>",
                "text/html");
        }
    });

    // Full state snapshot
    svr_->Get("/state", [this](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(sseMutex_);
        res.set_content(latestState_, "application/json");
    });

    // SSE live updates — each client gets its own mutable lastSeen counter
    svr_->Get("/stream", [this](const httplib::Request&, httplib::Response& res) {
        res.set_header("Cache-Control", "no-cache");
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_chunked_content_provider("text/event-stream",
            [this, myVersion = size_t(0)](size_t, httplib::DataSink& sink) mutable -> bool {
                std::unique_lock<std::mutex> lk(sseMutex_);
                sseCv_.wait(lk, [&]{
                    return stateVersion_ > myVersion || stopped_.load();
                });
                if (stopped_) return false;
                myVersion      = stateVersion_;
                std::string js = latestState_;
                lk.unlock();
                std::string msg = "data: " + js + "\n\n";
                return sink.write(msg.c_str(), msg.size());
            });
    });

    // Commands: next_mode, prev_mode, cycle_filter, set_filter, set_scheme
    svr_->Post("/command", [this](const httplib::Request& req, httplib::Response& res) {
        auto j = nlohmann::json::parse(req.body, nullptr, false);
        if (j.is_discarded() || !j.contains("action")) {
            res.set_content("{\"error\":\"invalid json\"}", "application/json");
            return;
        }
        const std::string action = j["action"];
        Command cmd;
        if      (action == "next_mode")    { cmd.type = Command::Type::NextMode; }
        else if (action == "prev_mode")    { cmd.type = Command::Type::PrevMode; }
        else if (action == "cycle_filter") { cmd.type = Command::Type::CycleFilter; }
        else if (action == "set_filter")   {
            cmd.type     = Command::Type::SetFilter;
            cmd.strValue = j.value("value", "None");
        }
        else if (action == "set_scheme")   {
            cmd.type     = Command::Type::SetScheme;
            cmd.strValue = j.value("value", "default");
        }
        else {
            res.set_content("{\"error\":\"unknown action\"}", "application/json");
            return;
        }
        queue_.push(cmd);
        res.set_content("{\"ok\":true}", "application/json");
    });

    // Set a named float param on a visualizer
    svr_->Post("/param", [this](const httplib::Request& req, httplib::Response& res) {
        auto j = nlohmann::json::parse(req.body, nullptr, false);
        if (j.is_discarded() ||
            !j.contains("visualizer") || !j.contains("param") || !j.contains("value")) {
            res.set_content("{\"error\":\"invalid json\"}", "application/json");
            return;
        }
        Command cmd;
        cmd.type       = Command::Type::SetParam;
        cmd.visualizer = j["visualizer"].get<std::string>();
        cmd.param      = j["param"].get<std::string>();
        cmd.value      = j["value"].get<float>();
        queue_.push(cmd);
        res.set_content("{\"ok\":true}", "application/json");
    });

    // List available color scheme names
    svr_->Get("/schemes", [this](const httplib::Request&, httplib::Response& res) {
        res.set_content(listSchemes(), "application/json");
    });

    // CORS preflight
    svr_->Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin",  "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.set_content("", "text/plain");
    });
}
