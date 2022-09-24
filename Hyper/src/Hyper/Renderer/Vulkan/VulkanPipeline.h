#pragma once
#include <vulkan/vulkan.hpp>

#include "VulkanShader.h"

namespace Hyper
{
	struct RenderContext;
	
	struct GraphicsPipelineSpecification
	{
		std::string debugName;
		VulkanShader* pShader;
		vk::PolygonMode polygonMode;
		vk::CullModeFlagBits cullMode;
		vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
		bool depthTest;
		bool depthWrite;
		vk::CompareOp compareOp;
		vk::Viewport viewport;
		vk::Rect2D scissor;
		bool blendEnable;
		std::vector<vk::Format> colorFormats{};
		vk::Format depthStencilFormat;

		std::vector<vk::DynamicState> dynamicStates;
		vk::PipelineCreateFlags flags;
	};

	class VulkanGraphicsPipeline
	{
	public:
		VulkanGraphicsPipeline(RenderContext* pRenderCtx, const GraphicsPipelineSpecification& spec);
		~VulkanGraphicsPipeline();

		VulkanGraphicsPipeline(const VulkanGraphicsPipeline& other) = delete;
		VulkanGraphicsPipeline& operator=(const VulkanGraphicsPipeline& other) = delete;
		VulkanGraphicsPipeline(VulkanGraphicsPipeline&& other) noexcept;
		VulkanGraphicsPipeline& operator=(VulkanGraphicsPipeline&& other) noexcept;

		void Recreate();

		[[nodiscard]] const vk::Pipeline& GetPipeline() const { return m_Pipeline; }
		[[nodiscard]] const vk::PipelineCache& GetCache() const { return m_Cache; }
		[[nodiscard]] const vk::PipelineLayout& GetLayout() const { return m_Layout; }

	private:
		void Create();
		void Destroy();

	private:
		RenderContext* m_pRenderCtx;
		GraphicsPipelineSpecification m_Specification;

		vk::Pipeline m_Pipeline;
		vk::PipelineCache m_Cache;
		vk::PipelineLayout m_Layout;
	};


	struct RayTracingPipelineSpecification
	{
		std::string debugName;
		VulkanShader* pShader;
		std::vector<vk::DescriptorSetLayout> layouts;
		std::vector<vk::PushConstantRange> pushConstants;
		std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroupCreateInfos;
		u32 rayRecursionDepth;

		std::vector<vk::DynamicState> dynamicStates;
		vk::PipelineCreateFlags flags;
	};

	class VulkanRayTracingPipeline
	{
	public:
		VulkanRayTracingPipeline(RenderContext* pRenderCtx, const RayTracingPipelineSpecification& spec);
		~VulkanRayTracingPipeline();

		VulkanRayTracingPipeline(const VulkanRayTracingPipeline& other) = delete;
		VulkanRayTracingPipeline& operator=(const VulkanRayTracingPipeline& other) = delete;
		VulkanRayTracingPipeline(VulkanRayTracingPipeline&& other) noexcept;
		VulkanRayTracingPipeline& operator=(VulkanRayTracingPipeline&& other) noexcept;

		void Recreate();

		[[nodiscard]] const vk::Pipeline& GetPipeline() const { return m_Pipeline; }
		[[nodiscard]] const vk::PipelineCache& GetCache() const { return m_Cache; }
		[[nodiscard]] const vk::PipelineLayout& GetLayout() const { return m_Layout; }

	private:
		void Create();
		void Destroy();

	private:
		RenderContext* m_pRenderCtx;
		RayTracingPipelineSpecification m_Specification;

		vk::Pipeline m_Pipeline;
		vk::PipelineCache m_Cache;
		vk::PipelineLayout m_Layout;
	};
}
