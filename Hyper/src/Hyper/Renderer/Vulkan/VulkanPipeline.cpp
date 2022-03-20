#include "HyperPCH.h"
#include "VulkanPipeline.h"

#include "Vertex.h"
#include "VulkanShader.h"

namespace Hyper
{
	VulkanPipeline::VulkanPipeline(RenderContext* pRenderCtx, const vk::Pipeline& pipeline,
		const vk::PipelineCache& cache, const vk::PipelineLayout& layout)
		: m_pRenderCtx(pRenderCtx)
		, m_Pipeline(pipeline)
		, m_Cache(cache)
		, m_Layout(layout)
	{
	}

	VulkanPipeline::~VulkanPipeline()
	{
		m_pRenderCtx->device.destroyPipeline(m_Pipeline);
		m_pRenderCtx->device.destroyPipelineCache(m_Cache);
		m_pRenderCtx->device.destroyPipelineLayout(m_Layout);
	}

	PipelineBuilder::PipelineBuilder(RenderContext* pRenderCtx)
		: m_pRenderCtx(pRenderCtx)
	{
	}

	void PipelineBuilder::SetShader(VulkanShader* pShader)
	{
		m_pShader = pShader;
	}

	void PipelineBuilder::SetInputAssembly(vk::PrimitiveTopology topology, bool primitiveRestartEnable)
	{
		m_InputAssembly = vk::PipelineInputAssemblyStateCreateInfo{};
		m_InputAssembly.topology = topology;
		m_InputAssembly.primitiveRestartEnable = primitiveRestartEnable;
	}

	void PipelineBuilder::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
	{
		m_Viewport = vk::Viewport{};
		m_Viewport.x = x;
		m_Viewport.y = y;
		m_Viewport.width = width;
		m_Viewport.height = height;
		m_Viewport.minDepth = minDepth;
		m_Viewport.maxDepth = maxDepth;
	}

	void PipelineBuilder::SetScissor(const vk::Offset2D& offset, const vk::Extent2D& extent)
	{
		m_Scissor = vk::Rect2D{};
		m_Scissor.offset = offset;
		m_Scissor.extent = extent;
	}

	void PipelineBuilder::SetRasterizer(vk::PolygonMode polygonMode, vk::CullModeFlagBits cullMode)
	{
		m_Rasterizer = vk::PipelineRasterizationStateCreateInfo{};
		m_Rasterizer.depthClampEnable = false;
		m_Rasterizer.rasterizerDiscardEnable = false;
		m_Rasterizer.polygonMode = polygonMode;
		m_Rasterizer.lineWidth = 1.0f;
		m_Rasterizer.cullMode = cullMode;
		m_Rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
		m_Rasterizer.depthBiasEnable = false;
		m_Rasterizer.depthBiasConstantFactor = 0.0f;
		m_Rasterizer.depthBiasClamp = 0.0f;
		m_Rasterizer.depthBiasSlopeFactor = 0.0f;
	}

	void PipelineBuilder::SetMultisampling()
	{
		m_Multisampling = vk::PipelineMultisampleStateCreateInfo{};
		m_Multisampling.sampleShadingEnable = false;
		m_Multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
		m_Multisampling.minSampleShading = 1.0f;
		m_Multisampling.pSampleMask = nullptr;
		m_Multisampling.alphaToCoverageEnable = false;
		m_Multisampling.alphaToOneEnable = false;
	}

	void PipelineBuilder::SetDepthStencil(bool depthTest, bool depthWrite, vk::CompareOp compareOp)
	{
		m_DepthStencil = vk::PipelineDepthStencilStateCreateInfo{};
		m_DepthStencil.depthTestEnable = depthTest;
		m_DepthStencil.depthWriteEnable = depthWrite;
		m_DepthStencil.depthCompareOp = depthTest ? compareOp : vk::CompareOp::eAlways;
		m_DepthStencil.depthBoundsTestEnable = false;
		m_DepthStencil.minDepthBounds = 0.0f;
		m_DepthStencil.maxDepthBounds = 1.0f;
		m_DepthStencil.stencilTestEnable = false;
	}

