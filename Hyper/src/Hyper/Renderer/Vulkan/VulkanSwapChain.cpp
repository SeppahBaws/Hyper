#include "HyperPCH.h"
#include "VulkanSwapChain.h"

#include <GLFW/glfw3.h>

#include "VulkanDebug.h"
#include "VulkanUtility.h"
#include "Hyper/Core/Window.h"
#include "Hyper/Debug/Profiler.h"
#include "Hyper/Renderer/RenderContext.h"

namespace Hyper
{
	VulkanSwapChain::VulkanSwapChain(const Window* pWindow, RenderContext* pRenderCtx, u32 width, u32 height)
		: m_pRenderCtx(pRenderCtx)
		, m_pWindow(pWindow)
	{
		CreateSwapChain(width, height);
	}

	VulkanSwapChain::~VulkanSwapChain()
	{
		DestroySwapChain();
	}

	void VulkanSwapChain::Present(const vk::Semaphore& waitSemaphore)
	{
		const vk::Result result = m_pRenderCtx->graphicsQueue.Present(
			{ waitSemaphore },
			{ m_ImageIdx },
			{ m_SwapChain });

		if (result != vk::Result::eSuccess)
		{
			HPR_VKLOG_ERROR("Failed to present - VkResult was {}", vk::to_string(result));
			return;
		}

		AcquireNextImage();
	}

	u32 VulkanSwapChain::AcquireNextImage()
	{
		HPR_PROFILE_SCOPE();

		m_SemaphoreIdx = (m_SemaphoreIdx + 1) % m_ImageAcquiredSemaphores.size();
		const vk::Semaphore signalSemaphore = m_ImageAcquiredSemaphores[m_SemaphoreIdx];

		const vk::Result result = m_pRenderCtx->device.acquireNextImageKHR(
			m_SwapChain,
			1'000'000'000, // 1 second in nanoseconds
			signalSemaphore,
			nullptr,
			&m_ImageIdx);

		if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
		{
			Resize(m_Width, m_Height);
			return AcquireNextImage();
		}

		VulkanUtils::VkCheck(result);

		return m_SemaphoreIdx;
	}

	void VulkanSwapChain::Resize(u32 width, u32 height)
	{
		// Check for a valid resolution
		m_PresentEnabled = width > 0 && height > 0;

		// window can be resized, causing the size to be (0, 0) - so just return
		if (!m_PresentEnabled)
		{
			return;
		}

		// Wait for all commands to be finished (in case there are any still running)
		m_pRenderCtx->graphicsQueue.queue.waitIdle();

		DestroySwapChain();
		CreateSwapChain(width, height);
	}

	std::array<vk::RenderingAttachmentInfo, 2> VulkanSwapChain::GetRenderingAttachments() const
	{
		std::array<vk::RenderingAttachmentInfo, 2> attachments{};

		attachments[0].imageView = GetImageView();
		attachments[0].imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		attachments[0].resolveMode = vk::ResolveModeFlagBits::eNone;
		attachments[0].loadOp = vk::AttachmentLoadOp::eClear;
		attachments[0].storeOp = vk::AttachmentStoreOp::eStore;
		attachments[0].clearValue = vk::ClearColorValue(std::array{ 0.0f, 0.0f, 0.0f, 1.0f });

		attachments[1].imageView = m_pDepthBuffer->GetImageView();
		attachments[1].imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
		attachments[1].resolveMode = vk::ResolveModeFlagBits::eNone;
		attachments[1].loadOp = vk::AttachmentLoadOp::eClear;
		attachments[1].storeOp = vk::AttachmentStoreOp::eStore;
		attachments[1].clearValue = vk::ClearDepthStencilValue(1.0f);

		return attachments;
	}

