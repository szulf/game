#pragma once

#include <thread>
#include <vector>
#include <filesystem>

#include "base/base.h"
#include "base/enum_array.h"
#include "base/spsc_queue.h"

#include "os/os.h"

enum class SoundHandle
{
  // TODO: remove test sounds
  SINE,
  SHOTGUN,

  COUNT,
};

// NOTE: assumes 48'000 sample rate, 2 channels and i16 encoding
struct SoundData
{
  std::vector<i16> samples{};
  u32 frames{};
};

SoundData load_wav(const std::filesystem::path& path);

struct SoundCmd
{
  SoundHandle sound{};
  f32 volume{1.0f};
};

struct SoundSource
{
  SoundHandle handle;
  u32 frame_idx{};
  f32 volume{};
};

class SoundSystem
{
public:
  SoundSystem(os::Audio& audio);
  ~SoundSystem()
  {
    m_thread.request_stop();
    m_thread.join();
  }

  void play(const SoundCmd& cmd);

  void sound_loop(std::stop_token st);

private:
  std::jthread m_thread{};

  os::Audio& m_audio;

  SPSCQueue<SoundCmd> m_cmds{1024};
  EnumArray<SoundHandle, SoundData> m_sound_data{};
  std::vector<SoundSource> m_active_sources{};

  // TODO: what is this frames count chosen based off of?
  static constexpr u32 FRAMES = 512;
  static constexpr u32 CHANNELS = 2;
  static constexpr usize BYTES_PER_BUFFER = FRAMES * CHANNELS * sizeof(i16);
  std::array<i16, FRAMES * CHANNELS> mix_buffer{};
};
