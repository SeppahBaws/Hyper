#pragma once
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "Hyper/Renderer/Vulkan/VulkanQueue.h"
#include "Hyper/Renderer/Vulkan/VulkanCommands.h"
#include "Vulkan/VulkanDescriptors.h"

namespace Hyper
{
	class TextureManager;
	class ShaderLibrary;
	class MaterialLibrary;

	struct RenderContext
	{
		vk::Instance instance;
		vk::PhysicalDevice physicalDevice;
		vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties;
		vk::Device device;
		VulkanQueue graphicsQueue;
		VulkanCommandPool* commandPool;
		VmaAllocator allocator;
		u32 imagesInFlight;

		u64 frameNumber = 0;

		static constexpr u32 maxBindlessResources = 1024;
		static constexpr u32 bindlessTextureBinding = 10;
		std::unique_ptr<DescriptorPool> bindlessDescriptorPool;
		vk::DescriptorSetLayout bindlessDescriptorLayout;
		vk::DescriptorSet bindlessDescriptorSet;
		std::unique_ptr<DescriptorWriter> bindlessDescriptorWriter;

		vk::Sampler defaultSampler;

		vk::Format imageColorFormat;
		vk::Format imageDepthFormat;
		vk::Extent2D imageExtent;

		TextureManager* pTextureManager;

		ShaderLibrary* pShaderLibrary;
		MaterialLibrary* pMaterialLibrary;

		bool drawImGui{ true };
	};
}
