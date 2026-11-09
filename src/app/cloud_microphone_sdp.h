#pragma once

#include <string>

namespace lunar::app {

// Only the cloud-audio m-line changes. Video stays receive-only and the
// default offer is preserved byte-for-byte when the microphone is disabled.
inline std::string offerCloudMicrophoneAudio(std::string offer) {
    const size_t begin = offer.find("m=audio ");
    if (begin == std::string::npos) return offer;
    const size_t next = offer.find("\r\nm=", begin);
    const size_t end = next == std::string::npos ? offer.size() : next + 2;
    std::string audio = offer.substr(begin, end - begin);
    const size_t direction = audio.find("a=recvonly");
    if (direction == std::string::npos) return offer;
    audio.replace(direction, sizeof("a=recvonly") - 1, "a=sendrecv");
    if (audio.find("a=ssrc:6 ") == std::string::npos) {
        if (audio.size() >= 2 && audio.substr(audio.size() - 2) != "\r\n") {
            audio += "\r\n";
        }
        audio += "a=ssrc:6 cname:lunarnx-cloud-mic\r\n";
    }
    offer.replace(begin, end - begin, audio);
    return offer;
}

// The answer describes the *server's* direction. recvonly/sendrecv permits
// our microphone to send; sendonly/inactive/rejected does not.
inline bool cloudAnswerAcceptsMicrophone(const std::string& sdp) {
    const size_t begin = sdp.find("m=audio ");
    if (begin == std::string::npos) return false;
    const size_t port_begin = begin + sizeof("m=audio ") - 1;
    const size_t port_end = sdp.find_first_of(" \r\n", port_begin);
    if (sdp.substr(port_begin, port_end - port_begin) == "0") return false;
    const size_t next = sdp.find("\r\nm=", begin);
    const std::string audio = sdp.substr(begin, next == std::string::npos
        ? std::string::npos : next + 2 - begin);
    if (audio.find("a=inactive") != std::string::npos ||
        audio.find("a=sendonly") != std::string::npos) return false;
    if (audio.find("a=recvonly") != std::string::npos ||
        audio.find("a=sendrecv") != std::string::npos) return true;
    // No media-level direction means the session-level attribute applies;
    // otherwise RFC 3264 defaults to sendrecv.
    const size_t first_media = sdp.find("m=");
    const std::string session = sdp.substr(0, first_media);
    return session.find("a=inactive") == std::string::npos &&
           session.find("a=sendonly") == std::string::npos;
}

} // namespace lunar::app
