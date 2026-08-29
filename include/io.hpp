#pragma once
#include <iostream>
#include <fstream>

inline std::string read_file(const std::string& filename) {
  std::ifstream file(
      filename,
      std::ios::ate | std::ios::binary
  );  //> Read from file end + SPIRV is in binary

  if (!file.is_open()) {
    throw std::runtime_error("File " + filename + " failed to open");
  }

  std::streamsize size = static_cast<std::streamsize>(file.tellg());
  std::string buffer(size, ' ');  //> Use current caracter position to find the size of the file
  file.seekg(0, std::ios::beg);   //> And return at file beginning
  file.read(buffer.data(), size);
  file.close();

  return buffer;
}