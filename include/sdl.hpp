#pragma once
#include <SDL3/SDL.h>  // IWYU pragma: export
#include "stb_image.hpp"

const std::string texturePath = "beer.png";

class sdl_app {
 public:
  void init() {
    image.load(texturePath);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
      std::cerr << "SDL initialisation failed\n" << SDL_GetError();
      std::exit(1);
    }

    window = SDL_CreateWindow("My window", width, height, SDL_WINDOW_RESIZABLE);
    if (!window) {
      std::cerr << "SDL window creation failed\n" << SDL_GetError();
      std::exit(1);
    }

    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
      std::cerr << "SDL renderer creation failed\n" << SDL_GetError();
      std::exit(1);
    }

    imageSurface = SDL_CreateSurfaceFrom(
        image.width(),
        image.height(),
        SDL_PIXELFORMAT_RGBA32,
        image.data(),
        image.width() * image.channels()
    );
    if (!imageSurface) {
      std::cerr << "SDL image surface creation failed\n" << SDL_GetError();
      std::exit(1);
    }

    imageTexture = SDL_CreateTextureFromSurface(renderer, imageSurface);
    if (!imageTexture) {
      std::cerr << "SDL image texture creation failed\n" << SDL_GetError();
      std::exit(1);
    }
  }

  void run() {
    running = true;
    SDL_Event event{0};
    while (running) {
      while (SDL_PollEvent(&event)) {
        switch (event.type) {
          case (SDL_EVENT_QUIT):
            running = false;
            break;

          case (SDL_EVENT_KEY_DOWN):
            if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
              running = false;
            }
            break;

          case (SDL_EVENT_WINDOW_RESIZED):
            width  = event.window.data1;
            height = event.window.data2;
            break;

          default:
            break;
        }
      }
      SDL_zero(event);

      SDL_SetRenderDrawColorFloat(renderer, 0.3, 0.4, 0.9, 1.);
      SDL_RenderClear(renderer);
      SDL_RenderTexture(renderer, imageTexture, nullptr, nullptr);
      SDL_RenderPresent(renderer);
    }
  }

  void quit() {
    SDL_DestroyTexture(imageTexture);
    imageTexture = nullptr;
    SDL_DestroySurface(imageSurface);
    imageSurface = nullptr;
    SDL_DestroyRenderer(renderer);
    renderer = nullptr;
    SDL_DestroyWindow(window);
    window = nullptr;
    SDL_Quit();
  }

 private:
  SDL_Window* window        = nullptr;
  SDL_Renderer* renderer    = nullptr;
  SDL_Surface* imageSurface = nullptr;
  SDL_Texture* imageTexture = nullptr;

  bool running{};
  int width  = 1280;
  int height = 720;

  stb_image image;
};