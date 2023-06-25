#pragma once
#include <vulkan/vulkan.hpp>

#include "VulkanImage.h"

namespace Hyper
{
	struct RenderContext;
	class VulkanDevice;
	class Window;

	class VulkanSwapChain
	{
	public:
		VulkanSwapChain(const Window* pWindow, RenderContext* pRenderCtx, u32 width, u32 height);
		~VulkanSwapChain();

		[[nodiscard]] vk::Image GetImage() const { return m_Images[m_ImageIdx]; }
		[[nodiscard]] vk::ImageView GetImageView() const { return m_ImageViews[m_ImageIdx]; }
		[[nodiscard]] vk::SwapchainKHR GetSwapchain() const { return m_SwapChain; }
		[[nodiscard]] u32 GetNumFrames() const { return static_cast<u32>(m_Images.size()); }

		[[nodiscard]] u32 GetWidth() const { return m_Width; }
		[[nodiscard]] u32 GetHeight() const { return m_Height; }

		void Present(const vk::Semaphore& waitSemaphore);
		u32 AcquireNextImage();
		[[nodiscard]] VkSemaphore GetSemaphore() const { return m_ImageAcquiredSemaphores[m_SemaphoreIdx]; }
		bool IsPresentEnabled() const { return m_PresentEnabled; }

		void Resize(u32 width, u32 height);

		std::array<vk::RenderingAttachmentInfo, 2> GetRenderingAttachments() const;

	private:
		void CreateSwapChain(u32 width, u32 height);
		void DestroySwapChain();

	private:
		RenderContext* m_pRenderCtx;

		const Window* m_pWindow{};
		u32 m_Width{};
		u32 m_Height{};
		bool m_PresentEnabled{ true };

		u32 m_SemaphoreIdx{};
		u32 m_ImageIdx{};

		vk::SurfaceKHR m_Surface{};
		vk::SwapchainKHR m_SwapChain{};
		std::vector<vk::Image> m_Images{};
		std::vector<vk::ImageView> m_ImageViews{};

		std::vector<std::unique_ptr<VulkanImage>> m_pDepthBuffers{};

		vk::Format m_ImageColorFormat;
		vk::Format m_ImageDepthFormat;
		vk::ColorSpaceKHR m_ColorSpace;
		// vk::Extent2D m_Extent;

		std::vector<vk::Semaphore> m_ImageAcquiredSemaphores{};
	};
}
