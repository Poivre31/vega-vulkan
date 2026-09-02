#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>
#include <console/console.hpp>
#include <timer/timer.hpp>

#include "imgui_impl_sdl3.h"
#include "layer.hpp"
#include "config.hpp"

struct sdl_exception : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

class sdl_layer final : public Ilayer {
  using Ilayer::Ilayer;

  bool init() noexcept final {
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
        // case (SDL_EVENT_MOUSE_BUTTON_DOWN):
        //   break;
        case (SDL_EVENT_WINDOW_RESIZED):
          get_app_context()->width  = event.window.data1;
          get_app_context()->height = event.window.data2;

          get_app_context()->frame_buffer_resized = true;
          break;

        default:
          break;
      }
      if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LALT]) {
        ImGui_ImplSDL3_ProcessEvent(&event);
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