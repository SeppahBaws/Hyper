#pragma once
#include <vulkan/vulkan.hpp>

#include "FlyCamera.h"
#include "MaterialLibrary.h"
#include "Mesh.h"
#include "RenderContext.h"
#include "RenderTarget.h"
#include "ShaderLibrary.h"
#include "Hyper/Asset/TextureManager.h"
#include "Hyper/Core/Subsystem.h"
#include "ImGui/ImGuiWrapper.h"
#include "Vulkan/VulkanCommands.h"
#include "Vulkan/VulkanDescriptors.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanPipeline.h"
#include "Vulkan/VulkanRaytracer.h"
#include "Vulkan/VulkanSwapChain.h"

namespace Hyper
{
	class Scene;

	struct MeshPushConst
	{
		glm::mat4 modelMatrix;
		glm::uvec4 textureIndices;
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
		bool OnPostInitialize() override;
		void OnTick(f32 dt) override;
		void OnShutdown() override;

		void WaitIdle();

		[[nodiscard]] RenderContext* GetRenderContext() const { return m_pRenderContext.get(); }

	private:
		std::unique_ptr<RenderContext> m_pRenderContext;
		std::unique_ptr<VulkanDevice> m_pDevice;
		std::unique_ptr<VulkanCommandPool> m_pCommandPool;
		std::unique_ptr<VulkanSwapChain> m_pSwapChain;
		std::unique_ptr<ImGuiWrapper> m_pImGuiWrapper;
		std::unique_ptr<TextureManager> m_pTextureManager;
		std::unique_ptr<ShaderLibrary> m_pShaderLibrary;
		std::unique_ptr<MaterialLibrary> m_pMaterialLibrary;

		std::unique_ptr<VulkanRaytracer> m_pRayTracer;

		std::vector<FrameData> m_GeometryFrameDatas;
		std::unique_ptr<RenderTarget> m_pGeometryRenderTarget{};
		std::unique_ptr<DescriptorPool> m_pGeometryDescriptorPool{};
		VulkanShader* m_pGeometryShader;
		std::unique_ptr<VulkanGraphicsPipeline> m_pGeometryPipeline;

		std::unique_ptr<DescriptorPool> m_pCompositeDescriptorPool{};
		vk::DescriptorSet m_CompositeDescriptorSet{};
		VulkanShader* m_pCompositeShader;
		std::unique_ptr<VulkanGraphicsPipeline> m_pCompositePipeline;

		u32 m_FrameIdx = 0;

		std::vector<vk::CommandBuffer> m_CommandBuffers;
		std::vector<vk::Semaphore> m_RenderFinishedSemaphores;
		std::vector<vk::Fence> m_InFlightFences;
		vk::RenderPass m_RenderPass;

		std::unique_ptr<FlyCamera> m_pCamera;

		Scene* m_pScene{};
	};
}
