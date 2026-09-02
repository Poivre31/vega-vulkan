#pragma once

#include "graphics/gpu_objects.hpp"
#include "resources/stb_image.hpp"

#include "image_layout.hpp"
#include "single_command_buffer.hpp"
#include "context.hpp"
#include "vulkan/vulkan.hpp"

class simple_sampler {
 public:
  simple_sampler(std::nullptr_t) {}
  simple_sampler(
      vulkan_context& vk_context,
      bool linear_filtering,
      vk::SamplerAddressMode adress_mode,
      float mip_lod_bias
  )
      : _linear_filtering(linear_filtering),
        _adress_mode(adress_mode),
        _mip_lod_bias(mip_lod_bias) {
    auto properties = vk_context.physical_device->getProperties();
    vk::SamplerCreateInfo sampler_info{
        .magFilter        = linear_filtering ? vk::Filter::eLinear : vk::Filter::eNearest,
        .minFilter        = linear_filtering ? vk::Filter::eLinear : vk::Filter::eNearest,
        .mipmapMode       = linear_filtering ? vk::SamplerMipmapMode::eLinear
                                             : vk::SamplerMipmapMode::eNearest,
        .addressModeU     = adress_mode,
        .addressModeV     = adress_mode,
        .addressModeW     = adress_mode,
        .mipLodBias       = _mip_lod_bias,
        .anisotropyEnable = vk::True,
        .maxAnisotropy    = properties.limits.maxSamplerAnisotropy,
        .minLod           = 0.0F,
        .maxLod           = linear_filtering ? vk::LodClampNone : 0.F,
        .borderColor      = vk::BorderColor::eIntOpaqueBlack,
    };
    _sampler = vk::raii::Sampler(*vk_context.device, sampler_info);
  }

  [[nodiscard]] bool is_using_linear_filtering() const { return _linear_filtering; }
  [[nodiscard]] vk::raii::Sampler& get() { return _sampler; }

 private:
  vk::raii::Sampler _sampler = nullptr;
  bool _linear_filtering{};
  vk::SamplerAddressMode _adress_mode{};
  float _mip_lod_bias = 0.F;
};

