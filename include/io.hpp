#pragma once
#include <iostream>
#include <fstream>
#include <console/console.hpp>

inline std::string read_file(const std::string& filename) {
  std::ifstream file(
      filename,
      std::ios::ate | std::ios::binary
  );  //> Read from file end + SPIRV is in binary

  if (!file.is_open()) {
    throw std::runtime_error("File " + filename + " failed to open");
  }

  auto size = static_cast<std::streamsize>(file.tellg());
  std::string buffer(size + 1, '\0');
  file.seekg(0, std::ios::beg);
  file.read(buffer.data(), size);
  file.close();

  console::get(consoles::assets)->trace("Succesfully loaded file {}", filename);

  return buffer;
}