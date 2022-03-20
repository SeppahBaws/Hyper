#include "HyperPCH.h"
#include "VulkanSwapChain.h"

#include <GLFW/glfw3.h>

#include "VulkanUtility.h"
#include "Hyper/Core/Window.h"
#include "Hyper/Renderer/RenderContext.h"

namespace Hyper
{
	VulkanSwapChain::VulkanSwapChain(const Window* pWindow, RenderContext* pRenderCtx, u32 width, u32 height)
		: m_pRenderCtx(pRenderCtx)
	{
		// Create surface.
		{
			VkSurfaceKHR surface;
			VulkanUtils::VkCheck(glfwCreateWindowSurface(pRenderCtx->instance, pWindow->GetHandle(), nullptr, &surface));
			m_Surface = surface;

			HPR_VKLOG_INFO("Created the surface");
		}

		// Get surface capabilities
		vk::SurfaceCapabilitiesKHR surfaceCapabilities = pRenderCtx->physicalDevice.getSurfaceCapabilitiesKHR(m_Surface);

		// Detect surface format and color space
		{
			const auto availableFormats = pRenderCtx->physicalDevice.getSurfaceFormatsKHR(m_Surface);
			for (const auto& availableFormat : availableFormats)
			{
				if (availableFormat.format == vk::Format::eR8G8B8A8Unorm &&
					availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
				{
					m_ImageFormat = availableFormat.format;
					m_ColorSpace = availableFormat.colorSpace;
				}
			}

			m_ImageFormat = availableFormats[0].format;
			m_ColorSpace = availableFormats[0].colorSpace;

			pRenderCtx->imageFormat = m_ImageFormat;
		}

		width = std::max(surfaceCapabilities.maxImageExtent.width, std::min(surfaceCapabilities.minImageExtent.width, width));
		height = std::max(surfaceCapabilities.maxImageExtent.height, std::min(surfaceCapabilities.minImageExtent.height, height));
		vk::Extent2D extent = { width, height };
		pRenderCtx->imageExtent = extent;

		// TODO: make this a parameter?
		const u32 imageCount = surfaceCapabilities.minImageCount + 1;

		// Create swapchain
		{
			vk::SwapchainCreateInfoKHR createInfo = {};
			createInfo.surface = m_Surface;
			createInfo.minImageCount = imageCount;
			createInfo.imageFormat = m_ImageFormat;
			createInfo.imageColorSpace = m_ColorSpace;
			createInfo.imageExtent = extent;
			createInfo.imageArrayLayers = 1;
			createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

			// TODO: multiple queues?
			createInfo.imageSharingMode = vk::SharingMode::eExclusive;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;

			createInfo.preTransform = surfaceCapabilities.currentTransform;
			createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
			createInfo.presentMode = vk::PresentModeKHR::eMailbox; // TODO: allow for customization.
			createInfo.clipped = true;
			createInfo.oldSwapchain = nullptr;

			try
			{
				m_SwapChain = pRenderCtx->device.createSwapchainKHR(createInfo);
			}
			catch (vk::SystemError& e)
			{
				throw std::runtime_error("Failed to create the swapchain: "s + e.what());
			}

			HPR_VKLOG_INFO("Created the swap chain");
		}

		// Get swap chain images and create image views
		{
			// Get swapchain images
			try
			{
				m_Images = pRenderCtx->device.getSwapchainImagesKHR(m_SwapChain);
			}
			catch (vk::SystemError& e)
			{
				throw std::runtime_error("Failed to acquire the swapchain images: "s + e.what());
			}

			HPR_VKLOG_INFO("Acquired swap chain images ({})", m_Images.size());

			// Create image views
			for (const auto& image : m_Images)
			{
				vk::ImageViewCreateInfo viewInfo{};
				viewInfo.image = image;
				viewInfo.viewType = vk::ImageViewType::e2D;
				viewInfo.format = m_ImageFormat;
				viewInfo.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

				vk::ImageView imageView;

				try
				{
					imageView = pRenderCtx->device.createImageView(viewInfo);
				}
				catch (vk::SystemError& e)
				{
					throw std::runtime_error("Failed to create image view: "s + e.what());
				}
				m_ImageViews.push_back(imageView);

			}
			HPR_VKLOG_INFO("Created swap chain image views ({})", m_ImageViews.size());
		}
	}

	VulkanSwapChain::~VulkanSwapChain()
	{
		for (const auto& imageView : m_ImageViews)
		{
			m_pRenderCtx->device.destroyImageView(imageView);
		}

		m_pRenderCtx->device.destroySwapchainKHR(m_SwapChain);
		m_pRenderCtx->instance.destroySurfaceKHR(m_Surface);
	}
}
