// #pragma once
// #include "layer.hpp"
// #include "graphics.hpp"

// struct gpu_buffer {
//   vma::raii::Buffer buffer         = nullptr;
//   vma::raii::Buffer staging_buffer = nullptr;
//   uint64_t size{};
// };

// struct cpu_gpu_buffer {
//   vma::raii::Buffer staging_buffer;
//   vma::raii::Buffer buffer;
//   uint64_t size;
// };

// gpu_buffer create_buffer(
//     const vulkan_context& context,
//     uint64_t size,
//     vk::BufferUsageFlags usage,
//     vma::MemoryUsage memory_usage
// ) {
//   vk::BufferCreateInfo buffer_info{
//       .size                  = size,
//       .usage                 = usage | vk::BufferUsageFlagBits::eTransferDst,
//       .sharingMode           = vk::SharingMode::eExclusive,
//       .queueFamilyIndexCount = 1,
//       .pQueueFamilyIndices   = &context.graphics_queue_family,
//   };
//   vma::AllocationCreateInfo allocation_info{
//       .flags = vma::AllocationCreateFlagBits::eDedicatedMemory
//                | vma::AllocationCreateFlagBits::eHostAccessAllowTransferInstead,
//       .usage = vma::MemoryUsage::eAutoPreferDevice
//   };
//   return {.buffer = context.allocator->createBuffer(buffer_info, allocation_info), .size = size};
// }

// cpu_gpu_buffer create_staged_buffer(
//     const vulkan_context& context,
//     uint64_t size,
//     vk::BufferUsageFlags usage,
//     vma::MemoryUsage memory_usage
// ) {
//   gpu_buffer staging_buffer = create_buffer(
//       context, size, true, vk::BufferUsageFlagBits::eTransferSrc,
//       vma::MemoryUsage::eAutoPreferHost
//   );
//   vk::BufferCreateInfo buffer_info{
//       .size                  = size,
//       .usage                 = usage,
//       .sharingMode           = vk::SharingMode::eExclusive,
//       .queueFamilyIndexCount = 1,
//       .pQueueFamilyIndices   = &context.graphics_queue_family,
//   };
//   vma::AllocationCreateInfo allocation_info{
//       .flags = vma::AllocationCreateFlagBits::eDedicatedMemory, .usage = memory_usage
//   };
//   return {
//       .staging_buffer = std::move(staging_buffer.buffer),
//       .buffer         = context.allocator->createBuffer(buffer_info, allocation_info),
//       .size           = size
//   };
// }