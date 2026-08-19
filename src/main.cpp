#include <iostream>
#include <SDL3/SDL.h>

int main() {
  std::cout << "Hello world" << std::endl;
  bool running = true;
  int width    = 1280;
  int height   = 720;

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    std::cerr << "SDL initialisation failed\n";
    std::exit(1);
  }
  SDL_Window* window = SDL_CreateWindow(
      "My window", width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN
  );
  if (!window) {
    std::cerr << "SDL window creation failed\n";
    std::exit(1);
  }
  SDL_Surface* surface = SDL_GetWindowSurface(window);
  if (!surface) {
    std::cerr << "SDL surface creation failed\n";
    std::exit(1);
  }

  SDL_Event event{0};
  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT
          || (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE)) {
        std::cout << "Quit" << std::endl;
        running = false;
        break;
      } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
        std::cout << "Resize" << std::endl;
        width  = event.window.data1;
        height = event.window.data2;
        break;
      }
    }
    SDL_zero(event);

    SDL_FillSurfaceRect(surface, nullptr, SDL_MapSurfaceRGB(surface, 0xFF, 0xFF, 0xFF));
    SDL_UpdateWindowSurface(window);
  }

  SDL_DestroyWindow(window);
  surface = nullptr;
  window  = nullptr;
  SDL_Quit();
}