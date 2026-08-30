#pragma once
#include <concepts>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename T>
struct handle {
  uint32_t index{};
  uint16_t generation{};
  uint16_t allocator_id{};
};

template <std::move_constructible T>
class slot {
 public:
  slot() = default;

  ~slot() {
    if (!_is_free) {
      free();
    }
  }

  slot(const slot&)            = delete;
  slot& operator=(const slot&) = delete;
  slot& operator=(slot&&)      = delete;

  template <typename... Args>
    requires(!std::same_as<std::decay_t<Args>, slot> && ...)
  slot(Args&&... args) : _is_free(false) {
    new (_data) T(std::forward<Args>(args)...);
  }

  slot(slot&& other) noexcept : _is_free(other._is_free), _generation(other._generation) {
    if (!other._is_free) {
      new (_data) T(std::move(*other.get()));
      other._is_free = true;
    }
  }

  slot(T&& value) : _is_free(false) { new (_data) T(std::forward<T>(value)); }  // NOLINT

  [[nodiscard]] T* get() { return reinterpret_cast<T*>(_data); }

  template <typename... Args>
  void emplace(Args&&... args) {
    if (!_is_free) {
      throw std::runtime_error("Free a slot before emplacing");
    }
    new (_data) T(std::forward<Args>(args)...);

    _is_free = false;
  }

  void emplace(T&& value) {  // NOLINT
    if (!_is_free) {
      throw std::runtime_error("Free a slot before emplacing");
    }
    new (_data) T(std::forward<T>(value));

    _is_free = false;
  }

  void free() {
    _is_free = true;
    std::destroy_at(reinterpret_cast<T*>(_data));
    _generation++;
  }

  [[nodiscard]] bool check_generation(handle<T> handle) const {
    return _generation == handle.generation;
  }
  [[nodiscard]] uint16_t generation() const { return _generation; }
  [[nodiscard]] bool isFree() const { return _is_free; }

 private:
  alignas(T) std::byte _data[sizeof(T)]{};  // NOLINT
  uint16_t _is_free    : 1  = true;
  uint16_t _generation : 15 = 0;
};

template <std::move_constructible T>
class allocator {
 public:
  allocator() {
    static uint16_t allocator_id = 0;
    static std::mutex allocator_id_mutex;

    _id = allocator_id;

    std::lock_guard<std::mutex> lock(allocator_id_mutex);
    allocator_id++;
  }

  void reserve(uint32_t size) { _objects.reserve(size); }

  template <typename... Args>
  [[nodiscard]] handle<T> push(Args&&... args) {
    uint32_t index{};
    if (_free_indices.empty()) {
      index = _objects.size();
      _objects.emplace_back(std::forward<Args>(args)...);
    } else {
      index = _free_indices.back();
      _free_indices.pop_back();
      _objects.at(index).emplace(std::forward<Args>(args)...);
    }

    return {.index = index, .generation = _objects.at(index).generation(), .allocatorId = _id};
  }

  void pop(handle<T> handle) {
    uint32_t index = get_index(handle);
    _objects.at(index).free();
    _free_indices.push_back(index);
  }

  [[nodiscard]] T* get(handle<T> handle) { return _objects.at(get_index(handle)).get(); }
  // [[nodiscard]] std::optional<Handle> getHandle(const T& object) {
  //   for (auto& slot : _objects) {
  //     if (slot.get() == &object && slot.isFree()) {
  //       return Handle{slot.generation()};
  //     }
  //   }
  //   return std::nullopt;
  // }
  [[nodiscard]] T* at_index(uint32_t index) {
    if (_objects.at(index).isFree()) {
      throw std::domain_error("Trying to access index that is not assigned");
    }
    return _objects.at(index).get();
  }

  [[nodiscard]] bool is_slot_free(uint32_t index) const { return _objects.at(index).isFree(); }

  [[nodiscard]] uint32_t size() const { return _objects.size(); }

  void clear() {
    _free_indices.clear();
    _objects.clear();
  }

