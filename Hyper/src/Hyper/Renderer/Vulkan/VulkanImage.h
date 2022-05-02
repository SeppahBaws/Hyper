#pragma once
#include <vulkan/vulkan.hpp>

#include "VulkanDevice.h"

namespace Hyper
{
	class VulkanImage
	{
	public:
		VulkanImage(RenderContext* pRenderCtx, vk::Format format, vk::ImageType type, vk::ImageUsageFlags usage, vk::ImageAspectFlags aspectFlags,
		            const std::string& debugName, u32 width, u32 height, u32 depth = 1);
		~VulkanImage();

		[[nodiscard]] vk::Image GetImage() const { return m_Image; }
		[[nodiscard]] vk::ImageView GetImageView() const { return m_ImageView; }

		void Resize(u32 width, u32 height, u32 depth = 1);

	private:
		void CreateImageAndView();
		void DestroyImageAndView();

	private:
		RenderContext* m_pRenderCtx;

		vk::Image m_Image;
		vk::ImageView m_ImageView;
		VmaAllocation m_Allocation{};

		vk::Format m_Format{};
		vk::ImageType m_Type{};
		vk::ImageUsageFlags m_Usage;
		vk::ImageAspectFlags m_AspectFlags;
		u32 m_Width{}, m_Height{}, m_Depth{};
		std::string m_DebugName;
	};
}
