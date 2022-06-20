#pragma once
#include "GLFW/glfw3.h"
#include "Hyper/Renderer/RenderContext.h"
#include "Hyper/Renderer/Vulkan/VulkanDescriptors.h"

namespace Hyper
{
	class ImGuiWrapper
	{
	public:
		ImGuiWrapper(RenderContext* pRenderCtx, GLFWwindow* window, vk::Format colorFormat);
		~ImGuiWrapper();

		void NewFrame();
		void Draw(vk::CommandBuffer cmd);

	private:
		RenderContext* m_pRenderCtx;

		std::unique_ptr<DescriptorPool> m_pDescriptorPool;
	};
	
}
