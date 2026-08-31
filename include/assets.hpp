#pragma once

#include "image.hpp"
#include "layer.hpp"
#include "mesh.hpp"
#include "object_loader.hpp"  // IWYU pragma: keep
#include "resources/mesh_imports.hpp"
#include "primitive_meshes.hpp"  // IWYU pragma: keep
#include "resources/texture_imports.hpp"
#include "stb_image.hpp"
#include "vulkan_context.hpp"

void update_texture_descriptor(application_context* context) {
  console::get(consoles::assets)
      ->warn(
          "TO DO: move to a fixed size arena for textures and make it efficient to update "
          "descriptors"
      );
  std::vector<vk::DescriptorImageInfo> image_descriptors;
  for (auto& texture : context->resources.textures_) {
    vk::DescriptorImageInfo image_info{
        .sampler     = context->resources.sampler,
        .imageView   = texture.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };
    image_descriptors.push_back(image_info);
  }

  for (uint32_t i = 0; i < vulkan_config::frames_in_flight; i++) {
    vk::WriteDescriptorSet write_descriptor_set{
        .dstSet          = context->vulkan.descriptor_sets->at(i),
        .dstBinding      = 0,
        .dstArrayElement = 0,
        .descriptorCount = static_cast<uint32_t>(image_descriptors.size()),
        .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo      = image_descriptors.data()
    };
    context->vulkan.device->updateDescriptorSets(write_descriptor_set, {});
  }
}

class assets_layer final : public Ilayer {
 public:
  using Ilayer::Ilayer;
  bool init() noexcept final {
    try {
      vulkan_context vk_context = get_app_context()->vulkan;
      auto& resources           = get_app_context()->resources;

      auto properties = vk_context.physical_device->getProperties();
      vk::SamplerCreateInfo sampler_info{
          .magFilter               = vk::Filter::eNearest,
          .minFilter               = vk::Filter::eNearest,
          .mipmapMode              = vk::SamplerMipmapMode::eNearest,
          .addressModeU            = vk::SamplerAddressMode::eRepeat,
          .addressModeV            = vk::SamplerAddressMode::eRepeat,
          .addressModeW            = vk::SamplerAddressMode::eRepeat,
          .mipLodBias              = 0.0F,
          .anisotropyEnable        = vk::True,
          .maxAnisotropy           = properties.limits.maxSamplerAnisotropy,
          .compareEnable           = vk::False,
          .compareOp               = vk::CompareOp::eAlways,
          .minLod                  = 0.0F,
          .maxLod                  = 0.0F,
          .borderColor             = vk::BorderColor::eIntOpaqueBlack,
          .unnormalizedCoordinates = vk::False
      };
      resources.sampler = vk::raii::Sampler(*vk_context.device, sampler_info);

      // FALLBACK TEXTURE
      auto h = resources.textures_.push(std::move(load_texture_to_gpu(vk_context, stb_image())));

      load_object_and_materials(get_app_context(), meshes::sponza);
      load_object_and_materials(get_app_context(), meshes::tank);
      load_object_and_materials(get_app_context(), meshes::tyra);

      auto beer = resources.textures_.push(
          std::move(load_texture_to_gpu(vk_context, stb_image(textures::beer.texture_path)))
      );

      auto cube = resources.meshes_.push(create_cube({1.F, 0.F, 1.F}, 0.5F, beer));
      for (auto& mesh : resources.meshes_) {
        mesh.create_vertex_buffer(vk_context);
      }

      std::vector<stb_image> cpu_images;
      cpu_images.emplace_back(textures::beer.texture_path);
      for (auto& image : cpu_images) {
        resources.textures_.push(std::move(load_texture_to_gpu(vk_context, std::move(image))));
      }

      update_texture_descriptor(get_app_context());

    } catch (const std::exception& e) {
      console::get(consoles::assets)->error("Exception during assets initialisation: {}", e.what());
    } catch (...) {
      console::get(consoles::assets)->error("Unknown error during assets initialisation {}");
    }

    return true;
  }
  void update(double dt) noexcept final {}
  void cleanup() noexcept final {}

 private:
};