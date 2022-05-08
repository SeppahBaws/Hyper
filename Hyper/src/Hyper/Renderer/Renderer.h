#pragma once
#include <vulkan/vulkan.hpp>

#include "EditorCamera.h"
#include "TestMesh.h"
#include "Hyper/Core/Subsystem.h"
#include "Vulkan/VulkanCommands.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanPipeline.h"
#include "Vulkan/VulkanSwapChain.h"

namespace Hyper
{
	struct RenderMatrixPushConst
	{
		glm::mat4 renderMatrix;
	};
	
	class Renderer final : public Subsystem
	{
	public:
		Renderer(Context* pContext);
		~Renderer() override = default;

		bool OnInitialize() override;
		void OnTick(f32 dt) override;
		void OnShutdown() override;

	private:
		std::unique_ptr<RenderContext> m_pRenderContext;
		std::unique_ptr<VulkanDevice> m_pDevice;
		std::unique_ptr<VulkanCommandPool> m_pCommandPool;
		std::unique_ptr<VulkanSwapChain> m_pSwapChain;
		std::unique_ptr<VulkanPipeline> m_pPipeline;

		std::unique_ptr<TestMesh> m_pMesh;

		u32 m_FrameIdx = 0;
		u64 m_FrameNumber = 0;

		f32 m_Rot = 0;

		std::vector<vk::CommandBuffer> m_CommandBuffers;
		std::vector<vk::Semaphore> m_RenderFinishedSemaphores;
		std::vector<vk::Fence> m_InFlightFences;
		vk::RenderPass m_RenderPass;

		std::unique_ptr<EditorCamera> m_pCamera;
	};
}
