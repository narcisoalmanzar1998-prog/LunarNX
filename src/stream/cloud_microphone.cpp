#include "cloud_microphone.h"

#ifdef __SWITCH__
#include <algorithm>
#include <array>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <malloc.h>

namespace lunar::stream {

bool CloudMicrophone::start(std::string& reason) {
    stop();
    encoded_ = dropped_ = capture_errors_ = peak_ = 0;

    Result rc = audinInitialize();
    if (R_FAILED(rc)) {
        reason = "audinInitialize failed: " + std::to_string(rc);
        return false;
    }
    audin_ready_ = true;
    if (audinGetSampleRate() != kSampleRate ||
        audinGetChannelCount() != kChannels ||
        audinGetPcmFormat() != PcmFormat_Int16) {
        reason = "microphone PCM is not 48kHz stereo s16";
        stop();
        return false;
    }

    int opus_error = OPUS_OK;
    encoder_ = opus_encoder_create(kSampleRate, kChannels,
                                   OPUS_APPLICATION_VOIP, &opus_error);
    if (!encoder_ || opus_error != OPUS_OK) {
        reason = "Opus encoder could not start";
        stop();
        return false;
    }
    opus_encoder_ctl(encoder_, OPUS_SET_BITRATE(24000));
    opus_encoder_ctl(encoder_, OPUS_SET_COMPLEXITY(4));
    opus_encoder_ctl(encoder_, OPUS_SET_INBAND_FEC(1));

    input_memory_ = memalign(kBufferSize, kBufferSize);
    if (!input_memory_) {
        reason = "microphone buffer allocation failed";
        stop();
        return false;
    }
    std::memset(input_memory_, 0, kBufferSize);
    rc = audinStartAudioIn();
    if (R_FAILED(rc)) {
        reason = "audinStartAudioIn failed: " + std::to_string(rc);
        stop();
        return false;
    }
    audin_started_ = true;
    running_ = true;
    capture_thread_ = std::thread(&CloudMicrophone::captureLoop, this);
    return true;
}

void CloudMicrophone::stop() {
    running_ = false;
    if (capture_thread_.joinable()) capture_thread_.join();
    if (audin_started_) audinStopAudioIn();
    audin_started_ = false;
    if (audin_ready_) audinExit();
    audin_ready_ = false;
    if (encoder_) opus_encoder_destroy(encoder_);
    encoder_ = nullptr;
    std::free(input_memory_);
    input_memory_ = nullptr;
    std::lock_guard<std::mutex> lock(queue_mutex_);
    queue_.clear();
}

bool CloudMicrophone::takePacket(std::vector<uint8_t>& packet) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    if (queue_.empty()) return false;
    packet = std::move(queue_.front());
    queue_.pop_front();
    return true;
}

CloudMicrophone::Stats CloudMicrophone::stats() const {
    return {encoded_.load(), dropped_.load(), capture_errors_.load(), peak_.load()};
}

void CloudMicrophone::captureLoop() {
    AudioInBuffer buffer{};
    buffer.buffer = input_memory_;
    buffer.buffer_size = kBufferSize;
    std::vector<int16_t> pending;
    pending.reserve(kFrameSamples * kChannels + kBufferSize / sizeof(int16_t));
    std::array<uint8_t, 1200> encoded{};

    while (running_) {
        buffer.data_size = kBufferSize;
        if (R_FAILED(audinAppendAudioInBuffer(&buffer))) {
            capture_errors_++;
            break;
        }
        AudioInBuffer* released = nullptr;
        u32 released_count = 0;
        int waits_without_buffer = 0;
        while (running_ && released != &buffer) {
            Result rc = audinWaitCaptureFinish(&released, &released_count,
                                               100000000ULL);
            if (R_SUCCEEDED(rc) && released && released != &buffer) {
                capture_errors_++;
                running_ = false;
            }
            if (released != &buffer && ++waits_without_buffer >= 50) {
                capture_errors_++;
                running_ = false;
            }
        }
        if (!running_) break;

        armDCacheFlush(input_memory_, kBufferSize);
        const size_t used = std::min(static_cast<size_t>(buffer.data_size), kBufferSize);
        if (used % (kChannels * sizeof(int16_t)) != 0) {
            capture_errors_++;
            continue;
        }
        const int16_t* pcm = static_cast<const int16_t*>(input_memory_);
        const size_t count = used / sizeof(int16_t);
        uint32_t peak = peak_.load(std::memory_order_relaxed);
        for (size_t i = 0; i < count; ++i) {
            const uint32_t amplitude = pcm[i] == INT16_MIN ? 32768u
                : static_cast<uint32_t>(std::abs(pcm[i]));
            peak = std::max(peak, amplitude);
        }
        peak_.store(peak, std::memory_order_relaxed);
        pending.insert(pending.end(), pcm, pcm + count);

        constexpr size_t frame_size = kFrameSamples * kChannels;
        while (pending.size() >= frame_size) {
            const int bytes = opus_encode(encoder_, pending.data(), kFrameSamples,
                                          encoded.data(), encoded.size());
            pending.erase(pending.begin(), pending.begin() + frame_size);
            if (bytes <= 0) {
                capture_errors_++;
                continue;
            }
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                if (queue_.size() >= kMaxQueuedPackets) {
                    queue_.pop_front();
                    dropped_++;
                }
                queue_.emplace_back(encoded.begin(), encoded.begin() + bytes);
            }
            encoded_++;
        }
    }
}

} // namespace lunar::stream
#else
namespace lunar::stream {
bool CloudMicrophone::start(std::string& reason) {
    reason = "microphone capture requires Nintendo Switch";
    return false;
}
void CloudMicrophone::stop() {}
bool CloudMicrophone::takePacket(std::vector<uint8_t>&) { return false; }
CloudMicrophone::Stats CloudMicrophone::stats() const { return {}; }
} // namespace lunar::stream
#endif
