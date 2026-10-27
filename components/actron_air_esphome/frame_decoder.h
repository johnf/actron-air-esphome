#pragma once

#include <cstddef>
#include <cstdint>

#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

namespace esphome {
namespace actron_air_esphome {

// Protocol timing (microseconds). Bits are encoded in the gap between falling
// edges: ~490us = 1, ~1470us = 0. A frame follows an idle gap of ~200ms and,
// unless the timer LED is lit, a ~2.9ms start gap.
constexpr uint32_t DEBOUNCE_US = 250;
constexpr uint32_t FRAME_START_US = 2700;
constexpr uint32_t PULSE_THRESHOLD_US = 1000;

constexpr std::size_t NPULSE = 40;

/// Turns falling-edge timestamps into 40-bit frames.
///
/// The line bounces for ~1us around edges; a bounce on the rising edge adds a
/// falling edge 62us after the real one, so edges closer than DEBOUNCE_US to
/// the last accepted edge are ignored. Any gap of FRAME_START_US or more starts
/// a new frame, which completes the previous one.
class FrameDecoder {
public:
  enum class Result : uint8_t { NONE, FRAME, ERROR };

  inline Result IRAM_ATTR on_edge(uint32_t now_us) {
    uint32_t delta_us = now_us - last_us_;
    if (started_ && delta_us < DEBOUNCE_US) {
      return Result::NONE;
    }
    last_us_ = now_us;

    if (!started_ || delta_us >= FRAME_START_US) {
      started_ = true;
      Result result = Result::NONE;
      if (count_ == NPULSE && !overflow_) {
        frame_ = bits_;
        result = Result::FRAME;
      } else if (count_ != 0) {
        result = Result::ERROR;
      }
      bits_ = 0;
      count_ = 0;
      overflow_ = false;

      return result;
    }

    if (count_ >= NPULSE) {
      overflow_ = true;

      return Result::NONE;
    }

    if (delta_us < PULSE_THRESHOLD_US) {
      bits_ |= 1ULL << count_;
    }
    ++count_;

    return Result::NONE;
  }

  /// The frame completed by the last on_edge() that returned FRAME.
  uint64_t frame() const { return frame_; }

private:
  uint32_t last_us_{0};
  bool started_{false};
  uint64_t bits_{0};
  uint8_t count_{0};
  bool overflow_{false};
  uint64_t frame_{0};
};

constexpr uint8_t REQUIRED_MATCHING_FRAMES = 2;

/// Only lets a changed frame through once it has arrived
/// REQUIRED_MATCHING_FRAMES times in a row, so a single corrupted frame is
/// never published. Blinking displays alternate every frame and so hold the
/// last steady state.
class FrameConfirmer {
public:
  bool accept(uint64_t frame) {
    if (frame == published_) {
      matches_ = 0;

      return false;
    }

    if (matches_ > 0 && frame == candidate_) {
      ++matches_;
    } else {
      candidate_ = frame;
      matches_ = 1;
    }

    if (matches_ < REQUIRED_MATCHING_FRAMES) {
      return false;
    }

    published_ = frame;
    matches_ = 0;

    return true;
  }

private:
  // Impossible frame value so the first real frame is always a change.
  uint64_t published_{~0ULL};
  uint64_t candidate_{0};
  uint8_t matches_{0};
};

} // namespace actron_air_esphome
} // namespace esphome
