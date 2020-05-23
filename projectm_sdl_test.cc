#include <SDL2/SDL.h>
#include <iostream>
#define _USE_MATH_DEFINES
#include <cmath>

int main(int argc, char *argv[]) {
  SDL_Window *window = nullptr;
  SDL_Surface *surface = nullptr;
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cout << "could not initialize SDL" << std::endl;
    return 1;
  }

  window =
      SDL_CreateWindow("Test Window", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, 100, 100, SDL_WINDOW_SHOWN);

  if (window == nullptr) {
    std::cout << "Cound not create window" << std::endl;
    return 1;
  }

  surface = SDL_GetWindowSurface(window);
  float t = 0;
  for (int i = 0; i < 1000; i++) {
    SDL_FillRect(surface, nullptr, SDL_MapRGB(surface->format, 0, 0, 0));
    SDL_Rect dest_rect;
    dest_rect.w = 10;
    dest_rect.h = 10;
    dest_rect.x = 45 + 50 * cos(t * M_PI);
    dest_rect.y = 45 + 50 * sin(t * M_PI);
    SDL_FillRect(surface, &dest_rect, SDL_MapRGB(surface->format, 255, 0, 0));
    SDL_UpdateWindowSurface(window);
    SDL_Delay(1000 / 60);
    t += 1.0f / 60;
  }
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
