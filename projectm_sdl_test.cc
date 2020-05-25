#define _USE_MATH_DEFINES
#include <GL/gl.h>
#include <GL/glext.h>
#include <SDL2/SDL.h>
#include <cmath>
#include <iostream>
#include <memory>

#include "libprojectm/projectM.hpp"

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"

ABSL_FLAG(std::string, preset_path, "/usr/share/projectM/presets",
          "Path where preset files are located");
ABSL_FLAG(std::string, menu_font_path, "/usr/share/projectM/fonts/VeraMono.ttf",
          "Path of the font used for the menu");
ABSL_FLAG(std::string, title_font_path, "/usr/share/projectM/fonts/Vera.ttf",
          "Path of the font used for the title");
ABSL_FLAG(int, runtime_seconds, 2, "How long to run the test for, in seconds");

namespace {
// Width of the ProjectM window.
constexpr static int kWindowWidth = 320;
// Height of the ProjectM window.
constexpr static int kWindowHeight = 320;

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
} // namespace

int main(int argc, char *argv[]) {
  absl::ParseCommandLine(argc, argv);

  {
    std::unique_ptr<SDL_Window, SdlWindowDestroyer> window;
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      std::cout << "could not initialize SDL" << std::endl;
      return 1;
    }

    window.reset(SDL_CreateWindow(
        "Test Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        kWindowWidth, kWindowHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI));

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
    settings.textureSize = 256;
    settings.meshX = 32;
    settings.meshY = 32;
    settings.fps = 60;
    settings.smoothPresetDuration = 3;
    settings.presetDuration = 3;
    settings.beatSensitivity = 1;
    settings.aspectCorrection = true;
    settings.shuffleEnabled = true;
    settings.softCutRatingsEnabled = true;
    settings.easterEgg = 1.0f;
    settings.textureSize = 256;

    settings.presetURL = absl::GetFlag(FLAGS_preset_path);
    settings.menuFontURL = absl::GetFlag(FLAGS_menu_font_path);
    settings.titleFontURL = absl::GetFlag(FLAGS_title_font_path);

    int flags = 0;
    std::cout << "Initializing projectM" << std::endl;
    std::unique_ptr<projectM> projectm =
        std::make_unique<projectM>(settings, flags);

    float buffer[1024] = {0};

    float t = 0;
    for (int i = 0; i < 60 * absl::GetFlag(FLAGS_runtime_seconds); i++) {
      glClearColor(0.0, 0.0, 0.0, 0.0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      projectm->renderFrame();
      projectm->pcm()->addPCMfloat(buffer, 1024);
      SDL_GL_SwapWindow(window.get());
      SDL_Delay(1000 / 60);
      t += 1.0f / 60;
    }
  }
  SDL_Quit();
  return 0;
}
