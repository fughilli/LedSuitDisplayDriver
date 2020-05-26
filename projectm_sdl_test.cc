//
// LED Suit Driver - Embedded host driver software for Kevin's LED suit
// controller. Copyright (C) 2019-2020 Kevin Balke
//
// This file is part of LED Suit Driver.
//
// LED Suit Driver is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// LED Suit Driver is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with LED Suit Driver.  If not, see <http://www.gnu.org/licenses/>.
//

#define _USE_MATH_DEFINES
#include <GL/gl.h>
#include <GL/glext.h>
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "libprojectm/projectM.hpp"
#include "performance_timer.h"

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"

ABSL_FLAG(std::string, preset_path, "/usr/share/projectM/presets",
          "Path where preset files are located");
ABSL_FLAG(std::string, menu_font_path, "/usr/share/projectM/fonts/VeraMono.ttf",
          "Path of the font used for the menu");
ABSL_FLAG(std::string, title_font_path, "/usr/share/projectM/fonts/Vera.ttf",
          "Path of the font used for the title");
ABSL_FLAG(int, audio_device_id, 0, "ID of audio device to record from");

namespace led_driver {

namespace {
// Width of the ProjectM window.
constexpr static int kWindowWidth = 320;
// Height of the ProjectM window.
constexpr static int kWindowHeight = 320;
// Target FPS.
constexpr static int kFps = 60;
// Target frame time, in milliseconds.
constexpr static int kTargetFrameTimeMs = 1000 / kFps;

// Enables vsync for the SDL application.
void EnableVsync() {
  int avsync = SDL_GL_SetSwapInterval(-1);
  if (avsync == -1) {
    SDL_GL_SetSwapInterval(1);
  }
}

struct SdlWindowDestroyer {
  void operator()(SDL_Window *window) { SDL_DestroyWindow(window); }
};

struct CallbackData {
  std::shared_ptr<projectM> projectm;
  int channel_count;
};

void AudioInputCallbackF32(void *user_data, unsigned char *stream, int length) {
  CallbackData *callback_data = reinterpret_cast<CallbackData *>(user_data);
  switch (callback_data->channel_count) {
  case 1:
    callback_data->projectm->pcm()->addPCMfloat(
        reinterpret_cast<float *>(stream), length / sizeof(float));
    break;
  case 2:
    callback_data->projectm->pcm()->addPCMfloat_2ch(
        reinterpret_cast<float *>(stream), length / sizeof(float));
    break;
  default:
    std::cerr << "Unsupported PCM channel count: "
              << callback_data->channel_count << std::endl;
    SDL_Quit();
  }
}

std::shared_ptr<CallbackData>
InitializeAudio(std::shared_ptr<projectM> projectm, int audio_device_id) {
  const char *driver_name = SDL_GetCurrentAudioDriver();
  std::cout << "Using driver: " << driver_name << std::endl;

  int num_devices = SDL_GetNumAudioDevices(true);
  std::cout << "Number of devices: " << num_devices << std::endl;

  auto callback_data = std::make_shared<CallbackData>();
  callback_data->projectm = projectm;
  callback_data->channel_count = 0;

  SDL_AudioSpec desired_spec = {0}, actual_spec = {0};
  desired_spec.freq = 44100;
  desired_spec.format = AUDIO_F32;
  desired_spec.channels = 2;
  desired_spec.samples = 1024;
  desired_spec.callback = &AudioInputCallbackF32;
  desired_spec.userdata = reinterpret_cast<void *>(callback_data.get());

  std::vector<std::string> device_names;
  for (int i = 0; i < num_devices; ++i) {
    device_names.emplace_back(SDL_GetAudioDeviceName(i, true));
    std::cout << "Audio device name: " << device_names[i] << std::endl;
  }
  if (audio_device_id >= num_devices) {
    std::cerr << "Invalid audio device id: " << audio_device_id << std::endl;
    SDL_Quit();
  }
  std::cout << "Selected device: " << device_names[audio_device_id]
            << std::endl;
  int opened_audio_device_id =
      SDL_OpenAudioDevice(device_names[audio_device_id].c_str(), true,
                          &desired_spec, &actual_spec, 0);
  if (opened_audio_device_id == 0) {
    std::cerr << "Failed to open audio device; SDL_OpenAudioDevice returned "
              << opened_audio_device_id << std::endl;
    SDL_Quit();
  }

  if (actual_spec.format != AUDIO_F32) {
    std::cerr << "Failed to open audio device with format AUDIO_F32; "
                 "SDL_OpenAudioDevice returned "
              << actual_spec.format << std::endl;
    SDL_Quit();
  }
  callback_data->channel_count = actual_spec.channels;

  SDL_PauseAudioDevice(audio_device_id, false);

  return callback_data;
}

} // namespace

extern "C" int main(int argc, char *argv[]) {
  absl::ParseCommandLine(argc, argv);

  {
    std::unique_ptr<SDL_Window, SdlWindowDestroyer> window;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
      std::cout << "could not initialize SDL" << std::endl;
      return 1;
    }

    window.reset(
        SDL_CreateWindow("Test Window", SDL_WINDOWPOS_UNDEFINED,
                         SDL_WINDOWPOS_UNDEFINED, kWindowWidth, kWindowHeight,
                         SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN |
                             SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE));

    if (window == nullptr) {
      std::cout << "Cound not create window" << std::endl;
      return 1;
    }

    SDL_GLContext opengl_context = SDL_GL_CreateContext(window.get());
    SDL_GL_MakeCurrent(window.get(), opengl_context);

    EnableVsync();

    projectM::Settings settings;
    settings.windowWidth = kWindowWidth;
    settings.windowHeight = kWindowHeight;
    settings.meshX = 32;
    settings.meshY = 32;
    settings.fps = 60;
    settings.smoothPresetDuration = 3;
    settings.presetDuration = 10;
    settings.beatSensitivity = 1;
    settings.aspectCorrection = false;
    settings.shuffleEnabled = true;
    settings.softCutRatingsEnabled = true;
    settings.easterEgg = 1.0f;
    settings.textureSize = 256;

    settings.presetURL = absl::GetFlag(FLAGS_preset_path);
    settings.menuFontURL = absl::GetFlag(FLAGS_menu_font_path);
    settings.titleFontURL = absl::GetFlag(FLAGS_title_font_path);

    int flags = 0;
    std::cout << "Initializing projectM" << std::endl;
    std::shared_ptr<projectM> projectm =
        std::make_unique<projectM>(settings, flags);
    std::shared_ptr<CallbackData> callback_data_structure =
        InitializeAudio(projectm, absl::GetFlag(FLAGS_audio_device_id));

    float buffer[1024];
    for (int i = 0; i < 1024; ++i) {
      buffer[i] = sin(i * M_PI * 2 / 1024);
      if (i < 100) {
        buffer[i] += sin(i * 20 * M_PI * 2 / 1024);
        buffer[i] /= 2;
      }
    }

    bool exit_event_received = false;
    PerformanceTimer<uint32_t> frame_timer;
    while (!exit_event_received) {
      frame_timer.Start(SDL_GetTicks());
      glClearColor(0.0, 0.0, 0.0, 0.0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      projectm->renderFrame();

      SDL_Event event;
      while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_WINDOWEVENT:
          int new_width, new_height;
          SDL_GL_GetDrawableSize(window.get(), &new_width, &new_height);
          switch (event.window.event) {
          case SDL_WINDOWEVENT_RESIZED:
          case SDL_WINDOWEVENT_SIZE_CHANGED:
            projectm->projectM_resetGL(new_width, new_height);
            break;
          }
          break;
        case SDL_KEYDOWN:
          // Handle key
          switch (event.key.keysym.sym) {
          case SDLK_LEFT:
            projectm->selectPrevious(true);
            break;
          case SDLK_RIGHT:
            projectm->selectNext(true);
            break;
          }
          break;
        case SDL_QUIT:
          exit_event_received = true;
          break;
        }
      }
      SDL_GL_SwapWindow(window.get());

      uint32_t frame_time = frame_timer.End(SDL_GetTicks());
      if (frame_time >= kTargetFrameTimeMs) {
        continue;
      }
      SDL_Delay(kTargetFrameTimeMs - frame_time);
    }
  }
  SDL_Quit();
  return 0;
}
} // namespace led_driver
