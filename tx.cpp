#include <SDL_audio.h>
#include <iostream>
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <chrono>

#include <cmath>

enum {
  SAMPLE_RATE = 44100,
  SYMBOL_RATE = 100,

  FREQ_START0  = 1000,
  FREQ_START1  = 2000,
  FREQ_LOW     = 1000,
  FREQ_HIGH    = 1500,
};

static std::queue<float> output_fifo;
static std::mutex        output_fifo_mtx;

static constexpr float SYMBOL_LENGTH = 1.0 / SYMBOL_RATE;

static float symbol_position = 0.0;
static float period_length = 0;

static constexpr float PI = 3.141592653589793238462643383279502884L;

/// Fetches a new frequency from our fifo.
static void update_frequency()
{
  std::lock_guard<std::mutex> _(output_fifo_mtx);
  if (output_fifo.empty()) {
    period_length = 0;
  } else {
    period_length = output_fifo.front();
    output_fifo.pop();
  }
}

// Fills the output buffer.
static void audio_cb(void *, uint8_t *stream, int len)
{
  uint16_t *audio = reinterpret_cast<uint16_t *>(stream);

  for (int i = 0; i < len / 2; i++) {
    if (symbol_position >= SYMBOL_LENGTH) {
      symbol_position -= SYMBOL_LENGTH;
      update_frequency();
    }

    audio[i] = 32768;
    if (period_length) {
      audio[i] += 20000 * sinf(2*PI * symbol_position / period_length);
    }

    symbol_position += 1.0 / SAMPLE_RATE;
  }
}

static void encode(uint8_t v)
{
  std::lock_guard<std::mutex> _(output_fifo_mtx);

  output_fifo.push(1.0 / FREQ_START0);
  output_fifo.push(1.0 / FREQ_START1);

  for (unsigned i = 0; i < 8; i++) {
    output_fifo.push(1.0 / ((v & (1 << i)) ? FREQ_HIGH : FREQ_LOW));
  }
}

int main()
{
  static SDL_AudioSpec spec;

  spec.freq     = SAMPLE_RATE;
  spec.format   = AUDIO_U16SYS;
  spec.channels = 1;
  spec.samples  = SAMPLE_RATE / (2*SYMBOL_RATE);
  spec.userdata = nullptr;
  spec.callback = audio_cb;

  spec.size = 0;
  spec.silence = 0;

  if (SDL_OpenAudio(&spec, nullptr) < 0) {
    std::cout << "Failed to set up audio.\n";
    return 1;
  }

  SDL_PauseAudio(0);

  std::cout << "Let's go.\n";

  for (std::string line; std::getline(std::cin, line);) {
    for (char x : line) {
      encode(uint8_t(x));
    }
    encode('\n');
  }

  for (int i = SYMBOL_RATE / 2; i; i--) {
    output_fifo.push(0.0);
  }

  while (true) {
    {
      std::lock_guard<std::mutex> _(output_fifo_mtx);
      if (output_fifo.empty()) {
        break;
      }
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  return 0;
}
