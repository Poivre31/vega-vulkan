#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>
#include <console/console.hpp>
#include <timer/timer.hpp>
#include "layer.hpp"
#include "global_config.hpp"

const std::string texturePath = "beer.png";

struct sdl_exception : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

class sdl_layer final : public Ilayer {
  using Ilayer::Ilayer;

  bool init() noexcept final {
    auto timer = scoped_timer("sdl-init");

    bool success = SDL_Init(SDL_INIT_VIDEO);
    if (!success) {
      console::get(consoles::graphics)
          ->error("SDL error during initialisation : {:s}\nExiting program", SDL_GetError());
      cleanup();
      return false;
    }

    get_app_context()->window = SDL_CreateWindow(
        config::window_name.data(),
        get_app_context()->width,
        get_app_context()->height,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN
    );
    if (!get_app_context()->window) {
      console::get(consoles::graphics)
          ->error("SDL error during window creation : {:s}\nExiting program", SDL_GetError());
      cleanup();
      return false;
    }

    return true;
  }

  void update(double dt) noexcept final {
    SDL_Event event{0};
    get_app_context()->mouse_data.dx = 0;
    get_app_context()->mouse_data.dy = 0;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case (SDL_EVENT_QUIT):
          get_app_context()->running = false;
          break;

        case (SDL_EVENT_KEY_DOWN):
          if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
            get_app_context()->running = false;
          } else if (event.key.scancode == SDL_SCANCODE_R && !event.key.repeat) {
            get_app_context()->recompile_shaders = true;
          }
          break;

        case (SDL_EVENT_MOUSE_MOTION):
          get_app_context()->mouse_data = {
              .x  = event.motion.x,
              .y  = event.motion.y,
              .dx = -event.motion.xrel,
              .dy = -event.motion.yrel
          };
          break;

        case (SDL_EVENT_WINDOW_RESIZED):
          get_app_context()->width  = event.window.data1;
          get_app_context()->height = event.window.data2;
          break;

        default:
          break;
      }
    }
  }

  void fixed_update(double ts) noexcept final {
    SDL_SetWindowTitle(
        get_app_context()->window,
        (config::window_name
         + "    FPS: " + std::to_string(uint32_t(round(1. / get_app_context()->avg_dt))))
            .data()
    );
  }

  void cleanup() noexcept final {
    SDL_DestroyWindow(get_app_context()->window);
    get_app_context()->window = nullptr;
    get_app_context()->window = nullptr;
    SDL_Quit();
  }

  [[nodiscard]] SDL_Window* get_window() const noexcept { return get_app_context()->window; }
};

// class sdl_app {
//  public:
//   void init() noexcept {
//     auto timer = scoped_timer("sdl-init");
//     _image.load(texturePath);

//     bool success = SDL_Init(SDL_INIT_VIDEO);
//     if (!success) {
//       console::get(consoles::graphics)
//           ->error("SDL error during initialisation : {:s}\nAborting program", SDL_GetError());
//       quit();
//       std::abort();
//     }

//     _window = SDL_CreateWindow("My window", _width, _height, SDL_WINDOW_RESIZABLE);
//     if (!_window) {
//       console::get(consoles::graphics)
//           ->error("SDL error during window creation : {:s}\nAborting program", SDL_GetError());
//       quit();
//       std::abort();
//     }

//     _renderer = SDL_CreateRenderer(_window, nullptr);
//     if (!_renderer) {
//       console::get(consoles::graphics)
//           ->error("SDL error during renderer creation : {:s}\nAborting program", SDL_GetError());
//       quit();
//       std::abort();
//     }

//     _imageSurface = SDL_CreateSurfaceFrom(
//         _image.width(), _image.height(), SDL_PIXELFORMAT_RGBA32, _image.data(), _image.pitch()
//     );
//     if (!_imageSurface) {
//       console::get(consoles::graphics)
//           ->error("SDL error during surface creation : {:s}\nAborting program", SDL_GetError());
//       quit();
//       std::abort();
//     }

//     _imageTexture = SDL_CreateTextureFromSurface(_renderer, _imageSurface);
//     if (!_imageTexture) {
//       console::get(consoles::graphics)
//           ->error("SDL error during texture creation: {:s}\nAborting program", SDL_GetError());
//       quit();
//       std::abort();
//     }
//   }

//   void run() {
//     _running = true;
//     SDL_Event event{0};
//     while (_running) {
//       while (SDL_PollEvent(&event)) {
//         switch (event.type) {
//           case (SDL_EVENT_QUIT):
//             _running = false;
//             break;

//           case (SDL_EVENT_KEY_DOWN):
//             if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
//               _running = false;
//             }
//             break;

//           case (SDL_EVENT_WINDOW_RESIZED):
//             _width  = event.window.data1;
//             _height = event.window.data2;
//             break;

//           default:
//             break;
//         }
//       }
//       SDL_zero(event);

//       try {
//         if (!SDL_SetRenderDrawColorFloat(_renderer, 0.3, 0.4, 0.9, 1.)) {
//           throw sdl_exception(SDL_GetError());
//         }
//         if (!SDL_RenderClear(_renderer)) {
//           throw sdl_exception(SDL_GetError());
//         }
//         if (!SDL_RenderTexture(_renderer, _imageTexture, nullptr, nullptr)) {
//           throw sdl_exception(SDL_GetError());
//         }
//         if (!SDL_RenderPresent(_renderer)) {
//           throw sdl_exception(SDL_GetError());
//         }
//       } catch (const sdl_exception& e) {
//         console::get(consoles::graphics)
//             ->error("SDL error during rendering : {}\nStopping application loop", e.what());
//         _running = false;
//       }
//     }
//   }

//   void quit() {
//     SDL_DestroyTexture(_imageTexture);
//     _imageTexture = nullptr;
//     SDL_DestroySurface(_imageSurface);
//     _imageSurface = nullptr;
//     SDL_DestroyRenderer(_renderer);
//     _renderer = nullptr;
//     SDL_DestroyWindow(_window);
//     _window = nullptr;
//     SDL_Quit();
//   }

//  private:
//   SDL_Window* _window        = nullptr;
//   SDL_Renderer* _renderer    = nullptr;
//   SDL_Surface* _imageSurface = nullptr;
//   SDL_Texture* _imageTexture = nullptr;

//   bool _running{};
//   int _width  = 1280;
//   int _height = 720;

//   stb_image _image;
// };