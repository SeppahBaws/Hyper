#pragma once
#include "GLFW/glfw3.h"
#include "Hyper/Renderer/Vulkan/VulkanDescriptors.h"

namespace Hyper
{
	struct RenderContext;

	class ImGuiWrapper
	{
	public:
		ImGuiWrapper(RenderContext* pRenderCtx, GLFWwindow* window, vk::Format colorFormat, vk::Format depthFormat);
		~ImGuiWrapper();

		void NewFrame();
		void Draw(vk::CommandBuffer cmd);

	private:
		RenderContext* m_pRenderCtx;

		std::unique_ptr<DescriptorPool> m_pDescriptorPool;
	};
	
}
