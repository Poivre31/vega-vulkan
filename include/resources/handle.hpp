#pragma once

template <typename T>
struct handle {
  uint32_t index{};
  uint16_t allocator_id{};
  uint16_t generation : 15 {};
  uint16_t valid      : 1 = false;
};