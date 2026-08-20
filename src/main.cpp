#include "graphics.hpp"
#include "sdl.hpp"
#include <console/console.hpp>

const vk::ApplicationInfo appInfo{
    .pApplicationName   = "Vega Vulkan",
    .applicationVersion = vk::makeVersion(0, 1, 0),
    .apiVersion         = vk::ApiVersion13
};

int main() {
  sdl_app app;
  app.init();
  app.run();
  app.quit();
}