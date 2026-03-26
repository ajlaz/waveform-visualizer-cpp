#pragma once
#include "CommandQueue.h"
#include <string>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <thread>

namespace httplib { class Server; }

// Runs an embedded HTTP server on a background thread.
// Never touches GL or SDL — all state changes go through CommandQueue.
// Call pushState() from the main thread after applying commands to broadcast
// the new state JSON to all connected SSE clients.
class ControlServer {
public:
    ControlServer(CommandQueue& queue, std::string schemeDir);
    ~ControlServer();

    bool start(int port);  // Returns false if bind fails.
    void stop();

    // Main thread calls this after applying commands: pushes JSON to SSE clients.
    void pushState(const std::string& stateJson);

private:
    void setupRoutes();
    std::string listSchemes() const;

    CommandQueue&                    queue_;
    std::string                      schemeDir_;
    std::unique_ptr<httplib::Server> svr_;
    std::thread                      thread_;

    std::mutex              sseMutex_;
    std::condition_variable sseCv_;
    std::string             latestState_  = "{}";
    size_t                  stateVersion_ = 0;
    std::atomic<bool>       stopped_{false};
};
