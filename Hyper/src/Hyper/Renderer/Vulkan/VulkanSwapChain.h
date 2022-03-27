#pragma once
#include <vulkan/vulkan.hpp>

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

		[[nodiscard]] vk::Image GetImage(size_t frameIdx) const { return m_Images[frameIdx]; }
		[[nodiscard]] vk::ImageView GetImageView(size_t frameIdx) const { return m_ImageViews[frameIdx]; }
		[[nodiscard]] vk::SwapchainKHR GetSwapchain() const { return m_SwapChain; }
		[[nodiscard]] u32 GetNumFrames() const { return static_cast<u32>(m_Images.size()); }

	private:
		RenderContext* m_pRenderCtx;

		vk::SurfaceKHR m_Surface{};
		vk::SwapchainKHR m_SwapChain{};
		std::vector<vk::Image> m_Images{};
		std::vector<vk::ImageView> m_ImageViews{};
		std::vector<vk::Framebuffer> m_Framebuffers{};

		vk::Format m_ImageFormat;
		vk::ColorSpaceKHR m_ColorSpace;
		// vk::Extent2D m_Extent;
	};
}
