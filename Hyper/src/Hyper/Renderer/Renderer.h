#pragma once
#include <vulkan/vulkan.hpp>

#include "FlyCamera.h"
#include "Mesh.h"
#include "Model.h"
#include "RenderTarget.h"
#include "Hyper/Core/Subsystem.h"
#include "Hyper/Scene/Scene.h"
#include "ImGui/ImGuiWrapper.h"
#include "Vulkan/VulkanCommands.h"
#include "Vulkan/VulkanDescriptors.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanPipeline.h"
#include "Vulkan/VulkanSwapChain.h"
#include "Vulkan/VulkanRaytracer.h"

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
		std::unique_ptr<ImGuiWrapper> m_pImGuiWrapper;

		std::unique_ptr<VulkanRaytracer> m_pRayTracer;

		std::vector<FrameData> m_GeometryFrameDatas;
		std::unique_ptr<RenderTarget> m_pGeometryRenderTarget{};
		std::unique_ptr<DescriptorPool> m_pGeometryDescriptorPool{};
		std::unique_ptr<vk::DescriptorSetLayout> m_pGeometryGlobalSetLayout{};
		std::unique_ptr<VulkanShader> m_pGeometryShader;
		std::unique_ptr<VulkanPipeline> m_pGeometryPipeline;

		std::unique_ptr<DescriptorPool> m_pCompositeDescriptorPool{};
		vk::DescriptorSet m_CompositeDescriptorSet{};
		std::unique_ptr<vk::DescriptorSetLayout> m_pCompositeSetLayout{};
		std::unique_ptr<VulkanShader> m_pCompositeShader;
		std::unique_ptr<VulkanPipeline> m_pCompositePipeline;

		u32 m_FrameIdx = 0;
		u64 m_FrameNumber = 0;

		f32 m_Rot = 0;

		std::vector<vk::CommandBuffer> m_CommandBuffers;
		std::vector<vk::Semaphore> m_RenderFinishedSemaphores;
		std::vector<vk::Fence> m_InFlightFences;
		vk::RenderPass m_RenderPass;

		std::unique_ptr<FlyCamera> m_pCamera;

		std::unique_ptr<Scene> m_pScene;
	};
}
