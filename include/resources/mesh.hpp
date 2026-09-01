#pragma once

#include <console/console.hpp>

#include "graphics/graphics.hpp"
#include "graphics/context.hpp"
#include "graphics/single_command_buffer.hpp"

struct vertex_3D {
  glm::vec3 position{};
  glm::vec3 normal{};
  glm::vec2 uv{};
  uint32_t material_id{};

  static vk::VertexInputBindingDescription binding_description() {
    return {.binding = 0, .stride = sizeof(vertex_3D), .inputRate = vk::VertexInputRate::eVertex};
  }

  static std::vector<vk::VertexInputAttributeDescription> attribute_description() {
    return {
        vk::VertexInputAttributeDescription{
            .location = 0,
            .binding  = 0,
            .format   = vk::Format::eR32G32B32Sfloat,
            .offset   = offsetof(vertex_3D, position)
        },
        vk::VertexInputAttributeDescription{
            .location = 1,
            .binding  = 0,
            .format   = vk::Format::eR32G32B32Sfloat,
            .offset   = offsetof(vertex_3D, normal)
        },
        vk::VertexInputAttributeDescription{
            .location = 2,
            .binding  = 0,
            .format   = vk::Format::eR32G32Sfloat,
            .offset   = offsetof(vertex_3D, uv)
        },
        vk::VertexInputAttributeDescription{
            .location = 3,
            .binding  = 0,
            .format   = vk::Format::eR32Uint,
            .offset   = offsetof(vertex_3D, material_id)
        }
    };
  }
};

class mesh_3D {
 public:
  mesh_3D() = default;
  explicit mesh_3D(const std::vector<vertex_3D>& vertices) : _vertices(vertices) {}

  void load_vertices(const std::vector<vertex_3D>& vertices) { _vertices = vertices; }
  [[nodiscard]] std::vector<vertex_3D> get_vertices() const noexcept { return _vertices; }

  void create_vertex_buffer(vulkan_context& context) {
    if (_vertices.empty()) {
      console::get(consoles::assets)
          ->warn("No vertex buffer created because mesh doesn't have any vertices loaded");
      return;
    }
    uint64_t size = _vertices.size() * sizeof(vertex_3D);
    vk::BufferCreateInfo staging_buffer_info{
        .size        = size,
        .usage       = vk::BufferUsageFlagBits::eTransferSrc,
        .sharingMode = vk::SharingMode::eExclusive
    };
    vma::AllocationCreateInfo staging_allocation_info{
        .flags = vma::AllocationCreateFlagBits::eMapped
                 | vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
        .usage = vma::MemoryUsage::eAutoPreferHost
    };
    auto staging_buffer = context.allocator->createBuffer(
        staging_buffer_info, staging_allocation_info
    );
    staging_buffer.getAllocation().copyFromMemory(_vertices.data(), 0, size);

    vk::BufferCreateInfo buffer_info{
        .size  = size,
        .usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        .sharingMode = vk::SharingMode::eExclusive
    };
    vma::AllocationCreateInfo allocation_info{.usage = vma::MemoryUsage::eAutoPreferDevice};
    _buffer  = context.allocator->createBuffer(buffer_info, allocation_info);
    auto cmd = begin_transient_command_buffer(context);
    cmd.copyBuffer(
        staging_buffer, _buffer, vk::BufferCopy{.srcOffset = 0, .dstOffset = 0, .size = size}
    );
    submit_single_command_buffer(context, std::move(cmd));
  }

  //   void set_texture_handles(const std::vector<handle<gpu_image>>& handles) {
  //     _texture_handles = handles;
  //   }

  void render(vk::raii::CommandBuffer& command_buffer) const {
    if (!*_buffer) {
      console::get(consoles::assets)
          ->warn("Skipped rendering mesh because its vertex buffer hasn't been created yet");
      return;
    }
    command_buffer.bindVertexBuffers(0, *_buffer, {0});
    command_buffer.draw(_vertices.size(), 1, 0, 0);
  }

 private:
  std::vector<vertex_3D> _vertices;
  //   std::vector<handle<gpu_image>> _texture_handles;
  vma::raii::Buffer _buffer = nullptr;
};