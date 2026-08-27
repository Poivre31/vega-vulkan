#pragma once

#include "layer.hpp"
#include "mesh.hpp"
#include "object_loader.hpp"  // IWYU pragma: keep
#include "image.hpp"

class assets_layer final : public Ilayer {
 public:
  using Ilayer::Ilayer;
  bool init() noexcept final {
    try {
      vulkan_context context       = get_app_context()->vulkan_context;
      auto [sponza_mesh, textures] = load_object_and_materials(
          "resources/models/sponza/", "sponza.obj", 1. / 100
      );
      _meshes.emplace_back(sponza_mesh);
      // _meshes.emplace_back(create_cube({1.F, 0.F, 0.F}, 0.3F));
      for (auto& mesh : _meshes) {
        mesh.create_vertex_buffer(context);
      }

      auto properties = context.physical_device->getProperties();
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

      _samplers.emplace_back(*context.device, sampler_info);

      std::vector<stb_image> cpu_images = std::move(textures);
      // beer.load("resources/textures/beer.png");
      // cpu_images.emplace_back(std::move(beer));
      // beer.load("resources/textures/beer2.png");
      // cpu_images.emplace_back(std::move(beer));
      // beer.load("resources/textures/statue.jpg");
      // cpu_images.emplace_back(std::move(beer));
      // beer.load("resources/textures/viking_room.png");
      // cpu_images.emplace_back(std::move(beer));

      std::vector<vk::DescriptorImageInfo> image_descriptors;
      for (auto& image : cpu_images) {
        _textures.emplace_back(std::move(load_texture_to_gpu(context, std::move(image))));
        vk::DescriptorImageInfo imageInfo{
            .sampler     = _samplers.back(),
            .imageView   = _textures.back().view,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };
        image_descriptors.push_back(imageInfo);
      }

      for (uint32_t i = 0; i < vulkan_config::frames_in_flight; i++) {
        vk::WriteDescriptorSet write_descriptor_set{
            .dstSet          = context.descriptor_sets->at(i),
            .dstBinding      = 0,
            .dstArrayElement = 0,
            .descriptorCount = static_cast<uint32_t>(image_descriptors.size()),
            .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo      = image_descriptors.data()
        };
        context.device->updateDescriptorSets(write_descriptor_set, {});
      }

      get_app_context()->resources.meshes   = &_meshes;
      get_app_context()->resources.textures = &_textures;
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
  std::vector<mesh_3D> _meshes;
  std::vector<gpu_image> _textures;
  std::vector<vk::raii::Sampler> _samplers;
};