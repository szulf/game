#pragma once

#include <thread>
#include <vector>

#include "base/base.h"
#include "base/enum_array.h"
#include "base/spsc_queue.h"

#include "os/os.h"

enum class SoundHandle
{
  SINE,
  // TODO: remove, test sound
  SHOTGUN,

  COUNT,
};

struct SoundData
{
  std::vector<i16> samples{};
  u32 frames{};
};

// TODO: move this to the SoundSystem class?
extern EnumArray<SoundHandle, SoundData> sound_data;

// TODO: move this to the SoundSystem class?
void load_wav(SoundHandle sound);

struct SoundCmd
{
  SoundHandle sound{};
  f32 volume{1.0f};
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

  void sound_loop();

private:
  std::jthread m_thread{};

  os::Audio& m_audio;

  SPSCQueue<SoundCmd> m_cmds{1024};

  // TODO: is this a standard name? i dont like it
  struct Voice
  {
    const SoundData* data;
    u32 frame_idx{};
    f32 volume{};
  };

  std::vector<Voice> m_active_voices{};

  static constexpr u32 FRAMES = 512;
  static constexpr u32 CHANNELS = 2;
  std::array<i16, FRAMES * CHANNELS> mix_buffer{};
};
