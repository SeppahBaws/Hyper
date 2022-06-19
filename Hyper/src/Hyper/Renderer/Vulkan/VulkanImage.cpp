#include "HyperPCH.h"
#include "VulkanImage.h"

#include "VulkanDebug.h"
#include "VulkanUtility.h"
#include "Hyper/Debug/Profiler.h"
#include "Hyper/Renderer/RenderContext.h"

namespace Hyper
{
	VulkanImage::VulkanImage(RenderContext* pRenderCtx, vk::Format format, vk::ImageType type, vk::ImageUsageFlags usage, vk::ImageAspectFlags aspectFlags,
		const std::string& debugName, u32 width, u32 height, u32 depth)
		: m_pRenderCtx(pRenderCtx)
		, m_Format(format)
		, m_Type(type)
		, m_Usage(usage)
		, m_AspectFlags(aspectFlags)
		, m_Width(width)
		, m_Height(height)
		, m_Depth(depth)
		, m_DebugName(debugName)
	{
		CreateImageAndView();
	}

	VulkanImage::~VulkanImage()
	{
		DestroyImageAndView();
	}

	void VulkanImage::Resize(u32 width, u32 height, u32 depth)
	{
		// Make sure that the sizes have *actually* changed
		if (m_Width == width && m_Height == height && m_Depth == depth)
			return;

		m_Width = width;
		m_Height = height;
		m_Depth = depth;

		HPR_VKLOG_INFO("Resizing '{}' with dimensions: ({}, {}, {})", m_DebugName, m_Width, m_Height, m_Depth);

		DestroyImageAndView();
		CreateImageAndView();
	}

	void VulkanImage::TransitionLayout(vk::CommandBuffer cmd, vk::AccessFlags newAccessFlags, vk::ImageLayout newLayout, vk::PipelineStageFlags newStageFlags)
	{
		vk::ImageMemoryBarrier barrier = {};
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.srcAccessMask = m_AccessFlags;
		barrier.dstAccessMask = newAccessFlags;
		barrier.oldLayout = m_Layout;
		barrier.newLayout = newLayout;
		barrier.image = m_Image;
		barrier.subresourceRange = vk::ImageSubresourceRange{
			m_AspectFlags,
			0, 1,
			0, 1
		};

		cmd.pipelineBarrier(m_PipelineStageFlags, newStageFlags, {}, {}, {}, { barrier });

		m_Layout = newLayout;
		m_AccessFlags = newAccessFlags;
		m_PipelineStageFlags = newStageFlags;
	}

	void VulkanImage::CopyFrom(vk::CommandBuffer cmd, const VulkanBuffer& srcBuffer)
	{
		vk::BufferImageCopy copyRegion = {};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;
		copyRegion.imageSubresource.aspectMask = m_AspectFlags;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageExtent = vk::Extent3D{
			m_Width,
			m_Height,
			1
		};

		cmd.copyBufferToImage(srcBuffer.GetBuffer(), m_Image, m_Layout, copyRegion);
	}

	void VulkanImage::CreateImageAndView()
	{
		HPR_PROFILE_SCOPE("VulkanImage::Create");

		m_Layout = vk::ImageLayout::eUndefined;
		m_AccessFlags = {};
		m_PipelineStageFlags = vk::PipelineStageFlagBits::eTopOfPipe;

		vk::ImageCreateInfo imageInfo{};
		imageInfo.imageType = m_Type;
		imageInfo.format = m_Format;
		imageInfo.extent = vk::Extent3D{ m_Width, m_Height, m_Depth };
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = vk::SampleCountFlagBits::e1;
		imageInfo.tiling = vk::ImageTiling::eOptimal;
		imageInfo.usage = m_Usage;

		VmaAllocationCreateInfo alloc{};
		alloc.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		alloc.requiredFlags = static_cast<VkMemoryPropertyFlags>(vk::MemoryPropertyFlagBits::eDeviceLocal);

		VulkanUtils::Check(
			vmaCreateImage(m_pRenderCtx->allocator, reinterpret_cast<VkImageCreateInfo*>(&imageInfo), &alloc, reinterpret_cast<VkImage*>(&m_Image), &m_Allocation,
				nullptr)
		);

		vk::ImageViewCreateInfo imageViewInfo{};
		imageViewInfo.image = m_Image;
		switch (m_Type)
		{
		case vk::ImageType::e1D:
			imageViewInfo.viewType = vk::ImageViewType::e1D;
			break;
		case vk::ImageType::e2D:
			imageViewInfo.viewType = vk::ImageViewType::e2D;
			break;
		case vk::ImageType::e3D:
			imageViewInfo.viewType = vk::ImageViewType::e3D;
			break;
		}

		imageViewInfo.format = m_Format;
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.levelCount = 1;
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.subresourceRange.layerCount = 1;
		imageViewInfo.subresourceRange.aspectMask = m_AspectFlags;
		
		m_ImageView = VulkanUtils::Check(m_pRenderCtx->device.createImageView(imageViewInfo));

		VkDebug::SetObjectName(m_pRenderCtx->device, vk::ObjectType::eImage, m_Image, m_DebugName);
		VkDebug::SetObjectName(m_pRenderCtx->device, vk::ObjectType::eImageView, m_ImageView, fmt::format("{} image view", m_DebugName));
	}

	void VulkanImage::DestroyImageAndView()
	{
		HPR_PROFILE_SCOPE("VulkanImage::Destroy");

		m_pRenderCtx->device.destroyImageView(m_ImageView);
		vmaDestroyImage(m_pRenderCtx->allocator, m_Image, m_Allocation);
	}
}
