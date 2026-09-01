#pragma once

#include <console/console.hpp>

#include "graphics.hpp"
#include "context.hpp"
#include "single_command_buffer.hpp"

template <class T>
class host_buffer {
 public:
  host_buffer(std::nullptr_t) {};
  host_buffer(
      vulkan_context vk_context,
      size_t number_of_elements,
      bool permanent,
      bool buffer_device_adress = true
  )
      : _size(number_of_elements * sizeof(T)),
        _number(number_of_elements),
        _permanent(permanent),
        _bda(buffer_device_adress) {
    _create_staging_buffer(vk_context);
    if (_staging_buffer.getAllocation().getMemoryProperties()
        & vk::MemoryPropertyFlagBits::eDeviceLocal) {
      console::get(consoles::graphics)
          ->trace("Allocated {}B on device without staging buffer", _size);
      _buffer         = std::move(_staging_buffer);
      _staging_buffer = nullptr;

      _staged = false;
    } else {
      auto usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;
      if (_bda) {
        usage |= vk::BufferUsageFlagBits::eShaderDeviceAddress;
      }
      vk::BufferCreateInfo buffer_info{.size = _size, .usage = usage};
      vma::AllocationCreateFlags allocation_flags;
      if (_permanent) {
        allocation_flags |= vma::AllocationCreateFlagBits::eMapped;
      }

      vma::AllocationCreateInfo allocation_info{
          .flags = allocation_flags,
          .usage = vma::MemoryUsage::eAutoPreferDevice,
      };
      _buffer = vk_context.allocator->createBuffer(buffer_info, allocation_info);
      if (_buffer.getAllocation().getMemoryProperties()
          & vk::MemoryPropertyFlagBits::eDeviceLocal) {
        console::get(consoles::graphics)
            ->trace("Allocated {}B on device and created associated host staging buffer", _size);
      } else {
        console::get(consoles::graphics)
            ->warn(
                "Tried to allocate {}B on device but vma allocated it on host memory. A staging "
                "buffer has also been allocated on host memory",
                _size
            );
      }
      _staged = true;
    }
  }

  void upload_data(vulkan_context vk_context, const std::vector<T>& data) {
    if (!data.data()) {
      throw std::runtime_error("Uploading invalid data (nullptr) to buffer");
    }
    if (data.size() > _number) {
      throw std::runtime_error("Uploading more data than the buffer was created for");
    }
    if (!*_buffer) {
      throw std::runtime_error("Uploading data before buffer creation");
    }
    if (_staged) {
      if (!*_staging_buffer) {
        _create_staging_buffer(vk_context);
        console::get(consoles::graphics)
            ->warn(
                "Recreating a staging buffer after host_buffer creation, this is valid but it's "
                "best to set 'permanent=true' at creation if you intend to upload several times to "
                "the buffer"
            );
      }
      _staging_buffer.getAllocation().copyFromMemory(data.data(), 0, _size);
      auto cmd = begin_transient_command_buffer(vk_context);
      vk::BufferCopy2 region{.srcOffset = 0, .dstOffset = 0, .size = _size};
      vk::CopyBufferInfo2 copy_info{
          .srcBuffer = _staging_buffer, .dstBuffer = _buffer, .regionCount = 1, .pRegions = &region
      };
      cmd.copyBuffer2(copy_info);
      submit_single_command_buffer(vk_context, std::move(cmd));
      if (!_permanent) {
        _staging_buffer = nullptr;
      }
    } else {
      _buffer.getAllocation().copyFromMemory(data.data(), 0, _size);
    }
  }

  [[nodiscard]] vma::raii::Buffer& get() { return _buffer; }

  [[nodiscard]] vk::DeviceAddress get_adress(vulkan_context& vk_context) {
    if (!_bda) {
      throw std::runtime_error(
          "Trying to get the adress of a buffer that doesn't use buffer device adress"
      );
    }
    if (!*_buffer) {
      throw std::runtime_error(
          "Trying to get the buffer adress of a buffer that hasn't been created"
      );
    }
    vk::BufferDeviceAddressInfo address_info{.buffer = _buffer};
    vk::DeviceAddress adress = vk_context.device->getBufferAddress(address_info);
    if (!adress) {
      throw std::runtime_error("Material buffer adresss is invalid");
    }
    return adress;
  }

 private:
  void _create_staging_buffer(vulkan_context& vk_context) {
    if (*_staging_buffer) {
      return;
    }
    auto usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc;
    if (_bda) {
      usage |= vk::BufferUsageFlagBits::eShaderDeviceAddress;
    }
    vk::BufferCreateInfo staging_buffer_info{
        .size        = _size,
        .usage       = usage,
        .sharingMode = vk::SharingMode::eExclusive,
    };
    auto allocation_flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite
                            | vma::AllocationCreateFlagBits::eHostAccessAllowTransferInstead;
    if (_permanent) {
      allocation_flags |= vma::AllocationCreateFlagBits::eMapped;
    }
    vma::AllocationCreateInfo allocation_info{
        .flags = allocation_flags, .usage = vma::MemoryUsage::eAutoPreferHost
    };
    _staging_buffer = vk_context.allocator->createBuffer(staging_buffer_info, allocation_info);
  }

  vma::raii::Buffer _buffer         = nullptr;
  vma::raii::Buffer _staging_buffer = nullptr;
  bool _staged{};
  bool _permanent{};
  bool _bda{};
  size_t _size{};
  size_t _number{};
};