	void PipelineBuilder::SetColorBlend(bool blendEnable, vk::BlendOp colorBlendOp, vk::BlendOp alphaBlendOp,
		bool logicOpEnable, vk::LogicOp logicOp)
	{
		m_ColorBlendAttachment = vk::PipelineColorBlendAttachmentState{};
		m_ColorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		m_ColorBlendAttachment.blendEnable = blendEnable;
		m_ColorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
		m_ColorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		m_ColorBlendAttachment.colorBlendOp = colorBlendOp;
		m_ColorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
		m_ColorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		m_ColorBlendAttachment.alphaBlendOp = alphaBlendOp;

		m_ColorBlending = vk::PipelineColorBlendStateCreateInfo{};
		m_ColorBlending.logicOpEnable = logicOpEnable;
		m_ColorBlending.logicOp = logicOp;
		m_ColorBlending.setAttachments(m_ColorBlendAttachment);
		m_ColorBlending.setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });
	}

	void PipelineBuilder::SetDescriptorSetLayout(const std::vector<vk::DescriptorSetLayout>& layouts,
		const std::vector<vk::PushConstantRange>& pushConstants)
	{
		m_PipelineLayoutInfo = vk::PipelineLayoutCreateInfo{};
		m_PipelineLayoutInfo.setLayoutCount = layouts.size();
		m_PipelineLayoutInfo.pSetLayouts = layouts.data();

		m_PipelineLayoutInfo.pushConstantRangeCount = pushConstants.size();
		m_PipelineLayoutInfo.pPushConstantRanges = pushConstants.data();
	}

	VulkanPipeline PipelineBuilder::BuildGraphicsDynamicRendering()
	{
		auto bindingDescription = Vertex::GetBindingDescription();
		auto attributeDescriptions = Vertex::GetAttributeDescriptions();

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.setVertexBindingDescriptions(bindingDescription);
		vertexInputInfo.setVertexAttributeDescriptions(attributeDescriptions);

		vk::PipelineViewportStateCreateInfo viewportState{};
		viewportState.setViewports(m_Viewport);
		viewportState.setScissors(m_Scissor);

		// First, create the pipeline layout
		vk::PipelineLayout layout{};

		try
		{
			layout = m_pRenderCtx->device.createPipelineLayout(m_PipelineLayoutInfo);
		}
		catch (vk::SystemError& e)
		{
			throw std::runtime_error("Failed to create pipeline layout: "s + e.what());
		}

		vk::PipelineRenderingCreateInfo pipelineRendering{};
		pipelineRendering.setColorAttachmentFormats(m_pRenderCtx->imageFormat);
		// TODO: get these from the swap chain
		pipelineRendering.depthAttachmentFormat = vk::Format::eD24UnormS8Uint;
		pipelineRendering.stencilAttachmentFormat = vk::Format::eD24UnormS8Uint;

		std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = m_pShader->GetAllShaderStages();
		vk::GraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.pNext = &pipelineRendering;
		pipelineInfo.setStages(shaderStages);

		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &m_InputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &m_Rasterizer;
		pipelineInfo.pMultisampleState = &m_Multisampling;
		pipelineInfo.pDepthStencilState = &m_DepthStencil;
		pipelineInfo.pColorBlendState = &m_ColorBlending;
		pipelineInfo.pDynamicState = nullptr;

		pipelineInfo.layout = layout;
		pipelineInfo.renderPass = nullptr;
		pipelineInfo.subpass = 0;

		const vk::PipelineCache cache = m_pRenderCtx->device.createPipelineCache(vk::PipelineCacheCreateInfo{});
		const vk::ResultValue<vk::Pipeline> pipelineResult = m_pRenderCtx->device.createGraphicsPipeline(cache, pipelineInfo);

		if (pipelineResult.result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create graphics pipeline");
		}

		return VulkanPipeline(m_pRenderCtx, pipelineResult.value, cache, layout);
	}

	// VulkanPipeline PipelineBuilder::BuildCompute()
	// {
	// }
}