	void VulkanSwapChain::CreateSwapChain(u32 width, u32 height)
	{
		// Create surface.
		{
			VkSurfaceKHR surface;
			VulkanUtils::VkCheck(glfwCreateWindowSurface(m_pRenderCtx->instance, m_pWindow->GetHandle(), nullptr, &surface));
			m_Surface = surface;

			HPR_VKLOG_INFO("Created the surface");
		}

		// Get surface capabilities
		vk::SurfaceCapabilitiesKHR surfaceCapabilities = m_pRenderCtx->physicalDevice.getSurfaceCapabilitiesKHR(m_Surface);

		// Detect surface format and color space
		{
			const auto availableFormats = m_pRenderCtx->physicalDevice.getSurfaceFormatsKHR(m_Surface);
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

			m_pRenderCtx->imageFormat = m_ImageFormat;
		}

		m_Width = std::clamp(width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
		m_Height = std::clamp(height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
		m_pRenderCtx->imageExtent = vk::Extent2D{ m_Width, m_Height };

		// TODO: make this a parameter?
		const u32 imageCount = surfaceCapabilities.minImageCount + 1;
		m_pRenderCtx->imagesInFlight = imageCount;

		// Create swapchain
		{
			vk::SwapchainCreateInfoKHR createInfo = {};
			createInfo.surface = m_Surface;
			createInfo.minImageCount = imageCount;
			createInfo.imageFormat = m_ImageFormat;
			createInfo.imageColorSpace = m_ColorSpace;
			createInfo.imageExtent = m_pRenderCtx->imageExtent;
			createInfo.imageArrayLayers = 1;
			createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

			// TODO: multiple queues?
			createInfo.imageSharingMode = vk::SharingMode::eExclusive;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;

			createInfo.preTransform = surfaceCapabilities.currentTransform;
			createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
			createInfo.presentMode = vk::PresentModeKHR::eFifo; // TODO: allow for customization.
			createInfo.clipped = true;
			createInfo.oldSwapchain = nullptr;

			try
			{
				m_SwapChain = m_pRenderCtx->device.createSwapchainKHR(createInfo);
				VkDebug::SetObjectName(m_pRenderCtx->device, vk::ObjectType::eSwapchainKHR, m_SwapChain, "Swapchain");
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
				m_Images = m_pRenderCtx->device.getSwapchainImagesKHR(m_SwapChain);
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
				viewInfo.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

				vk::ImageView imageView;

				try
				{
					imageView = m_pRenderCtx->device.createImageView(viewInfo);
				}
				catch (vk::SystemError& e)
				{
					throw std::runtime_error("Failed to create image view: "s + e.what());
				}
				m_ImageViews.push_back(imageView);
			}

			HPR_VKLOG_INFO("Created swap chain image views ({})", m_ImageViews.size());
		}

		// Create the depth buffer
		{
			m_pDepthBuffer = std::make_unique<VulkanImage>(
				m_pRenderCtx, vk::Format::eD24UnormS8Uint, vk::ImageType::e2D, vk::ImageUsageFlagBits::eDepthStencilAttachment,
				vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, "Depth Buffer", m_Width, m_Height);

			HPR_VKLOG_INFO("Created depth buffer");
		}

		// Create image acquired semaphores
		{
			const vk::SemaphoreCreateInfo semaphoreInfo{};

			for (u32 i = 0; i < imageCount; i++)
			{
				m_ImageAcquiredSemaphores.emplace_back(m_pRenderCtx->device.createSemaphore(semaphoreInfo));
				VkDebug::SetObjectName(m_pRenderCtx->device, vk::ObjectType::eSemaphore, m_ImageAcquiredSemaphores[i], fmt::format("Swapchain semaphore {}", i));
			}
		}

		m_SemaphoreIdx = AcquireNextImage();
	}

	void VulkanSwapChain::DestroySwapChain()
	{
		for (const auto& semaphore : m_ImageAcquiredSemaphores)
		{
			m_pRenderCtx->device.destroySemaphore(semaphore);
		}
		m_ImageAcquiredSemaphores.clear();

		m_pDepthBuffer.reset();
		for (const auto& imageView : m_ImageViews)
		{
			m_pRenderCtx->device.destroyImageView(imageView);
		}
		m_ImageViews.clear();

		m_pRenderCtx->device.destroySwapchainKHR(m_SwapChain);
		m_pRenderCtx->instance.destroySurfaceKHR(m_Surface);
	}
}
