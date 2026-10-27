// Host tests for the keypad frame decoder. Run with scripts/test-decoder.sh.

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "frame_decoder.h"

using esphome::actron_air_esphome::FrameConfirmer;
using esphome::actron_air_esphome::FrameDecoder;
using Result = FrameDecoder::Result;

namespace {

int failures = 0;

void check(bool ok, const std::string &what) {
  if (!ok) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", what.c_str());
  }
}

constexpr uint32_t IDLE_US = 200000;
constexpr uint32_t START_US = 2895;
constexpr uint32_t ZERO_US = 1466;
constexpr uint32_t ONE_US = 486;

// Falling-edge times for a clean frame starting at `t`. Returns the time of
// the last edge.
uint32_t frame_edges(std::vector<uint32_t> &edges, uint32_t t, uint64_t bits,
                     std::size_t nbits, bool start_gap) {
  edges.push_back(t);
  if (start_gap) {
    t += START_US;
    edges.push_back(t);
  }
  for (std::size_t i = 0; i < nbits; ++i) {
    t += ((bits >> i) & 1) ? ONE_US : ZERO_US;
    edges.push_back(t);
  }

  return t;
}

struct Outcome {
  std::vector<uint64_t> frames;
  int errors = 0;
};

Outcome feed(const std::vector<uint32_t> &edges) {
  FrameDecoder decoder;
  Outcome out;
  for (uint32_t e : edges) {
    switch (decoder.on_edge(e)) {
    case Result::FRAME:
      out.frames.push_back(decoder.frame());
      break;
    case Result::ERROR:
      ++out.errors;
      break;
    case Result::NONE:
      break;
    }
  }

  return out;
}

constexpr uint64_t IDLE_FRAME = 0xb9f5fb4000ULL;

void test_capture(const char *path) {
  std::ifstream in(path);
  check(in.good(), std::string("open fixture ") + path);

  std::vector<uint32_t> edges;
  std::vector<uint64_t> expected;
  uint32_t t = 1000;
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    std::istringstream fields(line);
    std::string hex;
    fields >> hex;
    expected.push_back(std::strtoull(hex.c_str(), nullptr, 16));

    long duration;
    while (fields >> duration) {
      if (duration < 0) {
        edges.push_back(t);
      }
      t += static_cast<uint32_t>(duration < 0 ? -duration : duration);
    }
    t += IDLE_US;
  }
  edges.push_back(t);  // Completes the last frame

  Outcome out = feed(edges);
  check(!expected.empty(), "fixture has frames");
  check(out.errors == 0, "capture decodes without errors, got " +
                             std::to_string(out.errors));
  check(out.frames.size() == expected.size(),
        "capture frame count " + std::to_string(out.frames.size()) + " of " +
            std::to_string(expected.size()));
  for (std::size_t i = 0; i < expected.size() && i < out.frames.size(); ++i) {
    if (out.frames[i] != expected[i]) {
      char msg[96];
      std::snprintf(msg, sizeof msg,
                    "capture frame %zu: got %010" PRIx64 " want %010" PRIx64, i,
                    out.frames[i], expected[i]);
      check(false, msg);
    }
  }
}

void test_start_gap_optional() {
  std::vector<uint32_t> edges;
  uint32_t t = frame_edges(edges, 1000, IDLE_FRAME, 40, true);
  t = frame_edges(edges, t + IDLE_US, IDLE_FRAME | 0x80, 40, false);
  edges.push_back(t + IDLE_US);

  Outcome out = feed(edges);
  check(out.errors == 0, "start gap optional: no errors");
  check(out.frames == std::vector<uint64_t>{IDLE_FRAME, IDLE_FRAME | 0x80},
        "start gap optional: both frames decoded");
}

void test_rising_edge_bounce_ignored() {
  std::vector<uint32_t> edges;
  uint32_t t = frame_edges(edges, 1000, IDLE_FRAME, 40, true);
  edges.insert(edges.begin() + 10, edges[9] + 62);
  edges.push_back(t + IDLE_US);

  Outcome out = feed(edges);
  check(out.errors == 0 && out.frames == std::vector<uint64_t>{IDLE_FRAME},
        "bounce 62us after an edge is ignored");
}

void test_wrong_bit_count_is_error() {
  for (std::size_t nbits : {39, 41}) {
    std::vector<uint32_t> edges;
    uint32_t t = frame_edges(edges, 1000, IDLE_FRAME, nbits, true);
    edges.push_back(t + IDLE_US);

    Outcome out = feed(edges);
    check(out.frames.empty() && out.errors == 1,
          std::to_string(nbits) + " bits is one error and no frame");
  }
}

void test_micros_wrap() {
  std::vector<uint32_t> edges;
  uint32_t t = frame_edges(edges, UINT32_MAX - 20000, IDLE_FRAME, 40, true);
  edges.push_back(t + IDLE_US);

  Outcome out = feed(edges);
  check(out.errors == 0 && out.frames == std::vector<uint64_t>{IDLE_FRAME},
        "frame spanning micros() wrap decodes");
}

void test_confirmer() {
  FrameConfirmer c;
  check(!c.accept(IDLE_FRAME), "first frame needs confirming");
  check(c.accept(IDLE_FRAME), "first frame published on second sight");
  check(!c.accept(IDLE_FRAME), "unchanged frame not republished");

  check(!c.accept(0x79f5fbc300ULL), "single corrupted frame not published");
  check(!c.accept(IDLE_FRAME), "back to published frame");

  check(!c.accept(IDLE_FRAME | 0x80), "blink on");
  check(!c.accept(IDLE_FRAME), "blink off");
  check(!c.accept(IDLE_FRAME | 0x80), "blink on again");
  check(c.accept(IDLE_FRAME | 0x80), "steady change published");
}

} // namespace

int main(int argc, char **argv) {
  if (argc != 2) {
    std::fprintf(stderr, "usage: %s FIXTURE\n", argv[0]);
    return 2;
  }

  test_capture(argv[1]);
  test_start_gap_optional();
  test_rising_edge_bounce_ignored();
  test_wrong_bit_count_is_error();
  test_micros_wrap();
  test_confirmer();

  if (failures) {
    std::fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
  }
  std::puts("frame decoder tests passed");

  return 0;
}
