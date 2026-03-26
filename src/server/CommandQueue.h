// src/server/CommandQueue.h
#pragma once
#include <queue>
#include <mutex>
#include <string>

struct Command {
    enum class Type {
        NextMode,
        PrevMode,
        CycleFilter,
        SetFilter,   // strValue: "None" | "CRT" | "Fisheye"
        SetScheme,   // strValue: scheme name
        SetParam,    // visualizer + param + value
    };
    Type        type  = Type::NextMode;
    std::string visualizer;  // SetParam only
    std::string param;       // SetParam only
    float       value  = 0.0f;
    std::string strValue;    // SetFilter / SetScheme
};

class CommandQueue {
public:
    CommandQueue() = default;

    void push(Command cmd) {
        std::lock_guard<std::mutex> lk(mutex_);
        queue_.push(std::move(cmd));
    }

    // Returns true and fills `out` if a command was available, false if empty.
    bool pop(Command& out) {
        std::lock_guard<std::mutex> lk(mutex_);
        if (queue_.empty()) return false;
        out = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    CommandQueue(const CommandQueue&)            = delete;
    CommandQueue& operator=(const CommandQueue&) = delete;

private:
    std::queue<Command> queue_;
    std::mutex          mutex_;
};
