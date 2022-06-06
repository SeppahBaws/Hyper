﻿#pragma once
#include "Vulkan/VulkanImage.h"

namespace Hyper
{
	class Texture
	{
	public:
		Texture(RenderContext* pRenderCtx, const std::filesystem::path& filePath);
		~Texture();

		Texture(Texture&& other) noexcept;
		Texture& operator=(Texture&& other) noexcept;
		// Disable copying
		Texture(const Texture& other) = delete;
		Texture& operator=(const Texture& other) = delete;

		[[nodiscard]] VulkanImage* GetImage() const { return m_pImage.get(); }
		[[nodiscard]] const vk::Sampler& GetSampler() const { return m_Sampler; }
		[[nodiscard]] vk::DescriptorImageInfo GetDescriptorImageInfo() const;

	private:
		RenderContext* m_pRenderCtx{};

		std::filesystem::path m_FilePath;
		std::unique_ptr<VulkanImage> m_pImage;
		vk::Sampler m_Sampler;
	};
}