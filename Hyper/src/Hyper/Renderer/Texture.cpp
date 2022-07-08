#include "HyperPCH.h"
#include "Texture.h"

#include "RenderContext.h"
#include "stb_image.h"
#include "Vulkan/VulkanBuffer.h"
#include "Vulkan/VulkanDebug.h"
#include "Vulkan/VulkanUtility.h"

namespace Hyper
{
	Texture::Texture(RenderContext* pRenderCtx, const std::filesystem::path& filePath)
		: m_pRenderCtx(pRenderCtx)
		, m_FilePath(filePath)
	{
		// Check if image exists.
		if (!std::filesystem::exists(filePath))
		{
			HPR_CORE_LOG_ERROR("Image '{}' doesn't exist!", filePath.string());
			throw std::runtime_error(fmt::format("Image '{}' doesn't exist!", filePath.string()));
		}

		stbi_uc* pixels;
		i32 width, height, nrChannels;

		// Load image pixels.
		{
			stbi_set_flip_vertically_on_load(true);
			pixels = stbi_load(filePath.string().c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);

			if (!pixels)
			{
				HPR_CORE_LOG_ERROR("Failed to load image file '{}'", filePath.string());
				return;
			}
		}

		// Copy pixels to a staging buffer.
		{
			// TODO: hard-coded to 4 channels per pixel, probably want to tweak this at some point
			const vk::DeviceSize imageSize = width * height * 4;
			VulkanBuffer stagingBuffer{ m_pRenderCtx, imageSize, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY, fmt::format("'{}' staging buffer", filePath.string()) };
			stagingBuffer.SetData(pixels, imageSize);
			stbi_image_free(pixels);

			// Create image
			m_pImage = std::make_unique<VulkanImage>(m_pRenderCtx, vk::Format::eR8G8B8A8Srgb, vk::ImageType::e2D,
				vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst, vk::ImageAspectFlagBits::eColor, m_FilePath.string(), static_cast<u32>(width),
				static_cast<u32>(height));

			// Copy staging buffer to the image.
			vk::CommandBuffer cmd = m_pRenderCtx->commandPool->GetCommandBuffer();
			VulkanCommandBuffer::Begin(cmd, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

			m_pImage->TransitionLayout(cmd, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer);
			m_pImage->CopyFrom(cmd, stagingBuffer);
			m_pImage->TransitionLayout(cmd, vk::AccessFlagBits::eShaderRead, vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits::eFragmentShader);

			VulkanCommandBuffer::End(cmd);

			// TODO: Use a fence instead of doing a vkQueueWaitIdle
			m_pRenderCtx->graphicsQueue.Submit({}, {}, {}, cmd, nullptr);
			m_pRenderCtx->graphicsQueue.WaitIdle();
			m_pRenderCtx->commandPool->FreeCommandBuffer(cmd);
		}

		// Create sampler
		{
			vk::SamplerCreateInfo samplerInfo = {};
			samplerInfo.minFilter = vk::Filter::eLinear;
			samplerInfo.magFilter = vk::Filter::eLinear;
			samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
			samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
			samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;

			m_Sampler = VulkanUtils::Check(m_pRenderCtx->device.createSampler(samplerInfo));
		}
	}

	Texture::~Texture()
	{
		if (m_Sampler)
			m_pRenderCtx->device.destroySampler(m_Sampler);

		m_pImage.reset();
	}

	Texture::Texture(Texture&& other) noexcept: m_pRenderCtx(other.m_pRenderCtx),
		m_pImage(std::move(other.m_pImage)),
		m_Sampler(std::move(other.m_Sampler))
	{
		other.m_Sampler = nullptr;
	}

	Texture& Texture::operator=(Texture&& other) noexcept
	{
		if (this == &other)
			return *this;

		m_pRenderCtx = other.m_pRenderCtx;
		m_pImage = std::move(other.m_pImage);
		m_Sampler = std::move(other.m_Sampler);

		other.m_Sampler = nullptr;
		
		return *this;
	}

	vk::DescriptorImageInfo Texture::GetDescriptorImageInfo() const
	{
		vk::DescriptorImageInfo imageInfo = {};
		imageInfo.sampler = m_Sampler;
		imageInfo.imageLayout = m_pImage->GetImageLayout();
		imageInfo.imageView = m_pImage->GetImageView();

		return imageInfo;
	}
}
