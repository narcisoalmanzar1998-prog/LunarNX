#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifdef __SWITCH__
#include <switch.h>
#include <opus/opus.h>
#endif

namespace lunar::stream {

// A 48 kHz stereo headset microphone encoded as 20 ms Opus frames. The
// WebRTC owner thread drains the bounded queue; audin never touches libpeer.
class CloudMicrophone {
public:
    struct Stats {
        uint32_t encoded = 0;
        uint32_t dropped = 0;
        uint32_t capture_errors = 0;
        uint32_t peak = 0;
    };

    CloudMicrophone() = default;
    CloudMicrophone(const CloudMicrophone&) = delete;
    CloudMicrophone& operator=(const CloudMicrophone&) = delete;
    ~CloudMicrophone() { stop(); }

    bool start(std::string& reason);
    void stop();
    bool takePacket(std::vector<uint8_t>& packet);
    Stats stats() const;

private:
#ifdef __SWITCH__
    void captureLoop();
    static constexpr int kSampleRate = 48000;
    static constexpr int kChannels = 2;
    static constexpr int kFrameSamples = 960;
    static constexpr size_t kBufferSize = 0x1000;
    static constexpr size_t kMaxQueuedPackets = 10;

    std::atomic<bool> running_{false};
    std::thread capture_thread_;
    bool audin_ready_ = false;
    bool audin_started_ = false;
    OpusEncoder* encoder_ = nullptr;
    void* input_memory_ = nullptr;
    std::mutex queue_mutex_;
    std::deque<std::vector<uint8_t>> queue_;
    std::atomic<uint32_t> encoded_{0};
    std::atomic<uint32_t> dropped_{0};
    std::atomic<uint32_t> capture_errors_{0};
    std::atomic<uint32_t> peak_{0};
#endif
};

} // namespace lunar::stream
