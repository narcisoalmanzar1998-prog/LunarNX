#include "../src/app/cloud_microphone_sdp.h"

#include <cassert>
#include <string>

int main() {
    using lunar::app::cloudAnswerAcceptsMicrophone;
    using lunar::app::offerCloudMicrophoneAudio;

    const std::string offer =
        "v=0\r\na=group:BUNDLE 0 1 2\r\n"
        "m=video 9 UDP/TLS/RTP/SAVPF 102\r\na=recvonly\r\n"
        "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
        "a=mid:1\r\na=rtpmap:111 opus/48000/2\r\na=recvonly\r\n"
        "m=application 9 UDP/DTLS/SCTP webrtc-datachannel\r\n";
    const auto converted = offerCloudMicrophoneAudio(offer);
    assert(converted != offer);
    assert(converted.find("m=video 9 UDP/TLS/RTP/SAVPF 102\r\na=recvonly") !=
           std::string::npos);
    assert(converted.find("a=sendrecv\r\na=ssrc:6 cname:lunarnx-cloud-mic\r\n"
                          "m=application") != std::string::npos);
    assert(offerCloudMicrophoneAudio(converted) == converted);
    assert(offerCloudMicrophoneAudio("v=0\r\nm=video 9\r\na=recvonly\r\n") ==
           "v=0\r\nm=video 9\r\na=recvonly\r\n");

    assert(cloudAnswerAcceptsMicrophone("v=0\r\nm=video 9\r\na=recvonly\r\n"
                                        "m=audio 9\r\na=recvonly\r\n"));
    assert(cloudAnswerAcceptsMicrophone("v=0\r\nm=audio 9\r\na=sendrecv\r\n"));
    assert(cloudAnswerAcceptsMicrophone("v=0\r\nm=audio 9\r\n"));
    assert(cloudAnswerAcceptsMicrophone("v=0\r\nm=video 9\r\na=sendonly\r\n"
                                        "m=audio 9\r\n"));
    assert(!cloudAnswerAcceptsMicrophone("v=0\r\nm=audio 0\r\na=recvonly\r\n"));
    assert(!cloudAnswerAcceptsMicrophone("v=0\r\nm=audio 9\r\na=sendonly\r\n"));
    assert(!cloudAnswerAcceptsMicrophone("v=0\r\nm=audio 9\r\na=inactive\r\n"));
    assert(!cloudAnswerAcceptsMicrophone("v=0\r\na=inactive\r\n"
                                         "m=audio 9\r\n"));
}
