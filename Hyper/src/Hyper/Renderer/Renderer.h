#pragma once
#include <vulkan/vulkan.hpp>

#include "FlyCamera.h"
#include "Mesh.h"
#include "Model.h"
#include "Hyper/Core/Subsystem.h"
#include "Vulkan/VulkanCommands.h"
#include "Vulkan/VulkanDescriptors.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanPipeline.h"
#include "Vulkan/VulkanSwapChain.h"

namespace Hyper
{
	struct ModelMatrixPushConst
	{
		glm::mat4 modelMatrix;
	};

	struct CameraData
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 viewProj;
	};

	struct FrameData
	{
		VulkanBuffer cameraBuffer;
		vk::DescriptorSet descriptor;
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
		std::unique_ptr<VulkanShader> m_pShader;
		std::unique_ptr<VulkanPipeline> m_pPipeline;

		std::unique_ptr<Model> m_pModel;

		u32 m_FrameIdx = 0;
		u64 m_FrameNumber = 0;

		f32 m_Rot = 0;

		std::vector<vk::CommandBuffer> m_CommandBuffers;
		std::vector<vk::Semaphore> m_RenderFinishedSemaphores;
		std::vector<vk::Fence> m_InFlightFences;
		vk::RenderPass m_RenderPass;

		std::unique_ptr<FlyCamera> m_pCamera;
		
		std::vector<FrameData> m_FrameDatas;
		std::unique_ptr<DescriptorSetLayout> m_pGlobalSetLayout{};
		std::unique_ptr<DescriptorPool> m_pDescriptorPool{};
	};
}
