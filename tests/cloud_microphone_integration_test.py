from pathlib import Path


def check(path, required):
    content = Path(path).read_text()
    for snippet in required:
        assert snippet in content, f"Missing {snippet!r} in {path}"


def test_cloud_microphone_contract():
    check("src/app/stream_controller.cpp", [
        "profile.cloud_microphone_enabled = cloud_microphone_enabled_.load()",
        "StreamController::setCloudMicrophoneEnabled",
    ])
    check("src/app/xbox_stream_session.cpp", [
        "profile.type == SessionType::Cloud && profile.cloud_microphone_enabled",
        "offerCloudMicrophoneAudio",
        "cloudAnswerAcceptsMicrophone(answer)",
        "mic_started = microphone.start(reason)",
        "transport_.sendAudioPacket(packet.data(), packet.size())",
        "microphone.stop()",
    ])
    check("src/stream/cloud_microphone.cpp", [
        "audinInitialize()", "audinStartAudioIn()", "audinWaitCaptureFinish",
        "opus_encode(", "armDCacheFlush(", "kMaxQueuedPackets",
    ])
    check("src/ui/stream_settings_activity.cpp", [
        '"cloud_microphone_enabled"', "settings_.cloud_microphone_enabled",
        "ctrl_->setCloudMicrophoneEnabled(enabled)",
    ])


if __name__ == "__main__":
    test_cloud_microphone_contract()
    print("cloud microphone integration: PASS")
