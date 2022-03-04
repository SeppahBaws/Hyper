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
		VulkanSwapChain(const Window* pWindow, std::shared_ptr<RenderContext> pRenderCtx,
		                std::shared_ptr<VulkanDevice> pDevice, u32 width, u32 height);
		~VulkanSwapChain();
	
	private:
		std::shared_ptr<RenderContext> m_pRenderCtx;

		vk::SurfaceKHR m_Surface{};
		vk::SwapchainKHR m_SwapChain{};
		std::vector<vk::Image> m_Images{};
		std::vector<vk::ImageView> m_ImageViews{};
		std::vector<vk::Framebuffer> m_Framebuffers{};

		vk::Format m_ImageFormat;
		vk::ColorSpaceKHR m_ColorSpace;
		vk::Extent2D m_Extent;
	};
}
