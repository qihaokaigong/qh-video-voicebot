#pragma once

#include <string>

namespace recording_runtime {

enum class State { kIdle, kRecording, kUploading, kError };

class Controller {
 public:
  const char *stateName() const {
    switch (state_) {
      case State::kRecording:
        return "recording";
      case State::kUploading:
        return "uploading";
      case State::kError:
        return "error";
      case State::kIdle:
      default:
        return "idle";
    }
  }

  bool stateChanged() const { return state_changed_; }
  const std::string &currentRecordingId() const { return recording_id_; }

  void markReported() { state_changed_ = false; }

  void startRecording() { transition(State::kRecording, ""); }

  void startUploading(const char *recording_id) {
    transition(State::kUploading, recording_id);
  }

  void finishUpload() { returnToIdle(); }

  void fail() { transition(State::kError, ""); }

  void returnToIdle() { transition(State::kIdle, ""); }

 private:
  void transition(State state, const char *recording_id) {
    state_ = state;
    recording_id_ = recording_id;
    state_changed_ = true;
  }

  State state_ = State::kIdle;
  std::string recording_id_;
  bool state_changed_ = true;
};

}  // namespace recording_runtime