 private:
  [[nodiscard]] uint32_t get_index(handle<T> handle) const {
    if (handle.allocator_id != _id) {
      throw std::invalid_argument(
          "Trying to access an allocator with a handle that hasn't been created with the same "
          "allocator"
      );
    } else if (handle.index >= _objects.size()) {
      throw std::invalid_argument(
          "Handle index is bigger than the number of stored objects, something is wrong"
      );
    } else if (!_objects.at(handle.index).checkGeneration(handle)) {
      throw std::invalid_argument(
          "Handle and accessed element generation do not match, the slot have been reusued by "
          "another object"
      );
    }
    return handle.index;
  }

  uint16_t _id{};
  std::vector<uint32_t> _free_indices;
  std::vector<slot<T>> _objects;
};

template <std::move_constructible T>
class static_allocator {
 public:
  static_allocator() {
    static uint16_t allocator_id = 0;
    static std::mutex allocator_id_mutex;

    _id = allocator_id;

    std::lock_guard<std::mutex> lock(allocator_id_mutex);
    allocator_id++;
  }

  void reserve(uint32_t size) { _objects.reserve(size); }

  template <typename... Args>
  [[nodiscard]] handle<T> push(Args&&... args) {
    uint32_t index{};
    index = _objects.size();
    _objects.emplace_back(std::forward<Args>(args)...);

    return {.index = index, .generation = _generation, .allocator_id = _id};
  }

  [[nodiscard]] T* get(handle<T> handle) { return _objects.at(get_index(handle)).get(); }
  [[nodiscard]] T* at_index(uint32_t index) { return &_objects.at(index); }

  [[nodiscard]] uint32_t size() const { return _objects.size(); }

  void clear() {
    _objects.clear();
    _generation++;
  }

  auto begin() { return _objects.begin(); }
  auto end() { return _objects.end(); }
  auto cbegin() { return _objects.cbegin(); }
  auto cend() { return _objects.cend(); }

 private:
  [[nodiscard]] uint32_t get_index(handle<T> handle) const {
    if (handle.allocator_id != _id) {
      throw std::invalid_argument(
          "Trying to access an allocator with a handle that hasn't been created with the same "
          "allocator"
      );
    } else if (handle.index >= _objects.size()) {
      throw std::invalid_argument(
          "Handle index is bigger than the number of stored objects, something is wrong"
      );
    } else if (!_generation != handle.generation) {
      throw std::invalid_argument(
          "Handle and static allocator generation do not match, the allocator has been cleared in "
          "between"
      );
    }
    return handle.index;
  }

  uint16_t _id{};
  uint16_t _generation = 0;
  std::vector<T> _objects;
};

template <std::move_constructible T>
class iterator {
 public:
  iterator(allocator<T>& allocator, uint32_t index)
      : allocator_ref(allocator), current_index(index) {}

  const T& operator*() const { return *allocator_ref.at_index(current_index); }

  iterator& operator++() {
    current_index++;
    while (current_index < allocator_ref.size() && allocator_ref.is_slot_free(current_index)) {
      current_index++;
    }
    return *this;
  }

  friend bool operator==(const iterator& lhs, const iterator& rhs) {
    if (&(lhs.allocator_ref) == &(rhs.allocator_ref)) {
      return lhs.current_index == rhs.current_index;
    }
    return false;
  }

  friend bool operator!=(const iterator& lhs, const iterator& rhs) { return !(lhs == rhs); }

 private:
  allocator<T>& allocator_ref;
  uint32_t current_index{};
};

template <std::move_constructible T>
auto begin(allocator<T>& allocator) {
  if (allocator.size() == 0) {
    return end(allocator);
  }
  iterator<T> iterator(allocator, 0);
  if (allocator.is_slot_free(0)) {
    ++iterator;
  }
  return iterator;
}

template <std::move_constructible T>
auto end(allocator<T>& allocator) {
  return iterator(allocator, allocator.size());
}