void generate_mip_maps(
    vulkan_context& vk_context,
    vk::raii::CommandBuffer& cmd,
    gpu_image& texture
) {
  auto format_properties = vk_context.physical_device->getFormatProperties(texture.format);
  if (!(format_properties.optimalTilingFeatures
        & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
    throw std::runtime_error(
        "Choosed image format doesn't support linear filtering necessary to generate mipmaps"
    );
  }
  if (texture.layout != vk::ImageLayout::eTransferDstOptimal) {
    throw std::runtime_error(
        "All mip maps have to be in transfer dst optimal layout before mipmap generation"
    );
  }
  texture.mip_level_count =
      static_cast<uint32_t>(std::floor(std::log2(std::max(texture.width, texture.height)))) + 1;

  uint32_t mip_width  = texture.width;
  uint32_t mip_height = texture.height;

  for (uint32_t mip_level = 1; mip_level < texture.mip_level_count; mip_level++) {
    transition_image_mip_layout(texture, cmd, layout_transition::dst_to_src, mip_level - 1);
    vk::ImageBlit blit = {
        .srcSubresource =
            {.aspectMask = texture.aspect, .mipLevel = mip_level - 1, .layerCount = 1},
        .srcOffsets = std::array<vk::Offset3D, 2>(
            {{},
             {.x = static_cast<int32_t>(mip_width), .y = static_cast<int32_t>(mip_height), .z = 1}}
        ),
        .dstSubresource = {.aspectMask = texture.aspect, .mipLevel = mip_level, .layerCount = 1},
        .dstOffsets     = std::array<vk::Offset3D, 2>(
            {{},
             {.x = std::max(static_cast<int32_t>(mip_width / 2), 1),
              .y = std::max(static_cast<int32_t>(mip_height / 2), 1),
              .z = 1}}
        ),
    };

    cmd.blitImage(
        texture.image,
        vk::ImageLayout::eTransferSrcOptimal,
        texture.image,
        vk::ImageLayout::eTransferDstOptimal,
        blit,
        vk::Filter::eLinear
    );

    transition_image_mip_layout(texture, cmd, layout_transition::src_to_shader_read, mip_level - 1);

    mip_width  = std::max(mip_width / 2, 1U);
    mip_height = std::max(mip_height / 2, 1U);
  }
  transition_image_mip_layout(
      texture, cmd, layout_transition::dst_to_shader_read, texture.mip_level_count - 1
  );

  texture.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
}

gpu_image create_image(
    vma::raii::Allocator& allocator,
    vk::raii::Device& device,
    vk::Format format,
    uint32_t width,
    uint32_t height,
    vk::ImageUsageFlags usage,
    vk::ImageAspectFlags aspect,
    bool transfer,
    uint32_t mip_level_count             = 1,
    vk::SampleCountFlagBits msaa_samples = vk::SampleCountFlagBits::e1
) {
  if (transfer) {
    usage |= vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
  }
  vk::ImageCreateInfo image_info{
      .imageType   = vk::ImageType::e2D,
      .format      = format,
      .extent      = {.width = width, .height = height, .depth = 1},
      .mipLevels   = mip_level_count,
      .arrayLayers = 1,
      .samples     = msaa_samples,
      .tiling      = vk::ImageTiling::eOptimal,
      .usage       = usage,
      .sharingMode = vk::SharingMode::eExclusive
  };
  vma::AllocationCreateInfo allocation_info{.usage = vma::MemoryUsage::eAutoPreferDevice};
  auto image = vma::raii::Image(allocator, image_info, allocation_info);

  vk::ImageViewCreateInfo view_info{
      .image            = image,
      .viewType         = vk::ImageViewType::e2D,
      .format           = format,
      .subresourceRange = {
          .aspectMask     = aspect,
          .baseMipLevel   = 0,
          .levelCount     = mip_level_count,
          .baseArrayLayer = 0,
          .layerCount     = 1
      }
  };
  auto view = vk::raii::ImageView(device, view_info);

  return {
      .image           = std::move(image),
      .view            = std::move(view),
      .width           = width,
      .height          = height,
      .format          = format,
      .aspect          = aspect,
      .layout          = vk::ImageLayout::eUndefined,
      .mip_level_count = mip_level_count
  };
}

gpu_image
load_texture_to_gpu(vulkan_context& vk_context, stb_image&& cpu_texture, simple_sampler& sampler) {
  bool mip_maps = sampler.is_using_linear_filtering();

  stb_image texture = std::move(cpu_texture);
  uint64_t size     = texture.byte_size();
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
  auto staging_buffer = vk_context.allocator->createBuffer(
      staging_buffer_info, staging_allocation_info
  );
  staging_buffer.getAllocation().copyFromMemory(texture.data(), 0, size);

  uint32_t mip_level_count = mip_maps ? static_cast<uint32_t>(std::floor(
                                            std::log2(std::max(texture.width(), texture.height()))
                                        )) + 1
                                      : 1;

  auto image = create_image(
      *vk_context.allocator,
      *vk_context.device,
      vk::Format::eR8G8B8A8Srgb,
      texture.width(),
      texture.height(),
      vk::ImageUsageFlagBits::eSampled,
      vk::ImageAspectFlagBits::eColor,
      true,
      mip_level_count
  );

  auto cmd = begin_transient_command_buffer(vk_context);
  transition_image_global_layout(
      image,
      cmd,
      vk::ImageLayout::eTransferDstOptimal,
      {},
      vk::AccessFlagBits2::eTransferRead,
      vk::PipelineStageFlagBits2::eTopOfPipe,
      vk::PipelineStageFlagBits2::eTransfer
  );

  cmd.copyBufferToImage(
      staging_buffer,
      image.image,
      vk::ImageLayout::eTransferDstOptimal,
      {vk::BufferImageCopy{
          .imageSubresource =
              {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .layerCount = 1},
          .imageExtent = {
              .width  = static_cast<uint32_t>(texture.width()),
              .height = static_cast<uint32_t>(texture.height()),
              .depth  = 1U
          }
      }}
  );
  if (mip_maps) {
    generate_mip_maps(vk_context, cmd, image);
  }
  //   transition_image_global_layout(
  //       image,
  //       cmd,
  //       vk::ImageLayout::eTransferDstOptimal,
  //       {},
  //       vk::AccessFlagBits2::eTransferRead,
  //       vk::PipelineStageFlagBits2::eTopOfPipe,
  //       vk::PipelineStageFlagBits2::eTransfer
  //   );
  submit_single_command_buffer(vk_context, std::move(cmd));
  image.sampler = &sampler.get();
  return std::move(image);
}
