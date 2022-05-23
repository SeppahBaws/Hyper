﻿#pragma once
#include <vulkan/vulkan.hpp>

#include "VulkanShader.h"
#include "Hyper/Renderer/RenderContext.h"

namespace Hyper
{
	class VulkanPipeline
	{
	public:
		VulkanPipeline(RenderContext* pRenderCtx, const vk::Pipeline& pipeline, const vk::PipelineCache& cache, const vk::PipelineLayout& layout);
		~VulkanPipeline();
		VulkanPipeline(const VulkanPipeline& other) = delete;
		VulkanPipeline(VulkanPipeline&& other) noexcept;
		VulkanPipeline& operator=(VulkanPipeline&& other) noexcept;

		[[nodiscard]] const vk::Pipeline& GetPipeline() const { return m_Pipeline; }
		[[nodiscard]] const vk::PipelineCache& GetCache() const { return m_Cache; }
		[[nodiscard]] const vk::PipelineLayout& GetLayout() const { return m_Layout; }

	private:
		RenderContext* m_pRenderCtx;
		vk::Pipeline m_Pipeline;
		vk::PipelineCache m_Cache;
		vk::PipelineLayout m_Layout;
	};

	class PipelineBuilder
	{
	public:
		PipelineBuilder(RenderContext* pRenderCtx);

		void SetShader(VulkanShader* pShader);
		void SetInputAssembly(vk::PrimitiveTopology topology, bool primitiveRestartEnable);
		void SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth);
		void SetScissor(const vk::Offset2D& offset, const vk::Extent2D& extent);
		void SetRasterizer(vk::PolygonMode polygonMode, vk::CullModeFlagBits cullMode, vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise);
		void SetMultisampling();
		void SetDepthStencil(bool depthTest, bool depthWrite, vk::CompareOp compareOp);
		void SetColorBlend(bool blendEnable, vk::BlendOp colorBlendOp, vk::BlendOp alphaBlendOp, bool logicOpEnable, vk::LogicOp logicOp);
		void SetDescriptorSetLayout(const std::vector<vk::DescriptorSetLayout>& layouts, const std::vector<vk::PushConstantRange>& pushConstants);
		void SetDynamicStates(const std::vector<vk::DynamicState>& dynamicStates);

		VulkanPipeline BuildGraphics();
		// VulkanPipeline BuildCompute();

	private:
		RenderContext* m_pRenderCtx;

		VulkanShader* m_pShader{};
		vk::PipelineInputAssemblyStateCreateInfo m_InputAssembly{};
		vk::Viewport m_Viewport{};
		vk::Rect2D m_Scissor{};
		vk::PipelineRasterizationStateCreateInfo m_Rasterizer{};
		vk::PipelineMultisampleStateCreateInfo m_Multisampling{};
		vk::PipelineDepthStencilStateCreateInfo m_DepthStencil{};
		vk::PipelineColorBlendAttachmentState m_ColorBlendAttachment{};
		vk::PipelineColorBlendStateCreateInfo m_ColorBlending{};
		vk::PipelineLayoutCreateInfo m_PipelineLayoutInfo{};
		std::vector<vk::DynamicState> m_DynamicStates{};
	};
}
