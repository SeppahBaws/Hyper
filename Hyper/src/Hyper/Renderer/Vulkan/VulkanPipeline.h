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

#if 0
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

		PipelineBuilder& SetDebugName(const std::string& debugName);
		PipelineBuilder& SetShader(VulkanShader* pShader);
		PipelineBuilder& SetInputAssembly(vk::PrimitiveTopology topology, bool primitiveRestartEnable);
		PipelineBuilder& SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth);
		PipelineBuilder& SetScissor(const vk::Offset2D& offset, const vk::Extent2D& extent);
		PipelineBuilder& SetRasterizer(vk::PolygonMode polygonMode, vk::CullModeFlagBits cullMode, vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise);
		PipelineBuilder& SetMultisampling();
		PipelineBuilder& SetDepthStencil(bool depthTest, bool depthWrite, vk::CompareOp compareOp);
		PipelineBuilder& SetColorBlend(bool blendEnable, vk::BlendOp colorBlendOp, vk::BlendOp alphaBlendOp, bool logicOpEnable, vk::LogicOp logicOp);
		PipelineBuilder& SetDescriptorSetLayout(const std::vector<vk::DescriptorSetLayout>& layouts, const std::vector<vk::PushConstantRange>& pushConstants);
		PipelineBuilder& SetDynamicStates(const std::vector<vk::DynamicState>& dynamicStates);
		PipelineBuilder& SetFormats(const std::vector<vk::Format>& colorFormats, vk::Format depthStencilFormat);

		// Ray-tracing specific
		PipelineBuilder& SetRayTracingShaderGroups(const std::vector<vk::RayTracingShaderGroupCreateInfoKHR>& shaderGroupCreateInfos);
		PipelineBuilder& SetMaxRayRecursionDepth(u32 maxRayRecursionDepth);

		VulkanPipeline BuildGraphics(vk::PipelineCache pipelineCache = nullptr, vk::PipelineCreateFlags flags = {});
		VulkanPipeline BuildRayTracing(vk::PipelineCache pipelineCache = nullptr, vk::PipelineCreateFlags flags = {});
		// VulkanPipeline BuildCompute();

	private:
		RenderContext* m_pRenderCtx;

		std::string m_DebugName{};
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
		std::vector<vk::Format> m_ColorFormats{};
		vk::Format m_DepthStencilFormat;

		std::vector<vk::RayTracingShaderGroupCreateInfoKHR> m_RayTracingShaderGroups{};
		u32 m_MaxRayRecursionDepth{};
	};
#endif
}
